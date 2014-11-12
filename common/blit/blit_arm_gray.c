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
 * $Id: blit_arm_gray.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(ARM)

typedef struct stack
{
	int EndOfRect;
	int DstPitch; 
	int DstNext;
	int SrcNext;

	int StackFrame[STACKFRAME];

	//void* this   R0
	//char* Dst    R1
	//char* Src    R2
	//int DstPitch R3 can be signed
	int SrcPitch; //can be signed
	int Width; 
	int Height;
	int Src2SrcLast;
} stack;

void Fix_Gray_UV(blit_soft* p)
{
	int SrcBlock2;
	bool_t Invert = (p->Dst.Flags & PF_INVERTED) != 0;
	int DitherMask = (1 << (8 - p->DstBPP))-1;
	int ValueMask = 255 - DitherMask;
	dyninst* LoopX;
	dyninst* LoopY;

	p->DstAlignPos = 4;
	p->DstAlignSize = 4;
	p->Caps = VC_BRIGHTNESS|VC_DITHER;

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2(LDR,R9,R1);  //Dst[0]
	I2(LDR,R12,R2); //Src[0]

	I3(MOV,R10,NONE,R3); //DstPitch
	I2C(LDR,R0,SP,OFS(stack,Height));
	I2C(LDR,R8,SP,OFS(stack,Width));
	I2C(LDR,R14,SP,OFS(stack,SrcPitch));

	if (p->SwapXY)
	{
		SrcBlock2 = -p->DstBPP2-p->DstDoubleX;

		I2C(MOV,R1,NONE,p->DstBPP * p->DirX);
		I3(MUL,R0,R1,R0);
		I3S(ADD,R11,R14,R14,LSL,1); //SrcPitch*3
		I3S(ADD,R0,R9,R0,ASR,3);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = DirX - Width * DstPitch;
		MB(); I3(MUL,R2,R10,R8);
		I2C(MOV,R0,NONE,p->DirX);
		I3(SUB,R0,R0,R2);		  
		I2C(STR,R0,SP,OFS(stack,DstNext));
	}
	else
	{
		SrcBlock2 = 0;

		I3(MUL,R0,R10,R0);
		I3(ADD,R0,R9,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = (DstPitch << DstDoubleY) - DirX * (Width >> -DstBPP2);
		I3S(MOV,R2,NONE,R10,LSL,p->DstDoubleY);
		I3S(p->DirX>0 ? SUB:ADD,R2,R2,R8,LSR,-p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}

	//SrcNext = (SrcPitch << SrcBlock2) - (Width >> SrcDoubleX);
	I3S(MOV,R1,NONE,R14,LSL,SrcBlock2);
	I3S(SUB,R1,R1,R8,LSR,p->SrcDoubleX); 
	I2C(STR,R1,SP,OFS(stack,SrcNext));

	if (p->FX.Flags & BLITFX_DITHER)
		I2C(MOV,R4,NONE,DitherMask >> 1); //Y0

	LoopY = Label(1);

	//R8 width
	//R4 dither

	if (p->SwapXY)
	{
		I3(MUL,R8,R10,R8);
		I3(ADD,R7,R9,R8);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R7,R9,R8,LSL,p->DstBPP2);
		else
			I3S(SUB,R7,R9,R8,LSL,p->DstBPP2);
	}

	LoopX = Label(1);
	{
		//R0..R3 y
		//R8 temp
		//R4 dither
		//R5 out
		//R6 out2 (doubley)

		//R7 EndOfLine
		//R9 Dst
		//R10 DstPitch
		//R12 Src
		//R14 SrcPitch
		//R11 SrcPitch*3

		int n,Count = 8 / p->DstBPP;
		for (n=0;n<Count;++n)
		{
			bool_t Reverse;
			int Pos;
			reg Tmp;

			reg Y = (reg)(R0+(n%4));
			MB(); Byte(); 
			if (p->SwapXY)
			{
				switch ((Count-1-n) >> p->DstDoubleX)
				{
				case 0: if (n==Count-1) I2C(LDR_POST,Y,R12,1); else I2(LDR,Y,R12); break;
				case 1: I3(LDR,Y,R12,R14); break;
				case 2: I3S(LDR,Y,R12,R14,LSL,1); break;
				case 3: I3(LDR,Y,R12,R11); break;
				} 
			}
			else
			{
				if (p->DstDoubleX && !(n&1))
					I2(LDR,Y,R12);
				else
					I2C(LDR_POST,Y,R12,1);
			}
			if (p->FX.Flags & BLITFX_DITHER)
			{
				I3(ADD,R4,R4,Y);
				Y = R4;
			}
			S(); I2C(ADD,Y,Y,p->FX.Brightness-16);
			C(MI); I2C(MOV,Y,NONE,0);
			C(PL); I2C(CMP,NONE,Y,255);
			C(GT); I2C(MOV,Y,NONE,255);

			Reverse = (p->DirX>0) ^ ((p->SwapXY)!=0);
			Pos = Reverse ? n*p->DstBPP : (8-(n+1)*p->DstBPP);
			Tmp = (reg)((!Invert && n==0 && Pos==0) ? R5 : R8);
			I2C(AND,Tmp,Y,ValueMask);

			if (n==0)
			{
				if (Tmp != R5)
					I3S(Invert ? MVN:MOV,R5,NONE,Tmp,LSR,Pos);
			}
			else
				I3S(EOR,R5,R5,Tmp,LSR,Pos);

			if (p->DstDoubleY)
			{
				int Pos2 = Reverse ? (n^p->DstDoubleX)*p->DstBPP : (8-((n^p->DstDoubleX)+1)*p->DstBPP);
				if (n==0)
					I3S(Invert ? MVN:MOV,R6,NONE,Tmp,LSR,Pos2);
				else
					I3S(EOR,R6,R6,Tmp,LSR,Pos2);
			}

			if (p->FX.Flags & BLITFX_DITHER)
				I2C(AND,R4,R4,DitherMask);
		}

		if (p->DstDoubleY)
		{
			MB(); Byte();
			if (p->SwapXY)
				I3(STR_POST,R6,R9,R10);
			else
				I3(STR,R6,R9,R10);
		}

		MB(); Byte();
		if (p->SwapXY)
			I3(STR_POST,R5,R9,R10);
		else
			I2C(STR_POST,R5,R9,p->DirX>0 ? 1:-1);

		I3(CMP,NONE,R9,R7);
		I0P(B,NE,LoopX);
	}

	I2C(LDR,R0,SP,OFS(stack,SrcNext));
	I2C(LDR,R8,SP,OFS(stack,DstNext));
	I2C(LDR,R5,SP,OFS(stack,EndOfRect));
	
	//increment pointers
	I3(ADD,R12,R12,R0);
	I3(ADD,R9,R9,R8);

	//prepare for next row
	I2C(LDR,R8,SP,OFS(stack,Width));

	I3(CMP,NONE,R9,R5);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();
}

#endif
