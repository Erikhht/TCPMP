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
 * $Id: blit_sh3_fix.c 271 2005-08-09 08:31:35Z picard $
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
	int EndOfLine;
	int DstNext;
	int SrcNext;
	int UVNext;
	int EndOfLineInc;

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

// R0 lookup

// R10 buffer_u
// R11 buffer_v
// R12 buffer_y1
// R14 buffer_y2

// R4  r / pal:dither acc
// R5  g / pal:mask
// R6  b / pal:mask
// R8  y1
// R9  y2	

// R2  tmp / pal:uv
// R3  tmp 
// R7  tmp / endofline 

// R13 dest1
// R1  pitch

static NOINLINE void Fix_RGB_UV_LoadUV(blit_soft* p)
{
	//set R4 =           RVMul*v + RAdd
	//set R5 = GUMul*u + GVMul*v + GAdd
	//set R6 = BUMul*u +           BAdd

	I2(MOVB_LDADD,R10,R2); //u
	I2(MOVB_LDADD,R11,R3); //v

	if (p->DstPalette)
	{
		I2(EXTUB,R2,R2);
		I2(EXTUB,R3,R3);
		I1(SHLR,R2);
		I1(SHLR,R3);
		I1(SHLL8,R2);
		I1(SHLL16,R3);
		I2(OR,R3,R2);
	}
	else
	{
		I1C(MOVI,R8,1);
		I1(SHLL8,R8);
		I2(ADD,R8,R2); //+=256
		I1(SHLL,R8);
		I2(ADD,R8,R3); //+=512

		I1(SHLL2,R2);
		I1(SHLL2,R3);

		I2(MOVW_LDR0,R2,R6);
		I2(MOVW_LDR0,R3,R4);

		I1C(ADDI,R2,2);
		I1C(ADDI,R3,2);

		I2(MOVW_LDR0,R2,R5);
		I2(MOVW_LDR0,R3,R9);

		I2(ADD,R9,R5);
	}
}

static NOINLINE void Fix_RGB_UV_Pixel(blit_soft* p, int Row)
{
	//load Y
	if (p->SwapXY)
	{
		if (p->DirX>0)
		{
			I2(MOVB_LDADD,R14,R9); //y2
			I2(MOVB_LDADD,R12,R8); //y1
		}
		else
		{
			I2(MOVB_LDADD,R14,R8); //y1
			I2(MOVB_LDADD,R12,R9); //y2
		}
	}
	else
	{
		reg Y = (reg)(Row==0?R12:R14);
		if (p->DirX>0)
		{
			I2(MOVB_LDADD,Y,R8); //y1
			I2(MOVB_LDADD,Y,R9); //y2
		}
		else
		{
			I2(MOVB_LDADD,Y,R9); //y2
			I2(MOVB_LDADD,Y,R8); //y1
		}
	}

	if (p->DstPalette)
	{
		I2(EXTUB,R8,R8);
		I2(EXTUB,R9,R9);
		I1(SHLR,R8);
		I1(SHLR,R9);

		I1(SHLL16,R8);
		I1(SHLL16,R9);
		I1(SHLL8,R8);
		I1(SHLL8,R9);
		I2(OR,R2,R8);
		I2(OR,R2,R9);

		I2(ADD,R8,R4); //      yyvvuu00
		I2(MOV,R4,R3); 
		I2(MOV,R4,R7);
		I2(AND,R5,R3); // mask F0F00000
		I2(AND,R6,R7); // mask 0000F000
		I1(SHLR16,R3);
		I1(SHLR8,R7);
		I1(SHLR2,R3);
		I1(SHLL2,R7);
		I2(OR,R7,R3);
		I2(MOVL_LDR0,R3,R8);
		I2(SUB,R8,R4);

		I2(ADD,R9,R4); //      yyvvuu00
		I2(MOV,R4,R3); 
		I2(MOV,R4,R7);
		I2(AND,R5,R3); // mask F0F00000
		I2(AND,R6,R7); // mask 0000F000
		I1(SHLR16,R3);
		I1(SHLR8,R7);
		I1(SHLR2,R3);
		I1(SHLL2,R7);
		I2(OR,R7,R3);
		I2(MOVL_LDR0,R3,R9);
		I2(SUB,R9,R4);

		//Break();
	}
	else
	{
		I2(MOVB_LDR0,R8,R8);
		I2(MOVB_LDR0,R9,R9);

		I2(MOV,R4,R2);
		I2(ADD,R8,R2);
		I2(MOVB_LDR0,R2,R2);

		I2(MOV,R5,R3);
		I2(ADD,R8,R3);
		I2(MOVB_LDR0,R3,R3);

		I2(ADD,R6,R8);
		I2(MOVB_LDR0,R8,R8);

		IShift(R2,R7,p->DstPos[0]);
		IShift(R3,R7,p->DstPos[1]);
		IShift(R8,R7,p->DstPos[2]);

		I2(OR,R2,R8);
		I2(OR,R3,R8);

		I2(MOV,R4,R2);
		I2(ADD,R9,R2);
		I2(MOVB_LDR0,R2,R2);

		I2(MOV,R5,R3);
		I2(ADD,R9,R3);
		I2(MOVB_LDR0,R3,R3);

		I2(ADD,R6,R9);
		I2(MOVB_LDR0,R9,R9);

		IShift(R2,R7,p->DstPos[0]);
		IShift(R3,R7,p->DstPos[1]);
		IShift(R9,R7,p->DstPos[2]);

		I2(OR,R2,R9);
		I2(OR,R3,R9);

		if (!p->DstDoubleX && p->DstBPP <= 16)
		{
			I1(SHLL16,R9);
			I2(OR,R9,R8);
		}
	}
}

