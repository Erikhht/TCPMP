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
 * $Id: blit_arm_yuv.c 375 2005-12-02 09:34:27Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

// DstAlignSize 8
// DstAlignPos  8
// SrcAlignPos  8

#if defined(ARM) 

// R0..R4 temporary
// R5 DiffMask (when Diff)
// R6 Src2SrcLast (when Diff)
// R5 7F7F7F7F (when !Diff and HalfY and !HalfX)
// R7 Src
// R8 MaskCarry (when Brightness != 0)
// R9 Add32		(when Brightness != 0)
// R10 EndOfLine (Src)
// R11 Dst
// R12 DstPitch
// R14 SrcPitch

typedef struct stack
{
	int Dst[3];
	int Src[3];
	int EndOfRect;
	int DstNext[2]; 
	int SrcNext[2];
	int SaveR0;

	int StackFrame[STACKFRAME];

	//void* this   R0
	//char* Dst    R1
	//char* Src    R2
	//int DstPitch R3 can be signed
	int SrcPitch;
	int Width; 
	int Height;
	int Src2SrcLast;
} stack;

static void NOINLINE SetDither(blit_soft* p,int Pos,int Part,int Plane)
{
	if (Plane==0 && (p->FX.Flags & BLITFX_DITHER) && p->SrcYUV)
	{
		// dither matrix:
		// 0 2
		// 3 1

		int Ofs;
		if (Pos & 8)
			Ofs = Part?2:0;
		else
			Ofs = Part?1:3;

		if (Ofs != p->LookUpOfs)
		{
			I2C(ADD,R8,R8,Ofs-p->LookUpOfs);
			p->LookUpOfs = Ofs;
		}
	}
}

