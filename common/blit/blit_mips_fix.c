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
 * $Id: blit_mips_fix.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(MIPS) 

typedef struct stack
{
	int EndOfRect;
	int DstNext;
	int SrcNext; // SrcPitch for scale
	int UVNext;
	int EndOfLineInc;
	int Y,U,V;

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

// R10 buffer_u
// R11 buffer_v
// R12 buffer_y1
// R14 buffer_y2

// R20 buffer_u2 (scale)
// R21 buffer_v2 (scale)
// R22 pos (scale)

// R17 radd / pal:accy
// R18 gadd / pal:accu
// R19 badd / pal:accv

// R13 negsat / LookUp_Y / pal:lookup
// R15 possat / LookUp_U / pal:U

// R4  r
// R5  g
// R6  b
// R8  y1/rgb out
// R9  y2/(rgb2 out)

// R16 tmp / LookUp_V / pal:V
// R2  tmp
// R3  tmp

// R7  endofline
// R24 dest
// R25 pitch
// R1  tmp dest

static NOINLINE int ConstAdjust(int v)
{
	if (v>0)
		return (v + 0x1000) >> (16-3);
	return -((-v + 0x1000) >> (16-3));
}

static NOINLINE void Fix_RGB_UV_LoadUV(blit_soft* p,int OfsUV,int OfsUV2)
{
	if (p->DstPalette)
	{
		if (!p->Scale)
		{
			I2C(LBU,R15,R10,0); //u
			I2C(ADDIU,R10,R10,1);
			I2C(LBU,R16,R11,0); //v
			I2C(ADDIU,R11,R11,1);
		}
		else
		{
			reg U = R10;
			reg V = R11;
			reg U2 = R10;
			reg V2 = R11;

			if (p->SwapXY)
			{
				if (p->DirX<0)
				{
					U = R20;
					V = R21;
				}
				else
				{
					U2 = R20;
					V2 = R21;
				}
			}

			I2C(LBU,R2,U2,OfsUV2); //u2
			I2C(LBU,R3,V2,OfsUV2); //v2

			I2C(LBU,R15,U,OfsUV); //u
			I2C(LBU,R16,V,OfsUV); //v
		}
	}
	else
	if (p->ColorLookup)
	{
		//set R4 =           RVMul*v + RAdd
		//set R5 = GUMul*u + GVMul*v + GAdd
		//set R6 = BUMul*u +           BAdd

		I2C(LBU,R2,R10,0); //u
		I2C(LBU,R3,R11,0); //v

		I2C(SLL,R2,R2,3);
		I2C(SLL,R3,R3,3);

		I3(ADDU,R2,R2,R15); //LookUp_U + u
		I3(ADDU,R3,R3,R16); //LookUp_V + v

		I2C(LW,R8,R2,4);
		I2C(ADDIU,R10,R10,1);
		I2C(LW,R9,R3,4);
		I2C(ADDIU,R11,R11,1);
		I2C(LW,R6,R2,0);
		I3(ADDU,R5,R8,R9);
		I2C(LW,R4,R3,0);
	}
	else
	{
		reg v;
		bool_t Neg;

		if (!p->Scale)
		{
			I2C(LBU,R4,R10,0); //u
			I2C(LBU,R5,R11,0); //v

			I2C(ADDIU,R10,R10,1);
			I2C(ADDIU,R11,R11,1);

			I2C(SLL,R2,R4,16);
			I2C(SLL,R3,R5,16);
		}
		else
		{
			reg U = R10;
			reg V = R11;
			reg U2 = R10;
			reg V2 = R11;

			if (p->SwapXY)
			{
				if (p->DirX<0)
				{
					U = R20;
					V = R21;
				}
				else
				{
					U2 = R20;
					V2 = R21;
				}
			}

			I2C(LBU,R2,U2,OfsUV2); //u2
			I2C(LBU,R3,V2,OfsUV2); //v2

			I2C(LBU,R4,U,OfsUV); //u
			I2C(SLL,R2,R2,16);
			I2C(LBU,R5,V,OfsUV); //v
			I2C(SLL,R3,R3,16);
		}

		I3(OR,R2,R4,R2);	// u | u
		I3(OR,R3,R5,R3);	// v | v

		v = IMul(R2,ConstAdjust(p->_GUMul),R16,R4,R6,&Neg);
		I3(Neg?SUBU:ADDU,R5,R18,v);
		v = IMul(R3,ConstAdjust(p->_GVMul),R16,R4,R6,&Neg);
		I3(Neg?SUBU:ADDU,R5,R5,v);
		v = IMul(R3,ConstAdjust(p->_RVMul),R16,R4,R6,&Neg);
		I3(Neg?SUBU:ADDU,R4,R17,v);
		v = IMul(R2,ConstAdjust(p->_BUMul),R16,R3,R6,&Neg);
		I3(Neg?SUBU:ADDU,R6,R19,v);
	}
}

static NOINLINE void Fix_RGB_UV_Pixel(blit_soft* p, int Row, int Col)
{
	int Shift;
	reg v;
	reg Y1,Y2;
	bool_t Neg;
	int OfsY;
	int OfsUV;
	int OfsY2;
	int OfsUV2;

	if (p->Scale)
	{
		OfsY = (Col * p->RScaleX) >> 4;
		OfsUV = (Col * p->RScaleX) >> (4+p->SrcUVX2);

		if (!p->SwapXY)
		{
			OfsY2 = ((Col+1) * p->RScaleX) >> 4;
			OfsUV2 = ((Col+1) * p->RScaleX) >> (4+p->SrcUVX2);
		}
		else
		{
			OfsY2 = OfsY;
			OfsUV2 = OfsUV;
		}
	
		Fix_RGB_UV_LoadUV(p,OfsUV,OfsUV2);
	}
	else
	{
		if (p->SwapXY)
		{
			OfsY = Row;
			OfsY2 = Row;
		}
		else
		{
			OfsY = 0;
			OfsY2 = 1;
		}
	}

	if (p->SwapXY)
	{
		Y1 = R12;
		Y2 = R14;
	}
	else
		Y1 = Y2 = (reg)(Row==0?R12:R14);

	//load Y
	if (p->DirX>0)
	{
		I2C(LBU,R9,Y2,OfsY2); //y2
		I2C(LBU,R8,Y1,OfsY);  //y1
	}
	else
	{
		I2C(LBU,R9,Y1,OfsY);  //y2
		I2C(LBU,R8,Y2,OfsY2); //y1
	}

	if (p->DstPalette)
	{
		I3(ADDU,R17,R17,R8);
		I3(ADDU,R18,R18,R15);
		I3(ADDU,R19,R19,R16);

		I2C(ANDI,R4,R17,0x1E0); 
		I2C(ANDI,R5,R18,0x1E0); 
		I2C(ANDI,R6,R19,0x1E0); 

		I2C(SLL,R4,R4,5); 
		I2C(SLL,R5,R5,1);
		I2C(SRL,R6,R6,3);

		I3(ADDU,R4,R4,R13);
		I3(ADDU,R4,R4,R5);
		I3(ADDU,R4,R4,R6);
		
		I2C(LBU,R8,R4,0);
		I2C(LBU,R6,R4,1);
		I2C(LBU,R5,R4,2);
		I2C(LBU,R4,R4,3);

		I3(SUBU,R17,R17,R6);
		I3(SUBU,R18,R18,R5);
		I3(SUBU,R19,R19,R4);

		I3(ADDU,R17,R17,R9);
		I3(ADDU,R18,R18,(reg)(p->Scale ? R2:R15));
		I3(ADDU,R19,R19,(reg)(p->Scale ? R3:R16));

		I2C(ANDI,R4,R17,0x1E0); 
		I2C(ANDI,R5,R18,0x1E0); 
		I2C(ANDI,R6,R19,0x1E0); 

		I2C(SLL,R4,R4,5); 
		I2C(SLL,R5,R5,1);
		I2C(SRL,R6,R6,3);

		I3(ADDU,R4,R4,R13);
		I3(ADDU,R4,R4,R5);
		I3(ADDU,R4,R4,R6);
		
		I2C(LBU,R9,R4,0);
		I2C(LBU,R6,R4,1);
		I2C(LBU,R5,R4,2);
		I2C(LBU,R4,R4,3);

		I3(SUBU,R17,R17,R6);
		I3(SUBU,R18,R18,R5);
		I3(SUBU,R19,R19,R4);
	}
	else
	if (p->ColorLookup)
	{
		I3(ADDU,R8,R8,R13); //LookUp_Y + y1
		I2C(LBU,R8,R8,0);
		
		I3(ADDU,R9,R9,R13); //LookUp_Y + y2
		I2C(LBU,R9,R9,0);

		I3(ADDU,R2,R4,R8);
		I2C(LBU,R2,R2,0);
		
		I3(ADDU,R3,R5,R8);
		I2C(LBU,R3,R3,0);
		
		I3(ADDU,R8,R6,R8);
		I2C(LBU,R8,R8,0);

		I2C(SLL,R2,R2,p->DstPos[0]);
		I2C(SLL,R3,R3,p->DstPos[1]);
		if (p->DstPos[2]) I2C(SLL,R8,R8,p->DstPos[2]);

		I3(OR,R8,R8,R2);
		I3(OR,R8,R8,R3);

		I3(ADDU,R2,R4,R9);
		I2C(LBU,R2,R2,0);
		
		I3(ADDU,R3,R5,R9);
		I2C(LBU,R3,R3,0);
		
		I3(ADDU,R9,R6,R9);
		I2C(LBU,R9,R9,0);

		if (p->DstDoubleX || p->DstBPP > 16)
		{
			I2C(SLL,R2,R2,p->DstPos[0]);
			I2C(SLL,R3,R3,p->DstPos[1]);
			if (p->DstPos[2]) I2C(SLL,R9,R9,p->DstPos[2]);

			I3(OR,R9,R9,R2);
			I3(OR,R9,R9,R3);
		}
		else
		{
			I2C(SLL,R2,R2,p->DstPos[0]+16);
			I2C(SLL,R3,R3,p->DstPos[1]+16);
			I2C(SLL,R9,R9,p->DstPos[2]+16);

			I3(OR,R8,R8,R2);
			I3(OR,R8,R8,R3);
			I3(OR,R8,R8,R9);
		}
	}
	else
	{
		I2C(SLL,R9,R9,16);
		I3(OR,R8,R8,R9);		// y1 | y2

		v = IMul(R8,ConstAdjust(p->_YMul),R16,R3,R2,&Neg);

		I3(Neg?SUBU:ADDU,R8,R4,v);
		I3(Neg?SUBU:ADDU,R9,R5,v);
		I3(Neg?SUBU:ADDU,R16,R6,v);

		// negative sat and mask final rbg
		I3(NOR,R2,R8,R13);
		I2C(SRL,R3,R2,4+p->DstSize[0]);
		I3(SUBU,R2,R2,R3);
		I3(AND,R8,R8,R2);

		I3(NOR,R2,R9,R13);
		I2C(SRL,R3,R2,4+p->DstSize[1]);
		I3(SUBU,R2,R2,R3);
		I3(AND,R9,R9,R2);

		I3(NOR,R2,R16,R13);
		I2C(SRL,R3,R2,4+p->DstSize[2]);
		I3(SUBU,R2,R2,R3);
		I3(AND,R16,R16,R2);

		// positive sat
		I3(AND,R2,R8,R15);
		I2C(SRL,R3,R2,p->DstSize[0]);
		I3(SUBU,R2,R2,R3);
		I3(OR,R8,R8,R2);

		I3(AND,R2,R9,R15);
		I2C(SRL,R3,R2,p->DstSize[1]);
		I3(SUBU,R2,R2,R3);
		I3(OR,R9,R9,R2);

		I3(AND,R2,R16,R15);
		I2C(SRL,R3,R2,p->DstSize[2]);
		I3(SUBU,R2,R2,R3);
		I3(OR,R16,R16,R2);

		Shift = p->DstPos[0] + p->DstSize[0] - (8+3);
		if (Shift) I2C(SLL,R8,R8,Shift);
		Shift = p->DstPos[1] + p->DstSize[1] - (8+3);
		if (Shift) I2C(SLL,R9,R9,Shift);
		Shift = p->DstPos[2] + p->DstSize[2] - (8+3);
		if (Shift) I2C(SLL,R16,R16,Shift);

		I3(OR,R8,R8,R9);
		I3(OR,R8,R8,R16);
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
				I2C(SLL,R3,A,8);
				I3(OR,A,A,R3);
				I2C(SLL,R3,B,8);
				I3(OR,B,B,R3);
				I2C(SLL,R3,B,16);
				I3(OR,A,A,R3);
			}
			I2C(SW,A,Dst,0);
		}
		else
		{
			if (Prepare)
			{
				I2C(SLL,R3,B,8);
				I3(OR,A,A,R3);
			}
			I2C(SH,A,Dst,0);
		}
		break;

	case 16:
		if (p->Dst.Flags & PF_16BITACCESS)
		{
			if (p->DstDoubleX)
			{
				if (Prepare && !p->ColorLookup)
					I2C(SRL,B,A,16);

				I2C(SH,A,Dst,0);
				I2C(SH,A,Dst,2);
				I2C(SH,B,Dst,4);
				I2C(SH,B,Dst,6);
			}
			else
			{
				if (Prepare)
					I2C(SRL,B,A,16);

				I2C(SH,A,Dst,0);
				I2C(SH,B,Dst,2);
			}
		}
		else
		{
			if (p->DstDoubleX)
			{
				if (Prepare)
				{
					if (p->ColorLookup)
					{
						I2C(SLL,R3,A,16);
						I2C(SLL,R2,B,16);
						I3(OR,A,A,R3);
						I3(OR,B,B,R2);
					}
					else
					{
						I2C(SLL,R3,A,16);
						I2C(SRL,R2,A,16);
						I2C(SRL,A,R3,16);
						I2C(SLL,B,R2,16);
						I3(OR,A,A,R3);
						I3(OR,B,B,R2);
					}
				}
				I2C(SW,B,Dst,4);
			}
			I2C(SW,A,Dst,0);
		}
		break;
	case 24:
		if (p->DstDoubleX)
		{
			if (Prepare)
			{
				I2C(SLL,R3,A,24);
				I2C(SRL,R2,A,8);
				I3(OR,A,A,R3);
				I2C(SLL,R3,B,16);
				I3(OR,R3,R3,R2);
				I2C(SRL,R2,B,16);
				I2C(SLL,B,B,8);
				I3(OR,B,B,R2);
			}
			I2C(SW,A,Dst,0);
			I2C(SW,R3,Dst,4);
			I2C(SW,B,Dst,8);
		}
		else
		{
			if (Prepare)
			{
				I2C(SRL,R3,A,16);
				I2C(SLL,B,B,8);
				I3(OR,R3,R3,B);
				I2C(SRL,B,B,16);
			}
			I2C(SH,A,Dst,0);
			I2C(SH,R3,Dst,2);
			I2C(SH,B,Dst,4);
		}
		break;
	case 32:
		if (p->DstDoubleX)
		{
			I2C(SW,A,Dst,0);
			I2C(SW,A,Dst,4);
			I2C(SW,B,Dst,8);
			I2C(SW,B,Dst,12);
		}
		else
		{
			I2C(SW,A,Dst,0);
			I2C(SW,B,Dst,4);
		}
		break;
	}
}

