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
 * $Id: idct_c.c 323 2005-11-01 20:52:32Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#define W1 2841  // 2048*sqrt(2)*cos(1*pi/16) 
#define W2 2676  // 2048*sqrt(2)*cos(2*pi/16) 
#define W3 2408  // 2048*sqrt(2)*cos(3*pi/16) 
#define W5 1609  // 2048*sqrt(2)*cos(5*pi/16) 
#define W6 1108  // 2048*sqrt(2)*cos(6*pi/16) 
#define W7 565   // 2048*sqrt(2)*cos(7*pi/16) 

#define ADDSAT32(a,Dst,Add32)		\
	b = a + Add32;					\
	c = a & Add32;					\
	a ^= Add32;						\
	a &= ~b;						\
	a |= c;							\
	a &= MaskCarry;					\
	c = a << 1;						\
	b -= c;	/* adjust neighbour */	\
	b |= c - (a >> 7); /* mask */	\
	Dst = b;						

#define SUBSAT32(a,Dst,Add32)		\
	a = ~a;							\
	b = a + Add32;					\
	c = a & Add32;					\
	a ^= Add32;						\
	a &= ~b;						\
	a |= c;							\
	a &= MaskCarry;					\
	c = a << 1;						\
	b -= c;	/* adjust neighbour */	\
	b |= c - (a >> 7); /* mask */	\
	Dst = ~b;						

#if !defined(ARM) 

#define SPLIT(d0,d1,d2)				\
	d0 = d1 + d2;					\
	d1 -= d2;

#define BUTTERFLY(d0,d1,W0,W1,tmp)	\
    tmp = W0 * (d0 + d1);			\
    d0 = tmp + (W1 - W0) * d0;		\
    d1 = tmp - (W1 + W0) * d1;

