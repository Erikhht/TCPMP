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
 * $Id: block_mips64.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

#ifdef MIPS64

#define MV_X(v) ((v<<16)>>17)
#define MV_Y(v) (v>>17)
#define MV_SUB(v) (v&1)+((v>>15)&2)

void Inter8x8Add(softidct* p,idct_block_t *Block,int Length)
{
	// mcomp part is done by Process()
	if (Length)
	{
		if (Length == 1)
			IDCT_Const8x8((Block[0]+4) >> 3,p->DstPtr,p->CurrPitch,p->DstPtr);
#ifdef SWAP8X4
		else if (Length < 11 || (Length<20 && Block[32]==0))
			IDCT_Block4x8(Block,p->DstPtr,p->CurrPitch,p->DstPtr);
#else
		else if (Length < 15 || (Length<26 && ((uint32_t*)Block)[2]==0 && ((uint32_t*)Block)[6]==0))
			IDCT_Block4x8(Block,p->DstPtr,p->CurrPitch,p->DstPtr);
#endif
		else
			IDCT_Block8x8(Block,p->DstPtr,p->CurrPitch,p->DstPtr);
	}
	
	IncPtr(p,0,0);
}

void MComp8x8(softidct* p,const int* MVPtr,const int* MVEnd,int Ref)
{
	uint8_t* Min = p->RefMin[Ref];
	uint8_t* Max = p->RefMax[Ref];
	uint8_t* Ptr;
	int MV;

	for (;MVPtr!=MVEnd;++MVPtr)
	{
		MV = *MVPtr;
		Ptr = p->RefPtr[Ref] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= Min && Ptr < Max)
			p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);
		IncPtr(p,1,1);
	}
}

void MComp8x8Add(softidct* p,const int* MVBack,const int* MVEnd,const int* MVFwd)
{
	uint8_t* Min0 = p->RefMin[0];
	uint8_t* Max0 = p->RefMax[0];
	uint8_t* Min1 = p->RefMin[1];
	uint8_t* Max1 = p->RefMax[1];
	uint8_t* Ptr;
	int MV;

	for (;MVBack!=MVEnd;++MVBack,++MVFwd)
	{
		MV = *MVBack;
		Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= Min0 && Ptr < Max0)
			p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);

		MV = *MVFwd;
		Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= Min1 && Ptr < Max1)
		{
			p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,8);
			AddBlock8x8(p->Tmp,p->DstPtr,8,p->CurrPitch);
		}

		IncPtr(p,1,1);
	}
}

void SoftMComp8x8(softidct* p,const int *MVBack,const int *MVFwd)
{
	uint8_t* SaveDstPtr = p->DstPtr;
	if (MVBack)
	{
		if (MVFwd)
			MComp8x8Add(p,MVBack,MVBack+6,MVFwd);
		else
			MComp8x8(p,MVBack,MVBack+6,0);
	}
	else
	if (MVFwd)
		MComp8x8(p,MVFwd,MVFwd+6,1);

	p->CurrPitch <<= 1;
	p->Ptr -= 6;
	p->DstPtr = SaveDstPtr;
}

void SoftMComp16x16(softidct* p,const int *MVBack,const int *MVFwd)
{
	uint8_t* SaveDstPtr = p->DstPtr;
	uint8_t* Ptr;
	int MV;

	if (MVBack)
	{
		MV = MVBack[0];
		Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
			p->CopyMBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);

		if (MVFwd)
		{
			MV = MVFwd[0];
			Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
			{
				p->CopyMBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,16);
				AddBlock16x16(p->Tmp,p->DstPtr,16,p->CurrPitch);
			}

			IncPtrLum(p);
			MComp8x8Add(p,MVBack+4,MVBack+6,MVFwd+4);
		}
		else
		{
			IncPtrLum(p);
			MComp8x8(p,MVBack+4,MVBack+6,0);
		}
	}
	else
	if (MVFwd)
	{
		MV = MVFwd[0];
		Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
			p->CopyMBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);

		IncPtrLum(p);
		MComp8x8(p,MVFwd+4,MVFwd+6,1);
	}

	p->CurrPitch <<= 1;
	p->Ptr -= 6;
	p->DstPtr = SaveDstPtr;
}

#endif

