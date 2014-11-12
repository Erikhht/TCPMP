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
 * $Id: blit_mips_gray.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(MIPS)

//	EndOfLine
//	EndOfRect
//	DstPitch 
//	DstNext
//	SrcNext
//	UVNext

typedef struct stack
{
	int StackFrame[4];

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
	dyninst* LoopX;
	dyninst* LoopY;
	int SrcBlock2;
	reg b,a;
	bool_t Invert = (p->Dst.Flags & PF_INVERTED) != 0;
	int DitherMask = (1 << (8 - p->DstBPP))-1;
	int ValueMask = 255 - DitherMask;
	int Count = 8 / p->DstBPP;
	int DstRows = (p->DstDoubleY && !(p->SwapXY)) ? 2:1;
	int SrcRows = (p->SwapXY) ? (Count >> p->DstDoubleX) : 1; 
	int n;

	p->DstAlignPos = 4;
	p->DstAlignSize = 4;
	p->Caps = VC_BRIGHTNESS|VC_DITHER;

	CodeBegin(4,0,0);

	I2C(LW,R2,R5,0);  //Dst[0]
	I2C(LW,R4,R6,0);  //Src[0]
	I3(ADDU,R16,ZERO,R7); //DstPitch

	I2C(LW,R17,SP,OFS(stack,SrcPitch));
	I2C(LW,R8,SP,OFS(stack,Height));
	I2C(LW,R1,SP,OFS(stack,Width));

	if (p->SwapXY)
	{
		SrcBlock2 = -p->DstBPP2 - p->DstDoubleX;

		//EndOfRect = Dst + (Height * DstBPP * DirX) >> 3;
		I2C(ADDIU,R9,ZERO,p->DstBPP * p->DirX);
		I2(MULT,R8,R9);
		I1(MFLO,R8);
		I2C(SRA,R8,R8,3);
		I3(ADDU,R25,R2,R8);
		
		//DstNext = DirX - Width * DstPitch;
		I2(MULT,R1,R16);
		I2C(ADDIU,R19,ZERO,p->DirX);
		I1(MFLO,R8);
		I3(SUBU,R19,R19,R8);		  
	}
	else
	{
		SrcBlock2 = 0;

		//EndOfRect = Dst + DstPitch * Height
		I2(MULT,R16,R8);
		I1(MFLO,R8);
		I3(ADDU,R25,R2,R8);

		//DstNext = (DstPitch << DstDoubleY) - DirX * (Width >> -DstBPP2);
		I2C(SLL,R19,R16,p->DstDoubleY);
		I2C(SRA,R8,R1,-p->DstBPP2);
		I3(p->DirX>0 ? SUBU:ADDU,R19,R19,R8);
	}

	//SrcNext = (SrcPitch << SrcBlock2) - (Width >> SrcDoubleX);
	b = (reg)(p->SrcDoubleX ? R18:R1);
	a = (reg)(SrcBlock2 ? R8:R17);
	if (p->SrcDoubleX) I2C(SRA,R18,R1,p->SrcDoubleX);
	if (SrcBlock2) I2C(SLL,R8,R17,SrcBlock2);
	I3(SUBU,R18,a,b); 

	if (p->FX.Flags & BLITFX_DITHER)
		I2C(ADDIU,R13,ZERO,DitherMask >> 1); //Y0

	for (n=1;n<SrcRows;++n)
		I3(ADDU,(reg)(R4+n),(reg)(R4+n-1),R17);
	for (n=1;n<DstRows;++n)
		I3(ADDU,(reg)(R2+n),(reg)(R2+n-1),R16);

	//R1 Width;
	if (p->SwapXY)
	{
		I2(MULT,R1,R16);
		I1(MFLO,R1);
	}
	else
	{
		if (p->DirX < 0)
			I3(SUBU,R1,ZERO,R1);
		I2C(SRA,R1,R1,-p->DstBPP2);
	}

	LoopY = Label(1);

	I3(ADDU,R24,R2,R1); //end of line

	//R1 adjusted width
	//R8..R11 y
	//R12 temp
	//R13 dither
	//R14 out
	//R15 out2
	//R24 EndOfLine
	//R25 EndOfRect
	//R16 DstPitch
	//R17 SrcPitch
	//R18 SrcNext
	//R19 DstNext

	//R2 Dst
	//R3 Dst2 (Double && !Swap)
	//R4 Src
	//R5 Src2 (Swap)
	//R6 Src3 (Swap)
	//R7 Src4 (Swap)

	LoopX = Label(1);
	{
		for (n=0;n<Count;++n)
		{
			int Pos;
			reg Tmp;
			bool_t Reverse;

			reg Y = (reg)(R8+(n%4));
			MB(); 
			if (p->SwapXY)
			{
				switch ((Count-1-n) >> p->DstDoubleX)
				{
				case 0: I2(LBU,Y,R4); break;
				case 1: I2(LBU,Y,R5); break;
				case 2: I2(LBU,Y,R6); break;
				case 3: I2(LBU,Y,R7); break;
				} 
			}
			else
				I2C(LBU,Y,R4,n >> p->DstDoubleX);

			if (p->FX.Flags & BLITFX_DITHER)
			{
				I3(ADDU,R13,R13,Y);
				Y = R13;
			}
			I2C(ADDIU,Y,Y,p->FX.Brightness-16);

			I2C(SRL,R12,Y,16);
			I3(SRLV,Y,Y,R12);
			I2C(SLL,R12,Y,23);
			I2C(SRA,R12,R12,31);
			I3(OR,Y,Y,R12);

			Reverse = (p->DirX>0) ^ ((p->SwapXY)!=0);
			Pos = Reverse ? n*p->DstBPP : (8-(n+1)*p->DstBPP);
			Tmp = (reg)((!Invert && n==0 && Pos==0) ? R14 : R12);
			I2C(ANDI,Tmp,Y,ValueMask);
			if (Pos)
				I2C(SRL,Tmp,Tmp,Pos);

			if (n==0)
			{
				if (Invert || Pos)
					I3(Invert ? NOR:OR,R14,Tmp,Tmp);
			}
			else
				I3(XOR,R14,R14,Tmp);

			if (p->DstDoubleY)
			{
				int Pos2 = Reverse ? (n^p->DstDoubleX)*p->DstBPP : (8-((n^p->DstDoubleX)+1)*p->DstBPP);
				if (Pos > Pos2)
					I2C(SLL,R12,Tmp,Pos-Pos2);
				else
					I2C(SRL,R12,Tmp,Pos2-Pos);

				if (n==0)
					I3(Invert ? NOR:OR,R15,R12,R12);
				else
					I3(XOR,R15,R15,R12);
			}

			if (p->FX.Flags & BLITFX_DITHER)
				I2C(ANDI,R13,R13,DitherMask);
		}

		if (p->DstDoubleY)
		{
			MB(); 
			if (p->SwapXY)
			{
				I2(SB,R15,R2);
				I3(ADDU,R2,R2,R16);
			}
			else
			{
				I2(SB,R15,R3);
				I2C(ADDIU,R3,R3,p->DirX>0 ? 1:-1);
			}
		}

		MB(); I2(SB,R14,R2);

		if (p->SwapXY)
		{
			I3(ADDU,R2,R2,R16);
			for (n=0;n<SrcRows;++n)
				I2C(ADDIU,(reg)(R4+n),(reg)(R4+n),1);
		}
		else
		{
			I2C(ADDIU,R2,R2,p->DirX>0 ? 1:-1);
			I2C(ADDIU,R4,R4,Count >> p->DstDoubleX);
		}

		DS(); I2P(BNE,R2,R24,LoopX);
	}

	//increment pointers
	for (n=0;n<DstRows;++n)
		I3(ADDU,(reg)(R2+n),(reg)(R2+n),R19);
	for (n=0;n<SrcRows;++n)
		I3(ADDU,(reg)(R4+n),(reg)(R4+n),R18);

	DS(); I2P(BNE,R2,R25,LoopY);

	CodeEnd(4,0,0);
}

#endif
