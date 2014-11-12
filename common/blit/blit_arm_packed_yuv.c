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
 * $Id: blit_arm_packed_yuv.c 339 2005-11-15 11:22:45Z picard $
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

static NOINLINE void PrepairUV(blit_soft* p,bool_t Swap)
{
	I3S(ORR,R1,(reg)(Swap?R2:R1),(reg)(Swap?R1:R2),LSL,16);
}

static NOINLINE void PackYUV(blit_soft* p,reg RA,reg RB,bool_t Low)
{
	if (p->FX.Brightness < 0)
	{
		S(); I2C(SUB,RA,RA,-p->FX.Brightness);
		C(LT); I2C(MOV,RA,NONE,0);
		S(); I2C(SUB,RB,RB,-p->FX.Brightness);
		C(LT); I2C(MOV,RB,NONE,0);
	}

	if (p->FX.Brightness > 0)
	{
		I2C(ADD,RA,RA,p->FX.Brightness);
		I2C(ADD,RB,RB,p->FX.Brightness);

		I2C(CMP,NONE,RA,255);
		C(GT); I2C(MOV,RA,NONE,255);
		I2C(CMP,NONE,RB,255);
		C(GT); I2C(MOV,RB,NONE,255);

	}

	I3S(ORR,R0,(reg)(p->DirX<0?RB:RA),(reg)(p->DirX<0?RA:RB),LSL,16);
	I3S(ORR,R0,(reg)(Low?R0:R1),(reg)(Low?R1:R0),LSL,8);
}

void Fix_PackedYUV_YUV(blit_soft* p)
{
	pixel Dst;
	dyninst* LoopY;
	dyninst* LoopX;
	bool_t DoubleLine;

	p->Caps = VC_BRIGHTNESS;
	
	Dst = p->Dst;
	FillInfo(&Dst);

	p->DstStepX = p->DirX*4;
	p->DoubleX = (boolmem_t)(p->SrcUVX2==1);
	p->DoubleY = (boolmem_t)(p->SrcUVY2==1);

	DoubleLine = p->SwapXY?p->DoubleX:p->DoubleY;

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2C(LDR,R9,R1,0); //Dst[0] RGB
	I2C(LDR,R10,R2,4); //Src[1] U
	I2C(LDR,R11,R2,8); //Src[2] V
	I2C(LDR,R12,R2,0); //Src[0] Y
	I2C(STR,R3,SP,OFS(stack,DstPitch));

	I3(MOV,R8,NONE,R3); //DstPitch
	I2C(LDR,R7,SP,OFS(stack,SrcPitch));
	I2C(LDR,R0,SP,OFS(stack,Height));
	I2C(LDR,R4,SP,OFS(stack,Width));

	//YNext = (Src->Pitch << DoubleLine) - Width
	I3S(RSB,R1,R4,R7,LSL,DoubleLine); 
	I2C(STR,R1,SP,OFS(stack,YNext));

	//UVNext = ((Src->Pitch >> p->SrcUVPitch2) << (p->SwapXY && !p->DoubleY) - (Width >> SrcUVX);
	I3S(MOV,R2,NONE,R7,ASR,p->SrcUVPitch2-(p->SwapXY && !p->DoubleY));
	I3S(SUB,R2,R2,R4,LSR,p->SrcUVX2); 
	I2C(STR,R2,SP,OFS(stack,UVNext));

	if (p->DirX<0) //adjust reversed destination for block size
		I2C(SUB,R9,R9,2);
	if ((Dst.BitMask[0] & 0xFF)==0) //adjust Y offset
		I2C(SUB,R9,R9,1);

	if (p->SwapXY)
	{
		// EndOfRect = Dst + ((Height * DstBPP * DirX) >> 3) - DstPitch
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
		// EndOfRect = Dst + DstPitch * Height
		I3(MUL,R0,R8,R0);
		I3(ADD,R0,R9,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = (DstPitch << DoubleLine) - DirX * Width << DstBPP2;
		I3S(MOV,R2,NONE,R8,LSL,DoubleLine);
		I3S(p->DirX>0?SUB:ADD,R2,R2,R4,LSL,p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}

	if (DoubleLine)
		I3(ADD,R14,R12,R7);

	LoopY = Label(1);

	if (p->SwapXY)
	{
		I3(MUL,R4,R8,R4);
		I3(ADD,R7,R9,R4);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R7,R9,R4,LSL,p->DstBPP2);
		else
			I3S(SUB,R7,R9,R4,LSL,p->DstBPP2);
	}

	LoopX = Label(1);
	{
		//R0,R1,R2,R3,R4,R5,R6 temp

		//R7 endofline
		//R8 dstpitch
		//R9 dst
		//R10 src U
		//R11 src V
		//R12 src Y
		//R14 src Y next line (if DoubleY)

		//load UV

		Byte(); I2C(LDR_POST,R1,R10,(!p->SwapXY && !p->DoubleX)?2:1); //u
		Byte(); I2C(LDR_POST,R2,R11,(!p->SwapXY && !p->DoubleX)?2:1); //v

		if (DoubleLine)
		{
			if (p->SwapXY)
			{
				Byte(); I2C(LDR_POST,R3,R12,1);
				Byte(); I2C(LDR_POST,R4,R14,1);
				Byte(); I2C(LDR_POST,R5,R12,1);
				Byte(); I2C(LDR_POST,R6,R14,1);
			}
			else
			{
				Byte(); I2C(LDR_POST,R3,R14,1);
				Byte(); I2C(LDR_POST,R4,R14,1);
				Byte(); I2C(LDR_POST,R5,R12,1);
				Byte(); I2C(LDR_POST,R6,R12,1);
			}

			PrepairUV(p,Dst.BitMask[1] > Dst.BitMask[2]);

			PackYUV(p,R3,R4,(Dst.BitMask[0] & 0xFF)!=0);
			if (p->SwapXY)
				I3(STR_POST,R0,R9,R8);
			else
				I3(STR,R0,R9,R8);
		}
		else
		{
			Byte(); I2C(LDR_POST,R5,R12,1);
			Byte(); I2C(LDR_POST,R6,(reg)(p->SwapXY?R14:R12),1);

			PrepairUV(p,Dst.BitMask[1] > Dst.BitMask[2]);
		}

		PackYUV(p,R5,R6,(Dst.BitMask[0] & 0xFF)!=0);
		if (p->SwapXY)
			I3(STR_POST,R0,R9,R8);
		else
			I2C(STR_POST,R0,R9,p->DstStepX);

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
	I2C(LDR,R4,SP,OFS(stack,Width));

	I3(CMP,NONE,R9,R5);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();
}

#endif
