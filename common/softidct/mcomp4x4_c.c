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
 * $Id: mcomp_c.c 323 2005-11-01 20:52:32Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#if !defined(MIPS64)

#define CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,y,x,r) \
{ \
	int i; \
	for (i=0;i<4;++i) \
	{ \
		Dst[0] = (uint8_t)((Src[0]*(4-x)*(4-y) + Src[1]*x*(4-y) + Src[SrcPitch+0]*(4-x)*y + Src[SrcPitch+1]*x*y + 8-r) >> 4); \
		Dst[1] = (uint8_t)((Src[1]*(4-x)*(4-y) + Src[2]*x*(4-y) + Src[SrcPitch+1]*(4-x)*y + Src[SrcPitch+2]*x*y + 8-r) >> 4); \
		Dst[2] = (uint8_t)((Src[2]*(4-x)*(4-y) + Src[3]*x*(4-y) + Src[SrcPitch+2]*(4-x)*y + Src[SrcPitch+3]*x*y + 8-r) >> 4); \
		Dst[3] = (uint8_t)((Src[3]*(4-x)*(4-y) + Src[4]*x*(4-y) + Src[SrcPitch+3]*(4-x)*y + Src[SrcPitch+4]*x*y + 8-r) >> 4); \
		Dst += DstPitch; \
		Src += SrcPitch; \
	} \
}

void STDCALL CopyBlock4x4_01(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,1,0); }
void STDCALL CopyBlock4x4_02(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,2,0); }
void STDCALL CopyBlock4x4_03(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,3,0); }
void STDCALL CopyBlock4x4_10(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,0,0); }
void STDCALL CopyBlock4x4_11(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,1,0); }
void STDCALL CopyBlock4x4_12(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,2,0); }
void STDCALL CopyBlock4x4_13(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,3,0); }
void STDCALL CopyBlock4x4_20(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,0,0); }
void STDCALL CopyBlock4x4_21(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,1,0); }
void STDCALL CopyBlock4x4_22(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,2,0); }
void STDCALL CopyBlock4x4_23(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,3,0); }
void STDCALL CopyBlock4x4_30(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,0,0); }
void STDCALL CopyBlock4x4_31(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,1,0); }
void STDCALL CopyBlock4x4_32(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,2,0); }
void STDCALL CopyBlock4x4_33(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,3,0); }

void STDCALL CopyBlock4x4_01R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,1,1); }
void STDCALL CopyBlock4x4_02R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,2,1); }
void STDCALL CopyBlock4x4_03R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,0,3,1); }
void STDCALL CopyBlock4x4_10R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,0,1); }
void STDCALL CopyBlock4x4_11R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,1,1); }
void STDCALL CopyBlock4x4_12R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,2,1); }
void STDCALL CopyBlock4x4_13R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,1,3,1); }
void STDCALL CopyBlock4x4_20R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,0,1); }
void STDCALL CopyBlock4x4_21R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,1,1); }
void STDCALL CopyBlock4x4_22R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,2,1); }
void STDCALL CopyBlock4x4_23R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,2,3,1); }
void STDCALL CopyBlock4x4_30R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,0,1); }
void STDCALL CopyBlock4x4_31R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,1,1); }
void STDCALL CopyBlock4x4_32R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,2,1); }
void STDCALL CopyBlock4x4_33R(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{ CopyBlock4x4_XX(Src,Dst,SrcPitch,DstPitch,3,3,1); }

// possible memory aligning problem with source
void STDCALL CopyBlock4x4_00(uint8_t *Src, uint8_t *Dst, int SrcPitch,int DstPitch)
{
	int i;
	for (i=0;i<4;++i)
	{
		Dst[0] = Src[0];
		Dst[1] = Src[1];
		Dst[2] = Src[2];
		Dst[3] = Src[3];
		Dst += DstPitch;
		Src += SrcPitch;
	}
}

#ifndef MIPS32

// Dst[p] = Src[p]
void STDCALL CopyBlock4x4(uint8_t *Src, uint8_t *Dst, int SrcPitch,int DstPitch)
{
	uint32_t a,b,c,d;
	a=((uint32_t*)Src)[0]; Src += SrcPitch;
	b=((uint32_t*)Src)[0]; Src += SrcPitch;
	c=((uint32_t*)Src)[0]; Src += SrcPitch;
	d=((uint32_t*)Src)[0];
	((uint32_t*)Dst)[0]=a; Dst += DstPitch;
	((uint32_t*)Dst)[0]=b; Dst += DstPitch;
	((uint32_t*)Dst)[0]=c; Dst += DstPitch;
	((uint32_t*)Dst)[0]=d;
}

#define AddBlock4x4_XX(Src,Dst,SrcPitch,y,x) \
{ \
	int i; \
	for (i=0;i<4;++i) \
	{ \
		Dst[0] = (uint8_t)((((Src[0]*(4-x)*(4-y) + Src[1]*x*(4-y) + Src[SrcPitch+0]*(4-x)*y + Src[SrcPitch+1]*x*y + 8) >> 4) + Dst[0] + 1) >> 1); \
		Dst[1] = (uint8_t)((((Src[1]*(4-x)*(4-y) + Src[2]*x*(4-y) + Src[SrcPitch+1]*(4-x)*y + Src[SrcPitch+2]*x*y + 8) >> 4) + Dst[1] + 1) >> 1); \
		Dst[2] = (uint8_t)((((Src[2]*(4-x)*(4-y) + Src[3]*x*(4-y) + Src[SrcPitch+2]*(4-x)*y + Src[SrcPitch+3]*x*y + 8) >> 4) + Dst[2] + 1) >> 1); \
		Dst[3] = (uint8_t)((((Src[3]*(4-x)*(4-y) + Src[4]*x*(4-y) + Src[SrcPitch+3]*(4-x)*y + Src[SrcPitch+4]*x*y + 8) >> 4) + Dst[3] + 1) >> 1); \
		Dst += 8; \
		Src += SrcPitch; \
	} \
}

void STDCALL AddBlock4x4_00(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,0,0); }
void STDCALL AddBlock4x4_01(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,0,1); }
void STDCALL AddBlock4x4_02(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,0,2); }
void STDCALL AddBlock4x4_03(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,0,3); }
void STDCALL AddBlock4x4_10(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,1,0); }
void STDCALL AddBlock4x4_11(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,1,1); }
void STDCALL AddBlock4x4_12(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,1,2); }
void STDCALL AddBlock4x4_13(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,1,3); }
void STDCALL AddBlock4x4_20(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,2,0); }
void STDCALL AddBlock4x4_21(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,2,1); }
void STDCALL AddBlock4x4_22(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,2,2); }
void STDCALL AddBlock4x4_23(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,2,3); }
void STDCALL AddBlock4x4_30(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,3,0); }
void STDCALL AddBlock4x4_31(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,3,1); }
void STDCALL AddBlock4x4_32(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,3,2); }
void STDCALL AddBlock4x4_33(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{ AddBlock4x4_XX(Src,Dst,SrcPitch,3,3); }

#endif

#endif