static NOINLINE int YUV_4(blit_soft* p, int Reg,int Pos,int Plane)
{
	reg RA = (reg)Reg;
	reg RB = R4;
	reg RC = R0;
	reg RD = R9;

	if (p->SwapXY)
	{
		if (Reg != R0)
		{
			if (!p->SrcYUV || (Plane!=0 && (p->HalfX || p->HalfY)))
				I2C(LDR,R0,SP,OFS(stack,SaveR0));
			else
				I2(LDR,R0,R7);
		}

		if (p->LookUp)
		{
			I2C(AND,RB,R2,0xFF << Pos);
			I2C(AND,RC,R0,0xFF << Pos);
			SetDither(p,Pos,1,Plane);
			Byte(); I3S(LDR,RB,R8,RB,LSR,Pos);
			I2C(AND,RD,R1,0xFF << Pos);
			Byte(); I3S(LDR,RC,R8,RC,LSR,Pos);
			I3S(MOV,RB,NONE,RB,LSR,-(p->DirX<0?8:16));
			I3S(ORR,RB,RB,RC,LSR,-(p->DirX<0?24:0));
			I2C(AND,RC,R3,0xFF << Pos);
			SetDither(p,Pos,0,Plane);
			Byte(); I3S(LDR,RD,R8,RD,LSR,Pos);
			Byte(); I3S(LDR,RC,R8,RC,LSR,Pos);
			I3S(ORR,RB,RB,RD,LSR,-(p->DirX<0?16:8));
			I3S(ORR,RB,RB,RC,LSR,-(p->DirX<0?0:24));
		}
		else
		{
			I2C(AND,RB,R2,0xFF << Pos);
			I3S(MOV,RB,NONE,RB,LSR,Pos-(p->DirX<0?8:16));
			I2C(AND,RC,R0,0xFF << Pos);
			I3S(ORR,RB,RB,RC,LSR,Pos-(p->DirX<0?24:0));
			I2C(AND,RC,R1,0xFF << Pos);
			I3S(ORR,RB,RB,RC,LSR,Pos-(p->DirX<0?16:8));
			I2C(AND,RC,R3,0xFF << Pos);
			I3S(ORR,RB,RB,RC,LSR,Pos-(p->DirX<0?0:24));
		}
		RA=RB;
	}
	else
	if (p->DirX<0)
	{
		if (p->LookUp)
		{
			I2C(AND,RB,RA,0xFF00);
			SetDither(p,Pos,1,Plane);
			Byte(); I3S(LDR,RD,R8,RA,LSR,24);
			Byte(); I3S(LDR,RB,R8,RB,LSR,8);
			I3S(MOV,RA,NONE,RA,ROR,24);
			I3S(ORR,RD,RD,RB,LSL,16);
			SetDither(p,Pos,0,Plane);
			Byte(); I3S(LDR,RB,R8,RA,LSR,24);
			I2C(AND,RA,RA,0xFF00);
			Byte(); I3S(LDR,RA,R8,RA,LSR,8);
			I3S(ORR,RD,RD,RB,LSL,8);
			I3S(ORR,RD,RD,RA,LSL,24);
			RA = RD;
		}
		else
		if (RA == RC)
		{
			// only RA and RB
			I3S(MOV,RB,NONE,RA,LSR,24);
			I3S(ORR,RB,RB,RA,LSL,24);
			I3S(MOV,RA,NONE,RA,ROR,16);
			I3S(MOV,RB,NONE,RB,ROR,16);
			I3S(ORR,RB,RB,RA,LSR,24);
			I3S(ORR,RB,RB,RA,LSL,24);
			I3S(MOV,RA,NONE,RB,ROR,16);
		}
		else
		{
			I2C(AND,RC,RA,0xFF00);
			I3S(MOV,RB,NONE,RA,LSL,24);
			I3S(ORR,RB,RB,RC,LSL,8);
			I2C(AND,RC,RA,0xFF0000);
			I3S(ORR,RB,RB,RA,LSR,24);
			I3S(ORR,RA,RB,RC,LSR,8);
		}
	}
	else
	{
		if (p->LookUp)
		{
			I2C(AND,RB,RA,0xFF00);
			SetDither(p,Pos,1,Plane);
			Byte(); I3S(LDR,RD,R8,RA,LSR,24);
			Byte(); I3S(LDR,RB,R8,RB,LSR,8);
			I3S(MOV,RA,NONE,RA,ROR,24);
			I3S(ORR,RB,RB,RD,LSL,16);
			SetDither(p,Pos,0,Plane);
			Byte(); I3S(LDR,RD,R8,RA,LSR,24);
			I2C(AND,RA,RA,0xFF00);
			Byte(); I3S(LDR,RA,R8,RA,LSR,8);
			I3S(ORR,RB,RB,RD,LSL,8);
			I3S(ORR,RA,RA,RB,LSL,8);
		}
		else
		{
			RB = R0;
			RC = R4;
		}
	}

	if (Plane==0 && p->FX.Brightness && !p->LookUp && p->SrcYUV)
	{
		if (p->FX.Brightness < 0)
		{
			I3(MVN,RB,NONE,RA);
			RA=RB;
		}
		I3(ADD,RC,RA,R9); //add32
		I3(BIC,RB,RA,RC);
		I3(AND,RB,RB,R8); //maskcarry
		I3S(MOV,RB,NONE,RB,LSR,7);
		I3S(SUB,RC,RC,RB,LSL,8);
		I3S(RSB,RB,RB,RB,LSL,8);
		I3(ORR,RB,RB,RC);
		if (p->FX.Brightness < 0)
			I3(MVN,RB,NONE,RB);
		RA=RB;
	}

	return RA;
}

