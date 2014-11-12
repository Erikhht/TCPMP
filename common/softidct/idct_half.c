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
 * $Id: idct_half.c 284 2005-10-04 08:54:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#define W1 2676  // 2048*sqrt(2)*cos(1*pi/8) 
#define W3 1108  // 2048*sqrt(2)*cos(3*pi/8) 

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

#define SPLIT(d0,d1,d2)				\
	d0 = d1 + d2;					\
	d1 -= d2;

#define BUTTERFLY(d0,d1,W0,W1,tmp)	\
    tmp = W0 * (d0 + d1);			\
    d0 = tmp + (W1 - W0) * d0;		\
    d1 = tmp - (W1 + W0) * d1;

	
static void IDCT_Col4(idct_block_t *Blk)
{
	int d0,d1,d2,d3,d4;

	d0 = Blk[0];
	d1 = Blk[8];
	d2 = Blk[16];
	d3 = Blk[24];

	if (!(d2|d3))
	{
		if (!d1) 
		{ 
			// d0
			if (d0) 
			{
				d0 <<= 3;
				Blk[0]  = (idct_block_t)d0;
				Blk[8]  = (idct_block_t)d0;
				Blk[16] = (idct_block_t)d0;
				Blk[24] = (idct_block_t)d0;
			}
		}
		else 
		{ 
			// d0,d1
			d0 = (d0 << 11) + 128; //+final rounding
			d4 = W1 * d1;
			d1 = W3 * d1;

			Blk[0]  = (idct_block_t)((d0 + d4) >> 8);
			Blk[8]  = (idct_block_t)((d0 + d1) >> 8);
			Blk[16] = (idct_block_t)((d0 - d1) >> 8);
			Blk[24] = (idct_block_t)((d0 - d4) >> 8);
		}
	}
	else
	{ 
		// d0,d1,d2,d3
		d0 = (d0 << 11) + 128; //+final rounding
		d2 = (d2 << 11);

		BUTTERFLY(d1,d3,W3,W1,d4)
		SPLIT(d4,d0,d2)				// d2->d1

		Blk[0]  = (idct_block_t)((d4 + d1) >> 8);
		Blk[8]  = (idct_block_t)((d0 + d3) >> 8);
		Blk[16] = (idct_block_t)((d0 - d3) >> 8);
		Blk[24] = (idct_block_t)((d4 - d1) >> 8);
	}
}

static void IDCT_Row4(idct_block_t *Blk, uint8_t *Dst, const uint8_t *Src)
{
	int d0,d1,d2,d3,d4;

	d1 = Blk[1];
  	d2 = Blk[2];
  	d3 = Blk[3];
	
	if (!(d1|d2|d3))
	{
		d0 = (Blk[0] + 32) >> 6;
		if (!Src)
		{
			SAT(d0);

			d0 &= 255;
			d0 |= d0 << 8;
			d0 |= d0 << 16;

			((uint32_t*)Dst)[0] = d0;
			return;
		}
		d2 = d4 = d1 = d0;
	}
	else
	{
		d0 = (Blk[0] << 11) + 65536; // +final rounding
		d2 = (d2 << 11);

		BUTTERFLY(d1,d3,W3,W1,d4)
		SPLIT(d4,d0,d2)				// d2->d1

		d2 = (d4 + d1) >> 17; //0
		d4 = (d4 - d1) >> 17; //3
		d1 = (d0 + d3) >> 17; //1
		d0 = (d0 - d3) >> 17; //2
	}

	if (Src)
	{
		d2 += Src[0];
		d1 += Src[1];
		d0 += Src[2];
		d4 += Src[3];
	}
	
	if ((d2|d1|d0|d4)>>8)
	{
		SAT(d2)
		SAT(d1)
		SAT(d0)
		SAT(d4)
	}

	Dst[0] = (uint8_t)d2;
	Dst[1] = (uint8_t)d1;
	Dst[2] = (uint8_t)d0;
	Dst[3] = (uint8_t)d4;
}

void STDCALL IDCT_Block4x4(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int SrcStride;

	IDCT_Col4(Block);
	IDCT_Col4(Block+1);
	IDCT_Col4(Block+2);
	IDCT_Col4(Block+3);

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
}

void STDCALL IDCT_Block2x2(idct_block_t *Block, uint8_t *Dest, int DestStride, const uint8_t *Src)
{
	int d0,d1,d2,d3,d4;

	d0 = Block[0] + Block[8];
	d1 = Block[0] - Block[8];
	d2 = Block[1] + Block[9];
	d3 = Block[1] - Block[9];

	d0 += 4;
	d4 = (d0 - d2) >> 3;
	d0 = (d0 + d2) >> 3;

	d1 += 4;
	d2 = (d1 - d3) >> 3;
	d1 = (d1 + d3) >> 3;

	if (Src)
	{
#ifdef MIPS64
		int SrcStride = DestStride;
#else
		int SrcStride = 8;
#endif
		d0 += Src[0];
		d4 += Src[1];
		d1 += Src[0+SrcStride];
		d2 += Src[1+SrcStride];
	}
	
	if ((d0|d4|d1|d2)>>8)
	{
		SAT(d0)
		SAT(d4)
		SAT(d1)
		SAT(d2)
	}

	Dest[0] = (uint8_t)d0;
	Dest[1] = (uint8_t)d4;

	Dest+=DestStride;

	Dest[0] = (uint8_t)d1;
	Dest[1] = (uint8_t)d2;
}

#ifndef MMX

void STDCALL IDCT_Const4x4(int v,uint8_t * Dst, int DstPitch, uint8_t * Src)
{
#ifndef MIPS64
	int SrcPitch = 8;
#else
	int SrcPitch = DstPitch;
#endif
	const uint8_t* SrcEnd = Src + 4*SrcPitch;
	uint32_t MaskCarry = 0x80808080U;
	uint32_t a,b,c;

	if (v>0)
	{
		v |= v << 8;
		v |= v << 16;

		do
		{
			a = ((uint32_t*)Src)[0]; 
			ADDSAT32(a,((uint32_t*)Dst)[0],v); 
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
			SUBSAT32(a,((uint32_t*)Dst)[0],v);
			Dst += DstPitch;
			Src += SrcPitch;
		}
		while (Src != SrcEnd);
	}
#ifndef MIPS64
	else
		CopyBlock4x4(Src,Dst,SrcPitch,DstPitch);
#endif
}

#endif