#ifdef MIPSVR41XX
#define WRITEBACK(Dst,n)			\
	__asm(  "andi	$7,%0,0x8;"		\
			"beq	$7,$0, skipwb" #n ";" \
			".set noreorder;"		\
			"cache	25,-8(%0);"	\
			".set reorder;"			\
			"skipwb" #n ":",Dst);
#else
#define WRITEBACK(Dst,n)
#endif

	
static void IDCT_Col8(idct_block_t *Blk)
{
	int d0,d1,d2,d3,d4,d5,d6,d7,d8;

	d0 = Blk[0];
	d4 = Blk[8];
	d3 = Blk[16];
	d7 = Blk[24];
	d1 = Blk[32];
	d6 = Blk[40];
	d2 = Blk[48];	
	d5 = Blk[56];

	d8 = d5|d6|d7;
	if (!(d1|d2|d3|d8))
	{
		if (!d4) 
		{ 
			// d0
			if (d0) 
			{
				d0 <<= 3;
				Blk[0]  = (idct_block_t)d0;
				Blk[8]  = (idct_block_t)d0;
				Blk[16] = (idct_block_t)d0;
				Blk[24] = (idct_block_t)d0;
				Blk[32] = (idct_block_t)d0;
				Blk[40] = (idct_block_t)d0;
				Blk[48] = (idct_block_t)d0;
				Blk[56] = (idct_block_t)d0;
			}
		}
		else 
		{ 
			// d0,d4
			d0 = (d0 << 11) + 128; //+final rounding
			//d1 = (d1 << 11);

			d1 = W7 * d4;
			d2 = W1 * d4;
			d3 = W3 * d4;
			d4 = W5 * d4;

			Blk[0]  = (idct_block_t)((d0 + d2) >> 8);
			Blk[8]  = (idct_block_t)((d0 + d3) >> 8);
			Blk[16] = (idct_block_t)((d0 + d4) >> 8);
			Blk[24] = (idct_block_t)((d0 + d1) >> 8);
			Blk[32] = (idct_block_t)((d0 - d1) >> 8);
			Blk[40] = (idct_block_t)((d0 - d4) >> 8);
			Blk[48] = (idct_block_t)((d0 - d3) >> 8);
			Blk[56] = (idct_block_t)((d0 - d2) >> 8);
		}
	}
	else if (!(d4|d8)) 
	{ 
		// d0,d1,d2,d3

		d0 = (d0 << 11) + 128; //+final rounding
		d1 = (d1 << 11);

		SPLIT(d4,d0,d1)			//d1->d4
		BUTTERFLY(d3,d2,W6,W2,d1)
		SPLIT(d5,d4,d3)			//d3->d5
		SPLIT(d3,d0,d2)			//d2->d3
		
		Blk[0]  = (idct_block_t)(d5 >> 8);
		Blk[8]  = (idct_block_t)(d3 >> 8);
		Blk[16] = (idct_block_t)(d0 >> 8);
		Blk[24] = (idct_block_t)(d4 >> 8);
		Blk[32] = (idct_block_t)(d4 >> 8);
		Blk[40] = (idct_block_t)(d0 >> 8);
		Blk[48] = (idct_block_t)(d3 >> 8);
		Blk[56] = (idct_block_t)(d5 >> 8);
	}
	else 
	{ 
		// d0,d1,d2,d3,d4,d5,d6,d7

		d0 = (d0 << 11) + 128; //+final rounding
		d1 = (d1 << 11);

		BUTTERFLY(d4,d5,W7,W1,d8)
		BUTTERFLY(d6,d7,W3,W5,d8)

		SPLIT(d8,d0,d1)			//d1->d8
		BUTTERFLY(d3,d2,W6,W2,d1)
		SPLIT(d1,d4,d6)			//d6->d1
		SPLIT(d6,d5,d7)			//d7->d6
		SPLIT(d7,d8,d3)			//d3->d7
		SPLIT(d3,d0,d2)			//d2->d3

		d2 = (181 * (d4+d5) + 128) >> 8;
		d4 = (181 * (d4-d5) + 128) >> 8;

		Blk[0]  = (idct_block_t)((d7 + d1) >> 8);
		Blk[8]  = (idct_block_t)((d3 + d2) >> 8);
		Blk[16] = (idct_block_t)((d0 + d4) >> 8);
		Blk[24] = (idct_block_t)((d8 + d6) >> 8);
		Blk[32] = (idct_block_t)((d8 - d6) >> 8);
		Blk[40] = (idct_block_t)((d0 - d4) >> 8);
		Blk[48] = (idct_block_t)((d3 - d2) >> 8);
		Blk[56] = (idct_block_t)((d7 - d1) >> 8);
	}
}

static void IDCT_Row8(idct_block_t *Blk, uint8_t *Dst, const uint8_t *Src)
{
	int d0,d1,d2,d3,d4,d5,d6,d7,d8;

	d4 = Blk[1];
  	d3 = Blk[2];
  	d7 = Blk[3];
	d1 = Blk[4];
	d6 = Blk[5];
	d2 = Blk[6];
	d5 = Blk[7];
	
	if (!(d1|d2|d3|d4|d5|d6|d7))
	{
		d0 = (Blk[0] + 32) >> 6;
		if (!Src)
		{
			SAT(d0);

			d0 &= 255;
			d0 |= d0 << 8;
			d0 |= d0 << 16;

			((uint32_t*)Dst)[0] = d0;
			((uint32_t*)Dst)[1] = d0;
			WRITEBACK(Dst,0);
			return;
		}
		d1 = d2 = d3 = d4 = d5 = d6 = d7 = d0;
	}
	else
	{
		d0 = (Blk[0] << 11) + 65536; // +final rounding
		d1 <<= 11;

		BUTTERFLY(d4,d5,W7,W1,d8)
		BUTTERFLY(d6,d7,W3,W5,d8)

		SPLIT(d8,d0,d1)			//d1->d8
		BUTTERFLY(d3,d2,W6,W2,d1)
		SPLIT(d1,d4,d6)			//d6->d1
		SPLIT(d6,d5,d7)			//d7->d6
		SPLIT(d7,d8,d3)			//d3->d7
		SPLIT(d3,d0,d2)			//d2->d3

		d2 = 181 * ((d4 + d5 + 128) >> 8);
		d4 = 181 * ((d4 - d5 + 128) >> 8);

		d5 = (d7 + d1) >> 17;
		d1 = (d7 - d1) >> 17;
		d7 = (d3 + d2) >> 17;
		d2 = (d3 - d2) >> 17;
		d3 = (d0 + d4) >> 17;
		d4 = (d0 - d4) >> 17;
		d0 = (d8 + d6) >> 17;
		d6 = (d8 - d6) >> 17;
	}

	if (Src)
	{
		d5 += Src[0];
		d1 += Src[7];
		d7 += Src[1];
		d2 += Src[6];
		d3 += Src[2];
		d4 += Src[5];
		d0 += Src[3];
		d6 += Src[4];
	}
	
	if ((d5|d1|d7|d2|d3|d4|d0|d6)>>8)
	{
		SAT(d5)
		SAT(d7)
		SAT(d3)
		SAT(d0)
		SAT(d6)
		SAT(d4)
		SAT(d2)
		SAT(d1)
	}

	Dst[0] = (uint8_t)d5;
	Dst[1] = (uint8_t)d7;
	Dst[2] = (uint8_t)d3;
	Dst[3] = (uint8_t)d0;
	Dst[4] = (uint8_t)d6;
	Dst[5] = (uint8_t)d4;
	Dst[6] = (uint8_t)d2;
	Dst[7] = (uint8_t)d1;
	WRITEBACK(Dst,1);
}

static void IDCT_Row4(idct_block_t *Blk, uint8_t *Dst, const uint8_t *Src)
{
	int d0,d1,d2,d3,d4,d5,d6,d7,d8;
  
	d4 = Blk[1];
  	d3 = Blk[2];
  	d7 = Blk[3];
	
	if (!(d3|d4|d7))
	{
		d0 = (Blk[0] + 32) >> 6;
		if (!Src)
		{
			SAT(d0);

			d0 &= 255;
			d0 |= d0 << 8;
			d0 |= d0 << 16;

			((uint32_t*)Dst)[0] = d0;
			((uint32_t*)Dst)[1] = d0;
			WRITEBACK(Dst,2);
			return;
		}
		d1 = d2 = d3 = d4 = d5 = d6 = d7 = d0;
	}
	else
	{
		d0 = (Blk[0] << 11) + 65536; // +final rounding

		d5 = W7 * d4;
		d4 = W1 * d4;
		d6 = W3 * d7;
		d7 = -W5 * d7;
		d2 = W6 * d3;
		d3 = W2 * d3;

		SPLIT(d1,d4,d6)
		SPLIT(d6,d5,d7)

		d8 = d0;
		SPLIT(d7,d8,d3)
		SPLIT(d3,d0,d2)

		d2 = 181 * ((d4 + d5 + 128) >> 8);
		d4 = 181 * ((d4 - d5 + 128) >> 8);

		d5 = (d7 + d1) >> 17;
		d1 = (d7 - d1) >> 17;
		d7 = (d3 + d2) >> 17;
		d2 = (d3 - d2) >> 17;
		d3 = (d0 + d4) >> 17;
		d4 = (d0 - d4) >> 17;
		d0 = (d8 + d6) >> 17;
		d6 = (d8 - d6) >> 17;
	}

	if (Src)
	{
		d5 += Src[0];
		d1 += Src[7];
		d7 += Src[1];
		d2 += Src[6];
		d3 += Src[2];
		d4 += Src[5];
		d0 += Src[3];
		d6 += Src[4];
	}
	
	if ((d5|d1|d7|d2|d3|d4|d0|d6)>>8)
	{
		SAT(d5)
		SAT(d7)
		SAT(d3)
		SAT(d0)
		SAT(d6)
		SAT(d4)
		SAT(d2)
		SAT(d1)
	}

	Dst[0] = (uint8_t)d5;
	Dst[1] = (uint8_t)d7;
	Dst[2] = (uint8_t)d3;
	Dst[3] = (uint8_t)d0;
	Dst[4] = (uint8_t)d6;
	Dst[5] = (uint8_t)d4;
	Dst[6] = (uint8_t)d2;
	Dst[7] = (uint8_t)d1;
	WRITEBACK(Dst,3);
}

void STDCALL IDCT_Block8x8(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int SrcStride;

	IDCT_Col8(Block);
	IDCT_Col8(Block+1);
	IDCT_Col8(Block+2);
	IDCT_Col8(Block+3);
	IDCT_Col8(Block+4);
	IDCT_Col8(Block+5);
	IDCT_Col8(Block+6);
	IDCT_Col8(Block+7);

	SrcStride = 0;
#ifdef MIPS64
	if (Src) SrcStride = DestStride;
#else
	if (Src) SrcStride = 8;
#endif

	IDCT_Row8(Block,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+8,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+16,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+24,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+32,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+40,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+48,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row8(Block+56,Dest,Src);
}

void STDCALL IDCT_Block4x8(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int SrcStride;

	IDCT_Col8(Block);
	IDCT_Col8(Block+1);
	IDCT_Col8(Block+2);
	IDCT_Col8(Block+3);

	SrcStride = 0;
#ifdef MIPS64
	if (Src) SrcStride = DestStride;
#else
	if (Src) SrcStride = 8;
#endif

	IDCT_Row4(Block,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+8,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+16,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+24,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+32,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+40,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+48,Dest,Src);
	Dest+=DestStride;
	Src+=SrcStride;
	IDCT_Row4(Block+56,Dest,Src);
}

#ifdef CONFIG_IDCT_SWAP
// just for testing
void STDCALL IDCT_Block4x8Swap(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int x;
	for (x=0;x<64;++x)
		Block[64+x] = Block[((x&7)<<3)+(x>>3)];

	IDCT_Block4x8(Block+64,Dest,DestStride,Src);
}
void STDCALL IDCT_Block8x8Swap(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int x;
	for (x=0;x<64;++x)
		Block[64+x] = Block[((x&7)<<3)+(x>>3)];

	IDCT_Block8x8(Block+64,Dest,DestStride,Src);
}
#endif

#endif

#ifndef MMX

void STDCALL IDCT_Const8x8(int v,uint8_t * Dst, int DstPitch, uint8_t * Src)
{
#ifndef MIPS64
	int SrcPitch = 8;
#else
	int SrcPitch = DstPitch;
#endif
	const uint8_t* SrcEnd = Src + 8*SrcPitch;
	uint32_t MaskCarry = 0x80808080U;
	uint32_t a,b,c,d;

	if (v>0)
	{
		v |= v << 8;
		v |= v << 16;

		do
		{
			a = ((uint32_t*)Src)[0];
			d = ((uint32_t*)Src)[1];
			ADDSAT32(a,((uint32_t*)Dst)[0],v);
			ADDSAT32(d,((uint32_t*)Dst)[1],v);
			Dst += DstPitch;
			Src += SrcPitch;
		}
		while (Src != SrcEnd);
	}
	else
	if (v<0)
	{
		v = -v;
		v |= v << 8;
		v |= v << 16;

		do
		{
			a = ((uint32_t*)Src)[0];
			d = ((uint32_t*)Src)[1];
			SUBSAT32(a,((uint32_t*)Dst)[0],v);
			SUBSAT32(d,((uint32_t*)Dst)[1],v);
			Dst += DstPitch;
			Src += SrcPitch;
		}
		while (Src != SrcEnd);
	}
#ifndef MIPS64
	else
		CopyBlock8x8(Src,Dst,SrcPitch,DstPitch);
#endif
}

#endif