// R3,R2 for temp
static NOINLINE void Write(blit_soft* p,reg Dst,reg A,reg B,bool_t Prepare)
{
	switch (p->DstBPP)
	{
	case 8:
		if (p->DstDoubleX)
		{
			if (Prepare)
			{
				I2(EXTUB,B,B);
				I2(EXTUB,A,A);
				I1(SHLL16,B);
				I2(OR,B,A);
				I2(MOV,A,R3);
				I1(SHLL8,R3);
				I2(OR,R3,A);
			}
			I2(MOVL_ST,A,Dst);
		}
		else
		{
			if (Prepare)
			{
				I2(EXTUB,A,A);
				I1(SHLL8,B);
				I2(OR,B,A);
			}
			I2(MOVW_ST,A,Dst);
		}
		break;
	case 16:
		if (p->DstDoubleX)
		{
			if (Prepare)
			{
				I2(MOV,A,R3);
				I2(MOV,B,R2);
				I1(SHLL16,R3);
				I1(SHLL16,R2);
				I2(OR,R3,A);
				I2(OR,R2,B);
			}
			I2C(MOVL_STOFS,B,Dst,4);
		}
		I2C(MOVL_STOFS,A,Dst,0);
		break;
	}
}

void Fix_RGB_UV(blit_soft* p)
{
	dyninst* Quit;
	dyninst* LoopYAddr;
	dyninst* Mask1;
	dyninst* Mask2;
	dyninst* LoopY;
	dyninst* LoopX;

	if (p->RScaleX == 8 || p->RScaleY == 8)
		p->DstAlignSize = 4;
	p->DstStepX = p->DirX * ((p->DstBPP*2) >> 3) << p->DstDoubleX;

	CodeBegin(7,OFS(stack,StackFrame));

	I2C(MOVL_LDOFS,R4,R0,OFS(blit_soft,LookUp_Data));
	if (!p->DstPalette)
	{
		CalcLookUp(p,0);
		I1C(ADDI,R0,64);
		I1C(ADDI,R0,64);
	}
	
	I2C(MOVL_LDOFS,R5,R13,0); //dst
	I2C(MOVL_LDOFS,R6,R10,4); //U
	I2C(MOVL_LDOFS,R6,R11,8); //V
	I2C(MOVL_LDOFS,R6,R12,0); //Y

	I2(MOV,R7,R1); //dstpitch

	I1C(MOVI,R2,OFS(stack,This));
	I2(ADD,SP,R2);
	I2C(MOVL_LDOFS,R2,R7,OFS(stack,SrcPitch)-OFS(stack,This));
	I2C(MOVL_LDOFS,R2,R5,OFS(stack,Height)-OFS(stack,This));
	I2C(MOVL_LDOFS,R2,R4,OFS(stack,Width)-OFS(stack,This));
	
	//SrcNext = 2*Src->Pitch - (Width >> SrcDoubleX)
	I2(MOV,R7,R2);
	I1(SHLL,R2);
	I2(MOV,R4,R3);
	IShift(R3,NONE,-p->SrcDoubleX);
	I2(SUB,R3,R2);
	I2C(MOVL_STOFS,R2,SP,OFS(stack,SrcNext));

	//UVNext = (Src->Pitch >> 1) - (Width >> SrcDoubleX >> 1);
	I2(MOV,R7,R2);
	I1(SHAR,R2);
	I2(MOV,R4,R3);
	IShift(R3,NONE,-p->SrcDoubleX-1);
	I2(SUB,R3,R2);
	I2C(MOVL_STOFS,R2,SP,OFS(stack,UVNext));

	if (p->DirX<0) //adjust reversed destination for block size
		I1C(ADDI,R13,p->DstStepX+(p->DstBPP >> 3));

	if (p->SwapXY)
	{
		I1C(MOVI,R3,p->DstBPP * p->DirX);
		I2(MULL,R3,R5); // p->DstBPP * p->DirX * Height
		I1C(MOVI,R2,-3);
		I1(STS_MACL,R3);
		I2(SHAD,R2,R3);
		I2(ADD,R13,R3);
		I2C(MOVL_STOFS,R3,SP,OFS(stack,EndOfRect));

		//DstNext = DstStepX - Width * DstPitch;
		I2(MULL,R1,R4);
		I1C(MOVI,R2,p->DstStepX);
		I1(STS_MACL,R3);
		I2(SUB,R3,R2);
		I2C(MOVL_STOFS,R2,SP,OFS(stack,DstNext));
	}
	else
	{
		I2(MULL,R1,R5); //Height * DstPitch
		I1(STS_MACL,R3);
		I2(ADD,R13,R3);
		I2C(MOVL_STOFS,R3,SP,OFS(stack,EndOfRect));

		//DstNext = ((DstPitch*2 << DstDoubleY) - DirX * Width << DstBPP2;
		I2(MOV,R1,R2);
		IShift(R2,NONE,p->DstDoubleY+1);
		I2(MOV,R4,R3);
		IShift(R3,NONE,p->DstBPP2);
		I2(p->DirX>0?SUB:ADD,R3,R2);
		I2C(MOVL_STOFS,R2,SP,OFS(stack,DstNext));
	}

	if (p->SwapXY)
	{
		I2(MULL,R1,R4); //width*dstpitch
		I1(STS_MACL,R3);
		I2(SUB,R1,R3); // - DstPitch
	}
	else
	{
		I2(MOV,R4,R3);
		IShift(R3,NONE,p->DstBPP2);
		if (p->DirX < 0)
			I2(NEG,R3,R3);
		I1C(ADDI,R3,-p->DstStepX); // -p->DstStepX
	}

	I2C(MOVL_STOFS,R3,SP,OFS(stack,EndOfLineInc));

	I2(MOV,R12,R14);
	I2(ADD,R7,R14);

	Quit = Label(0);
	LoopY = Label(1);
	LoopYAddr = ICode(LoopY,0);

	if (p->DstPalette)
	{
		Mask1 = InstCreate32(0xF0F00000,NONE,NONE,NONE,0,0);
		Mask2 = InstCreate32(0x0000F000,NONE,NONE,NONE,0,0);
		I1P(MOVL_PC,R5,Mask1,0);
		I1P(MOVL_PC,R6,Mask2,0);
		I1C(MOVI,R4,0);
	}

	// R3 = EndOfLineInc
	I2(MOV,R13,R7);
	I2(ADD,R3,R7);
	I2C(MOVL_STOFS,R7,SP,OFS(stack,EndOfLine));

	LoopX = Label(1);
	{
		Fix_RGB_UV_LoadUV(p);
		Fix_RGB_UV_Pixel(p,0);

		Write(p,R13,R8,R9,1);
		I2(ADD,R1,R13);

		if (p->DstDoubleY)
		{
			Write(p,R13,R8,R9,0);
			I2(ADD,R1,R13);
		}
	
		Fix_RGB_UV_Pixel(p,1);

		I2C(MOVL_LDOFS,SP,R7,OFS(stack,EndOfLine));

		Write(p,R13,R8,R9,1);

		if (p->DstDoubleY)
		{
			I2(ADD,R1,R13);
			Write(p,R13,R8,R9,0);
		}

		if (p->SwapXY)
		{
			I2(CMPEQ,R13,R7);
			I2(ADD,R1,R13);
		}
		else
		{
			I2(SUB,R1,R13);
			if (p->DstDoubleY)
			{
				I2(SUB,R1,R13);
				I2(SUB,R1,R13);
			}
			I2(CMPEQ,R13,R7);
			I1C(ADDI,R13,p->DstStepX);
		}

		DS(); I0P(BFS,LoopX);
	}

	I2C(MOVL_LDOFS,SP,R2,OFS(stack,SrcNext));
	I2C(MOVL_LDOFS,SP,R3,OFS(stack,DstNext));
	I2C(MOVL_LDOFS,SP,R9,OFS(stack,UVNext));
	I2C(MOVL_LDOFS,SP,R8,OFS(stack,EndOfRect));
	
	//increment pointers
	I2(ADD,R2,R12);
	I2(ADD,R2,R14);
	I2(ADD,R3,R13);

	I1P(MOVL_PC,R2,LoopYAddr,0);
	I2(ADD,R9,R10);
	I2(ADD,R9,R11);

	MB(); I2(CMPEQ,R13,R8);
	DS(); I0P(BTS,Quit);

	I2C(MOVL_LDOFS,SP,R3,OFS(stack,EndOfLineInc)); //prepare registers for next row
	DS(); I1(JMP,R2);

	InstPost(Quit);

	CodeEnd(7,OFS(stack,StackFrame));

	Align(4);
	InstPost(LoopYAddr);

	if (p->DstPalette)
	{
		InstPost(Mask1);
		InstPost(Mask2);
	}
}

#endif
