/*****************************************************************************
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: platform.c 593 2006-01-17 22:25:08Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#include "cpu/cpu.h"

int DebugMask() 
{
	return DEBUG_SYS;//|DEBUG_FORMAT|DEBUG_PLAYER;//|DEBUG_VIDEO;//|DEBUG_AUDIO|DEBUG_VIDEO|DEBUG_PLAYER;//|DEBUG_VIDEO;//DEBUG_TEST;//|DEBUG_FORMAT|DEBUG_PLAYER;
}

static const datatable Params[] = 
{
	{ PLATFORM_LANG,		TYPE_INT, DF_SETUP|DF_ENUMSTRING|DF_RESTART, LANG_ID },
	{ PLATFORM_TYPE,		TYPE_STRING, DF_SETUP|DF_RDONLY|DF_GAP },
	{ PLATFORM_VER,			TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_VERSION,		TYPE_STRING, DF_SETUP|DF_RDONLY },
	{ PLATFORM_OEMINFO,		TYPE_STRING, DF_SETUP|DF_RDONLY },
	{ PLATFORM_TYPENO,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_MODEL,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_CAPS,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN|DF_HEX },
	{ PLATFORM_CPU,			TYPE_STRING, DF_SETUP|DF_RDONLY },
	{ PLATFORM_CPUMHZ,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_MHZ },
	{ PLATFORM_ICACHE,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_DCACHE,		TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_WMPVERSION,	TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },
	{ PLATFORM_ORIENTATION, TYPE_INT, DF_SETUP|DF_RDONLY|DF_HIDDEN },

	DATATABLE_END(PLATFORM_ID)
};

#if defined(TARGET_SYMBIAN) && !defined(ARM)
static INLINE int64_t STDCALL CPUSpeedClk(int i) { return 0; }
#else
extern int64_t STDCALL CPUSpeedClk(int);
#endif

int CPUSpeed()
{
	int Speed = 0;

#if defined(MIPS) || defined(ARM) || defined(_M_IX86) || defined(SH3)
	int Old = ThreadPriority(NULL,-100);
	int TimeFreq = GetTimeFreq();
	if (TimeFreq>0)
	{

#ifdef TARGET_PALMOS
		int Iter = 1;
#else
		int Iter = 4;
#endif

		int64_t Clk,Best = 0;
		int BestCount = 0;
		int n,Count;
		int Sub=1,SubTime=8;
		int c0[2],c1[2],c2[2];
		int t;
		int SubTimeLimit = TimeFreq/250;
		if (SubTimeLimit <= 1)
			SubTimeLimit = 2;
		Count = 800000 / TimeFreq;

		for (n=0;n<4*Iter;++n)
		{
			GetTimeCycle(c0);
			GetTimeCycle(c1);
			GetTimeCycle(c2);

			c2[0] -= c1[0];
			c1[0] -= c0[0];

			if (c2[1]<c1[1])
			{
				c1[0] = c2[0];
				c1[1] = c2[1];
			}

			if (c1[1]*SubTime > c1[0]*Sub)
			{
				Sub = c1[1];
				SubTime = c1[0];
			}

			if (n>6 && SubTime >= SubTimeLimit)
				break;
		}

		if (SubTime < SubTimeLimit)
		{
			// Sub = approx number of possbile GetTime() calls per SubTime ms

			for (n=0;n<16*Iter;++n)
			{
				CPUSpeedClk(1); // load instruction cache
				GetTimeCycle(c0);
				Clk = CPUSpeedClk(Count);
				GetTimeCycle(c1);

				t = c1[0]-c0[0];
#if defined(TARGET_WINCE)
				if (QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE)
					Clk += t*2000;
#elif defined(TARGET_WIN32)
				// we are assuming a 1ms timer interrupt with 
				// minimum 6500 cycles interrupt handler
				Clk += t*6500;
#endif
				t *= 100000;
				t -=(c1[1] * 100000 * SubTime)/Sub; // adjust with subtime

				if (t < 25000) // quater tick -> double count
					Count *= 2;
				else
				if (t < 50000) // we need at least half tick
					Count += Count/8;
				else
				{
#ifdef ARM
					Clk = (Clk *  99600 * TimeFreq) / t;
#else
					Clk = (Clk * 100000 * TimeFreq) / t;
#endif
					if (Best < Clk)
					{
						Best = Clk;
						BestCount = 1;
					}
					else
					if ((Best * 127)/128 < Clk && ++BestCount>4*Iter)
						break;
				}
			}
		}

		Speed = (int)((Best + 500000)/1000000);
	}

	//rounding
	if ((Speed % 104)==1 || Speed==207) --Speed;
	else if ((Speed % 104)==103 || Speed==205) ++Speed;
	else if ((Speed % 10)==1) --Speed;
	else if ((Speed % 10)==9) ++Speed;
	else if (((Speed % 100)%33)==1) --Speed;
	else if (((Speed % 100)%33)==32) ++Speed;

	ThreadPriority(NULL,Old);

#endif
	return Speed;
}

static int Enum(platform* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static int Get(platform* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLATFORM_LANG: GETVALUE(Context()->Lang,int); break;
	case PLATFORM_TYPE:	GETSTRING(p->PlatformType); break;
	case PLATFORM_WMPVERSION: GETVALUE(p->WMPVersion,int); break;
	case PLATFORM_ORIENTATION: GETVALUE(GetOrientation(),int); break;
	case PLATFORM_CPU: GETSTRING(p->CPU); break;
	case PLATFORM_OEMINFO: if (p->OemInfo[0]) GETSTRING(p->OemInfo); break;
	case PLATFORM_VER: GETVALUE(p->Ver,int); break;
	case PLATFORM_TYPENO: GETVALUE(p->Type,int); break;
	case PLATFORM_MODEL: GETVALUE(p->Model,int); break;
	case PLATFORM_CAPS: GETVALUE(p->Caps,int); break;
	case PLATFORM_ICACHE: GETVALUECOND(p->ICache,int,p->ICache>0); break;
	case PLATFORM_DCACHE: GETVALUECOND(p->DCache,int,p->DCache>0); break;
	case PLATFORM_VERSION: GETSTRING(p->Version); break;
	case PLATFORM_CPUMHZ: 
		assert(Size==sizeof(int));
		if ((*(int*)Data = CPUSpeed())>0)
			Result = ERR_NONE;
		break;
	}
	return Result;
}

static int Set(platform* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLATFORM_LANG: SETVALUE(Context()->Lang,int,ERR_NONE); break;
	}
	return Result;
}

int QueryPlatform(int Param)
{
	node* Platform = Context()->Platform;
	int Value = 0;
	if (Platform)
		Platform->Get(Platform,Param,&Value,sizeof(Value));
	return Value;
}

extern void PlatformDetect(platform*);

static int Create(platform* p)
{
	cpudetect Info;
	p->Node.Enum = (nodeenum)Enum;
	p->Node.Get = (nodeget)Get;
	p->Node.Set = (nodeset)Set;
	p->Model = MODEL_UNKNOWN;

	CPUDetect(&Info);
	p->Caps = Info.Caps;
	p->ICache = Info.ICache;
	p->DCache = Info.DCache;
	p->CPU[0] = 0;
	if (Info.Arch)
		tcscat_s(p->CPU,TSIZEOF(p->CPU),Info.Arch);
	if (Info.Vendor)
		stcatprintf_s(p->CPU,TSIZEOF(p->CPU),T(" %s"),Info.Vendor);
	if (Info.Model)
		stcatprintf_s(p->CPU,TSIZEOF(p->CPU),T(" %s"),Info.Model);

	PlatformDetect(p);
	return ERR_NONE;
}

const nodedef Platform = 
{
	sizeof(platform)|CF_GLOBAL|CF_SETTINGS,
	PLATFORM_ID,
	NODE_CLASS,
	PRI_MAXIMUM+650,
	(nodecreate)Create,
	NULL,
};

