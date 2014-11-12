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
 * $Id: blit_arm_half.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

// DstAlignSize 4
// DstAlignPos  4
// SrcAlignPos  2

#if defined(ARM) 

typedef struct stack
{
	int EndOfLine;
	int EndOfRect;
	int DstPitch; 
	int DstNext;
	int SrcNext;
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

// R0 result RGB
// R1..R3 R,G,B accumulator
// R4..R6 temporary
// R7 EndOfLine
// R8 DstPitch
// R9 Dst
// R10 U
// R11 V
// R12 Y
// R14 UVPitch (same as YPitch/2)

static NOINLINE void Half_RGB_UV_Pixel(blit_soft* p, int No)
{
	bool_t NextSub;
	bool_t NextX;

	int SatBit = p->QAdd ? 32 : 24;
	int RPos = p->DstPos[0];
	int GPos = p->DstPos[1];
	int BPos = p->DstPos[2];
	int NextLoad;

	// 3 <- 2
	// |    ^
	// v	|
	// 0 -> 1

	switch (No)
	{
	default:
	case 0: NextSub = 0; NextX = 1; p->Upper = 0; break;
	case 1: NextSub = 1; NextX = 0; p->Upper = 1; break;
	case 2: NextSub = 1; NextX = 1; p->Upper = 1; break;
	case 3: NextSub = 0; NextX = 0; p->Upper = 0; break;
	}
	
	if (p->DirX<0)
		p->Upper = !p->Upper;
	if (p->SwapXY)
	{
		if (!NextX) NextSub = !NextSub;
		NextX = !NextX;
	}

	if (p->Upper && p->DstBPP<=16)
	{
		RPos += p->DstBPP;
		GPos += p->DstBPP;
		BPos += p->DstBPP;
	}

	if (p->FX.Flags & BLITFX_DITHER)
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

	NextLoad = NextSub ? LDR_POSTSUB : LDR_POST;
	if (p->ArithStretch)
	{
		// sum a 2x2 block of Y
		if (NextX)
		{
			MB(); Byte(); I3S(LDR_POST,R4,R12,R14,LSL,1);	  //y00
			MB(); Byte(); I2C(LDR_POST,R6,R12,1);			  //y10
			MB(); Byte(); I3S(LDR_POSTSUB,R5,R12,R14,LSL,1);  //y11

			I3(ADD,R4,R4,R6);
			Byte(); I2C(NextLoad,R6,R12,NextSub?3:1);		  //y01
			I3(ADD,R4,R4,R5);
			I3(ADD,R4,R4,R6);
		}
		else
		{
			MB(); Byte(); I2C(LDR_POST,R4,R12,1);			//y00
			MB(); Byte(); I3S(LDR_POST,R6,R12,R14,LSL,1);	//y01
			MB(); Byte(); I2C(LDR_POSTSUB,R5,R12,1);		//y11

			I3(ADD,R4,R4,R6);
			Byte(); I3S(NextLoad,R6,R12,R14,LSL,1);	    //y10
			I3(ADD,R4,R4,R5);
			if (NextSub) I3S(SUB,R12,R12,R14,LSL,2);
			I3(ADD,R4,R4,R6);
		}

		MB(); IConst(R5,p->_YMul >> 2);
	}
	else
	{
		MB(); Byte(); 
		if (NextX)
			I2C(NextLoad,R4,R12,2);		  //y
		else
			I3S(NextLoad,R4,R12,R14,LSL,2); //y
		IConst(R5,p->_YMul);
	}

	I4(MLA,R1,R5,R4,R1);
	I4(MLA,R2,R5,R4,R2);
	I4(MLA,R3,R5,R4,R3);

	if (NextX)
	{
		MB(); Byte(); I2C(NextLoad,R6,R11,1); //v
		MB(); Byte(); I2C(NextLoad,R4,R10,1); //u
	}
	else
	{
		MB(); Byte(); I3(NextLoad,R6,R11,R14); //v
		MB(); Byte(); I3(NextLoad,R4,R10,R14); //u
	}
	IConst(R5,p->_GVMul);
	I4(MLA,R2,R5,R6,R2);
	IConst(R5,p->_RVMul);
	I4(MLA,R1,R5,R6,R1);
	IConst(R5,p->_GUMul);
	I4(MLA,R2,R5,R4,R2);
	IConst(R5,p->_BUMul);
	I4(MLA,R3,R5,R4,R3);	
	
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
		MB(); I1P(LDR,R0,p->InvertMask,0);
	}
	if (p->Pos!=RPos && p->Pos>=0) I3S(MOV,R0,NONE,R0,ROR,RPos-p->Pos);
	I3S(p->Pos<0?MOV:EOR,R0,(reg)(p->Pos<0?NONE:R0),R1,LSR,SatBit-p->DstSize[0]);
	I3S(MOV,R5,NONE,R2,LSR,SatBit-p->DstSize[1]);
	I3S(MOV,R0,NONE,R0,ROR,BPos-RPos);
	I3S(EOR,R0,R0,R5,ROR,BPos-GPos);
	I3S(EOR,R0,R0,R3,LSR,SatBit-p->DstSize[2]);
	p->Pos = BPos;

	if (p->FX.Flags & BLITFX_DITHER)
	{
		MB(); I3S(MOV,R1,NONE,R1,LSL,32-SatBit+p->DstSize[0]);
		MB(); I3S(MOV,R2,NONE,R2,LSL,32-SatBit+p->DstSize[1]);
		MB(); I3S(MOV,R3,NONE,R3,LSL,32-SatBit+p->DstSize[2]);
	}
}