static NOINLINE void YUV_4X4(blit_soft* p, int Plane)
{
	int y;
	int UV = Plane != 0;
	int Rows = 2;
	int HalfX = UV ? p->HalfX:0;
	int HalfY = UV ? p->HalfY:0;
	int SrcUVX = UV?p->SrcUVX2:0;
	int SrcUVPitch = UV?p->SrcUVPitch2:0;
	int DstUVPitch = UV?p->DstUVPitch2:0;
	if (UV) Rows >>= p->SrcUVY2+p->HalfY;

	I2C(LDR,R7,SP,OFS(stack,Src[Plane]));
	I2C(LDR,R11,SP,OFS(stack,Dst[Plane]));

	for (y=0;y<Rows;++y)
	{
		reg RA,RWidth=R0;
		dyninst* LoopX = Label(0);

		MB(); I2C(LDR,R0,SP,OFS(stack,Width));
		if (p->SrcBPP2>0)
		{
			IMul(R1,R0,p->SrcBPP/8);
			RWidth = R1;
		}
		I3S(ADD,R10,R7,RWidth,LSR,SrcUVX); //end of line

		//preload
		if (!p->Slices)
		{
			dyninst* PreLoad1;
			dyninst* PreLoad2;
			dyninst* PreLoad3;
			dyninst* PreLoad4;

			I2C(ADD,R0,R7,32);
			I3(CMP,NONE,R0,R10);
			I0P(B,CS,LoopX);

			PreLoad1 = Label(1);
			Byte(); I2C(LDR,R2,R0,-32);
			I2C(ADD,R0,R0,64);
			I3(CMP,NONE,R0,R10);
			Byte(); I2C(LDR,R3,R0,-64);
			I0P(B,CC,PreLoad1);

			I3S(ADD,R7,R7,R14,ASR,SrcUVPitch-1);
			I3S(ADD,R10,R10,R14,ASR,SrcUVPitch-1);
			I2C(ADD,R0,R7,32);

			PreLoad2 = Label(1);
			Byte(); I2C(LDR,R2,R0,-32);
			I2C(ADD,R0,R0,64);
			I3(CMP,NONE,R0,R10);
			Byte(); I2C(LDR,R3,R0,-64);
			I0P(B,CC,PreLoad2);

			I3S(ADD,R7,R7,R14,ASR,SrcUVPitch);
			I3S(ADD,R10,R10,R14,ASR,SrcUVPitch);
			I2C(ADD,R0,R7,32);

			PreLoad3 = Label(1);
			Byte(); I2C(LDR,R2,R0,-32);
			I2C(ADD,R0,R0,64);
			I3(CMP,NONE,R0,R10);
			Byte(); I2C(LDR,R3,R0,-64);
			I0P(B,CC,PreLoad3);

			I3S(SUB,R7,R7,R14,ASR,SrcUVPitch-1);
			I3S(SUB,R10,R10,R14,ASR,SrcUVPitch-1);
			I2C(ADD,R0,R7,32);

			PreLoad4 = Label(1);
			Byte(); I2C(LDR,R2,R0,-32);
			I2C(ADD,R0,R0,64);
			I3(CMP,NONE,R0,R10);
			Byte(); I2C(LDR,R3,R0,-64);
			I0P(B,CC,PreLoad4);

			I3S(SUB,R7,R7,R14,ASR,SrcUVPitch);
			I3S(SUB,R10,R10,R14,ASR,SrcUVPitch);
		}
		else
		if (p->ARM5)
		{
			//preload next
			I3S(ADD,R0,R7,R14,ASR,SrcUVPitch-2);
			I3S(ADD,R1,R0,R14,ASR,SrcUVPitch-1);
			I2(PLD,NONE,R0);
			I2(PLD,NONE,R1);
			I3S(PLD,NONE,R0,R14,ASR,SrcUVPitch);
			I3S(PLD,NONE,R1,R14,ASR,SrcUVPitch);
		}

		InstPost(LoopX);

		if (p->RealOnlyDiff)
		{
			p->Skip = Label(0);

			I3(LDR,R4,R7,R6);
			I3S(LDR_POST,R0,R7,R14,ASR,SrcUVPitch-1);
			I3(LDR,R1,R7,R6);
			I3S(LDR_POST,R2,R7,R14,ASR,SrcUVPitch);
			I3(EOR,R4,R4,R0);
			S(); I3(BIC,R4,R4,R5);
			I3(LDR,R0,R7,R6);
			I3S(LDR_POSTSUB,R3,R7,R14,ASR,SrcUVPitch-1);
			I3(EOR,R1,R1,R2);
			S(); C(EQ); I3(BIC,R1,R1,R5);
			I3(LDR,R4,R7,R6);
			I3S(LDR_POSTSUB,R1,R7,R14,ASR,SrcUVPitch);
			I3(EOR,R0,R0,R3);
			S(); C(EQ); I3(BIC,R0,R0,R5);
			I2(LDR,R0,R7);
			I3(EOR,R4,R4,R1);
			S(); C(EQ); I3(BIC,R4,R4,R5);
			I0P(B,EQ,p->Skip);
		}
		else
		if (!p->SrcYUV && !p->LookUp)
		{
			int x,y;

			// r0,r2,r3,r1 (tmp r4,r5,r6)
			for (y=0;y<4;++y)
			{
				reg Out;
				switch (y)
				{
				case 1: Out = R1; break;
				case 2: Out = R2; break;
				case 3: Out = R3; break;
				default:Out = R0; break;
				}
				for (x=0;x<4;++x)
				{
					reg Val = (reg)(x?R4:Out);
					int ofs = ((p->SrcBPP/8) << HalfX)*x;
					switch (p->SrcBPP)
					{
					// 8bit handled by lookup (with normal YUV blitting)
					case 16:
						Half(); I2C(LDR,R4,R7,ofs);
						I2C(MOV,R5,NONE,((1 << p->SrcSize[1])-1) << (8-p->SrcSize[1]));
						I2C(MOV,R6,NONE,((1 << p->SrcSize[2])-1) << (8-p->SrcSize[2]));
						I3S(AND,R5,R5,R4,LSR,p->SrcPos[1]+p->SrcSize[1]-8);
						I3S(AND,R6,R6,R4,LSR,p->SrcPos[2]+p->SrcSize[2]-8);
						I2C(AND,R4,R4,p->Src.BitMask[0]);
						I3S(MOV,R4,NONE,R4,LSR,p->SrcPos[0]+p->SrcSize[0]-8);
						break;
					case 24:
					case 32:
						Byte(); I2C(LDR,R6,R7,ofs+(p->SrcPos[2]>>3));
						Byte(); I2C(LDR,R4,R7,ofs+(p->SrcPos[0]>>3));
						Byte(); I2C(LDR,R5,R7,ofs+(p->SrcPos[1]>>3));
						break;
					}

					// R4,R5,R6 r,g,b

					// Y = (2105*R + 4128*G + 802*B)/0x2000 + 16;
					// U = (-1212*R - 2384*G + 3596*B)/0x2000 + 128;
					// V = (3596*R - 3015*G - 582*B)/0x2000 + 128;

					switch (Plane)
					{
					case 0:
						// Y = (8*R + 16*G + 3*B)/32 + 16;
						I3S(ADD,R6,R6,R6,LSL,1);
						I3S(ADD,R4,R6,R4,LSL,3);
						I3S(ADD,R4,R4,R5,LSL,4);
						I3S(MOV,R4,NONE,R4,ASR,5);
						S(); I2C(ADD,Val,R4,16+p->FX.Brightness);
						if (p->FX.Brightness+16<0)
						{ 
							C(MI);I2C(MOV,Val,NONE,0x00); 
						}
						if (p->FX.Brightness+16+215>=255)
						{
							I2C(CMP,NONE,Val,0xFF);
							C(GT);I2C(MOV,Val,NONE,0xFF);
						}
						break;
					case 1:
						// U = (-5*R - 9*G + 14*B)/32 + 128;
						I3S(RSB,R6,R6,R6,LSL,3);
						I3(ADD,R6,R6,R6);
						I3S(ADD,R4,R4,R4,LSL,2);
						I3S(ADD,R5,R5,R5,LSL,3);
						I3(SUB,R4,R6,R4);
						I3(SUB,R4,R4,R5);
						I3S(MOV,R4,NONE,R4,ASR,5);
						I2C(ADD,Val,R4,128);
						break;
					case 2:
						// V = (14*R - 12*G - 2*B)/32 + 128;
						I3(ADD,R6,R6,R6);
						I3S(RSB,R4,R4,R4,LSL,3);
						I3S(ADD,R5,R5,R5,LSL,1);
						I3S(RSB,R4,R6,R4,LSL,1);
						I3S(SUB,R4,R4,R5,LSL,2);
						I3S(MOV,R4,NONE,R4,ASR,5);
						I2C(ADD,Val,R4,128);
						break;
					}

					if (x>0)
						I3S(ORR,Out,Out,Val,LSL,x*8);
				}
				I3S(ADD,R7,R7,R14,ASR,SrcUVPitch-HalfY);
			}
			I3S(SUB,R7,R7,R14,ASR,SrcUVPitch-2-HalfY);
		}
		else
		{
			int mode = HalfX + 2*HalfY;
			int x = 2;

			if (!p->SrcYUV && mode==0)
			{
				//possible alignment problems...
				mode = 1;
				x = 1;
			}

			switch (mode)
			{
			case 0:
				I3S(LDR_POST,R0,R7,R14,ASR,SrcUVPitch-1);
				I3S(LDR_POST,R2,R7,R14,ASR,SrcUVPitch);
				I3S(LDR_POSTSUB,R3,R7,R14,ASR,SrcUVPitch-1);
				I3S(LDR_POSTSUB,R1,R7,R14,ASR,SrcUVPitch);
				break;
			case 1: // halfx (not average...)
				Byte(); I2C(LDR,R0,R7,3*x);
				Byte(); I2C(LDR,R4,R7,2*x);
				Byte(); I2C(LDR,R5,R7,1*x);
				Byte(); I3S(LDR_POST,R6,R7,R14,ASR,SrcUVPitch-1);
				I3S(ORR,R0,R4,R0,LSL,8);
				I3S(ORR,R0,R5,R0,LSL,8);
				I3S(ORR,R0,R6,R0,LSL,8);

				Byte(); I2C(LDR,R2,R7,3*x);
				Byte(); I2C(LDR,R4,R7,2*x);
				Byte(); I2C(LDR,R5,R7,1*x);
				Byte(); I3S(LDR_POST,R6,R7,R14,ASR,SrcUVPitch);
				I3S(ORR,R2,R4,R2,LSL,8);
				I3S(ORR,R2,R5,R2,LSL,8);
				I3S(ORR,R2,R6,R2,LSL,8);

				Byte(); I2C(LDR,R3,R7,3*x);
				Byte(); I2C(LDR,R4,R7,2*x);
				Byte(); I2C(LDR,R5,R7,1*x);
				Byte(); I3S(LDR_POSTSUB,R6,R7,R14,ASR,SrcUVPitch-1);
				I3S(ORR,R3,R4,R3,LSL,8);
				I3S(ORR,R3,R5,R3,LSL,8);
				I3S(ORR,R3,R6,R3,LSL,8);

				Byte(); I2C(LDR,R1,R7,3*x);
				Byte(); I2C(LDR,R4,R7,2*x);
				Byte(); I2C(LDR,R5,R7,1*x);
				Byte(); I3S(LDR_POSTSUB,R6,R7,R14,ASR,SrcUVPitch);
				I3S(ORR,R1,R4,R1,LSL,8);
				I3S(ORR,R1,R5,R1,LSL,8);
				I3S(ORR,R1,R6,R1,LSL,8);
				break;
			case 2: // halfy
				I3S(LDR,R4,R7,R14,ASR,SrcUVPitch);
				I3S(LDR_POST,R0,R7,R14,ASR,SrcUVPitch-1-1);

				I3S(LDR,R6,R7,R14,ASR,SrcUVPitch);
				I3S(LDR_POST,R2,R7,R14,ASR,SrcUVPitch-1);

				I3S(AND,R4,R5,R4,LSR,1);
				I3S(AND,R0,R5,R0,LSR,1);
				I3(ADD,R0,R0,R4);

				I3S(AND,R6,R5,R6,LSR,1);
				I3S(AND,R2,R5,R2,LSR,1);
				I3(ADD,R2,R2,R6);

				I3S(LDR,R4,R7,R14,ASR,SrcUVPitch);
				I3S(LDR_POSTSUB,R3,R7,R14,ASR,SrcUVPitch-1-1);

				I3S(LDR,R6,R7,R14,ASR,SrcUVPitch);
				I3S(LDR_POSTSUB,R1,R7,R14,ASR,SrcUVPitch-1);

				I3S(AND,R4,R5,R4,LSR,1);
				I3S(AND,R3,R5,R3,LSR,1);
				I3(ADD,R3,R3,R4);

				I3S(AND,R6,R5,R6,LSR,1);
				I3S(AND,R1,R5,R1,LSR,1);
				I3(ADD,R1,R1,R4);
				break;
			case 3: // halfx + halfy (no average..., used by palette mode as well)
				Byte(); I2C(LDR,R0,R7,6);
				Byte(); I2C(LDR,R4,R7,4);
				Byte(); I2C(LDR,R5,R7,2);
				Byte(); I3S(LDR_POST,R6,R7,R14,ASR,SrcUVPitch-1-1);
				I3S(ORR,R0,R4,R0,LSL,8);
				I3S(ORR,R0,R5,R0,LSL,8);
				I3S(ORR,R0,R6,R0,LSL,8);

				Byte(); I2C(LDR,R2,R7,6);
				Byte(); I2C(LDR,R4,R7,4);
				Byte(); I2C(LDR,R5,R7,2);
				Byte(); I3S(LDR_POST,R6,R7,R14,ASR,SrcUVPitch-1);
				I3S(ORR,R2,R4,R2,LSL,8);
				I3S(ORR,R2,R5,R2,LSL,8);
				I3S(ORR,R2,R6,R2,LSL,8);

				Byte(); I2C(LDR,R3,R7,6);
				Byte(); I2C(LDR,R4,R7,4);
				Byte(); I2C(LDR,R5,R7,2);
				Byte(); I3S(LDR_POSTSUB,R6,R7,R14,ASR,SrcUVPitch-1-1);
				I3S(ORR,R3,R4,R3,LSL,8);
				I3S(ORR,R3,R5,R3,LSL,8);
				I3S(ORR,R3,R6,R3,LSL,8);

				Byte(); I2C(LDR,R1,R7,6);
				Byte(); I2C(LDR,R4,R7,4);
				Byte(); I2C(LDR,R5,R7,2);
				Byte(); I3S(LDR_POSTSUB,R6,R7,R14,ASR,SrcUVPitch-1);
				I3S(ORR,R1,R4,R1,LSL,8);
				I3S(ORR,R1,R5,R1,LSL,8);
				I3S(ORR,R1,R6,R1,LSL,8);
				break;
			}
		}

		if (p->SwapXY && (HalfX || HalfY || !p->SrcYUV))
			I2C(STR,R0,SP,OFS(stack,SaveR0));

		RA = (reg)YUV_4(p,R0,0,Plane);
		I3S(STR_POST,RA,R11,R12,ASR,DstUVPitch-1);
		RA = (reg)YUV_4(p,R2,16,Plane);
		I3S(STR_POST,RA,R11,R12,ASR,DstUVPitch);
		RA = (reg)YUV_4(p,R3,24,Plane);
		I3S(STR_POSTSUB,RA,R11,R12,ASR,DstUVPitch-1);
		RA = (reg)YUV_4(p,R1,8,Plane);
		I3S(STR_POSTSUB,RA,R11,R12,ASR,DstUVPitch);

		if (p->LookUp && p->LookUpOfs)
		{
			I2C(SUB,R8,R8,p->LookUpOfs);
			p->LookUpOfs = 0;
		}

		if (p->RealOnlyDiff)
			InstPost(p->Skip);

		I2C(ADD,R7,R7,(4 << HalfX)*(p->SrcBPP/8));
		if (p->SwapXY)
			I3S(ADD,R11,R11,R12,ASR,DstUVPitch-2);
		else
			I2C(ADD,R11,R11,4*p->DirX);
		I3(CMP,NONE,R7,R10);
		I0P(B,NE,LoopX);

		I2C(LDR,R0,SP,OFS(stack,SrcNext[UV]));
		I2C(LDR,R1,SP,OFS(stack,DstNext[UV]));
		I3(ADD,R7,R7,R0);
		I3(ADD,R11,R11,R1);
	}

	I2C(STR,R7,SP,OFS(stack,Src[Plane]));
	I2C(STR,R11,SP,OFS(stack,Dst[Plane]));
}

