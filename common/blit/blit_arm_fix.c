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
 * $Id: blit_arm_fix.c 543 2006-01-07 22:06:24Z picard $
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
	int EndOfLine;
	int EndOfRect;
	int DstPitch; 
	int DstNext;
	int YNext;
	int UVNext;

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

static NOINLINE void Fix_RGB_UV_LoadUV(blit_soft* p)
{
	//set R4 =           RVMul*v + RAdd
	//set R5 = GUMul*u + GVMul*v + GAdd
	//set R6 = BUMul*u +           BAdd

	if (p->Dither)
	{
		//R0,R4,R8 for temporary
		//R5(DiffMask),R6(Src2SrcLast),R7(EndOfLine)

		if (p->OnlyDiff)
		{
			Half(); I3(LDR,R8,R12,R6);
			Half(); I2C(LDR_POST,R0,R12,2);
			Half(); I3(LDR,R4,R14,R6);
			//xscale stall
			I3(EOR,R8,R8,R0);
			Half(); I2C(LDR_POST,R0,R14,2);
			S(); I3(TST,NONE,R8,R5);
			C(EQ); Byte(); I3(LDR,R8,R10,R6);
			C(EQ); I3(EOR,R4,R4,R0);
			Byte(); I2C(LDR_POST,R0,R10,1); //u
			C(EQ); S(); I3(TST,NONE,R4,R5);
			C(EQ); Byte(); I3(LDR,R4,R11,R6);
			C(EQ); I3(EOR,R8,R8,R0);
			Byte(); I2C(LDR_POST,R0,R11,1); //v
			C(EQ); S(); I3(TST,NONE,R8,R5);
			//xscale stall

			C(EQ); I3(EOR,R4,R4,R0);
			C(EQ); S(); I3(TST,NONE,R4,R5);

			if (p->SwapXY)
			{
				MB(); I2C(LDR,R8,SP,OFS(stack,DstPitch));
				C(EQ); I3S(ADD,R9,R9,R8,LSL,1+p->DstDoubleY);
			}
			else
			{
				C(EQ); I2C(ADD,R9,R9,p->DstStepX);
			}
			I0P(B,EQ,p->Skip);

			// R0=v
			if (p->ColorLookup)
			{
				Byte(); I2C(LDR,R5,R10,-1);	  //u
				I1P(MOV,R8,p->LookUp,0);
			}
		}
		else
		{
			Byte(); I2C(LDR_POST,R0,R11,1); //v
			if (p->ColorLookup)
			{
				Byte(); I2C(LDR_POST,R5,R10,1); //u
			}
		}

		if (!p->ColorLookup)
		{
			I1P(LDR,R5,p->GAdd,0);
			I1P(LDR,R8,p->GVMul,0);
			I1P(LDR,R4,p->RAdd,0);
			I1P(LDR,R6,p->RVMul,0);
			I4(MLA,R5,R8,R0,R5);
			
			Byte(); 
			if (p->OnlyDiff) //already incremented
				I2C(LDR,R8,R10,-1);	  //u
			else
				I2C(LDR_POST,R8,R10,1); //u

			I4(MLA,R4,R6,R0,R4);
			I1P(LDR,R7,p->GUMul,0);
			I1P(LDR,R6,p->BAdd,0);
			I1P(LDR,R0,p->BUMul,0);
			I4(MLA,R5,R7,R8,R5);

			//R7 Y read will moveback here (this will prevent stall by R0)

			I4(MLA,R6,R0,R8,R6);
			I1P(LDR,R8,p->YMul,0); //restore R8
		}
		else
		{
			I2C(ADD,R0,R0,p->LookUp_V);
			I3S(LDR,R4,R8,R0,LSL,2);	// RVMul+RAdd | GVMul+GAdd
			I2C(ADD,R5,R5,p->LookUp_U);
			I3S(LDR,R6,R8,R5,LSL,2);	// BUMul | GUMul
			I3S(MOV,R5,NONE,R4,LSL,16); 
			I3S(ADD,R5,R5,R6,LSL,16);	// GUMul+GVMul+GAdd | 0000
		}
	}
	else
	{
		//R0,R1,R2,R3,R4 for temporary
		//R5(DiffMask),R6(Src2SrcLast),R7(EndOfLine)

		if (p->OnlyDiff)
		{
			Half(); I3(LDR,R0,R12,R6);
			Half(); I2C(LDR_POST,R1,R12,2);
			Half(); I3(LDR,R4,R14,R6);	
			Half(); I2C(LDR_POST,R2,R14,2);
			I3(EOR,R0,R0,R1);
			S(); I3(TST,NONE,R0,R5);
			I3(EOR,R4,R4,R2);
			C(EQ); S(); I3(TST,NONE,R4,R5);

			C(EQ); Byte(); I3(LDR,R0,R10,R6);
			Byte(); I2C(LDR_POST,R1,R10,1); //u
			C(EQ); Byte(); I3(LDR,R4,R11,R6);
			Byte(); I2C(LDR_POST,R2,R11,1); //v
			C(EQ); I3(EOR,R0,R0,R1);
			C(EQ); S(); I3(TST,NONE,R0,R5);
			C(EQ); I3(EOR,R4,R4,R2);
			C(EQ); S(); I3(TST,NONE,R4,R5);

			if (p->SwapXY)
			{
				MB(); I2C(LDR,R3,SP,OFS(stack,DstPitch));
				C(EQ); I3S(ADD,R9,R9,R3,LSL,1+p->DstDoubleY);
			}
			else
			{
				C(EQ); I2C(ADD,R9,R9,p->DstStepX);
			}
			I0P(B,EQ,p->Skip);
		}
		else
		{
			Byte(); I2C(LDR_POST,R1,R10,1); //u
			Byte(); I2C(LDR_POST,R2,R11,1); //v
		}

		if (!p->ColorLookup)
		{
			I1P(LDR,R5,p->GAdd,0);
			I1P(LDR,R0,p->GVMul,0);
			I1P(LDR,R4,p->RAdd,0);
			I1P(LDR,R7,p->RVMul,0);
			I1P(LDR,R6,p->BAdd,0);
			I4(MLA,R5,R0,R2,R5); 
			I1P(LDR,R0,p->BUMul,0);
			I1P(LDR,R3,p->GUMul,0);
			I4(MLA,R4,R7,R2,R4); 
			I4(MLA,R6,R0,R1,R6); 
			I4(MLA,R5,R3,R1,R5); 
		}
		else
		{
			I2C(ADD,R1,R1,p->LookUp_U);
			I2C(ADD,R2,R2,p->LookUp_V);
			I3S(LDR,R6,R8,R1,LSL,2);	// BUMul+BAdd | GUMul
			I3S(LDR,R4,R8,R2,LSL,2);	// RVMul+RAdd | GVMul+GAdd
			//double xscale stall
			I3S(MOV,R5,NONE,R6,LSL,16); 
			I3S(ADD,R5,R5,R4,LSL,16);	// GUMul+GVMul+GAdd | 0000
		}
	}
}