void Fix_RGB_UV(blit_soft* p)
{
	int Col,ColCount,i,RowStep;
	dyninst* LoopY;
	dyninst* LoopX;
	dyninst* EndOfLine;

	p->Scale = (boolmem_t)(!(p->RScaleX == 8 || p->RScaleX == 16) || 
				!(p->RScaleY == 8 || p->RScaleY == 16) ||
				p->SrcUVX2!=1 || p->SrcUVY2!=1);

	if (p->Scale)
	{
		p->DstDoubleX = p->DstDoubleY = 0;
		p->ColorLookup = 0;
	}

	if (p->RScaleX == 8 || p->RScaleY == 8)
		p->DstAlignSize = 4;
	p->DstStepX = p->DirX * ((p->DstBPP*2) >> 3) << p->DstDoubleX;

	CodeBegin(7,OFS(stack,StackFrame),0);

	if (p->DstPalette)
	{
		IConst(R17,0);
		IConst(R18,0);
		IConst(R19,0);
		I2C(LW,R13,R4,OFS(blit_soft,LookUp_Data));
	}	
	else
	if (p->ColorLookup)
	{
		CalcLookUp(p,0);
		I2C(LW,R13,R4,OFS(blit_soft,LookUp_Data));
		I2C(ADDIU,R15,R13,p->LookUp_U);
		I2C(ADDIU,R16,R13,p->LookUp_V);
	}
	else
	{
		IConst(R17,(ConstAdjust(p->_RAdd) & 0xFFFF) * 0x10001);
		IConst(R18,(ConstAdjust(p->_GAdd) & 0xFFFF) * 0x10001);
		IConst(R19,(ConstAdjust(p->_BAdd) & 0xFFFF) * 0x10001);

		IConst(R15,0x01000100 << 3); //possat
		IConst(R13,0x7FFF7FFF); //negsat
	}
	
	I2C(LW,R24,R5,0); //Dst[0] RGB
	I2C(LW,R10,R6,4); //Src[1] U
	I2C(LW,R11,R6,8); //Src[2] V
	I2C(LW,R12,R6,0); //Src[0] Y
	I3(ADDU,R25,R7,ZERO); //DstPitch

	if (p->Scale)
	{
		I2C(SW,R10,SP,OFS(stack,U));
		I2C(SW,R11,SP,OFS(stack,V));
		I2C(SW,R12,SP,OFS(stack,Y));
		I2C(LW,R7,SP,OFS(stack,SrcPitch));
	}

	I2C(LW,R7,SP,OFS(stack,SrcPitch));
	I2C(LW,R5,SP,OFS(stack,Height));
	I2C(LW,R4,SP,OFS(stack,Width));

	if (!p->Scale)
	{
		//SrcNext = 2*Src->Pitch - (Width >> SrcDoubleX)
		I2C(SLL,R2,R7,1);
		if (p->SrcDoubleX)
		{
			I2C(SRL,R3,R4,p->SrcDoubleX);
			I3(SUBU,R2,R2,R3); 
		}
		else
			I3(SUBU,R2,R2,R4); 
		I2C(SW,R2,SP,OFS(stack,SrcNext));

		//UVNext = (Src->Pitch >> 1) - (Width >> SrcDoubleX >> 1);
		I2C(SRA,R2,R7,1);
		I2C(SRL,R3,R4,p->SrcDoubleX+1);
		I3(SUBU,R2,R2,R3); 
		I2C(SW,R2,SP,OFS(stack,UVNext));
	}
	else
		I2C(SW,R7,SP,OFS(stack,SrcNext));

	if (p->DirX<0) //adjust reversed destination for block size
		I2C(ADDIU,R24,R24,p->DstStepX+(p->DstBPP >> 3));

	if (p->SwapXY)
	{
		I2C(ADDIU,R3,ZERO,p->DstBPP * p->DirX);
		I2(MULT,R5,R3); // p->DstBPP * p->DirX * Height
		I1(MFLO,R3);
		I2C(SRA,R3,R3,3); // bits->byte
		I3(ADDU,R3,R3,R24);
		I2C(SW,R3,SP,OFS(stack,EndOfRect));

		//DstNext = DstStepX - Width * DstPitch;
		I2(MULT,R25,R4);
		I1(MFLO,R2);
		I2C(ADDIU,R3,ZERO,p->DstStepX);
		I3(SUBU,R3,R3,R2);
		I2C(SW,R3,SP,OFS(stack,DstNext));
	}
	else
	{
		I2(MULT,R5,R25); //Height * DstPitch
		I1(MFLO,R3);
		I3(ADDU,R3,R3,R24);
		I2C(SW,R3,SP,OFS(stack,EndOfRect));

		//DstNext = ((DstPitch*(Scale?1:2) << DstDoubleY) - DirX * Width << DstBPP2;
		I2C(SLL,R2,R25,p->DstDoubleY+(p->Scale?0:1));
		I2C(SLL,R3,R4,p->DstBPP2);
		I3(p->DirX>0?SUBU:ADDU,R2,R2,R3);
		I2C(SW,R2,SP,OFS(stack,DstNext));
	}

	if (p->SwapXY)
	{
		I2(MULT,R25,R4); // Width * DstPitch
		I1(MFLO,R4);
	}
	else
	{
		I2C(SLL,R4,R4,p->DstBPP2);
		if (p->DirX < 0)
			I3(SUBU,R4,ZERO,R4);
	}
	I2C(SW,R4,SP,OFS(stack,EndOfLineInc));

	if (p->Scale)
		I3(ADDU,R22,ZERO,ZERO); //pos=0
	else
		I3(ADDU,R14,R12,R7);

	ColCount = 32;
	for (i=1;i<16 && !(p->RScaleX & i);i<<=1)
		ColCount >>= 1;

	LoopY = Label(1);

	// R4 = EndOfLineInc
	I3(ADDU,R7,R24,R4);

	LoopX = Label(1);

	if (p->Scale)
	{
		EndOfLine = Label(0);

		if (p->SwapXY)
		{
			//we need next row in R14,R20,R21 (YUV)
			I2C(LW,R2,SP,OFS(stack,SrcNext)); //srcpitch

			//increment row of Y,U,V according to Pos -> Pos+RowStep*RScaleY
			I2C(ADDIU,R9,R22,p->RScaleY);		//Pos += RScaleY
			I2C(SRL,R8,R22,4);
			I2C(SRL,R9,R9,4);

			// dY  = ((Pos+RowStep*RScaleY) >> 4) - (Pos >> 4);
			I3(SUBU,R4,R9,R8); 
			I2(MULT,R4,R2); // dY * SrcPitch
			I1(MFLO,R4);
			I3(ADDU,R14,R12,R4);

			// dUV = ((Pos+RowStep*RScaleY) >> (4+SrcUVY2)) - (Pos >> (4+SrcUVY2));
			if (p->SrcUVY2) I2C(SRL,R9,R9,p->SrcUVY2);
			if (p->SrcUVY2) I2C(SRL,R8,R8,p->SrcUVY2);
			I3(SUBU,R4,R9,R8); 
			I2(MULT,R4,R2); // dUV * (SrcPitch >> SrcUVX2)
			I1(MFLO,R4);
			if (p->SrcUVX2) I2C(SRA,R4,R4,p->SrcUVX2);
			I3(ADDU,R20,R10,R4);
			I3(ADDU,R21,R11,R4);
		}

		for (Col=0;Col<ColCount;)
		{
			Fix_RGB_UV_Pixel(p,0,Col);

			++Col;
			if (!p->SwapXY)
				++Col;

			Write(p,R24,R8,R9,1);

			if (p->SwapXY)
				I3(ADDU,R24,R24,R25);
			else
				I2C(ADDIU,R24,R24,p->DstStepX);

			if (Col == ColCount) 
			{
				//last item in block, increase source pointers

				I2C(ADDIU,R12,R12,(p->RScaleX * ColCount) >> 4);
				I2C(ADDIU,R10,R10,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));
				I2C(ADDIU,R11,R11,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));

				if (p->SwapXY)
				{
					I2C(ADDIU,R14,R14,(p->RScaleX * ColCount) >> 4);
					I2C(ADDIU,R20,R20,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));
					I2C(ADDIU,R21,R21,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));
				}

				DS(); I2P(BNE,R24,R7,LoopX);
			}
			else
			{
				I2P(BEQ,R24,R7,EndOfLine); // delay slot will be the next item
			}
		}

		InstPost(EndOfLine);

		I2C(LW,R2,SP,OFS(stack,SrcNext)); //srcpitch
		I2C(LW,R4,SP,OFS(stack,DstNext));
		I2C(LW,R5,SP,OFS(stack,EndOfRect));
		
		I2C(LW,R10,SP,OFS(stack,U));
		I2C(LW,R11,SP,OFS(stack,V));
		I2C(LW,R12,SP,OFS(stack,Y));

		//increment pointers
		I3(ADDU,R24,R24,R4);

		RowStep = (p->SwapXY) ? 2:1;

		//increment row of Y,U,V according to Pos -> Pos+RowStep*RScaleY
		I2C(SRL,R8,R22,4);
		I2C(ADDIU,R22,R22,RowStep*p->RScaleY);		//Pos += RowStep*RScaleY
		I2C(SRL,R9,R22,4);

		// dY  = ((Pos+RowStep*RScaleY) >> 4) - (Pos >> 4);
		I3(SUBU,R4,R9,R8); 
		I2(MULT,R4,R2); // dY * SrcPitch
		I1(MFLO,R4);
		I3(ADDU,R12,R12,R4);

	    // dUV = ((Pos+RowStep*RScaleY) >> (4+SrcUVY2)) - (Pos >> (4+SrcUVY2));
		if (p->SrcUVY2) I2C(SRL,R9,R9,p->SrcUVY2);
		if (p->SrcUVY2) I2C(SRL,R8,R8,p->SrcUVY2);
		I3(SUBU,R4,R9,R8); 
		I2(MULT,R4,R2); // dUV * (SrcPitch >> SrcUVX2)
		I1(MFLO,R4);
		if (p->SrcUVX2) I2C(SRA,R4,R4,p->SrcUVX2);
		I3(ADDU,R10,R10,R4);
		I3(ADDU,R11,R11,R4);

		I2C(SW,R10,SP,OFS(stack,U));
		I2C(SW,R11,SP,OFS(stack,V));
		I2C(SW,R12,SP,OFS(stack,Y));
	}
	else
	{
		reg Dst;

		Fix_RGB_UV_LoadUV(p,0,0);
		Fix_RGB_UV_Pixel(p,0,0);

		Dst = R24;
		if (!p->SwapXY)
			Dst = R1;

		Write(p,R24,R8,R9,1);
		I3(ADDU,Dst,R24,R25);

		if (p->DstDoubleY)
		{
			Write(p,Dst,R8,R9,0);
			I3(ADDU,Dst,Dst,R25);
		}
	
		Fix_RGB_UV_Pixel(p,1,0);

		Write(p,Dst,R8,R9,1);

		if (p->DstDoubleY)
		{
			I3(ADDU,Dst,Dst,R25);
			Write(p,Dst,R8,R9,0);
		}

		if (p->SwapXY)
			I3(ADDU,Dst,Dst,R25);
		else
			I2C(ADDIU,R24,R24,p->DstStepX);

		I2C(ADDIU,R12,R12,2);
		I2C(ADDIU,R14,R14,2);

		DS(); I2P(BNE,R24,R7,LoopX);

		I2C(LW,R2,SP,OFS(stack,SrcNext));
		I2C(LW,R4,SP,OFS(stack,DstNext));
		I2C(LW,R6,SP,OFS(stack,UVNext));
		I2C(LW,R5,SP,OFS(stack,EndOfRect));
		
		//increment pointers
		I3(ADDU,R12,R12,R2);
		I3(ADDU,R14,R14,R2);
		I3(ADDU,R24,R24,R4);
		I3(ADDU,R10,R10,R6);
		I3(ADDU,R11,R11,R6);
	}

	//prepare registers for next row
	I2C(LW,R4,SP,OFS(stack,EndOfLineInc));

	DS(); I2P(BNE,R24,R5,LoopY);

	CodeEnd(7,OFS(stack,StackFrame),0);
}

#endif
