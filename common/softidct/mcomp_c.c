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
 * $Id: mcomp_c.c 329 2005-11-04 17:17:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#if !defined(ARM) && !defined(MIPS64) && !defined(MIPS32) && !defined(MMX)

#define LoadRow(a,b,ofs) \
{ \
	a=LOAD32(Src+ofs+0); \
	b=LOAD32(Src+ofs+4); \
}
#define LoadMRow(a,b,c,d,ofs) \
{ \
	a=LOAD32(Src+ofs+0); \
	b=LOAD32(Src+ofs+4); \
	c=LOAD32(Src+ofs+8); \
	d=LOAD32(Src+ofs+12); \
}
#define SaveRow(a,b) \
{ \
	((uint32_t*)Dst)[0]=a; \
	((uint32_t*)Dst)[1]=b; \
}
#define SaveMRow(a,b,c,d)	\
{							\
	((uint32_t*)Dst)[0]=a;	\
	((uint32_t*)Dst)[1]=b;	\
	((uint32_t*)Dst)[2]=c;	\
	((uint32_t*)Dst)[3]=d;	\
}
#define AddRow(a,b,t0,t1)	\
{ \
	t0=((uint32_t*)Dst)[0]; \
	t1=((uint32_t*)Dst)[1]; \
	AVG32R(a,b,t0,t1); \
	((uint32_t*)Dst)[0]=a; \
	((uint32_t*)Dst)[1]=b; \
}
#define PrepareAvg4(a,b,c,d)				\
{											\
	uint32_t q,w;							\
	q=(a & 0x03030303);						\
	w=(b & 0x03030303);						\
	q+=(c & 0x03030303);					\
	w+=(d & 0x03030303);					\
	a=(a>>2) & 0x3F3F3F3F;					\
	b=(b>>2) & 0x3F3F3F3F;					\
	a+=(c>>2) & 0x3F3F3F3F;					\
	b+=(d>>2) & 0x3F3F3F3F;					\
	c=q;									\
	d=w;									\
}
#define Avg4(a,b,c,d,g,h,i,j)				\
{											\
	a+=g;									\
	b+=h;									\
	c+=i+0x02020202;						\
	d+=j+0x02020202;						\
	a+=(c>>2) & 0x03030303;					\
	b+=(d>>2) & 0x03030303;					\
}
#define Avg4Round(a,b,c,d,g,h,i,j)			\
{											\
	a+=g;									\
	b+=h;									\
	c+=i+0x01010101;						\
	d+=j+0x01010101;						\
	a+=(c>>2) & 0x03030303;					\
	b+=(d>>2) & 0x03030303;					\
}

// Dst[p] = Src[p]
void STDCALL CopyBlock(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b;
	do
	{
		LoadRow(a,b,0)
		SaveRow(a,b)
		Dst += DstPitch;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+1]+1) >> 1;
void STDCALL CopyBlockHor(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		LoadRow(a,b,0)
		LoadRow(c,d,1)
		AVG32R(a,b,c,d)
		SaveRow(a,b)
		Dst += DstPitch;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+SrcPitch]+1) >> 1;
void STDCALL CopyBlockVer(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,e,f;
	LoadRow(a,b,0)
	do
	{
		Src += SrcPitch;
		LoadRow(c,d,0)
		e = c;
		f = d;
		AVG32R(a,b,e,f)
		SaveRow(a,b)
		Dst += DstPitch;

		Src += SrcPitch;
		LoadRow(a,b,0)
		e = c;
		f = d;
		AVG32R(c,d,e,f)
		SaveRow(c,d)
		Dst += DstPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+1] + Src[p+SrcPitch] + Src[p+SrcPitch+1] + 2) >> 2;
void STDCALL CopyBlockHorVer(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,g,h,i,j;
	LoadRow(a,b,0)
	LoadRow(c,d,1)
	PrepareAvg4(a,b,c,d)
	do
	{
		Src += SrcPitch;
		LoadRow(g,h,0)
		LoadRow(i,j,1)
		PrepareAvg4(g,h,i,j)
		Avg4(a,b,c,d,g,h,i,j)
		SaveRow(a,b)
		Dst += DstPitch;

		Src += SrcPitch;
		LoadRow(a,b,0)
		LoadRow(c,d,1)
		PrepareAvg4(a,b,c,d)
		Avg4(g,h,i,j,a,b,c,d)
		SaveRow(g,h)
		Dst += DstPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+1]) >> 1;
void STDCALL CopyBlockHorRound(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		LoadRow(a,b,0)
		LoadRow(c,d,1)
		AVG32NR(a,b,c,d)
		SaveRow(a,b)
		Dst += DstPitch;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+SrcPitch]) >> 1;
