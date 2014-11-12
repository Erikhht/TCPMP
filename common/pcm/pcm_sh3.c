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
 * $Id: pcm_sh3.c 304 2005-10-20 11:02:59Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "pcm_soft.h"

#if defined(SH3)

typedef struct stack
{
	int Step;		//backup

	int StackFrame[7];

	void* this;    //R4
	char* Dst;	   //R5
	char* Src;     //R6
	int DstLength; //R7
	pcmstate* State;
	int Volume;

} stack;

// R0 temp index
// R13,R1 dither (or R13=shift,-shift) (or R1=volume)
// R2,R3 value
// R4 Min
// R5 Max
// R7 Step
// R8 Pos
// R9 SrcLeft
// R10 SrcRight
// R11 DstLeft
// R12 DstRight
// R14 DstLeftEnd

void PCMLoop(pcm_soft* p,bool_t Speed)
{
	bool_t SrcPacked = !(p->Src.Flags & PCM_PLANES) && p->Src.Channels==2;
	dyninst* Loop;
	reg Left;
	reg Right;
	reg Shift;

	if (!p->ActualDither && p->Shift)
		I1C(MOVI,R13,p->Shift);

	if (p->ActualDither)
		if (Speed)
			I2C(MOVL_STOFS,R7,SP,OFS(stack,Step)); // save backup
		else
			I1C(MOVI,R7,-p->Shift);

	Loop = Label(1);

	Left = R2;
	if (p->UseLeft)
		Right = R3;
	else
		Right = R2;

	if (Speed)
	{
		I2(MOV,R8,R0);
		IShift(R0,NONE,-8);
		IShift(R0,NONE,p->SrcShift);

		switch (p->Src.Bits)
		{
		case 8:
			if (p->UseLeft)
				I2(MOVB_LDR0,R9,Left);
			if (p->UseRight)
				I2(MOVB_LDR0,R10,Right);

			if (p->SrcUnsigned)	
			{
				I2(EXTUB,R2,R2);
				if (p->SrcChannels>1)
					I2(EXTUB,R3,R3);
			}
			break;

		case 16:
			if (p->UseLeft)
				I2(MOVW_LDR0,R9,Left);
			if (p->UseRight)
				I2(MOVW_LDR0,R10,Right);

			if (p->SrcUnsigned)	
			{
				I2(EXTUW,R2,R2);
				if (p->SrcChannels>1)
					I2(EXTUW,R3,R3);
			}
			break;

		case 32:
			if (p->UseLeft)
				I2(MOVL_LDR0,R9,Left);
			if (p->UseRight)
				I2(MOVL_LDR0,R10,Right);
			break;
		}

		I2(ADD,R7,R8);
	}
	else
	{
		switch (p->Src.Bits)
		{
		case 8:
			if (p->UseLeft)
			{
				I2(MOVB_LDADD,R9,Left);
				if (SrcPacked && !p->UseRight) I1C(ADDI,R9,1);
			}
			if (p->UseRight)
				if (SrcPacked)
				{
					if (!p->UseLeft) I1C(ADDI,R9,1);
					I2(MOVB_LDADD,R9,Right);
				}
				else
					I2(MOVB_LDADD,R10,Right);

			if (p->SrcUnsigned)	
			{
				I2(EXTUB,R2,R2);
				if (p->SrcChannels>1)
					I2(EXTUB,R3,R3);
			}
			break;

		case 16:
			if (p->UseLeft)
			{
				I2(MOVW_LDADD,R9,Left);
				if (SrcPacked && !p->UseRight) I1C(ADDI,R9,2);
			}
			if (p->UseRight)
				if (SrcPacked)
				{
					if (!p->UseLeft) I1C(ADDI,R9,2);
					I2(MOVW_LDADD,R9,Right);
				}
				else
					I2(MOVW_LDADD,R10,Right);

			if (p->SrcUnsigned)	
			{
				I2(EXTUW,R2,R2);
				if (p->SrcChannels>1)
					I2(EXTUW,R3,R3);
			}
			break;

		case 32:
			if (p->UseLeft)
			{
				I2(MOVL_LDADD,R9,Left);
				if (SrcPacked && !p->UseRight) I1C(ADDI,R9,4);
			}
			if (p->UseRight)
				if (SrcPacked)
				{
					if (!p->UseLeft) I1C(ADDI,R9,4);
					I2(MOVL_LDADD,R9,Right);
				}
				else
					I2(MOVL_LDADD,R10,Right);
			break;
		}
	}

	if (p->InstSrcUnsigned)
	{
		MB(); I1P(MOVL_PC,R0,p->InstSrcUnsigned,0);
		I2(SUB,R0,R2);
		if (p->SrcChannels>1)
			I2(SUB,R0,R3);
	}
	else
	if (p->SrcUnsigned == 128) 
	{
		I1C(ADDI,R2,-128);
		if (p->SrcChannels>1)
			I1C(ADDI,R3,-128);
	}

	if (p->Stereo)
	{
		if ((p->Src.Flags ^ p->Dst.Flags) & PCM_SWAPPEDSTEREO)
		{
			Left = R3;
			Right = R2;
		}
		else
		{
			Left = R2;
			Right = R3;
		}
	}
	else
	{
		if (p->Join)
			I2(ADD,R3,R2);
		Right = Left = R2;
	}


	if (p->ActualDither)
	{
		I2(ADD,R13,R2);
		I2(MOV,R2,R13);
		if (p->Stereo) 
		{
			I2(ADD,R1,R3);
			I2(MOV,R3,R1);
		}
	}

	if (p->Clip>0)
	{
		dyninst* ClipMin = Label(0);
		dyninst* ClipMax = Label(0);

		I2(CMPGT,R2,R4);
		I0P(BFS,ClipMin);
		I2(CMPGT,R5,R2); // delay slot
		I2(MOV,R4,R2);
		InstPost(ClipMin);

		I0P(BFS,ClipMax);
		I1C(ADDI,R11,1<<p->DstShift); // delay slot
		I2(MOV,R5,R2);
		InstPost(ClipMax);

		if (p->Stereo)
		{
			ClipMin = Label(0);
			ClipMax = Label(0);

			I2(CMPGT,R3,R4);
			I0P(BFS,ClipMin);
			I2(CMPGT,R5,R3); // delay slot
			I2(MOV,R4,R3);
			InstPost(ClipMin);

			I0P(BFS,ClipMax);
			I1C(ADDI,R12,1<<p->DstShift); // delay slot
			I2(MOV,R5,R3);
			InstPost(ClipMax);
		}
		else
		if (p->Dst.Channels>1)
			I1C(ADDI,R12,1<<p->DstShift); // delay slot
	}
	else
	{
		I1C(ADDI,R11,1<<p->DstShift);
		if (p->Dst.Channels>1)
			I1C(ADDI,R12,1<<p->DstShift);
	}

	I2(CMPEQ,R11,R14);

	if (p->Shift)
	{
		if (p->ActualDither)
		{
			I1C(MOVI,R0,p->Shift);
			Shift = R0;
		}
		else
			Shift = R13;
		I2(SHAD,Shift,R2);
		if (p->Stereo) I2(SHAD,Shift,R3);
	}

	if (p->ActualDither)
	{
		if (Speed)
			I1C(MOVI,R7,-p->Shift);

		I2(MOV,R2,R0);
		I2(SHAD,R7,R0);
		I2(SUB,R0,R13);
		if (p->Stereo)
		{
			I2(MOV,R3,R0);
			I2(SHAD,R7,R0);
			I2(SUB,R0,R1);
		}

		if (Speed)
			I2C(MOVL_LDOFS,SP,R7,OFS(stack,Step));
	}

	if (p->InstDstUnsigned)
	{
		MB(); I1P(MOVL_PC,R0,p->InstDstUnsigned,0);
		I1C(ADD,R0,Left);
		if (Left != Right)
			I1C(ADD,R0,Right);
	}
	else
	if (p->DstUnsigned == 128) 
	{
		// p->Dst.Bits=8 so doesn't matter if it's add or sub
		I1C(ADDI,Left,-128); 
		if (Left != Right)
			I1C(ADDI,Right,-128);
	}

	switch (p->Dst.Bits)
	{
	case 8:
		I2(MOVB_ST,Left,R11);
		if (p->Dst.Channels>1)
			I2(MOVB_ST,Right,R12);
		break;

	case 16:
		I2(MOVW_ST,Left,R11);
		if (p->Dst.Channels>1)
			I2(MOVW_ST,Right,R12);
		break;

	case 32:
		I2(MOVL_ST,Left,R11);
		if (p->Dst.Channels>1)
			I2(MOVL_ST,Right,R12);
		break;
	}

	DS(); I0P(BFS,Loop);

	if (p->ActualDither)
	{
		I2(MOV,SP,R0);
		I1C(ADDI,R0,OFS(stack,State));
		I2(MOVL_LD,R0,R4);

		I2C(MOVL_STOFS,R13,R4,OFS(pcmstate,Dither[0]));
		I2C(MOVL_STOFS,R1,R4,OFS(pcmstate,Dither[1]));
	}

	CodeEnd(7,OFS(stack,StackFrame));
}

