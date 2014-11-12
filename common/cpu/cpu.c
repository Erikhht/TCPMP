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
 * $Id: cpu.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "cpu.h"

#ifdef ARM
extern int STDCALL CheckARM5E();
extern int STDCALL CheckARMXScale();
#endif

#ifndef SH3
extern void STDCALL GetCpuId(int,uint32_t*);

#if defined(TARGET_SYMBIAN) && !defined(ARM)
#define GetCpuId(a,b)
#endif

static NOINLINE void SafeGetCpuId(int Id, uint32_t* p)
{
	memset(p,0,4*sizeof(uint32_t));
	TRY_BEGIN
	{
		bool_t Mode = KernelMode(1);
		GetCpuId(Id,p); 
		KernelMode(Mode);
	}
	TRY_END
}
#endif

int CPUCaps()
{
	cpudetect p;
	CPUDetect(&p);
	return p.Caps;
}

void CPUDetect(cpudetect* p)
{
	int Caps = 0;
	uint32_t CpuId[4];
	memset(p,0,sizeof(cpudetect));

#ifdef ARM
	p->Arch = T("ARM");
	SafeGetCpuId(0,CpuId);

	if (CpuId[0])
	{
		p->ICache = 512 << ((CpuId[1] >> 6) & 7);
		p->DCache = 512 << ((CpuId[1] >> 18) & 7);
	}
	else
	{

#if !defined(TARGET_PALMOS) && !defined(TARGET_SYMBIAN)
		// when need to detect cpu features somehow
		// (only works if we can catch cpu exceptions)
		TRY_BEGIN
		{	
			if (CheckARM5E())
			{
				int XScale;
				Caps |= CAPS_ARM_5E;

				XScale = CheckARMXScale();
				if (XScale)
				{
					p->ICache = p->DCache = 32768;
					Caps |= CAPS_ARM_XSCALE;
					if (XScale > 1)
						Caps |= CAPS_ARM_WMMX;
				}
			}
		}
		TRY_END
#endif
	}

	if ((CpuId[0] & 0xFF000000) == 0x54000000) //TI
	{
		p->Vendor = T("TI");
		Caps |= CAPS_ARM_GENERAL;
		switch ((CpuId[0] >> 4) & 0xFFF)
		{
		case 0x915: p->Model = T("915T"); break;
		case 0x925: p->Model = T("925T"); break;
		case 0x926: p->Model = T("926T"); Caps |= CAPS_ARM_5E; break;
		}
	}
	else
	if ((CpuId[0] & 0xFF000000) == 0x41000000) //arm
	{
		Caps |= CAPS_ARM_GENERAL;
		switch ((CpuId[0] >> 4) & 0xFFF)
		{
		case 0x920: p->Model = T("920T"); break;
		case 0x922: p->Model = T("922T"); break;
		case 0x926: p->Model = T("926E"); Caps |= CAPS_ARM_5E; break;
		case 0x940: p->Model = T("940T"); break;
		case 0x946: p->Model = T("946E"); Caps |= CAPS_ARM_5E; break;
		case 0xA22: p->Model = T("1020E"); Caps |= CAPS_ARM_5E; break;
		}
	}
	else
	if ((CpuId[0] & 0xFF000000) == 0x69000000) //intel
	{
		p->Vendor = T("Intel");

		if ((CpuId[0] & 0xFF0000) == 0x050000) //intel arm5e
			Caps |= CAPS_ARM_5E|CAPS_ARM_XSCALE;

		if (((CpuId[0] >> 4) & 0xFFF) == 0xB11)
		{
			p->Model = T("SA1110");
		}
		else
		{
			switch ((CpuId[0] >> 13) & 7)
			{
			case 0x2: Caps |= CAPS_ARM_WMMX; break;
			}

			switch ((CpuId[0] >> 4) & 31)
			{
			case 0x10: 
				p->Model = T("PXA25x/26x");
				break;
			case 0x11: 
				p->Model = T("PXA27x");
				break;
			case 0x12:
				p->Model = T("PXA210");
				break;
			}
		}
	}

#elif defined(MIPS)
	SafeGetCpuId(0,CpuId);
	p->Arch = T("MIPS");

	if (((CpuId[0] >> 8) & 255) == 0x0c)
	{
		if ((CpuId[0] & 0xF0) == 0x50)
		{
			Caps |= CAPS_MIPS_VR4110;
			p->Model = T("VR411X");
		}
		else
		{
			Caps |= CAPS_MIPS_VR4120;
			if ((CpuId[0] & 0xF0) == 0x80)
				p->Model = T("VR413X");
			else
				p->Model = T("VR412X");
		}
	}

#elif defined(SH3)
	CpuId[0] = 0; // avoid warning
	Caps = CpuId[0];
	p->Arch = T("SH3");

#elif defined(_M_IX86)
	p->Arch = T("x86");
	SafeGetCpuId(0,CpuId);

    if (CpuId[1] == 0x756e6547 &&
        CpuId[3] == 0x49656e69 &&
        CpuId[2] == 0x6c65746e) 
	{
		p->Vendor = T("Intel");

Intel:
		SafeGetCpuId(1,CpuId);
        if (CpuId[3] & 0x00800000)
		{
			Caps |= CAPS_X86_MMX;
			if (CpuId[3] & 0x02000000) 
				Caps |= CAPS_X86_MMX2 | CAPS_X86_SSE;
			if (CpuId[3] & 0x04000000) 
				Caps |= CAPS_X86_SSE2;
		}
    } 
	else if (CpuId[1] == 0x68747541 &&
             CpuId[3] == 0x69746e65 &&
             CpuId[2] == 0x444d4163) 
	{
		p->Vendor = T("AMD");

		SafeGetCpuId(0x80000000,CpuId);
        if (CpuId[0] < 0x80000001)
            goto Intel;

		SafeGetCpuId(0x80000001,CpuId);
        if (CpuId[3] & 0x00800000)
		{
			Caps |= CAPS_X86_MMX;
			if (CpuId[3] & 0x80000000)
				Caps |= CAPS_X86_3DNOW;
			if (CpuId[3] & 0x00400000)
				Caps |= CAPS_X86_MMX2;
		}
    } 
	else if (CpuId[1] == 0x746e6543 &&
             CpuId[3] == 0x48727561 &&
             CpuId[2] == 0x736c7561) 
	{
		p->Vendor = T("VIA C3");

		SafeGetCpuId(0x80000000,CpuId);
        if (CpuId[0] < 0x80000001)
            goto Intel;

		SafeGetCpuId(0x80000001,CpuId);
		if (CpuId[3] & (1<<31))
			Caps |= CAPS_X86_3DNOW;
		if (CpuId[3] & (1<<23))
			Caps |= CAPS_X86_MMX;
		if (CpuId[3] & (1<<24))
			Caps |= CAPS_X86_MMX2;
	}
	else if (CpuId[1] == 0x69727943 &&
             CpuId[3] == 0x736e4978 &&
             CpuId[2] == 0x64616574) 
	{
		p->Vendor = T("Cyrix");

        if (CpuId[0] != 2) 
            goto Intel;

		SafeGetCpuId(0x80000001,CpuId);
        if (CpuId[3] & 0x00800000)
		{
			Caps |= CAPS_X86_MMX;
			if (CpuId[3] & 0x01000000)
				Caps |= CAPS_X86_MMX2;
		}
    }

#endif
	p->Caps = Caps;
}

