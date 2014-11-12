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
 * $Id: blit_arm_stretch.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

//todo: ReUse with OnlyDiff condition. optimize IncPtr (change to pre increment)

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(ARM)

typedef struct stack
{
	int InvertMask;
	int DiffMask;
	int PalPtr;
	int EndOfLine;
	int EndOfRect;
	int DstPitch;
	int DstNext;
	int Y;
	int U;
	int V;
	int Pos;
	int SaveR7; //onlydifference && dither
	int SaveR8; //onlydifference && dither

	int StackFrame[STACKFRAME];

	//void* this   R0
	//char* Dst    R1
	//char* Src    R2
	//int DstPitch R3
	int SrcPitch;
	int Width; 
	int Height;
	int Src2SrcLast;
} stack;

// palette
// 

// R0 result RGB
// R1..R3 R,G,B accumulator
// R4..R6 temporary

// R9 Dst
// R7 U+UVNextRow (when SwapXY)
// R8 V+UVNextRow (when SwapXY)
// R10 U
// R11 V
// R12 Y
// R14 Y+YNextRow (when SwapXY)

static NOINLINE void Inc_RGB_UV_Pixel(blit_soft* p, int dY, int dUV)
{
	bool_t NextRow = (p->SwapXY) && (p->Upper == (p->DirX>0));
	reg Y,V,U;

	Y = (reg)(NextRow ? R14:R12);
	V = (reg)(NextRow ? R8:R11);
	U = (reg)(NextRow ? R7:R10);

	if (dY) I2C(ADD,Y,Y,dY);
	if (dUV) I2C(ADD,U,U,dUV);
	if (dUV) I2C(ADD,V,V,dUV);
}

static NOINLINE void Add_RGB_UV_Pixel(blit_soft* p, int Amount, int dY, int dUV, bool_t IncPtr)
{
	assert(Amount>0 || !IncPtr);

	if (Amount)
	{
		int Load = IncPtr ? LDR_POST : LDR;
		reg Y,V,U;
		bool_t NextRow = (p->SwapXY) && (p->Upper == (p->DirX>0));

		Y = (reg)(NextRow ? R14:R12);
		V = (reg)(NextRow ? R8:R11);
		U = (reg)(NextRow ? R7:R10);

		if (p->DstPalette)
		{
			Byte(); I2C(Load,R4,Y,dY);  //y
			Byte(); I2C(Load,R6,V,dUV); //v
			Byte(); I2C(Load,R5,U,dUV); //u
		}
		else
		{
			Byte(); I2C(Load,R4,Y,dY); //y
			IConst(R5,(p->_YMul >> 8) * Amount);
			Byte(); I2C(Load,R6,V,dUV); //v
			I4(MLA,R1,R5,R4,R1);
			I4(MLA,R2,R5,R4,R2);
			I4(MLA,R3,R5,R4,R3);
			Byte(); I2C(Load,R4,U,dUV); //u
			IConst(R5,(p->_GVMul >> 8) * Amount);
			I4(MLA,R2,R5,R6,R2);
			IConst(R5,(p->_RVMul >> 8) * Amount);
			I4(MLA,R1,R5,R6,R1);
			IConst(R5,(p->_GUMul >> 8) * Amount);
			I4(MLA,R2,R5,R4,R2);
			IConst(R5,(p->_BUMul >> 8) * Amount);
			I4(MLA,R3,R5,R4,R3);
		}
	}
}

