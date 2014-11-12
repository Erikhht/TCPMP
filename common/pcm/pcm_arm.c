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
 * $Id: pcm_arm.c 304 2005-10-20 11:02:59Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "pcm_soft.h"

#if defined(ARM)

typedef struct stack
{
	int StackFrame[STACKFRAME];

	//void* this   R0
	//plane Dst    R1
	//plane Src    R2
	//int DstLength R3
	pcmstate* State;
	int Volume;

} stack;

// R0,R1 dither 
// R2,R3 value
// R4 Min
// R5 Max
// R6 tmp
// R7 Step
// R8 Pos
// R9 SrcLeft
// R10 SrcRight
// R11 DstLeft
// R12 DstRight
// R14 DstLeftEnd

void PCMLoop(pcm_soft* p,bool_t Speed)
{
	dyninst* Loop;
	reg Left;
	reg Right;

	Loop = Label(1);

	Left = R2;
	if (p->UseLeft)
		Right = R3;
	else
		Right = R2;

	if (Speed)
	{
		I3S(MOV,R3,NONE,R8,LSR,8);

		switch (p->Src.Bits)
		{
		case 8:
			if (p->SrcUnsigned)	
			{
				if (p->UseLeft)
				{
					Byte();	I3S(LDR,Left,R9,R3,LSL,p->SrcShift);
				}
				if (p->UseRight)
				{
					Byte();	I3S(LDR,Right,R10,R3,LSL,p->SrcShift);
				}
			}
			else
			{
				I3S(MOV,R3,NONE,R3,LSL,p->SrcShift);
				if (p->UseLeft)
				{
					SByte(); I3(LDR,Left,R9,R3);
				}
				if (p->UseRight)
				{
					SByte(); I3(LDR,Right,R10,R3);
				}
			}
			break;

		case 16:
			I3S(MOV,R3,NONE,R3,LSL,p->SrcShift);
			if (p->UseLeft)
			{
				if (p->SrcUnsigned || p->SrcSwap) Half(); else SHalf();
				I3(LDR,Left,R9,R3);
				if (p->SrcSwap)
				{
					I3S(MOV,R6,NONE,Left,LSL,24);
					I3S(MOV,Left,NONE,Left,LSR,8);
					if (p->SrcUnsigned)
						I3S(ORR,Left,Left,R6,LSR,16);
					else
						I3S(ORR,Left,Left,R6,ASR,16);
				}
			}
			if (p->UseRight)
			{
				if (p->SrcUnsigned || p->SrcSwap) Half();	else SHalf();
				I3(LDR,Right,R10,R3);
				if (p->SrcSwap)
				{
					I3S(MOV,R6,NONE,Right,LSL,24);
					I3S(MOV,Right,NONE,Right,LSR,8);
					if (p->SrcUnsigned)
						I3S(ORR,Right,Right,R6,LSR,16);
					else
						I3S(ORR,Right,Right,R6,ASR,16);
				}
			}
			break;

		case 32:
			if (p->UseLeft)
				I3S(LDR,Left,R9,R3,LSL,p->SrcShift);
			if (p->UseRight)
				I3S(LDR,Right,R10,R3,LSL,p->SrcShift);
			break;
		}
		I3(ADD,R8,R8,R7);
	}
	else
	{
		switch (p->Src.Bits)
		{
		case 8:
			if (p->UseLeft)
			{
				if (p->SrcUnsigned)	Byte();	else SByte();
				I2C(LDR_POST,Left,R9,1<<p->SrcShift);
			}
			if (p->UseRight)
			{
				if (p->SrcUnsigned)	Byte();	else SByte();
				I2C(LDR_POST,Right,R10,1<<p->SrcShift);
			}
			break;

		case 16:
			if (p->UseLeft)
			{
				if (p->SrcUnsigned || p->SrcSwap) Half(); else SHalf();
				I2C(LDR_POST,Left,R9,1<<p->SrcShift);
				if (p->SrcSwap)
				{
					I3S(MOV,R6,NONE,Left,LSL,24);
					I3S(MOV,Left,NONE,Left,LSR,8);
					if (p->SrcUnsigned)
						I3S(ORR,Left,Left,R6,LSR,16);
					else
						I3S(ORR,Left,Left,R6,ASR,16);
				}
			}
			if (p->UseRight)
			{
				if (p->SrcUnsigned || p->SrcSwap) Half(); else SHalf();
				I2C(LDR_POST,Right,R10,1<<p->SrcShift);
				if (p->SrcSwap)
				{
					I3S(MOV,R6,NONE,Right,LSL,24);
					I3S(MOV,Right,NONE,Right,LSR,8);
					if (p->SrcUnsigned)
						I3S(ORR,Right,Right,R6,LSR,16);
					else
						I3S(ORR,Right,Right,R6,ASR,16);
				}
			}
			break;

		case 32:
			if (p->UseLeft)
				I2C(LDR_POST,Left,R9,1<<p->SrcShift);
			if (p->UseRight)
				I2C(LDR_POST,Right,R10,1<<p->SrcShift);
			break;
		}
	}

	if (p->SrcUnsigned) 
	{
		I2C(SUB,R2,R2,p->SrcUnsigned);
		if (p->SrcChannels>1)
			I2C(SUB,R3,R3,p->SrcUnsigned);
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
			I3(ADD,R2,R2,R3);
		Right = Left = R2;
	}

	if (p->ActualDither)
	{
		I3(ADD,R2,R2,R0);
		I3(MOV,R0,NONE,R2);
		if (p->Stereo) 
		{
			I3(ADD,R3,R3,R1);
			I3(MOV,R1,NONE,R3);
		}
	}

	if (p->UseVolume)
	{
		if (p->Src.Bits >= 23)
			I3S(MOV,R2,NONE,R2,ASR,8);
		I3(MUL,R2,R0,R2);

		if (p->Stereo)
		{
			if (p->Src.Bits >= 23)
				I3S(MOV,R3,NONE,R3,ASR,8);
			I3(MUL,R3,R0,R3);
		}
	}

	if (p->Clip>0)
	{
		I3(CMP,NONE,R2,R4);
		C(LT); I3(MOV,R2,NONE,R4);
		I3(CMP,NONE,R2,R5);
		C(GT); I3(MOV,R2,NONE,R5);

		if (p->Stereo)
		{
			I3(CMP,NONE,R3,R4);
			C(LT); I3(MOV,R3,NONE,R4);
			I3(CMP,NONE,R3,R5);
			C(GT); I3(MOV,R3,NONE,R5);
		}
	}

	if (p->Shift)
	{
		I3S(MOV,R2,NONE,R2,ASR,-p->Shift);
		if (p->Stereo) I3S(MOV,R3,NONE,R3,ASR,-p->Shift);
	}

#ifdef TARGET_PALMOS
	if (p->LifeDriveFix)
	{
		I3S(ADD,R6,R2,R1,LSL,1);
		I3S(CMP,NONE,R6,R1,LSL,2);
		C(LS); I2C(ORR,R2,R2,R0);
		if (p->Stereo)
		{
			I3S(ADD,R6,R3,R1,LSL,1);
			I3S(CMP,NONE,R6,R1,LSL,2);
			C(LS); I3(ORR,R3,R3,R0);
		}
	}
#endif

	if (p->ActualDither)
	{
		I3S(SUB,R0,R0,R2,ASR,p->Shift);
		if (p->Stereo) I3S(SUB,R1,R1,R3,ASR,p->Shift);
	}

	if (p->DstUnsigned) 
	{
		I2C(ADD,Left,Left,p->DstUnsigned);
		if (Left != Right)
			I2C(ADD,Right,Right,p->DstUnsigned);
	}

	switch (p->Dst.Bits)
	{
	case 8:
		Byte(); I2C(STR_POST,Left,R11,1<<p->DstShift);
		if (p->Dst.Channels>1)
		{
			Byte(); I2C(STR_POST,Right,R12,1<<p->DstShift);
		}
		break;

	case 16:
		Half(); I2C(STR_POST,Left,R11,1<<p->DstShift);
		if (p->Dst.Channels>1)
		{
			Half(); I2C(STR_POST,Right,R12,1<<p->DstShift);
		}
		break;

	case 32:
		I2C(STR_POST,Left,R11,1<<p->DstShift);
		if (p->Dst.Channels>1)
			I2C(STR_POST,Right,R12,1<<p->DstShift);
		break;
	}

	I3(CMP,NONE,R11,R14);
	I0P(B,NE,Loop);
	
	if (p->ActualDither)
	{
		I2C(LDR,R4,SP,OFS(stack,State));
		I2C(STR,R0,R4,OFS(pcmstate,Dither[0]));
		I2C(STR,R1,R4,OFS(pcmstate,Dither[1]));
	}

	CodeEnd();
}

