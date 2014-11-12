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
 * $Id: block_mx1.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef SWAPXY
#define MV_X(v) ((v<<16)>>17)
#define MV_Y(v) (v>>17)
#define MV_SUB(v) (v&1)+((v>>15)&2)

static INLINE void Push(softidct* p,idct_block_t *Block)
{
	volatile int* MX1 = p->MX1;	
	idct_block_t *BlockEnd = Block+8;
	do
	{
		*MX1 = ((uint16_t*)Block)[0*8] + (((uint16_t*)Block)[1*8] << 16);
		*MX1 = ((uint16_t*)Block)[2*8] + (((uint16_t*)Block)[3*8] << 16);
		*MX1 = ((uint16_t*)Block)[4*8] + (((uint16_t*)Block)[5*8] << 16);
		*MX1 = ((uint16_t*)Block)[6*8] + (((uint16_t*)Block)[7*8] << 16);
		++Block;

	} while (Block != BlockEnd);
}

#else
#define MV_X(v) (v>>17)
#define MV_Y(v) ((v<<16)>>17)
#define MV_SUB(v) ((v<<1)&2)+((v>>16)&1)
#define Push PushSwap
static INLINE void Push(softidct* p,idct_block_t *Block)
{
	volatile int* MX1 = p->MX1;	
	idct_block_t *BlockEnd = Block+64;
	do
	{
		*MX1 = ((uint16_t*)Block)[0] + (((uint16_t*)Block)[1] << 16);
		*MX1 = ((uint16_t*)Block)[2] + (((uint16_t*)Block)[3] << 16);
		*MX1 = ((uint16_t*)Block)[4] + (((uint16_t*)Block)[5] << 16);
		*MX1 = ((uint16_t*)Block)[6] + (((uint16_t*)Block)[7] << 16);
		Block += 8;

	} while (Block != BlockEnd);
}

#endif

void Intra8x8(softidct* p,idct_block_t *Block,int Length,int ScanType)
{
	p->MX1Pop(p);
	p->MX1Dst = p->DstPtr;
	p->MX1Pitch = p->CurrPitch;
	p->MX1Pop = MX1Pop;
	Push(p,Block);
	EMMS();
	IncPtr(p,0,0);
}

void Inter8x8BackFwd(softidct* p,idct_block_t *Block,int Length)
{
	uint8_t* Ptr;
	int MV;

	p->MX1Pop(p);

	if (Length)
	{
		// mcomp and idct (using tmp buffer)

		if (Length != 1)
		{
			p->MX1Dst = p->DstPtr;
			p->MX1Pitch = p->CurrPitch;
			p->MX1Pop = MX1PopAdd;
			Push(p,Block);
		}

		if (p->MVBack)
		{
			MV = *(p->MVBack++);
			Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
				p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,8);

			if (p->MVFwd)
			{
				MV = *(p->MVFwd++);
				Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
				if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
				{
#if defined(MIPS32)
					p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp+64,p->CurrPitch,8);
					AddBlock8x8(p->Tmp+64,p->Tmp,8,8);
#else
					p->AddBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch);
#endif
				}
			}
		}
		else
		if (p->MVFwd)
		{
			MV = *(p->MVFwd++);
			Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
				p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,8);
		}

		if (Length == 1)
		{
			IDCT_Const8x8((Block[0]+4) >> 3,p->DstPtr,p->CurrPitch,p->Tmp);
			p->MX1Pop = MX1PopNone;
		}
	}
	else 
	{
		// interpolate back and foward (using tmp buffer)

		if (p->MVBack && p->MVFwd)
		{
			MV = *(p->MVBack++);
			Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
				p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,8);

			MV = *(p->MVFwd++);
			Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
			{
#if defined(MIPS32)
				p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);
				AddBlock8x8(p->Tmp,p->DstPtr,8,p->CurrPitch);
#else
				p->AddBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch);
				// copy Tmp to Dst
				CopyBlock(p->Tmp,p->DstPtr,8,p->CurrPitch);
#endif
			}
		}
		else
		if (p->MVBack)
		{
			MV = *(p->MVBack++);
			Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
				p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);
		}
		else
		if (p->MVFwd)
		{
			MV = *(p->MVFwd++);
			Ptr = p->RefPtr[1] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
			if (Ptr >= p->RefMin[1] && Ptr < p->RefMax[1])
				p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);
		}

		p->MX1Pop = MX1PopNone;
	}

	EMMS();
	IncPtr(p,1,1);
}

void Inter8x8Back(softidct* p,idct_block_t *Block,int Length)
{
	uint8_t* Ptr;
	int MV;

	p->MX1Pop(p);

	if (Length)
	{
		// mcomp and idct (using tmp buffer)

		if (Length != 1)
		{
			p->MX1Dst = p->DstPtr;
			p->MX1Pitch = p->CurrPitch;
			p->MX1Pop = MX1PopAdd;
			Push(p,Block);
		}

		MV = *(p->MVBack++);
		Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
			p->CopyBlock[MV_SUB(MV)](Ptr,p->Tmp,p->CurrPitch,8);

		if (Length == 1)
		{
			IDCT_Const8x8((Block[0]+4) >> 3,p->DstPtr,p->CurrPitch,p->Tmp);
			p->MX1Pop = MX1PopNone;
		}
	}
	else 
	{
		// only back mcomp
		MV = *(p->MVBack++);
		Ptr = p->RefPtr[0] + MV_X(MV) + p->CurrPitch * MV_Y(MV);
		if (Ptr >= p->RefMin[0] && Ptr < p->RefMax[0])
			p->CopyBlock[MV_SUB(MV)](Ptr,p->DstPtr,p->CurrPitch,p->CurrPitch);
		p->MX1Pop = MX1PopNone;
	}

	EMMS();
	IncPtr(p,1,0);
}

#undef MV_X
#undef MV_Y
#undef MV_SUB
#undef Push