static NOINLINE void Stretch_RGB_UV_Pixel(blit_soft* p, int Col, int dY, int dUV)
{
	int SatBit = p->QAdd ? 32 : 24;
	int RPos = p->DstPos[0];
	int GPos = p->DstPos[1];
	int BPos = p->DstPos[2];

	if (p->Upper && p->DstBPP<=16)
	{
		RPos += p->DstBPP;
		GPos += p->DstBPP;
		BPos += p->DstBPP;
	}

	if (!p->DstPalette)
	{
		if (p->Dither)
		{
			IConst(R4,p->_RAdd);
			IConst(R5,p->_GAdd);
			IConst(R6,p->_BAdd);
			I3S(ADD,R1,R4,R1,LSR,32-SatBit+p->DstSize[0]);
			I3S(ADD,R2,R5,R2,LSR,32-SatBit+p->DstSize[1]);
			I3S(ADD,R3,R6,R3,LSR,32-SatBit+p->DstSize[2]);
		}
		else
		{
			IConst(R1,p->_RAdd);
			IConst(R2,p->_GAdd);
			IConst(R3,p->_BAdd);
		}
	}

	if (p->ArithStretch)
	{
		int x = Col * p->RScaleX;

		Add_RGB_UV_Pixel(p,16*(16-(x & 15)),dY,dUV,!p->OnlyDiff);

		if (!p->OnlyDiff)
		{
			dY = -dY;
			dUV = -dUV;
		}

		++dY;
		if (((x >> 4)&1) || p->SrcUVX2==0) //need UV increment?
			++dUV;

		Add_RGB_UV_Pixel(p,16*(x & 15),dY,dUV,0);
	}
	else
		Add_RGB_UV_Pixel(p,256,dY,dUV,!p->OnlyDiff);

	if (p->DstPalette)
	{
		if (p->Dither)
		{
			I3S(ADD,R1,R1,R4,ROR,0);
			I3S(ADD,R2,R2,R5,ROR,0);
			I3S(ADD,R2,R2,R6,ROR,16);

			I2C(AND,R4,R1,0x1E0);
			I2C(AND,R5,R2,0x1E0);
			I2C(AND,R6,R2,0x1E00000);

			I3S(ADD,R0,R3,R4,LSL,5);
			I3S(ADD,R0,R0,R5,LSL,1);
			I3S(ADD,R0,R0,R6,LSR,3+16);

			Byte(); I2C(LDR,R4,R0,1);
			Byte(); I2C(LDR,R5,R0,2);
			Byte(); I2C(LDR,R6,R0,3);
			Byte(); I2C(LDR,R0,R0,0);

			I3S(SUB,R1,R1,R4,ROR,0);
			I3S(SUB,R2,R2,R5,ROR,0);
			I3S(SUB,R2,R2,R6,ROR,16);
		}
		else
		{
			I2C(AND,R4,R4,0x1E0);
			I2C(AND,R5,R5,0x1E0);
			I2C(AND,R6,R6,0x1E0);

			I3S(ADD,R0,R3,R4,LSL,5);
			I3S(ADD,R0,R0,R5,LSL,1);
			I3S(ADD,R0,R0,R6,LSR,3);

			Byte(); I2C(LDR,R0,R0,0);
		}
		p->Pos = 0;
	}
	else
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

		if (p->InvertMask && p->Pos<0)
		{
			p->Pos = RPos;
			MB(); I2C(LDR,R0,SP,OFS(stack,InvertMask));
		}
		if (p->Pos!=RPos && p->Pos>=0) I3S(MOV,R0,NONE,R0,ROR,RPos-p->Pos);
		I3S(p->Pos<0?MOV:EOR,R0,(reg)(p->Pos<0?NONE:R0),R1,LSR,SatBit-p->DstSize[0]);
		I3S(MOV,R4,NONE,R2,LSR,SatBit-p->DstSize[1]);
		I3S(MOV,R0,NONE,R0,ROR,BPos-RPos);
		I3S(EOR,R0,R0,R4,ROR,BPos-GPos);
		I3S(EOR,R0,R0,R3,LSR,SatBit-p->DstSize[2]);
		p->Pos = BPos;

		if (p->Dither)
		{
			MB(); I3S(MOV,R1,NONE,R1,LSL,32-SatBit+p->DstSize[0]);
			MB(); I3S(MOV,R2,NONE,R2,LSL,32-SatBit+p->DstSize[1]);
			MB(); I3S(MOV,R3,NONE,R3,LSL,32-SatBit+p->DstSize[2]);
		}
	}
}