void Fix_RGB_UV_Pixel(blit_soft* p, int Col, int Row)
{
	int SatBitR = p->QAdd ? 32 : 24;
	int SatBitG = SatBitR;
	int SatBitB = SatBitR;
	int RPos = p->DstPos[0];
	int GPos = p->DstPos[1];
	int BPos = p->DstPos[2];

	p->Upper = (p->DirX<0) ^ (Col>0);

	if (p->Upper && p->DstBPP==8 && p->DstDoubleX)
	{
		RPos += 16;
		GPos += 16;
		BPos += 16;
	}
	else
	if (p->Upper && p->DstBPP<=16)
	{
		RPos += p->DstBPP;
		GPos += p->DstBPP;
		BPos += p->DstBPP;
	}

	//load Y
	MB();
	Byte(); 

	if (p->OnlyDiff) //is R12,R14 already incremented?
	{
		if (p->SwapXY)
			I2C(LDR,R7,(reg)(Col==0?R12:R14),-2+Row);
		else
			I2C(LDR,R7,(reg)(Row==0?R14:R12),-2+Col);
	}
	else
		if (p->SwapXY)
			I2C(LDR_POST,R7,(reg)(Col==0?R12:R14),1);
		else
			I2C(LDR_POST,R7,(reg)(Row==0?R14:R12),1);

	if (p->Dither)
	{
		if (!p->ColorLookup)
		{
			I3S(ADD,R1,R4,R1,LSR,32-SatBitR+p->DstSize[0]);
			I3S(ADD,R2,R5,R2,LSR,32-SatBitG+p->DstSize[1]);
			I3S(ADD,R3,R6,R3,LSR,32-SatBitB+p->DstSize[2]);
			I4(MLA,R1,R8,R7,R1);
			I4(MLA,R2,R8,R7,R2);
			I4(MLA,R3,R8,R7,R3);
		}
		else
		{
			I3S(ADD,R1,R4,R1,LSL,16+LOOKUP_FIX-p->DstSize[0]);
			I3S(LDR,R7,R8,R7,LSL,2); // YMul * y
			I3S(ADD,R2,R5,R2,LSL,16+LOOKUP_FIX-p->DstSize[1]);
			I3S(ADD,R3,R6,R3,LSL,16+LOOKUP_FIX-p->DstSize[2]);
			I3(ADD,R1,R7,R1);
			I3(ADD,R2,R7,R2);
			I3(ADD,R3,R7,R3);
			Byte(); I3S(LDR,R1,R8,R1,LSR,16+LOOKUP_FIX); //sat and 8bit ror (8-RSize)
			Byte(); I3S(LDR,R2,R8,R2,LSR,16+LOOKUP_FIX); //sat and 8bit ror (8-GSize)
			Byte(); I3S(LDR,R3,R8,R3,LSR,16+LOOKUP_FIX); //sat and 8bit ror (8-BSize)

			// R1 = Dither[8-RSize] | Value[RSize]
			// R2 = Dither[8-GSize] | Value[GSize]
			// R3 = Dither[8-BSize] | Value[BSize]

			RPos += p->DstSize[0]; // LSB part -> MSB part
			GPos += p->DstSize[1]; // LSB part -> MSB part
			BPos += p->DstSize[2]; // LSB part -> MSB part

			SatBitR = 2*p->DstSize[0] - 32;
			SatBitG = 2*p->DstSize[1] - 32;
			SatBitB = 2*p->DstSize[2] - 32;
		}
	}
	else
	{
		if (!p->ColorLookup)
		{
			I4(MLA,R1,R8,R7,R4);
			I4(MLA,R2,R8,R7,R5);
			I4(MLA,R3,R8,R7,R6);
		}
		else
		{
			I3S(LDR,R7,R8,R7,LSL,2); // YMul * y
			I3(ADD,R1,R7,R4);
			I3(ADD,R2,R7,R5);
			I3(ADD,R3,R7,R6);
			Byte(); I3S(LDR,R1,R8,R1,LSR,16+LOOKUP_FIX); // sat to 8bit
			Byte(); I3S(LDR,R2,R8,R2,LSR,16+LOOKUP_FIX); // sat to 8bit
			Byte(); I3S(LDR,R3,R8,R3,LSR,16+LOOKUP_FIX); // sat to 8bit
			SatBitR = SatBitB = SatBitG = 8;
		}
	}

	if (!p->ColorLookup)
	{
		if (p->QAdd)
		{
			I3(QDADD,R1,R1,R1);
			I3(QDADD,R2,R2,R2);
			I3(QDADD,R3,R3,R3);
		}
		else
		{
			I2C(TST,NONE,R1,0xFF000000);
			C(NE);I2C(MVN,R1,NONE,0xFF000000);
			C(MI);I2C(MOV,R1,NONE,0x00000000);
			I2C(TST,NONE,R2,0xFF000000);
			C(NE);I2C(MVN,R2,NONE,0xFF000000);
			C(MI);I2C(MOV,R2,NONE,0x00000000);
			I2C(TST,NONE,R3,0xFF000000);
			C(NE);I2C(MVN,R3,NONE,0xFF000000);
			C(MI);I2C(MOV,R3,NONE,0x00000000);
		}
	}

	if (p->InvertMask && p->Pos<0)
	{
		p->Pos = RPos;
		MB(); I1P(LDR,R0,p->InvertMask,0);
	}
	if (p->Pos!=RPos && p->Pos>=0) I3S(MOV,R0,NONE,R0,ROR,RPos-p->Pos);
	I3S(p->Pos<0?MOV:EOR,R0,(reg)(p->Pos<0?NONE:R0),R1,LSR,SatBitR-p->DstSize[0]);
	I3S(MOV,R7,NONE,R2,LSR,SatBitG-p->DstSize[1]);
	I3S(MOV,R0,NONE,R0,ROR,BPos-RPos);
	I3S(EOR,R0,R0,R7,ROR,BPos-GPos);
	I3S(EOR,R0,R0,R3,LSR,SatBitB-p->DstSize[2]);
	p->Pos = BPos;

	if (p->Dither && !p->ColorLookup)
	{
		MB(); I3S(MOV,R1,NONE,R1,LSL,32-SatBitR+p->DstSize[0]);
		MB(); I3S(MOV,R2,NONE,R2,LSL,32-SatBitG+p->DstSize[1]);
		MB(); I3S(MOV,R3,NONE,R3,LSL,32-SatBitB+p->DstSize[2]);
	}
}