void STDCALL CopyBlockVerRound(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,e,f;
	LoadRow(a,b,0)
	do
	{
		Src += SrcPitch;
		LoadRow(c,d,0)
		e = c;
		f = d;
		AVG32NR(a,b,e,f)
		SaveRow(a,b)
		Dst += DstPitch;

		Src += SrcPitch;
		LoadRow(a,b,0)
		e = a;
		f = b;
		AVG32NR(c,d,e,f)
		SaveRow(c,d)
		Dst += DstPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Src[p+1] + Src[p+SrcPitch] + Src[p+SrcPitch+1] + 1) >> 2;
void STDCALL CopyBlockHorVerRound(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,g,h,i,j;
	LoadRow(a,b,0)
	LoadRow(c,d,1)
	PrepareAvg4(a,b,c,d)
	do
	{
		Src += SrcPitch;
		LoadRow(g,h,0)
		LoadRow(i,j,1)
		PrepareAvg4(g,h,i,j)
		Avg4Round(a,b,c,d,g,h,i,j)
		SaveRow(a,b)
		Dst += DstPitch;

		Src += SrcPitch;
		LoadRow(a,b,0)
		LoadRow(c,d,1)
		PrepareAvg4(a,b,c,d)
		Avg4Round(g,h,i,j,a,b,c,d)
		SaveRow(g,h)
		Dst += DstPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (Src[p] + Dst[p] + 1) >> 1
void STDCALL AddBlock(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		LoadRow(a,b,0)
		AddRow(a,b,c,d)
		Dst += 8;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (((Src[p] + Src[p+1]+1) >> 1) + Dst[p] + 1) >> 1
void STDCALL AddBlockHor(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		LoadRow(a,b,0)
		LoadRow(c,d,1)
		AVG32R(a,b,c,d)
		AddRow(a,b,c,d)
		Dst += 8;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (((Src[p] + Src[p+SrcPitch]+1) >> 1) + Dst[p] + 1) >> 1
void STDCALL AddBlockVer(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,e,f;
	LoadRow(a,b,0)
	do
	{
		Src += SrcPitch;
		c=a;
		d=b;
		LoadRow(a,b,0)
		e=a;
		f=b;
		AVG32R(c,d,e,f)
		AddRow(c,d,e,f)
		Dst += 8;
	}
	while (Src != SrcEnd);
}

// Dst[p] = (((Src[p] + Src[p+1] + Src[p+SrcPitch] + Src[p+SrcPitch+1] + 2) >> 2) + Dst[p] + 1) >> 1
void STDCALL AddBlockHorVer(uint8_t *Src, uint8_t *Dst, int SrcPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d,g,h,i,j;
	LoadRow(a,b,0)
	LoadRow(c,d,1)
	PrepareAvg4(a,b,c,d)
	do
	{
		Src += SrcPitch;
		LoadRow(g,h,0)
		LoadRow(i,j,1)
		PrepareAvg4(g,h,i,j)
		Avg4(a,b,c,d,g,h,i,j)
		AddRow(a,b,c,d)
		a=g;
		b=h;
		c=i;
		d=j;
		Dst += 8;
	}
	while (Src != SrcEnd);
}

// Dst[p] = Src[p]
void STDCALL CopyBlock16x16(uint8_t *Src, uint8_t *Dst, int SrcPitch,int DstPitch)
{
	uint8_t *SrcEnd = Src + 16*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		a=((uint32_t*)Src)[0];
		b=((uint32_t*)Src)[1];
		c=((uint32_t*)Src)[2];
		d=((uint32_t*)Src)[3];
		((uint32_t*)Dst)[0]=a;
		((uint32_t*)Dst)[1]=b;
		((uint32_t*)Dst)[2]=c;
		((uint32_t*)Dst)[3]=d;
		Dst += DstPitch;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = Src[p]
void STDCALL CopyBlock8x8(uint8_t *Src, uint8_t *Dst, int SrcPitch,int DstPitch)
{
	uint8_t *SrcEnd = Src + 8*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		a=((uint32_t*)Src)[0];
		b=((uint32_t*)Src)[1];
		Src += SrcPitch;
		c=((uint32_t*)Src)[0];
		d=((uint32_t*)Src)[1];
		Src += SrcPitch;
		((uint32_t*)Dst)[0]=a;
		((uint32_t*)Dst)[1]=b;
		Dst += DstPitch;
		((uint32_t*)Dst)[0]=c;
		((uint32_t*)Dst)[1]=d;
		Dst += DstPitch;
	}
	while (Src != SrcEnd);
}

// Dst[p] = Src[p]
void STDCALL CopyBlockM(uint8_t *Src, uint8_t *Dst, int SrcPitch, int DstPitch)
{
	uint8_t *SrcEnd = Src + 16*SrcPitch;
	uint32_t a,b,c,d;
	do
	{
		LoadMRow(a,b,c,d,0)
		SaveMRow(a,b,c,d)
		Dst += DstPitch;
		Src += SrcPitch;
	}
	while (Src != SrcEnd);
}

#endif