void Stretch_RGB_UV(blit_soft* p)
{
	bool_t Special;
	dyninst* LoopX;
	dyninst* LoopY;
	dyninst* EndOfLine;
	int i,Col,ColCount,RowStep;
	int Mask = 0;
	int Invert = 0;
	bool_t ReUse;
	bool_t NoInc;
	bool_t RegDirty; //R4,R5

	p->Dither = (boolmem_t)((p->DstBPP<=16) && (p->FX.Flags & BLITFX_DITHER)!=0);
	p->DstStepX = p->DirX * ((p->DstBPP*2) >> 3);
	p->PalPtr = NULL;
	p->DiffMask = NULL;
	p->InvertMask = NULL;

	if (p->Dst.Flags & PF_INVERTED)
		Invert = -1;

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2C(LDR,R9,R1,0);  //Dst[0] RGB
	I2C(LDR,R10,R2,4); //Src[1] U
	I2C(LDR,R11,R2,8); //Src[2] V
	I2C(LDR,R12,R2,0); //Src[0] Y

	I2C(STR,R10,SP,OFS(stack,U));
	I2C(STR,R11,SP,OFS(stack,V));
	I2C(STR,R12,SP,OFS(stack,Y));

	ReUse = p->RScaleX<16 && !Invert && !p->OnlyDiff && (!p->OnlyDiff || !p->SwapXY) && (!p->DstPalette || !p->SwapXY) && !p->ArithStretch; //R0 is saved in loop and not ArithStretch

	if (p->QAdd && ReUse && !p->SwapXY)
	{
		p->QAdd = 0; // invert not supported with ReUse
		CalcColor(p);
	}

	if (p->QAdd)
	{
		int Mask2,Shift,i;

		for (i=0;i<3;++i)
			Mask |= 1 << (p->DstPos[i] + p->DstSize[i] - 1);

		Mask2 = Mask;
		Shift = 0;
		if (p->DstBPP <= 16)
		{
			if (p->DirX<0) Shift = p->DstBPP;
			Mask2 |= Mask << p->DstBPP;
		}

		Invert ^= RotateRight(Mask2,Shift+p->DstPos[0]);
	}

	if (Invert)
	{
		p->InvertMask = InstCreate32(Invert,NONE,NONE,NONE,0,0);
		MB(); I1P(LDR,R5,p->InvertMask,0);
		I2C(STR,R5,SP,OFS(stack,InvertMask));
	}

	if (p->OnlyDiff)
	{
		p->DiffMask = InstCreate32(0xFCFCFCFC,NONE,NONE,NONE,0,0);
		MB(); I1P(LDR,R4,p->DiffMask,0);
		I2C(STR,R4,SP,OFS(stack,DiffMask));
	}

	I2C(STR,R3,SP,OFS(stack,DstPitch));

	I3(MOV,R6,NONE,R3); //DstPitch
	I2C(LDR,R7,SP,OFS(stack,SrcPitch));
	I2C(LDR,R0,SP,OFS(stack,Height));
	I2C(LDR,R4,SP,OFS(stack,Width));

	if (p->DirX<0 && p->DstBPP==16) //adjust reversed destination for block size
		I2C(SUB,R9,R9,-p->DstStepX-(p->DstBPP >> 3));
	if ((p->DstPalette || p->DstBPP==32) && p->SwapXY)
		I2C(ADD,R9,R9,p->DstStepX/2);

	if (p->SwapXY)
	{
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
		I3(MUL,R0,R6,R0); //DstPitch * Height
		I3(ADD,R0,R9,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = DstPitch - DirX * Width << DstBPP2;
		I3S(p->DirX>0?SUB:ADD,R2,R6,R4,LSL,p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}

	if (p->Dst.Flags & PF_PALETTE)
	{
		p->PalPtr = InstCreate32((int)p->LookUp_Data,NONE,NONE,NONE,0,0);
		MB(); I1P(LDR,R3,p->PalPtr,0);
		if (p->Dither)
		{
			I2C(MOV,R1,NONE,0);
			I2C(MOV,R2,NONE,0);
		}
	}
	else
	{
		if (p->Dither)
		{
			I2C(MVN,R1,NONE,0x80000000);
			I2C(MVN,R2,NONE,0x80000000);
			I2C(MVN,R3,NONE,0x80000000);
		}
	}

	I2C(MOV,R5,NONE,0); //Pos

	LoopY = Label(0);
	I0P(B,AL,LoopY);

	if (p->InvertMask) InstPost(p->InvertMask);
	if (p->DiffMask) InstPost(p->DiffMask);
	if (p->PalPtr) InstPost(p->PalPtr);

	ColCount = 32;
	for (i=1;i<16 && !(p->RScaleX & i);i<<=1)
		ColCount >>= 1;

	InstPost(LoopY);

	EndOfLine = Label(0);

	//R4 Width
	//R5 Pos
	//R7 SrcPitch

	if (p->SwapXY)
	{
		I3(MUL,R4,R6,R4); //R6=DstPitch
		I3(ADD,R4,R9,R4);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R4,R9,R4,LSL,p->DstBPP2);
		else
			I3S(SUB,R4,R9,R4,LSL,p->DstBPP2);
	}
	I2C(STR,R4,SP,OFS(stack,EndOfLine));

	if (p->SwapXY)
	{
		//we need next row in R14,R7,R8 (YUV)

		IMul(R6,R5,p->RScaleY); //R6 = RScaleY*Pos
		I2C(ADD,R8,R6,p->RScaleY);//R8 = RScaleY*(Pos+1)

		// dY  = ((RScaleY * (Pos+1)) >> 4) - ((RScaleY * Pos) >> 4);
		I3S(MOV,R14,NONE,R8,LSR,4);
		I3S(SUB,R14,R14,R6,LSR,4);
		I3(MUL,R14,R7,R14);

		// dUV = ((RScaleY * (Pos+1)) >> (4+SrcUVY2)) - ((RScaleY * Pos) >> (4+SrcUVY2));
		I3S(MOV,R8,NONE,R8,LSR,4+p->SrcUVY2);
		I3S(SUB,R8,R8,R6,LSR,4+p->SrcUVY2);
		I3(MUL,R8,R7,R8);

		I3(ADD,R14,R12,R14);
		I3S(ADD,R7,R10,R8,ASR,p->SrcUVX2);
		I3S(ADD,R8,R11,R8,ASR,p->SrcUVX2);
	}

	I2C(STR,R5,SP,OFS(stack,Pos));

	// special case, when R7,R8 must be saved/restored and incremented separatly
	Special = p->OnlyDiff && (p->Dither) && (p->SwapXY);
	if (Special)
	{
		I2C(STR,R7,SP,OFS(stack,SaveR7));
		I2C(STR,R8,SP,OFS(stack,SaveR8));
	}

	if (p->OnlyDiff)
	{
		MB(); I2C(LDR,R5,SP,OFS(stack,DiffMask));
		MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
	}

	LoopX = Label(1);
	RegDirty = 1;
	NoInc = 0;

	for (Col=0;Col<ColCount;)
	{
		int x = (Col * p->RScaleX) >> 4;

		int IncY;
		int IncUV;
		int IncY2 = 0;
		int IncUV2 = 0;
		int Col1 = Col;
		int Col2 = Col;

		IncY = (((Col+1) * p->RScaleX) >> 4) - ((Col * p->RScaleX) >> 4);
		IncUV = (((Col+1) * p->RScaleX) >> (4+p->SrcUVX2)) - ((Col * p->RScaleX) >> (4+p->SrcUVX2));
		Col++;

		if (!(p->SwapXY))
		{
			IncY2 = (((Col+1) * p->RScaleX) >> 4) - ((Col * p->RScaleX) >> 4);
			IncUV2 = (((Col+1) * p->RScaleX) >> (4+p->SrcUVX2)) - ((Col * p->RScaleX) >> (4+p->SrcUVX2));

			Col2++;
			Col++;
		}
		else
		if (!p->OnlyDiff)
		{
			IncY2 = IncY;
			IncUV2 = IncUV;
		}

		if (p->OnlyDiff)
		{
			reg RC = (reg)(p->SwapXY ? R0 : R14);
			reg RA = (reg)(p->Dither ? R7 : R1);
			reg RB = (reg)(p->Dither ? R8 : R2);

			RegDirty = 1;
			p->Skip = Label(0);

			//RC,RA,RB,R4 for temporary
			//R5(DiffMask),R6(Src2SrcLast)

			if (p->SwapXY)
			{
				Byte(); I3(LDR,RC,R12,R6);
				Byte(); I2C(LDR_POST,RA,R12,IncY);
				Byte(); I3(LDR,R4,R14,R6);	
				Byte(); I2C(LDR_POST,RB,R14,IncY);

				IncY = IncY2 = -IncY;
			}
			else
			{
				Byte(); I3(LDR,RC,R12,R6);
				Byte(); I2C(LDR_POST,RA,R12,IncY);
				Byte(); I3(LDR,R4,R12,R6);	
				Byte(); I2C(LDR_POST,RB,R12,IncY2);

				IncY = -IncY-IncY2;
				IncY2 = -IncY2;
			}

			I3(EOR,RC,RC,RA);
			S(); I3(TST,NONE,RC,R5);
			I3(EOR,R4,R4,RB);
			C(EQ); S(); I3(TST,NONE,R4,R5);

			C(EQ); Byte(); I3(LDR,RC,R10,R6); //last u
			Byte(); I2C(LDR_POST,RA,R10,IncUV+IncUV2); //u
			C(EQ); Byte(); I3(LDR,R4,R11,R6); //last v
			Byte(); I2C(LDR_POST,RB,R11,IncUV+IncUV2); //v

			IncUV = -IncUV-IncUV2;

			if (Special) //R7,R8 not incremented, using direct position
				IncUV2 = x >> p->SrcUVX2;
			else
			if (p->SwapXY) //IncUV2 will increment R7,R8
			{
				I2C(ADD,R7,R7,-IncUV);
				I2C(ADD,R8,R8,-IncUV);
				IncUV2 = IncUV;
			}
			else
				IncUV2 = -IncUV2;

			C(EQ); I3(EOR,RC,RC,RA);
			C(EQ); S(); I3(TST,NONE,RC,R5);
			C(EQ); I3(EOR,R4,R4,RB);
			C(EQ); S(); I3(TST,NONE,R4,R5);

			C(EQ); I2C(LDR,R4,SP,OFS(stack,EndOfLine));

#ifdef SHOWDIFF
			I2C(MVN,RC,NONE,0);
			if (DstBPP==8) Half();
			C(EQ); I2(STR,RC,R9);
#endif

			if (p->SwapXY)
			{
				MB(); I2C(LDR,RC,SP,OFS(stack,DstPitch));
				C(EQ); I3(ADD,R9,R9,RC);
			}
			else
			{
				C(EQ); I2C(ADD,R9,R9,p->DstStepX);
			}
			I0P(B,EQ,p->Skip);

			if (Special)
			{
				I2C(LDR,R7,SP,OFS(stack,SaveR7));
				I2C(LDR,R8,SP,OFS(stack,SaveR8));
			}
		}

		p->Upper = p->DirX < 0;
		if (ReUse && NoInc)
		{
			p->Pos = 0;
			if (!p->SwapXY && p->DstBPP==16)
				I3S(MOV,R0,NONE,R0,LSL,p->Upper?16:-16);
			if (!p->OnlyDiff)
				Inc_RGB_UV_Pixel(p, IncY, IncUV);
		}
		else
		{
			p->Pos = -1;
			Stretch_RGB_UV_Pixel(p, Col1, IncY, IncUV);
			RegDirty = 1;
		}

		if (!p->SwapXY) 
			NoInc = !IncY && !IncUV;

		if (p->DstPalette)
			Byte();

		if (p->DstPalette || p->DstBPP==32)
		{
			if (p->DstBPP==32 && p->Pos)
				I3S(MOV,R0,NONE,R0,ROR,-p->Pos);
			if (p->SwapXY)
				I2C(STR,R0,R9,-p->DstStepX/2);
			else
				I2C(STR_POST,R0,R9,p->DstStepX/2);
		}

		p->Upper = p->DirX > 0;
		if (ReUse && NoInc)
		{
			if (!p->SwapXY && p->DstBPP==16)
				I3S(ORR,R0,R0,R0,ROR,16);
			if (!p->OnlyDiff)
				Inc_RGB_UV_Pixel(p, IncY2, IncUV2);
		}
		else
		{
			if (p->DstBPP!=16)
				p->Pos = -1;
			Stretch_RGB_UV_Pixel(p, Col2, IncY2, IncUV2);
			RegDirty = 1;
		}

		if (!p->SwapXY) 
			NoInc = !IncY2 && !IncUV2;
		else
			NoInc = !IncY && !IncUV;

		if (p->Pos) 
			I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

		if (p->DstPalette || p->DstBPP==32)
		{
			if (p->SwapXY)
			{
				if (RegDirty) { MB(); I2C(LDR,R5,SP,OFS(stack,DstPitch)); }
				if (p->DstPalette) Byte(); 
				I3(STR_POST,R0,R9,R5);
			}
			else
			{
				if (p->DstPalette) Byte(); 
				I2C(STR_POST,R0,R9,p->DstStepX/2);
			}
		}
		else
		{
			if (p->SwapXY)
			{
				if (RegDirty) { MB(); I2C(LDR,R5,SP,OFS(stack,DstPitch)); }
				I3(STR_POST,R0,R9,R5);
			}
			else
				I2C(STR_POST,R0,R9,p->DstStepX);
		}

		if (RegDirty) { MB(); I2C(LDR,R4,SP,OFS(stack,EndOfLine)); }

		RegDirty = 0;

		if (p->OnlyDiff)
		{
			MB(); I2C(LDR,R5,SP,OFS(stack,DiffMask));
			MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
			InstPost(p->Skip);
		}

		I3(CMP,NONE,R9,R4);
		if (Col == ColCount) 
		{
			//last item in block
			if (Special) 
			{
				I2C(LDR,R7,SP,OFS(stack,SaveR7));
				I2C(LDR,R8,SP,OFS(stack,SaveR8));

				I2C(ADD,R7,R7,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));
				I2C(ADD,R8,R8,(p->RScaleX * ColCount) >> (4+p->SrcUVX2));

				I2C(STR,R7,SP,OFS(stack,SaveR7));
				I2C(STR,R8,SP,OFS(stack,SaveR8));
			}
			I0P(B,NE,LoopX);
		}
		else
			I0P(B,EQ,EndOfLine);
	}

	InstPost(EndOfLine);

	I2C(LDR,R6,SP,OFS(stack,DstNext));
	I2C(LDR,R7,SP,OFS(stack,SrcPitch));
	I2C(LDR,R5,SP,OFS(stack,Pos));

	I2C(LDR,R12,SP,OFS(stack,Y));
	I2C(LDR,R10,SP,OFS(stack,U));
	I2C(LDR,R11,SP,OFS(stack,V));

	I3(ADD,R9,R9,R6);
	
	//increment row of Y,U,V according to Pos -> Pos+RowStep
	RowStep = (p->SwapXY) ? 2:1;

	//R4,R6,R8 for temporary
	IMul(R6,R5,p->RScaleY); //R6 = RScaleY*Pos
	I2C(ADD,R5,R5,RowStep);
	IMul(R8,R5,p->RScaleY); //R8 = RScaleY*(Pos+RowStep)

	// dY  = ((RScaleY * (Pos+RowStep)) >> 4) - ((RScaleY * Pos) >> 4);
	I3S(MOV,R4,NONE,R8,LSR,4);
	I3S(SUB,R4,R4,R6,LSR,4);
	I3(MUL,R4,R7,R4);

	// dUV = ((RScaleY * (Pos+RowStep)) >> (4+SrcUVY2)) - ((RScaleY * Pos) >> (4+SrcUVY2));
	I3S(MOV,R8,NONE,R8,LSR,4+p->SrcUVY2);
	I3S(SUB,R8,R8,R6,LSR,4+p->SrcUVY2);
	I3(MUL,R8,R7,R8);

	I3(ADD,R12,R12,R4);
	I3S(ADD,R10,R10,R8,ASR,p->SrcUVPitch2); 
	I3S(ADD,R11,R11,R8,ASR,p->SrcUVPitch2);

	I2C(STR,R12,SP,OFS(stack,Y));
	I2C(STR,R10,SP,OFS(stack,U));
	I2C(STR,R11,SP,OFS(stack,V));

	//prepare registers for next row
	if (p->SwapXY) I2C(LDR,R6,SP,OFS(stack,DstPitch));
	I2C(LDR,R4,SP,OFS(stack,Width));

	MB(); I2C(LDR,R0,SP,OFS(stack,EndOfRect));
	I3(CMP,NONE,R9,R0);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();
}

#endif
