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
 * $Id: block_c.c 323 2005-11-01 20:52:32Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

//#define STAT

#ifdef STAT

int Len[16] = {0};
int Count = 0;
int Type[2][3] = {0};
int Spec[2][5] = {0};
int Skip = 0;
int Max = 0;

void Statistics(idct_block_t *Block, int Length, int ScanType, int Add)
{
	int i;
	for (i=0;i<Length;++i)
	{
		int v = Block[i];
		if (v<0) v=-v;
		if (v>Max) Max=v;
	}

	++Len[Length >> 2];
	++Count;
	++Type[Add][ScanType];
	if (Length==1)
		Spec[Add][0]++;
	else
	if (ScanType==0 && Length==2)
		Spec[Add][1]++;
	else
	if (!Add && ScanType!=1 && Length < 15)
	{
		Spec[Add][2]++;
	}
	else
	if (Add && ScanType==0 && (Length < 15 || (Length<26 && ((uint32_t*)Block)[2]==0 && ((uint32_t*)Block)[6]==0)))
	{
		Spec[Add][2]++;
	}

	DebugMessage(T("%d %d %d [%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d]"
		"[%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d][%d %d %d %d %d %d %d %d]"),
		Add,ScanType,Length,
		Block[0+0*8],Block[1+0*8],Block[2+0*8],Block[3+0*8],Block[4+0*8],Block[5+0*8],Block[6+0*8],Block[7+0*8],
		Block[0+1*8],Block[1+1*8],Block[2+1*8],Block[3+1*8],Block[4+1*8],Block[5+1*8],Block[6+1*8],Block[7+1*8],
		Block[0+2*8],Block[1+2*8],Block[2+2*8],Block[3+2*8],Block[4+2*8],Block[5+2*8],Block[6+2*8],Block[7+2*8],
		Block[0+3*8],Block[1+3*8],Block[2+3*8],Block[3+3*8],Block[4+3*8],Block[5+3*8],Block[6+3*8],Block[7+3*8],
		Block[0+4*8],Block[1+4*8],Block[2+4*8],Block[3+4*8],Block[4+4*8],Block[5+4*8],Block[6+4*8],Block[7+4*8],
		Block[0+5*8],Block[1+5*8],Block[2+5*8],Block[3+5*8],Block[4+5*8],Block[5+5*8],Block[6+5*8],Block[7+5*8],
		Block[0+6*8],Block[1+6*8],Block[2+6*8],Block[3+6*8],Block[4+6*8],Block[5+6*8],Block[6+6*8],Block[7+6*8],
		Block[0+7*8],Block[1+7*8],Block[2+7*8],Block[3+7*8],Block[4+7*8],Block[5+7*8],Block[6+7*8],Block[7+7*8]);
}
#else
#define Statistics(a,b,c,d)
#endif

void Copy420(softidct* p,int x,int y,int Forward)
{
	SetPtr420(p,x,y,0);
	CopyBlock16x16(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtrLum(p);
	CopyBlock8x8(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtr(p,1,1);
	CopyBlock8x8(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	EMMS();
}

void Process420(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr420(p,x,y,0);
	ProcessTail(p,x,y);
}

void Process422(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr422(p,x,y,0);
	ProcessTail(p,x,y);
}

void Process444(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr444(p,x,y,0);
	ProcessTail(p,x,y);
}

#ifndef MIPS64
void SoftMComp8x8(softidct* p,const int* MVBack,const int* MVFwd)
{
	p->MVBack = MVBack;
	p->MVFwd = MVFwd;
#ifdef STAT
	if (MVBack)
		DebugMessage(T("back 08x %08x %08x %08x %08x %08x"),MVBack[0],MVBack[1],MVBack[2],MVBack[3],MVBack[4],MVBack[5]);
	if (MVFwd)
		DebugMessage(T("fwd 08x %08x %08x %08x %08x %08x"),MVFwd[0],MVFwd[1],MVFwd[2],MVFwd[3],MVFwd[4],MVFwd[5]);
#endif
}
#endif

#include "block.h"

#ifdef CONFIG_IDCT_SWAP

void Copy420Swap(softidct* p,int x,int y,int Forward)
{
	SetPtr420(p,y,x,0);
	CopyBlock16x16(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtrLum(p);
	CopyBlock8x8(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtr(p,1,1);
	CopyBlock8x8(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	EMMS();
}

void Process420Swap(softidct* p,int x,int y)
{
	SetPtr420(p,y,x,0);
}
void Process422Swap(softidct* p,int x,int y)
{
	SetPtr422(p,y,x,0);
}
void Process444Swap(softidct* p,int x,int y)
{
	SetPtr444(p,y,x,0);
}

#ifndef MIPS64
void SoftMComp8x8Swap(softidct* p,const int* MVBack,const int* MVFwd)
{
	p->MVBack = MVBack;
	p->MVFwd = MVFwd;
}
#endif

#define SWAPXY
#define SWAP8X4
#define Intra8x8 Intra8x8Swap
#define Inter8x8Back Inter8x8BackSwap
#define Inter8x8BackFwd Inter8x8BackFwdSwap

#include "block.h"

#endif