void PCMCompile(pcm_soft* p)
{
	dyninst* Speed;
	dyninst* Min = NULL;
	dyninst* Max = NULL;

	CodeBegin(7,OFS(stack,StackFrame));

	p->InstSrcUnsigned = NULL;
	p->InstDstUnsigned = NULL;

	if (p->SrcUnsigned && p->SrcUnsigned != 128)
		p->InstSrcUnsigned = InstCreate32(p->SrcUnsigned,NONE,NONE,NONE,0,0);
	if (p->DstUnsigned && p->DstUnsigned != 128)
		p->InstDstUnsigned = InstCreate32(p->DstUnsigned,NONE,NONE,NONE,0,0);

	// dst pointers
	I2C(MOVL_LDOFS,R5,R11,0);
	I1C(ADDI,R11,-(1<<p->DstShift)); // back one step

	if (p->Dst.Channels > 1)
	{
		if (p->Dst.Flags & PCM_PLANES)
		{
			I2C(MOVL_LDOFS,R5,R12,4);
			I1C(ADDI,R12,-(1<<p->DstShift)); // back one step
		}
		else
		{
			I2(MOV,R11,R12);
			I1C(ADDI,R12,1<<(p->DstShift-1));
		}
	}
	I2(MOV,R7,R14);
	I2(ADD,R11,R14); // dstend

	// src pointers
	I2C(MOVL_LDOFS,R6,R9,0);

	if (p->Src.Channels > 1)
	{
		if (p->Src.Flags & PCM_PLANES)
			I2C(MOVL_LDOFS,R6,R10,4);
		else
		{
			I2(MOV,R9,R10);
			I1C(ADDI,R10,p->Src.Bits>>3);
		}
	}

	I2(MOV,SP,R0);
	I1C(ADDI,R0,OFS(stack,State));
	I2(MOVL_LD,R0,R4);

	I2C(MOVL_LDOFS,R4,R7,OFS(pcmstate,Step));
	I2C(MOVL_LDOFS,R4,R8,OFS(pcmstate,Pos));

	/*
	if (p->UseVolume)
	{
		I1C(ADDI,R0,OFS(stack,Volume)-OFS(stack,State));
		I2(MOVL_LD,R0,R1);
	}
	*/

	if (p->ActualDither)
	{
		I2C(MOVL_LDOFS,R4,R13,OFS(pcmstate,Dither[0]));
		I2C(MOVL_LDOFS,R4,R1,OFS(pcmstate,Dither[1]));
	}

	if (p->Clip>0)
	{
		Min = InstCreate32(p->MinLimit,NONE,NONE,NONE,0,0);
		Max = InstCreate32(p->MaxLimit,NONE,NONE,NONE,0,0);

		I1P(MOVL_PC,R4,Min,0);
		I1P(MOVL_PC,R5,Max,0);
	}

	Speed = Label(0);

	I1C(MOVI,R2,64);
	I1(SHLL2,R2);
	I2(CMPEQ,R2,R7);
	I0P(BF,Speed);

	PCMLoop(p,0);

	InstPost(Speed);

	PCMLoop(p,1);

	Align(4);
	if (Min) InstPost(Min);
	if (Max) InstPost(Max);
	if (p->InstSrcUnsigned) InstPost(p->InstSrcUnsigned);
	if (p->InstDstUnsigned) InstPost(p->InstDstUnsigned);
}

#endif
