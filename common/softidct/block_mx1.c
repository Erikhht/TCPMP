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
 * $Id: block_mx1.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

#ifdef FREESCALE_MX1

void MX1PopNone(struct softidct* p)
{
}

static void MX1Pop(struct softidct* p)
{
	int x0,x1,x2,x3,x4,x5,x6,x7;
	uint8_t *DstEnd;
	uint8_t *Dst = p->MX1Dst;
	int Pitch = p->MX1Pitch;
	volatile int* MX1 = p->MX1;

	while (!(MX1[-61] & 0x20));

	DstEnd = Dst + Pitch*8;
	do
	{
		x0 = *MX1; x1 = x0 >> (16+7); x0 <<= 16; x0 >>= (16+7);
		x2 = *MX1; x3 = x2 >> (16+7); x2 <<= 16; x2 >>= (16+7);
		x4 = *MX1; x5 = x4 >> (16+7); x4 <<= 16; x4 >>= (16+7);
		x6 = *MX1; x7 = x6 >> (16+7); x6 <<= 16; x6 >>= (16+7);

		if ((x0|x1|x2|x3|x4|x5|x6|x7)>>8)
		{
			SAT(x0)
			SAT(x1)
			SAT(x2)
			SAT(x3)
			SAT(x4)
			SAT(x5)
			SAT(x6)
			SAT(x7)
		}

		Dst[0] = (uint8_t)x0;
		Dst[1] = (uint8_t)x1;
		Dst[2] = (uint8_t)x2;
		Dst[3] = (uint8_t)x3;
		Dst[4] = (uint8_t)x4;
		Dst[5] = (uint8_t)x5;
		Dst[6] = (uint8_t)x6;
		Dst[7] = (uint8_t)x7;

		Dst += Pitch;

	} while (Dst != DstEnd);
}

static void MX1PopAdd(struct softidct* p)
{
	int x0,x1,x2,x3,x4,x5,x6,x7;
	uint8_t *DstEnd;
	uint8_t *Dst = p->MX1Dst;
	uint8_t *Src = p->Tmp;
	int Pitch = p->MX1Pitch;
	volatile int* MX1 = p->MX1;

	while (!(MX1[-61] & 0x20));

	DstEnd = Dst + Pitch*8;
	do
	{
		x0 = *MX1; x1 = x0 >> (16+7); x0 <<= 16; x0 >>= (16+7);
		x2 = *MX1; x3 = x2 >> (16+7); x2 <<= 16; x2 >>= (16+7);
		x4 = *MX1; x5 = x4 >> (16+7); x4 <<= 16; x4 >>= (16+7);
		x6 = *MX1; x7 = x6 >> (16+7); x6 <<= 16; x6 >>= (16+7);

		x0 += Src[0];
		x1 += Src[1];
		x2 += Src[2];
		x3 += Src[3];
		x4 += Src[4];
		x5 += Src[5];
		x6 += Src[6];
		x7 += Src[7];
		Src += 8;

		if ((x0|x1|x2|x3|x4|x5|x6|x7)>>8)
		{
			SAT(x0)
			SAT(x1)
			SAT(x2)
			SAT(x3)
			SAT(x4)
			SAT(x5)
			SAT(x6)
			SAT(x7)
		}

		Dst[0] = (uint8_t)x0;
		Dst[1] = (uint8_t)x1;
		Dst[2] = (uint8_t)x2;
		Dst[3] = (uint8_t)x3;
		Dst[4] = (uint8_t)x4;
		Dst[5] = (uint8_t)x5;
		Dst[6] = (uint8_t)x6;
		Dst[7] = (uint8_t)x7;

		Dst += Pitch;

	} while (Dst != DstEnd);
}

#define Intra8x8 MX1Intra8x8
#define Inter8x8Back MX1Inter8x8Back
#define Inter8x8BackFwd MX1Inter8x8BackFwd
#include "block_mx1.h"
#undef Intra8x8
#undef Inter8x8Back
#undef Inter8x8BackFwd

#ifdef CONFIG_IDCT_SWAP
#define SWAPXY
#define SWAP8X4
#define Intra8x8 MX1Intra8x8Swap
#define Inter8x8Back MX1Inter8x8BackSwap
#define Inter8x8BackFwd MX1Inter8x8BackFwdSwap
#include "block_mx1.h"
#endif

#endif