void PCMCompile(pcm_soft* p)
{
	dyninst* Speed;
	dyninst* Min = NULL;
	dyninst* Max = NULL;

	CodeBegin();

	// dst pointers
	I2C(LDR,R11,R1,0);
	if (p->Dst.Channels > 1)
	{
		if (p->Dst.Flags & PCM_PLANES)
			I2C(LDR,R12,R1,4);
		else
			I2C(ADD,R12,R11,1<<(p->DstShift-1));
	}
	I3(ADD,R14,R11,R3); // dstend

	// src pointers
	I2C(LDR,R9,R2,0);
	if (p->Src.Channels > 1)
	{
		if (p->Src.Flags & PCM_PLANES)
			I2C(LDR,R10,R2,4);
		else
			I2C(ADD,R10,R9,p->Src.Bits>>3);
	}

	I2C(LDR,R4,SP,OFS(stack,State));

	I2C(LDR,R7,R4,OFS(pcmstate,Step));
	I2C(LDR,R8,R4,OFS(pcmstate,Pos));

#ifdef TARGET_PALMOS
	if (p->LifeDriveFix)
	{
		I2C(LDR,R1,SP,OFS(stack,Volume));
		I2C(MOV,R0,NONE,63);
		I2C(CMP,NONE,R1,25);
		C(GT); I2C(MOV,R0,NONE,31);
		I2C(CMP,NONE,R1,50);
		C(GT); I2C(MOV,R0,NONE,15);
		I2C(CMP,NONE,R1,100);
		C(GT); I2C(MOV,R0,NONE,7);
		I2C(CMP,NONE,R1,200);
		C(GT); I2C(MOV,R0,NONE,3);
		I2C(ADD,R1,R0,1);
	}
#endif

	if (p->UseVolume)
		I2C(LDR,R0,SP,OFS(stack,Volume));

	if (p->ActualDither)
	{
		I2C(LDR,R0,R4,OFS(pcmstate,Dither[0]));
		I2C(LDR,R1,R4,OFS(pcmstate,Dither[1]));
	}

	if (p->Clip>0)
	{
		Min = InstCreate32(p->MinLimit,NONE,NONE,NONE,0,0);
		Max = InstCreate32(p->MaxLimit,NONE,NONE,NONE,0,0);

		I1P(LDR,R4,Min,0);
		I1P(LDR,R5,Max,0);
	}

	Speed = Label(0);
	I2C(CMP,NONE,R7,256);
	I0P(B,NE,Speed);

	PCMLoop(p,0);

	InstPost(Speed);

	PCMLoop(p,1);

	if (Min) InstPost(Min);
	if (Max) InstPost(Max);
}


#endif