void Half_RGB_UV(blit_soft* p)
{
	dyninst* LoopY;
	dyninst* LoopX;
	int Invert = 0;
	int Mask = 0;

	p->DstAlignPos = p->DstAlignSize = 4;

	p->DstStepX = p->DirX * ((p->DstBPP*2) >> 3);

	p->PalPtr = NULL;
	p->DiffMask = NULL;
	p->InvertMask = NULL;

	if (p->Dst.Flags & PF_INVERTED)
		Invert = -1;

	if (p->QAdd)
	{
		int Mask2;
		int i,Shift;

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
		p->InvertMask = InstCreate32(Invert,NONE,NONE,NONE,0,0);

	if (p->OnlyDiff)
	{
		int Mask = 0x03030303;
		if (!p->ArithStretch)
			Mask &= ~0x00FF0000; //lose Y3
		p->DiffMask = InstCreate32(Mask,NONE,NONE,NONE,0,0);
	}

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2C(LDR,R9,R1,0);  //Dst[0] RGB
	I2C(LDR,R10,R2,4); //Src[1] U
	I2C(LDR,R11,R2,8); //Src[2] V
	I2C(LDR,R12,R2,0); //Src[0] Y
	I2C(STR,R3,SP,OFS(stack,DstPitch));
	I3(MOV,R8,NONE,R3); //DstPitch

	I2C(LDR,R14,SP,OFS(stack,SrcPitch));
	I2C(LDR,R0,SP,OFS(stack,Height));
	I2C(LDR,R4,SP,OFS(stack,Width));

	//SrcNext = 4*Src->Pitch - (Width << 1)
	I3S(MOV,R1,NONE,R14,LSL,2);
	I3S(SUB,R1,R1,R4,LSL,1); 
	I2C(STR,R1,SP,OFS(stack,SrcNext));

	//UVNext = (Src->Pitch) - (Width << 1 >> 1);
	I3(SUB,R2,R14,R4); 
	I2C(STR,R2,SP,OFS(stack,UVNext));

	I3S(MOV,R14,NONE,R14,ASR,1);

	if (p->DirX<0 && p->DstBPP==16) //adjust reversed destination for block size
		I2C(SUB,R9,R9,-p->DstStepX-(p->DstBPP >> 3));

	if (p->SwapXY)
	{
		//EndOfRect = Dst + (Height * DstBPP * DirX) >> 3;
		I2C(MOV,R1,NONE,p->DstBPP * p->DirX);
		I3(MUL,R0,R1,R0);
		I3S(ADD,R0,R9,R0,ASR,3);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = DstStepX - Width*DstPitch;
		MB(); I3(MUL,R2,R8,R4);
		I2C(MOV,R0,NONE,p->DstStepX);
		I3(SUB,R0,R0,R2); 
		I2C(STR,R0,SP,OFS(stack,DstNext));
	}
	else
	{
		//EndOfRect = Dst + DstPitch * Height
		I3(MUL,R0,R8,R0);
		I3(ADD,R0,R9,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = DstPitch*2 - DirX * Width << DstBPP2;
		I3S(MOV,R2,NONE,R8,LSL,1);
		I3S(p->DirX>0?SUB:ADD,R2,R2,R4,LSL,p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}

	if (p->FX.Flags & BLITFX_DITHER)
	{
		I2C(MVN,R1,NONE,0x80000000);
		I2C(MVN,R2,NONE,0x80000000);
		I2C(MVN,R3,NONE,0x80000000);
	}

	if (!p->SwapXY) // starting in second row
	{
		I3(ADD,R10,R10,R14);
		I3(ADD,R11,R11,R14);
		I3S(ADD,R12,R12,R14,LSL,2);
	}

// R8 DstPitch
// R4 Width

	LoopY = Label(1);

	if (p->SwapXY)
	{
		I3(MUL,R4,R8,R4); //R8=dstpitch
		I3(ADD,R7,R9,R4);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R7,R9,R4,LSL,p->DstBPP2);
		else
			I3S(SUB,R7,R9,R4,LSL,p->DstBPP2);
	}

    if (p->OnlyDiff)
	{
		if (p->FX.Flags & BLITFX_DITHER)
			I2C(STR,R7,SP,OFS(stack,EndOfLine)); //needed for restoring
		MB(); I1P(LDR,R5,p->DiffMask,0);
		MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
		p->Skip = Label(0);
	}

	LoopX = Label(1);
	{
		if (p->OnlyDiff)
		{
			// R0 temp
			// R1..R3 R,G,B accumulator (if dither)
			// R4 temp
			// R5 DiffMask
			// R6 Src2SrcLast
			// R7 EndOfLine
			// R8 DstPitch
			// R9 Dst
			// R10 U (second row when not SwapXY)
			// R11 V (second row when not SwapXY)
			// R12 Y (second double row when not SwapXY)
			// R14 UVPitch (same as YPitch/2)

			int LoadSub = p->SwapXY?LDR_POST:LDR_POSTSUB;
			int LoadAdd = p->SwapXY?LDR_POSTSUB:LDR_POST;
			reg RA=R0,RB=R4,RC,RD;

			if (p->FX.Flags & BLITFX_DITHER)
			{
				RC=R7; //will be restored
				RD=R8; //will be restored
			}
			else
			{
				RC=R1;
				RD=R2;
			}

			I3(LDR,RA,R12,R6);
			I3S(LoadSub,RB,R12,R14,LSL,2);
			I3(EOR,RA,RA,RB);

			if (p->ArithStretch)
			{
				MB(); I3(LDR,RC,R12,R6);	
				MB(); I3S(LDR_POST,RD,R12,R14,LSL,1);
				I3(EOR,RC,RC,RD);
				I3(ORR,RA,RA,RC);

				MB(); I3(LDR,RB,R12,R6);
				MB(); I3S(LoadAdd,RD,R12,R14,LSL,2);
				I3(EOR,RB,RB,RD);
				I3(ORR,RA,RA,RB);

				MB(); I3(LDR,RC,R12,R6);
				MB(); I3S(LDR_POSTSUB,RD,R12,R14,LSL,1);
				I3(EOR,RC,RC,RD);
				I3(ORR,RA,RA,RC);

				I3S(BIC,RA,RA,R5,LSL,1); //lose 1 bit from Y (additionaly to DiffMask)
			}
			else
			{
				MB(); I3(LDR,RC,R12,R6);
				MB(); I3S(LoadAdd,RD,R12,R14,LSL,2);
				I3(EOR,RC,RC,RD);
				I3(ORR,RA,RA,RC);

				I2C(BIC,RA,RA,0xFF00); //lose Y1 (Y3 is masked out in DiffMask already)
			}

			MB(); Half(); I3(LDR,RB,R10,R6);
			MB(); Half(); I3(LoadSub,RD,R10,R14);
			I3(EOR,RB,RB,RD);
			I3(ORR,RA,RA,RB);

			MB(); Half(); I3(LDR,RC,R10,R6);
			MB(); Half(); I3(LoadAdd,RD,R10,R14);
			I3(EOR,RC,RC,RD);
			I3(ORR,RA,RA,RC);

			MB(); Half(); I3(LDR,RB,R11,R6);
			MB(); Half(); I3(LoadSub,RD,R11,R14);
			I3(EOR,RB,RB,RD);
			I3(ORR,RA,RA,RB);

			MB(); Half(); I3(LDR,RC,R11,R6);
			MB(); Half(); I3(LoadAdd,RD,R11,R14);
			I3(EOR,RC,RC,RD);
			I3(ORR,RA,RA,RC);

			S(); I3(BIC,RA,RA,R5);

			if (p->FX.Flags & BLITFX_DITHER) // restore R7,R8
			{
				MB(); I2C(LDR,R8,SP,OFS(stack,DstPitch));
				MB(); I2C(LDR,R7,SP,OFS(stack,EndOfLine));
			}

			if (p->SwapXY)
			{
				C(EQ); I3S(ADD,R9,R9,R8,LSL,1);
			}
			else
			{
				C(EQ); I2C(ADD,R9,R9,p->DstStepX);
			}

			I0P(B,EQ,p->Skip);
		}

		p->Pos = -1;

		Half_RGB_UV_Pixel(p,0);
	
		Half_RGB_UV_Pixel(p,1);
		if (p->Pos) 
			I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

		if (p->DstBPP==8) Half();
		if (p->SwapXY)
			I3(STR_POST,R0,R9,R8);
		else
			I3(STR,R0,R9,R8);

		p->Pos = -1;
		Half_RGB_UV_Pixel(p,2);

		Half_RGB_UV_Pixel(p,3);
		if (p->Pos) 
			I3S(MOV,R0,NONE,R0,ROR,-p->Pos);

		if (p->DstBPP==8) Half();
		if (p->SwapXY)
			I3(STR_POST,R0,R9,R8);
		else
			I2C(STR_POST,R0,R9,p->DstStepX);

		if (p->OnlyDiff)
		{
			MB(); I1P(LDR,R5,p->DiffMask,0);
			MB(); I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
			InstPost(p->Skip);
		}

		I3(CMP,NONE,R9,R7);
		I2C(ADD,R12,R12,4);
		I2C(ADD,R10,R10,2);
		I2C(ADD,R11,R11,2);
		I0P(B,NE,LoopX);
	}

	I2C(LDR,R0,SP,OFS(stack,SrcNext));
	I2C(LDR,R4,SP,OFS(stack,DstNext));
	I2C(LDR,R6,SP,OFS(stack,UVNext));
	I2C(LDR,R5,SP,OFS(stack,EndOfRect));
	
	//increment pointers
	I3(ADD,R12,R12,R0);
	I3(ADD,R9,R9,R4);
	I3(ADD,R10,R10,R6);
	I3(ADD,R11,R11,R6);

	//prepare registers for next row
	I2C(LDR,R4,SP,OFS(stack,Width));

	I3(CMP,NONE,R9,R5);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();

	if (p->InvertMask) InstPost(p->InvertMask);
	if (p->DiffMask) InstPost(p->DiffMask);
	if (p->PalPtr) InstPost(p->PalPtr);
}

#endif
