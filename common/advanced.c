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
 * $Id: advanced.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

typedef struct advanced
{
	node Node;
	bool_t OldShell;
	bool_t SlowVideo;
	bool_t ColorLookup;
	bool_t NoBackLight;
	bool_t HomeScreen;
	bool_t NoWMMX;
	bool_t IDCTSwap;
	bool_t VR41XX;
	bool_t KeyFollowDir;
	bool_t MemoryOverride;
	bool_t BenchFromPos;
	bool_t AviFrameRate;
	bool_t Priority;
	bool_t SystemVolume;
	bool_t WidcommAudio;
	bool_t WidcommDLL;
	tick_t DropTolerance;
	tick_t SkipTolerance;
	tick_t AVOffset;
	bool_t NoBatteryWarning;
	bool_t NoEventChecking;
	bool_t CardPlugins;
	bool_t WaveoutPriority;
	bool_t NoDeblocking;
	bool_t BlinkLED;

} advanced;

static const datatable Params[] = 
{
	{ ADVANCED_NOBACKLIGHT, TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ ADVANCED_HOMESCREEN, TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#if defined(TARGET_WINCE)
	{ ADVANCED_OLDSHELL,	TYPE_BOOL, DF_SETUP|DF_RESTART|DF_CHECKLIST },
#endif
#if defined(ARM) 
	{ ADVANCED_NOWMMX,		TYPE_BOOL, DF_SETUP|DF_CHECKLIST|DF_RESTART },
#endif
#if !defined(SH3) && !defined(MIPS)
	{ ADVANCED_SLOW_VIDEO,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
#if defined(ARM) || defined(_M_IX86)
	{ ADVANCED_IDCTSWAP,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
#if !defined(SH3) 
	{ ADVANCED_COLOR_LOOKUP,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
	{ ADVANCED_KEYFOLLOWDIR,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#if defined(TARGET_PALMOS)
	{ ADVANCED_MEMORYOVERRIDE, TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
	{ ADVANCED_PRIORITY,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#if !defined(SH3) && !defined(TARGET_PALMOS)
	{ ADVANCED_SYSTEMVOLUME, TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
#if defined(MIPS)
	{ ADVANCED_VR41XX,TYPE_BOOL, DF_SETUP|DF_CHECKLIST|DF_RESTART },
#endif
	{ ADVANCED_BENCHFROMPOS,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ ADVANCED_AVIFRAMERATE,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#if defined(TARGET_WINCE)
	{ ADVANCED_WIDCOMMAUDIO,TYPE_BOOL, DF_SETUP|DF_CHECKLIST|DF_RESTART },
#endif
#if defined(TARGET_WINCE) || defined(TARGET_WIN32)
//	{ ADVANCED_WAVEOUTPRIORITY,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
#if defined(TARGET_PALMOS)
	{ ADVANCED_NOBATTERYWARNING,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ ADVANCED_NOEVENTCHECKING,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ ADVANCED_CARDPLUGINS,TYPE_BOOL, DF_SETUP|DF_CHECKLIST|DF_NOSAVE|DF_RESTART },
	{ ADVANCED_BLINKLED,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
	{ ADVANCED_NODEBLOCKING,TYPE_BOOL, DF_SETUP|DF_CHECKLIST },

	{ ADVANCED_AVOFFSET,	TYPE_TICK, DF_SETUP|DF_MINMAX|DF_MSEC|DF_GAP|DF_NEG|DF_RESYNC, -2*TICKSPERSEC, 2*TICKSPERSEC },
	{ ADVANCED_DROPTOL,		TYPE_TICK, DF_SETUP|DF_MINMAX|DF_MSEC, 0, TICKSPERSEC },
	{ ADVANCED_SKIPTOL,		TYPE_TICK, DF_SETUP|DF_MINMAX|DF_MSEC, 0, 5*TICKSPERSEC },

	{ ADVANCED_WIDCOMMDLL, TYPE_BOOL, DF_SETUP|DF_HIDDEN },

	DATATABLE_END(ADVANCED_ID)
};

static int Enum(advanced* p, int* No, datadef* Param)
{
	int Result = NodeEnumTable(No,Param,Params);
	if (Result == ERR_NONE)
	{
#if defined(TARGET_WINCE)
		if (Param->No == ADVANCED_WIDCOMMAUDIO && !p->WidcommDLL)
			Param->Flags |= DF_HIDDEN;
		if (Param->No == ADVANCED_OLDSHELL && (
			(QueryPlatform(PLATFORM_CAPS) & CAPS_OLDSHELL) || 
			QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE || 
			(QueryPlatform(PLATFORM_TYPENO) == TYPE_POCKETPC && QueryPlatform(PLATFORM_VER) >= 421)))
			Param->Flags |= DF_HIDDEN;
#endif
#if defined(TARGET_PALMOS)
		if (Param->No == ADVANCED_MEMORYOVERRIDE && !MemGetInfo(NULL))
			Param->Flags |= DF_HIDDEN;
#endif
		if (Param->No == ADVANCED_NOWMMX && !(QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_WMMX))
			Param->Flags |= DF_HIDDEN;
		if (Param->No == ADVANCED_HOMESCREEN && QueryPlatform(PLATFORM_TYPENO) != TYPE_SMARTPHONE)
			Param->Flags |= DF_HIDDEN;
		if (Param->No == ADVANCED_VR41XX && !(QueryPlatform(PLATFORM_CAPS) != CAPS_MIPS_VR41XX))
			Param->Flags |= DF_HIDDEN;
	}
	return Result;
}

static int Get(advanced* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case ADVANCED_OLDSHELL: GETVALUE(p->OldShell,bool_t); break;
	case ADVANCED_HOMESCREEN: GETVALUE(p->HomeScreen,bool_t); break;
	case ADVANCED_NOBACKLIGHT: GETVALUE(p->NoBackLight,bool_t); break;
	case ADVANCED_VR41XX: GETVALUE(p->VR41XX,bool_t); break;
	case ADVANCED_IDCTSWAP: GETVALUE(p->IDCTSwap,bool_t); break;
	case ADVANCED_NOWMMX: GETVALUE(p->NoWMMX,bool_t); break;
	case ADVANCED_NODEBLOCKING: GETVALUE(p->NoDeblocking,bool_t); break;
	case ADVANCED_BLINKLED: GETVALUE(p->BlinkLED,bool_t); break;
	case ADVANCED_PRIORITY: GETVALUE(p->Priority,bool_t); break;
	case ADVANCED_SYSTEMVOLUME: GETVALUE(p->SystemVolume,bool_t); break;
	case ADVANCED_SLOW_VIDEO: GETVALUE(p->SlowVideo,bool_t); break;
	case ADVANCED_COLOR_LOOKUP: GETVALUE(p->ColorLookup,bool_t); break;
	case ADVANCED_DROPTOL: GETVALUE(p->DropTolerance,tick_t); break;
	case ADVANCED_SKIPTOL: GETVALUE(p->SkipTolerance,tick_t); break;
	case ADVANCED_AVOFFSET: GETVALUE(p->AVOffset,tick_t); break;
	case ADVANCED_BENCHFROMPOS: GETVALUE(p->BenchFromPos,bool_t); break;
	case ADVANCED_AVIFRAMERATE: GETVALUE(p->AviFrameRate,bool_t); break;
	case ADVANCED_WIDCOMMAUDIO: GETVALUE(p->WidcommAudio,bool_t); break;
	case ADVANCED_WIDCOMMDLL: GETVALUE(p->WidcommDLL,bool_t); break;
	case ADVANCED_KEYFOLLOWDIR: GETVALUE(p->KeyFollowDir,bool_t); break;
	case ADVANCED_WAVEOUTPRIORITY: GETVALUE(p->WaveoutPriority,bool_t); break;
	case ADVANCED_MEMORYOVERRIDE: GETVALUE(p->MemoryOverride,bool_t); break;
	case ADVANCED_NOBATTERYWARNING: GETVALUE(p->NoBatteryWarning,bool_t); break;
	case ADVANCED_NOEVENTCHECKING: GETVALUE(p->NoEventChecking,bool_t); break;
	case ADVANCED_CARDPLUGINS: GETVALUE(p->CardPlugins,bool_t); break;
	}
	return Result;
}

static int Set(advanced* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;

	switch (No)
	{
	case ADVANCED_OLDSHELL: SETVALUE(p->OldShell,bool_t,ERR_NONE); break;
	case ADVANCED_HOMESCREEN: SETVALUE(p->HomeScreen,bool_t,ERR_NONE); break;
	case ADVANCED_NOBACKLIGHT: SETVALUE(p->NoBackLight,bool_t,ERR_NONE); break;
	case ADVANCED_VR41XX: SETVALUE(p->VR41XX,bool_t,ERR_NONE); break;
	case ADVANCED_IDCTSWAP: SETVALUECMP(p->IDCTSwap,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_NOWMMX: SETVALUE(p->NoWMMX,bool_t,ERR_NONE); break;
	case ADVANCED_BLINKLED: SETVALUE(p->BlinkLED,bool_t,ERR_NONE); break;
	case ADVANCED_NODEBLOCKING: SETVALUECMP(p->NoDeblocking,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_PRIORITY: SETVALUECMP(p->Priority,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_SYSTEMVOLUME: SETVALUECMP(p->SystemVolume,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_SLOW_VIDEO: SETVALUECMP(p->SlowVideo,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_COLOR_LOOKUP: SETVALUECMP(p->ColorLookup,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_DROPTOL: SETVALUECMP(p->DropTolerance,tick_t,NodeSettingsChanged(),EqInt); break;
	case ADVANCED_SKIPTOL: SETVALUECMP(p->SkipTolerance,tick_t,NodeSettingsChanged(),EqInt); break;
	case ADVANCED_AVOFFSET: SETVALUECMP(p->AVOffset,tick_t,NodeSettingsChanged(),EqInt); break;
	case ADVANCED_BENCHFROMPOS: SETVALUE(p->BenchFromPos,bool_t,ERR_NONE); break;
	case ADVANCED_AVIFRAMERATE: SETVALUECMP(p->AviFrameRate,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_WIDCOMMAUDIO: SETVALUE(p->WidcommAudio,bool_t,ERR_NONE); break;
	case ADVANCED_KEYFOLLOWDIR: SETVALUE(p->KeyFollowDir,bool_t,ERR_NONE); break;
	case ADVANCED_WAVEOUTPRIORITY: SETVALUECMP(p->WaveoutPriority,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_MEMORYOVERRIDE: SETVALUECOND(p->MemoryOverride,bool_t,ERR_NONE,MemGetInfo(NULL)); break;
	case ADVANCED_NOBATTERYWARNING: SETVALUE(p->NoBatteryWarning,bool_t,ERR_NONE); break;
	case ADVANCED_NOEVENTCHECKING: SETVALUECMP(p->NoEventChecking,bool_t,NodeSettingsChanged(),EqBool); break;
	case ADVANCED_CARDPLUGINS: SETVALUE(p->CardPlugins,bool_t,ERR_NONE); break;
	case ADVANCED_WIDCOMMDLL: 
		assert(Size==sizeof(bool_t));
		if (p->WidcommDLL && !*(bool_t*)Data)
			p->WidcommAudio = 1;
		break;
	}
	return Result;
}

bool_t QueryAdvanced(int Param)
{
	node* Advanced = Context()->Advanced;
	bool_t Value=0;
	if (Advanced)
		Advanced->Get(Advanced,Param,&Value,sizeof(Value));
	return Value;
}

static int Create(advanced* p)
{
	int Caps = QueryPlatform(PLATFORM_CAPS);
	video Desktop;
	QueryDesktop(&Desktop);

	p->Node.Enum = (nodeenum)Enum,
	p->Node.Get = (nodeget)Get,
	p->Node.Set = (nodeset)Set,
	p->SlowVideo = 0;
#if defined(TARGET_WINCE)
	p->SystemVolume = QueryPlatform(PLATFORM_VER) < 421;
#else
	p->SystemVolume = 0;
#endif
	p->BenchFromPos = 0;
	p->VR41XX = 1;
	p->MemoryOverride = 0;
	p->KeyFollowDir = HaveDPad();
	p->ColorLookup = !(Caps & CAPS_MIPS_VR41XX) && !(Caps & CAPS_ARM_XSCALE);
	p->OldShell = (Caps & CAPS_OLDSHELL) != 0;
	p->DropTolerance = (TICKSPERSEC*55)/1000;
	p->SkipTolerance = (TICKSPERSEC*700)/1000;
	p->AVOffset = 0;
	p->BlinkLED = 1;

#if defined(TARGET_PALMOS)
	// causes major problems on Sony TJ35, like screen not turning off with audio, or hold/power button not working...
	//p->NoEventChecking = (QueryPlatform(PLATFORM_CAPS) & CAPS_SONY)!=0;
#endif

#if defined(TARGET_WINCE)
	{
		tchar_t FileName[MAXPATH];
		GetSystemPath(FileName,TSIZEOF(FileName),T("BtCeAvIf.dll"));
		p->WidcommAudio = p->WidcommDLL = FileExits(FileName);
	}
#endif

	return ERR_NONE;
}

static const nodedef Advanced =
{
	sizeof(advanced)|CF_GLOBAL|CF_SETTINGS,
	ADVANCED_ID,
	NODE_CLASS,
	PRI_MAXIMUM+50,
	(nodecreate)Create,
	NULL,
};

void Advanced_Init()
{
	NodeRegisterClass(&Advanced);
	Context()->Advanced = NodeEnumObject(NULL,ADVANCED_ID);
}

void Advanced_Done()
{
	Context()->Advanced = NULL;
	NodeUnRegisterClass(ADVANCED_ID);
}