void Fix_RGB_UV(blit_soft* p)
{
	dyninst* LoopY;
	dyninst* LoopX;
	int Invert = 0;
	int Mask = 0;

	p->DstAlignSize = 4;
	p->Dither = (boolmem_t)((p->FX.Flags & BLITFX_DITHER)!=0);
	if (p->DstDoubleX || p->DstDoubleY || p->DstBPP>16)
		p->Dither = 0;

	p->DstStepX = p->DirX * ((p->DstBPP*2) >> 3) << p->DstDoubleX;
	p->LookUp = NULL;
	p->PalPtr = NULL;
	p->DiffMask = NULL;
	p->InvertMask = NULL;

	if (p->Dst.Flags & PF_INVERTED)
		Invert = -1;

	if (p->ColorLookup)
	{
		CalcLookUp(p,p->Dither);
		p->LookUp = InstCreate(p->LookUp_Data,p->LookUp_Size,NONE,NONE,NONE,0,0);
		free(p->LookUp_Data);
		p->LookUp_Data = NULL;
	}

	if (p->QAdd)
	{
		int Mask2;
		int i,Shift;
		
		for (i=0;i<3;++i)
			Mask |= 1 << (p->DstPos[i] + p->DstSize[i] -1);

		Mask2 = Mask;
		Shift = 0;
		if (p->DstBPP==8 && p->DstDoubleX)
			Mask2 |= Mask << 16;
		else
		if (p->DstBPP <= 16)
		{
			if (p->DirX<0) Shift = p->DstBPP;
			Mask2 |= Mask << p->DstBPP;
		}
		
		Invert ^= RotateRight(Mask2,Shift+p->DstPos[0]);
	}

	p->YMul = InstCreate32(p->_YMul,NONE,NONE,NONE,0,0);
	p->RVMul = InstCreate32(p->_RVMul,NONE,NONE,NONE,0,0);
	p->RAdd = InstCreate32(p->_RAdd,NONE,NONE,NONE,0,0);
	p->GUMul = InstCreate32(p->_GUMul,NONE,NONE,NONE,0,0);
	p->GVMul = InstCreate32(p->_GVMul,NONE,NONE,NONE,0,0);
	p->GAdd = InstCreate32(p->_GAdd,NONE,NONE,NONE,0,0);
	p->BUMul = InstCreate32(p->_BUMul,NONE,NONE,NONE,0,0);
	p->BAdd = InstCreate32(p->_BAdd,NONE,NONE,NONE,0,0);

	if (Invert)
		p->InvertMask = InstCreate32(Invert,NONE,NONE,NONE,0,0);

	if (p->OnlyDiff)
		p->DiffMask = InstCreate32(0xFCFCFCFC,NONE,NONE,NONE,0,0);

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2C(LDR,R9,R1,0); //Dst[0] RGB
	I2C(LDR,R10,R2,4); //Src[1] U
	I2C(LDR,R11,R2,8); //Src[2] V
	I2C(LDR,R12,R2,0); //Src[0] Y
	I2C(STR,R3,SP,OFS(stack,DstPitch));

	I3(MOV,R6,NONE,R3); //DstPitch
	I2C(LDR,R7,SP,OFS(stack,SrcPitch));
	I2C(LDR,R0,SP,OFS(stack,Height));
	I2C(LDR,R4,SP,OFS(stack,Width));

	if (!p->ColorLookup)
		I1P(LDR,R8,p->YMul,0);
	else
		I1P(MOV,R8,p->LookUp,0);

	//YNext = 2*Src->Pitch - (Width >> SrcDoubleX)
	I3S(MOV,R1,NONE,R7,LSL,1);
	I3S(SUB,R1,R1,R4,LSR,p->SrcDoubleX); 
	I2C(STR,R1,SP,OFS(stack,YNext));

	//UVNext = (Src->Pitch >> 1) - (Width >> SrcDoubleX >> 1);
	I3S(MOV,R2,NONE,R7,ASR,1);
	I3S(SUB,R2,R2,R4,LSR,p->SrcDoubleX+1); 
	I2C(STR,R2,SP,OFS(stack,UVNext));

	if (p->DirX<0 && p->DstBPP==16) //adjust reversed destination for block size
		I2C(SUB,R9,R9,-p->DstStepX-(p->DstBPP >> 3));
	if (p->DstBPP==32)
		I2C(ADD,R9,R9,p->DstStepX/2);

	if (p->SwapXY)
	{
		// EndOfRect = Dst + ((Height * DstBPP * DirX) >> 3) - (DstPitch << DstDoubleY)
		I3S(SUB,R9,R9,R6,LSL,p->DstDoubleY);
		I2C(MOV,R1,NONE,p->DstBPP * p->DirX);
		I3(MUL,R0,R1,R0);
		I3S(ADD,R0,R9,R0,ASR,3);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = DstStepX - Width*DstPitch;
		MB(); I3(MUL,R2,R6,R4);
		I2C(MOV,R0,NONE,p->DstStepX); 
		I3(SUB,R0,R0,R2); 
		I2C(STR,R0,SP,OFS(stack,DstNext));
	}
	else
	{
		// EndOfRect = Dst + DstPitch * Height
		I3(MUL,R0,R6,R0);
		I3(ADD,R0,R9,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = ((DstPitch*2 << DstDoubleY) - DirX * Width << DstBPP2;
		I3S(MOV,R2,NONE,R6,LSL,p->DstDoubleY+1);
		I3S(p->DirX>0?SUB:ADD,R2,R2,R4,LSL,p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}
	I3(ADD,R14,R12,R7);

	if (p->Dither)
	{
		if (!p->ColorLookup)
		{
			I2C(MVN,R1,NONE,0x80000000);
			I2C(MVN,R2,NONE,0x80000000);
			I2C(MVN,R3,NONE,0x80000000);
		}
		else
		{
			I2C(MOV,R1,NONE,0x80);
			I2C(MOV,R2,NONE,0x80);
			I2C(MOV,R3,NONE,0x80);
		}
	}

	LoopY = Label(1);

	if (p->SwapXY)
	{
		I3(MUL,R0,R6,R4); //R6=dstpitch
		I3(ADD,R7,R9,R0);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R7,R9,R4,LSL,p->DstBPP2);
		else
			I3S(SUB,R7,R9,R4,LSL,p->DstBPP2);
	}
	I2C(STR,R7,SP,OFS(stack,EndOfLine));

	// preload
	if (p->ARM5) // not needed for non slices mode, but testing show it didn't help on ARM4
	{
		if (!p->Slices)
		{
			//R4 width
			//R0,R7,R5,R6 tmp

			dyninst* PreLoadEnd = Label(0);
			dyninst* PreLoad1;
			dyninst* PreLoad2;
			dyninst* PreLoad3;
			dyninst* PreLoad4;

			I3S(ADD,R0,R12,R4,ASR,(p->SrcDoubleX?1:0)-(p->SrcHalfX?1:0));
			I2C(ADD,R5,R12,32);
			I3(CMP,NONE,R5,R0);
			I0P(B,CS,PreLoadEnd);

			//y0
			PreLoad1 = Label(1);
			Byte(); I2C(LDR,R6,R5,-32);
			I2C(ADD,R5,R5,64);
			I3(CMP,NONE,R5,R0);
			Byte(); I2C(LDR,R7,R5,-64);
			I0P(B,CC,PreLoad1);

			I3S(ADD,R0,R14,R4,ASR,(p->SrcDoubleX?1:0)-(p->SrcHalfX?1:0));
			I2C(ADD,R5,R14,32);

			//y1
			PreLoad2 = Label(1);
			Byte(); I2C(LDR,R6,R5,-32);
			I2C(ADD,R5,R5,64);
			I3(CMP,NONE,R5,R0);
			Byte(); I2C(LDR,R7,R5,-64);
			I0P(B,CC,PreLoad2);

			I3S(ADD,R0,R10,R4,ASR,(p->SrcDoubleX?1:0)-(p->SrcHalfX?1:0)+p->SrcUVX2);
			I2C(ADD,R5,R10,32);
			I3(CMP,NONE,R5,R0);
			I0P(B,CS,PreLoadEnd);

			//u
			PreLoad3 = Label(1);
			Byte(); I2C(LDR,R6,R5,-32);
			I2C(ADD,R5,R5,64);
			I3(CMP,NONE,R5,R0);
			Byte(); I2C(LDR,R7,R5,-64);
			I0P(B,CC,PreLoad3);

			I3S(ADD,R0,R11,R4,ASR,(p->SrcDoubleX?1:0)-(p->SrcHalfX?1:0)+p->SrcUVX2);
			I2C(ADD,R5,R11,32);

			//v
			PreLoad4 = Label(1);
			Byte(); I2C(LDR,R6,R5,-32);
			I2C(ADD,R5,R5,64);
			I3(CMP,NONE,R5,R0);
			Byte(); I2C(LDR,R7,R5,-64);
			I0P(B,CC,PreLoad4);

			if (p->OnlyDiff) //restore R7
				I2C(LDR,R7,SP,OFS(stack,EndOfLine));

			InstPost(PreLoadEnd);
		}
		else
		{
			//preload next
			MB(); I2C(LDR,R6,SP,OFS(stack,SrcPitch));
			I3S(PLD,NONE,R12,R6,LSL,1);
			I3S(PLD,NONE,R14,R6,LSL,1);
			I3S(PLD,NONE,R10,R6,ASR,p->SrcUVPitch2);
			I3S(PLD,NONE,R11,R6,ASR,p->SrcUVPitch2);
		}
	}

    if (p->OnlyDiff)
	{
		MB(); I1P(LDR,R5,p->DiffMask,0);
		MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
		p->Skip = Label(0);
	}

	LoopX = Label(1);
	{
		int PitchDouble;
		reg Pitch;

		Fix_RGB_UV_LoadUV(p);

		p->Pos = -1;

		Fix_RGB_UV_Pixel(p,0,0);
		if (p->DstBPP==32)
		{
			if (p->Pos)
				I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

			assert(!p->DstDoubleX && !p->DstDoubleY);
			MB(); I2C(LDR,R1,SP,OFS(stack,DstPitch));
			I2C(ADD,R1,R1,-p->DstStepX/2);
			I3(STR,R0,R9,R1);
			p->Pos = -1;
		}

		Fix_RGB_UV_Pixel(p,1,0);
		if (p->Pos) 
			I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

		Pitch = (reg)(p->Dither ? R7:R1);
		MB(); I2C(LDR,Pitch,SP,OFS(stack,DstPitch));

		if (p->DstBPP==8 && p->DstDoubleX) 
			I3S(ORR,R0,R0,R0,LSL,8);

		if (p->DstBPP==16 && p->DstDoubleX) 
		{
			I2C(ADD,R9,R9,4);
			I3S(MOV,R3,NONE,R0,LSR,16);
			I3S(MOV,R0,NONE,R0,LSL,16);
			I3S(ORR,R3,R3,R3,LSL,16);
			I3S(ORR,R0,R0,R0,LSR,16);

			if (p->DstDoubleY)
			{
				I3S(ADD,R2,Pitch,Pitch,LSL,1); //R2=3*DstPitch
				I3(STR,R3,R9,R2);
			}
			I3S(STR,R3,R9,Pitch,LSL,p->DstDoubleY);
			I2C(SUB,R9,R9,4);
		}

		if (p->DstDoubleY)
		{
			I3S(ADD,R2,Pitch,Pitch,LSL,1); //R2=3*DstPitch
			if (p->DstBPP==8 && !p->DstDoubleX) Half();
			I3(STR,R0,R9,R2);
		}
		
		PitchDouble = p->DstDoubleY;
		if (p->DstBPP==8 && !p->DstDoubleX) 
		{
			if (PitchDouble) // can't use STR with Half() and LSL,#1 at the same time
			{
				PitchDouble = 0;
				I3(ADD,Pitch,Pitch,Pitch);
			}
			Half();
		}
		I3S(STR,R0,R9,Pitch,LSL,PitchDouble);
		if (p->SwapXY)
			I3S(ADD,R9,R9,Pitch,LSL,1+PitchDouble);

		p->Pos = -1;
		Fix_RGB_UV_Pixel(p,0,1);
		if (p->DstBPP==32)
		{
			if (p->Pos)
				I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

			assert(!p->DstDoubleX && !p->DstDoubleY);
			I2C(STR,R0,R9,-p->DstStepX/2);
			p->Pos = -1;
		}

		Fix_RGB_UV_Pixel(p,1,1);
		if (p->Pos) 
			I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

		if (p->DstBPP==8 && p->DstDoubleX) 
			I3S(ORR,R0,R0,R0,LSL,8);

		if (p->DstDoubleY)
		{
			MB(); I2C(LDR,R1,SP,OFS(stack,DstPitch));
		}

		if (p->DstBPP==16 && p->DstDoubleX) 
		{
			I3S(MOV,R3,NONE,R0,LSR,16);
			I3S(MOV,R0,NONE,R0,LSL,16);
			I3S(ORR,R3,R3,R3,LSL,16);
			I3S(ORR,R0,R0,R0,LSR,16);

			if (p->DstDoubleY)
			{
				I2C(ADD,R2,R1,4); //DstPitch+4
				I3(STR,R3,R9,R2);
			}
			I2C(STR,R3,R9,4);
		}

		if (p->DstDoubleY)
		{
			if (p->DstBPP==8 && !p->DstDoubleX) Half();
			I3(STR,R0,R9,R1);
		}

		if (p->DstBPP==8 && !p->DstDoubleX) Half();
		if (p->SwapXY)
			I2(STR,R0,R9);
		else
			I2C(STR_POST,R0,R9,p->DstStepX);

		MB(); I2C(LDR,R7,SP,OFS(stack,EndOfLine));
		if (p->OnlyDiff)
		{
			MB(); I1P(LDR,R5,p->DiffMask,0);
			MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
			InstPost(p->Skip);
		}

		I3(CMP,NONE,R9,R7);
		I0P(B,NE,LoopX);
	}

	I2C(LDR,R0,SP,OFS(stack,YNext));
	I2C(LDR,R4,SP,OFS(stack,DstNext));
	I2C(LDR,R6,SP,OFS(stack,UVNext));
	I2C(LDR,R5,SP,OFS(stack,EndOfRect));
	
	//increment pointers
	I3(ADD,R12,R12,R0);
	I3(ADD,R14,R14,R0);
	I3(ADD,R9,R9,R4);
	I3(ADD,R10,R10,R6);
	I3(ADD,R11,R11,R6);

	//prepare registers for next row
	if (p->SwapXY) I2C(LDR,R6,SP,OFS(stack,DstPitch));
	I2C(LDR,R4,SP,OFS(stack,Width));

	I3(CMP,NONE,R9,R5);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();

	InstPost(p->YMul);
	InstPost(p->RVMul);
	InstPost(p->RAdd);
	InstPost(p->GUMul);
	InstPost(p->GVMul);
	InstPost(p->GAdd);
	InstPost(p->BUMul);
	InstPost(p->BAdd);
	if (p->InvertMask) InstPost(p->InvertMask);
	if (p->DiffMask) InstPost(p->DiffMask);
	if (p->PalPtr) InstPost(p->PalPtr);
	if (p->LookUp)
	{
		Align(16);
		InstPost(p->LookUp);
	}
}

#endif
