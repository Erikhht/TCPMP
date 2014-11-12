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
 * $Id: block_half.c 284 2005-10-04 08:54:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

#if defined(CONFIG_IDCT_LOWRES)

#define Statistics(a,b,c,d)

void Copy420Half(softidct* p,int x,int y,int Forward)
{
	SetPtr420(p,x,y,1);
	CopyBlock8x8(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtrLum(p);
	CopyBlock4x4(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	IncPtr(p,1,1);
	CopyBlock4x4(p->RefPtr[Forward],p->DstPtr,p->CurrPitch,p->CurrPitch);
	EMMS();
}

void Process420Half(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr420(p,x,y,1);
	ProcessTail(p,x,y);
}

void Process422Half(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr422(p,x,y,1);
	ProcessTail(p,x,y);
}

void Process444Half(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr444(p,x,y,1);
	ProcessTail(p,x,y);
}

#define HALF
#define Intra8x8 Intra8x8Half
#define Inter8x8Back Inter8x8BackHalf
#define Inter8x8BackFwd Inter8x8BackFwdHalf
#include "block.h"

void Process420Quarter(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr420(p,x,y,2);
	ProcessTail(p,x,y);
}

void Process422Quarter(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr422(p,x,y,2);
	ProcessTail(p,x,y);
}

void Process444Quarter(softidct* p,int x,int y)
{
	ProcessHead(p,x,y);
	SetPtr444(p,x,y,2);
	ProcessTail(p,x,y);
}

#undef HALF
#define QUARTER
#undef Intra8x8
#define Intra8x8 Intra8x8Quarter
#include "block.h"

#endif
