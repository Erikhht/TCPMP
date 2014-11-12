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
 * $Id: blit_sh3_gray.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(SH3)

typedef struct stack
{
	int EndOfRect;
	int	DstNext;
	int SrcNext;
	int AdjWidth;

	int StackFrame[7];

	void* This;   //R4
	char* Dst;    //R5
	char* Src;    //R6
	int DstPitch; //R7
	int SrcPitch;
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
	int Count = 8 / p->DstBPP;
	int DstRows = (p->DstDoubleY && !(p->SwapXY)) ? 2:1;
	int SrcRows = (p->SwapXY) ? (Count >> p->DstDoubleX) : 1; 
	int n;
	dyninst* LoopY;
	dyninst* LoopX;

	p->DstAlignPos = 4;
	p->DstAlignSize = 4;
	p->Caps = VC_BRIGHTNESS|VC_DITHER;

	CodeBegin(7,OFS(stack,StackFrame));

	I2(MOVL_LD,R5,R2);	//Dst[0]
	I2(MOVL_LD,R6,R4);	//Src[0]
	I2(MOV,R7,R8);

	I1C(MOVI,R0,OFS(stack,This));
	I2(ADD,SP,R0);
	I2C(MOVL_LDOFS,R0,R14,OFS(stack,SrcPitch)-OFS(stack,This)); //SrcPitch
	I2C(MOVL_LDOFS,R0,R7,OFS(stack,Height)-OFS(stack,This));
	I2C(MOVL_LDOFS,R0,R1,OFS(stack,Width)-OFS(stack,This));

	if (p->SwapXY)
	{
		SrcBlock2 = -p->DstBPP2 - p->DstDoubleX;

		//EndOfRect = Dst + (Height * DstBPP * DirX) >> 3;
		I1C(MOVI,R9,p->DstBPP * p->DirX);
		I2(MULL,R7,R9);
		I1C(MOVI,R9,-3);
		I1(STS_MACL,R7);
		I2(SHAD,R9,R7);
		I2(ADD,R2,R7);
		I2C(MOVL_STOFS,R7,SP,OFS(stack,EndOfRect));
		
		//DstNext = DirX - Width * DstPitch;
		I2(MULL,R1,R8);
		I1C(MOVI,R9,p->DirX);
		I1(STS_MACL,R7);
		I2(SUB,R7,R9);
		I2C(MOVL_STOFS,R9,SP,OFS(stack,DstNext));
	}
	else
	{
		SrcBlock2 = 0;

		//EndOfRect = Dst + DstPitch * Height
		I2(MULL,R7,R8);
		I1(STS_MACL,R7);
		I2(ADD,R2,R7);
		I2C(MOVL_STOFS,R7,SP,OFS(stack,EndOfRect));

		//DstNext = (DstPitch << DstDoubleY) - DirX * (Width >> -DstBPP2);
		I1C(MOVI,R7,p->DstBPP2);
		I2(MOV,R1,R9);
		I2(SHAD,R7,R9);
		I2(MOV,R8,R7);
		if (p->DstDoubleY) I1(SHLL,R7);
		I2(p->DirX>0 ? SUB:ADD,R9,R7);
		I2C(MOVL_STOFS,R7,SP,OFS(stack,DstNext));
	}

	//SrcNext = (SrcPitch << SrcBlock2) - (Width >> SrcDoubleX);
	I2(MOV,R14,R7);
	if (SrcBlock2) 
	{
		I1C(MOVI,R9,SrcBlock2);
		I2(SHLD,R9,R7);
	}
	I2(MOV,R1,R9);
	if (p->SrcDoubleX) I1(SHLR,R9);
	I2(SUB,R9,R7); 
	I2C(MOVL_STOFS,R7,SP,OFS(stack,SrcNext));

	if (p->FX.Flags & BLITFX_DITHER)
		I1C(MOVI,R13,DitherMask >> 1); //Y0

	for (n=1;n<SrcRows;++n)
	{
		I2(MOV,(reg)(R4+n-1),(reg)(R4+n));
		I2(ADD,R14,(reg)(R4+n));
	}
	for (n=1;n<DstRows;++n)
	{
		I2(MOV,(reg)(R2+n-1),(reg)(R2+n));
		I2(ADD,R8,(reg)(R2+n));
	}

	if (p->SwapXY)
	{
		I2(MULL,R1,R8); //width*dstpitch
		I1(STS_MACL,R1);
	}
	else
	{
		if (p->DirX < 0)
			I2(NEG,R1,R1);
		I1C(MOVI,R7,p->DstBPP2);
		I2(SHAD,R7,R1);
	}
	I2C(MOVL_STOFS,R1,SP,OFS(stack,AdjWidth));
	I1C(MOVI,R0,(int8_t)ValueMask);
	I2(EXTUB,R0,R9);
	if (p->FX.Flags & BLITFX_DITHER)
		I1C(MOVI,R10,DitherMask);

	LoopY = Label(1);

	I2C(MOVL_LDOFS,SP,R1,OFS(stack,AdjWidth));
	I2(ADD,R2,R1); //end of line

	//R0 temp
	//R12 temp
	//R13 dither
	//R14 out
	//R11 out2 (doubley)
	
	//R1 Endofline
	//R2 Dst
	//R3 Dst2 (doubley)
	//R4 Src
	//R5 Src2 (Swap)
	//R6 Src3 (Swap)
	//R7 Src4 (Swap)
	//R8 DstPitch
	//R9 ValueMask
	//R10 DitherMask

	LoopX = Label(1);
	{
		for (n=0;n<Count;++n)
		{
			int Pos;
			reg Y;
			bool_t Reverse;
			reg Tmp;

			if (p->SwapXY)
			{
				switch ((Count-1-n) >> p->DstDoubleX)
				{
				case 0: I2(MOVB_LD,R4,R0); break;
				case 1: I2(MOVB_LD,R5,R0); break;
				case 2: I2(MOVB_LD,R6,R0); break;
				case 3: I2(MOVB_LD,R7,R0); break;
				} 
			}
			else
				I1C(MOVB_LDOFS,R4,n >> p->DstDoubleX);

			I2(EXTUB,R0,R0);

			Y=R0;
			if (p->FX.Flags & BLITFX_DITHER)
			{
				I2(ADD,R0,R13);
				Y = R13;
			}
			I1C(ADDI,Y,p->FX.Brightness-16);

			I2(MOV,Y,R12);
			I1(SHLR16,R12);
			I2(SHLD,R12,Y);

			I2(MOV,Y,R12);
			I1(SHLR8,R12);
			I2(NEG,R12,R12);
			I2(OR,R12,Y);

			Reverse = (p->DirX>0) ^ ((p->SwapXY)!=0);
			Pos = Reverse ? n*p->DstBPP : (8-(n+1)*p->DstBPP);
			Tmp = R12;
			if (!Invert && n==0 && Pos==0) 
				Tmp = R14;

			I2(MOV,Y,Tmp);
			I2(AND,R9,Tmp);
			if (Pos)
			{
				I1C(MOVI,R0,-Pos);
				I2(SHLD,R0,Tmp);
			}
			if (n==0)
			{
				if (Invert || Pos)
					I2(Invert ? NOT:MOV,Tmp,R14);
			}
			else
				I2(XOR,Tmp,R14);

			if (p->DstDoubleY)
			{
				int Pos2 = Reverse ? (n^p->DstDoubleX)*p->DstBPP : (8-((n^p->DstDoubleX)+1)*p->DstBPP);
				if (Tmp != R12)
					I2(MOV,Tmp,R12);
				I1C(MOVI,R0,Pos-Pos2);
				I2(SHLD,R0,R12);

				if (n==0)
					I2(Invert ? NOT:MOV,R12,R11);
				else
					I2(XOR,R12,R11);
			}

			if (p->FX.Flags & BLITFX_DITHER)
				I2(AND,R10,R13);
		}

		if (p->DstDoubleY)
		{
			MB(); 
			if (p->SwapXY)
			{
				I2(MOVB_ST,R11,R2);
				I2(ADD,R8,R2);
			}
			else
			{
				I2(MOVB_ST,R11,R3);
				I1C(ADDI,R3,p->DirX>0 ? 1:-1);
			}
		}

		MB(); I2(MOVB_ST,R14,R2);

		if (p->SwapXY)
		{
			I2(ADD,R8,R2);
			for (n=0;n<SrcRows;++n)
				I1C(ADDI,(reg)(R4+n),1);
		}
		else
		{
			I1C(ADDI,R2,p->DirX>0 ? 1:-1);
			I1C(ADDI,R4,Count >> p->DstDoubleX);
		}

		MB(); I2(CMPEQ,R2,R1); // has to go back at least one instruction (because of delay slot)
		DS(); I0P(BFS,LoopX);
	}

	//increment pointers
	I2C(MOVL_LDOFS,SP,R0,OFS(stack,DstNext));
	I2C(MOVL_LDOFS,SP,R1,OFS(stack,SrcNext));
	I2C(MOVL_LDOFS,SP,R12,OFS(stack,EndOfRect));

	for (n=0;n<DstRows;++n)
		I2(ADD,R0,(reg)(R2+n));

	for (n=0;n<SrcRows;++n)
		I2(ADD,R1,(reg)(R4+n));

	MB(); I2(CMPEQ,R2,R12); // has to go back at least one instruction (because of delay slot)
	DS(); I0P(BFS,LoopY);

	CodeEnd(7,OFS(stack,StackFrame));
}

#endif