void Fix_Any_YUV(blit_soft* p)
{
	dyninst* Add32 = NULL;
	dyninst* MaskCarry = NULL;
	dyninst* LoopY;
	reg RSrcWidth;

	p->SrcAlignPos = p->DstAlignPos = p->DstAlignSize = 8;

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	p->DiffMask = NULL;

	I2C(LDR,R6,R1,0);//Dst[0] U
	I2C(LDR,R7,R1,4);//Dst[1] V
	I2C(LDR,R8,R1,8);//Dst[2] Y

	I2C(LDR,R10,R2,0); //Src[0] U
	if (!p->SrcYUV)
	{
		I3(MOV,R11,NONE,R10);
		I3(MOV,R12,NONE,R10);
		p->SrcUVX2 = 0;
		p->SrcUVY2 = 0;
		p->SrcUVPitch2 = 0;
		p->Caps = VC_BRIGHTNESS;
	}	
	else
	{
		I2C(LDR,R11,R2,4); //Src[1] V
		I2C(LDR,R12,R2,8); //Src[2] Y
	}

	if (p->DirX<0)
	{
		//adjust reversed destination for block size
		I2C(SUB,R6,R6,3);
		I2C(SUB,R7,R7,3);
		I2C(SUB,R8,R8,3);
	}

	I2C(STR,R6,SP,OFS(stack,Dst[0]));
	I2C(STR,R7,SP,OFS(stack,Dst[1]));
	I2C(STR,R8,SP,OFS(stack,Dst[2]));

	I2C(STR,R10,SP,OFS(stack,Src[0]));
	I2C(STR,R11,SP,OFS(stack,Src[1]));
	I2C(STR,R12,SP,OFS(stack,Src[2]));

	I3(MOV,R12,NONE,R3);	 //DstPitch
	I2C(LDR,R14,SP,OFS(stack,SrcPitch));

	I2C(LDR,R2,SP,OFS(stack,Height));
	I2C(LDR,R1,SP,OFS(stack,Width));

	p->RealOnlyDiff = p->OnlyDiff;
	p->HalfX = (boolmem_t)(p->SrcUVX2+1 == p->DstUVX2);
	p->HalfY = (boolmem_t)(p->SrcUVY2+1 == p->DstUVY2);

	if (p->HalfX || p->HalfY)
		p->RealOnlyDiff = 0;

	if (p->RealOnlyDiff)
	{
		int Mask = 0x03030303;
		p->DiffMask = InstCreate32(Mask,NONE,NONE,NONE,0,0);
		I1P(LDR,R5,p->DiffMask,0);
		I2C(LDR,R6,SP,OFS(stack,Src2SrcLast));
	}
	else
	if ((p->HalfX + 2*p->HalfY)==2)
	{
		int Mask = 0x7F7F7F7F;
		p->DiffMask = InstCreate32(Mask,NONE,NONE,NONE,0,0);
		I1P(LDR,R5,p->DiffMask,0);
	}

	p->LookUp = NULL;
	if (p->SrcYUV)
		CalcYUVLookUp(p);
	else if (p->Src.Palette)
		CalcPalYUVLookUp(p);

	if (p->LookUp_Data)
	{
		p->LookUp = InstCreate(p->LookUp_Data,p->LookUp_Size,NONE,NONE,NONE,0,0);
		free(p->LookUp_Data);
		p->LookUp_Data = NULL;
		I1P(MOV,R8,p->LookUp,0);
		p->LookUpOfs = 0;
	}
	else
	if (p->FX.Brightness)
	{
		int i = p->FX.Brightness;
		if (i<0) i=-i;
		if (i>127) i=127;
		MaskCarry = InstCreate32(0x80808080U,NONE,NONE,NONE,0,0);
		Add32 = InstCreate32(0x01010101U * (uint8_t)i,NONE,NONE,NONE,0,0);
		I1P(LDR,R8,MaskCarry,0);
		I1P(LDR,R9,Add32,0);
	}

	//EndOfRect
	//DstNext[2] 
	//SrcNext[2]

	RSrcWidth = R1;
	if (p->SrcBPP2>0)
	{
		IMul(R3,R1,p->SrcBPP/8);
		RSrcWidth = R3;
	}

	//SrcYNext = 4*Src->Pitch - Width
	I3S(RSB,R0,RSrcWidth,R14,LSL,2); 
	I2C(STR,R0,SP,OFS(stack,SrcNext[0]));

	//SrcUVNext = (4 << HalfY)*(Src->Pitch >> SrcUVPitch2) - (Width >> SrcUVX)
	I3S(MOV,R0,NONE,R14,LSL,2+p->HalfY-p->SrcUVPitch2); 
	I3S(SUB,R0,R0,RSrcWidth,LSR,p->SrcUVX2); 
	I2C(STR,R0,SP,OFS(stack,SrcNext[1]));

	//EndOfRect = Src + Src->Pitch * Height
	I3(MUL,R0,R14,R2);
	I3(ADD,R0,R0,R10);
	I2C(STR,R0,SP,OFS(stack,EndOfRect));

	if (p->SwapXY)
	{
		//DstYNext = 4*DirX - Width*Dst->Pitch
		I3(MUL,R3,R1,R12);
		I2C(MOV,R0,NONE,4*p->DirX);
		I3(SUB,R0,R0,R3);
		I2C(STR,R0,SP,OFS(stack,DstNext[0]));

		//DstUVNext = 4*DirX - (Width >> DstUVY2)*(Dst->Pitch >> DstUVPitch2)
		if (p->DstUVY2+p->DstUVPitch2) I3S(MOV,R3,NONE,R3,ASR,p->DstUVY2+p->DstUVPitch2);
		I2C(MOV,R0,NONE,4*p->DirX);
		I3(SUB,R0,R0,R3);
		I2C(STR,R0,SP,OFS(stack,DstNext[1]));
	}
	else
	{
		//DstYNext = 4*Dst->Pitch - DirX * Width
		I3S(p->DirX<0?ADD:RSB,R0,R1,R12,LSL,2); 
		I2C(STR,R0,SP,OFS(stack,DstNext[0]));

		//DstUVNext = 4*Dst->Pitch >> DstUVPitch2 - DirX * (Width >> DstUVX2)
		I3S(MOV,R0,NONE,R12,LSL,2-p->DstUVPitch2);
		I3S(p->DirX<0?ADD:SUB,R0,R0,R1,LSR,p->DstUVX2); 
		I2C(STR,R0,SP,OFS(stack,DstNext[1]));
	}

	LoopY = Label(0);
	I0P(B,AL,LoopY);

	if (p->DiffMask) InstPost(p->DiffMask);
	if (Add32) InstPost(Add32);
	if (MaskCarry) InstPost(MaskCarry);
	if (p->LookUp)
	{
		Align(16);
		InstPost(p->LookUp);
	}

	InstPost(LoopY);
	{
		if (p->LookUp)
			I2C(ADD,R8,R8,256+4);
		YUV_4X4(p,1);
		if (p->LookUp)
			I2C(ADD,R8,R8,256);
		YUV_4X4(p,2);
		if (p->LookUp)
			I2C(SUB,R8,R8,256+256+4);
		YUV_4X4(p,0);

		MB(); I2C(LDR,R2,SP,OFS(stack,EndOfRect));
		I3(CMP,NONE,R2,R7);
		I0P(B,NE,LoopY);
	}

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();
}

#endif
