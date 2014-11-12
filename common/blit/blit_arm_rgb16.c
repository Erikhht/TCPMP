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
 * $Id: blit_arm_rgb16.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

#if defined(ARM) 

// R0..R6 temporary
// R7 Pos(when Stretch)
// R8 Src2SrcLast (when Diff)
// R8 Src0 (when Stretch)
// R9 Src
// R10 EndOfLine 
// R11 Dst
// R12 DstPitch
// R14 SrcPitch 
// R14 Src1 (when Stretch)

typedef struct stack
{
	int EndOfRect; 
	int DstNext; 
	int SrcNext;
	void* Palette;

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

void RGB_4X2(blit_soft* p)
{
	if (p->RealOnlyDiff)
	{
		p->Skip = Label(0);

		I3(LDR,R4,R9,R8);
		I3(LDR_POST,R0,R9,R14);

		I3(LDR,R6,R9,R8);
		I2C(LDR_POST,R2,R9,4);

		I3(LDR,R5,R9,R8);
		I3(LDR_POSTSUB,R3,R9,R14);

		I3(CMP,NONE,R0,R4);
		C(EQ); I3(CMP,NONE,R2,R6);

		I3(LDR,R6,R9,R8);
		I2C(LDR_POST,R1,R9,4);

		C(EQ); I3(CMP,NONE,R3,R5);
		C(EQ); I3(CMP,NONE,R1,R6);

		C(EQ);
		if (p->SwapXY)
			I3S(ADD,R11,R11,R12,LSL,2+p->DstDoubleY);
		else
			I2C(ADD,R11,R11,(2*4*p->DirX)<<p->DstDoubleX);

		I0P(B,EQ,p->Skip);
	}

	if (p->DstDoubleX && p->DstDoubleY)
	{
		//todo...
	}
	else
	{
		if (p->SwapXY)
		{
			if (p->RealOnlyDiff)
				I2C(SUB,R9,R9,8);

			Half(); I3(LDR_POST,R0,R9,R14);
			Half(); I2C(LDR_POST,R1,R9,2);

			Half(); I3(LDR_POSTSUB,R3,R9,R14);
			Half(); I2C(LDR_POST,R2,R9,2);

			if (p->DirX>0)
			{
				I3S(ORR,R4,R0,R1,LSL,16);
				I3S(ORR,R5,R2,R3,LSL,16);
			}
			else
			{
				I3S(ORR,R4,R1,R0,LSL,16);
				I3S(ORR,R5,R3,R2,LSL,16);
			}

			MB(); Half(); I3(LDR_POST,R0,R9,R14);
			MB(); Half(); I2C(LDR_POST,R1,R9,2);

			MB(); Half(); I3(LDR_POSTSUB,R3,R9,R14);
			MB(); Half(); I2C(LDR_POST,R2,R9,2);

			if (p->DirX>0)
			{
				I3S(ORR,R1,R0,R1,LSL,16);
				I3S(ORR,R3,R2,R3,LSL,16);
			}
			else
			{
				I3S(ORR,R1,R1,R0,LSL,16);
				I3S(ORR,R3,R3,R2,LSL,16);
			}

			I3(STR_POST,R4,R11,R12);
			I3(STR_POST,R5,R11,R12);
			I3(STR_POST,R1,R11,R12);
			I3(STR_POST,R3,R11,R12);
		}
		else
		{
			if (!p->RealOnlyDiff)
			{
				I3(LDR_POST,R0,R9,R14);
				I2C(LDR_POST,R2,R9,4);
				I3(LDR_POSTSUB,R3,R9,R14);
				I2C(LDR_POST,R1,R9,4);
			}
			if (p->DirX<0)
			{
				I3S(MOV,R0,NONE,R0,ROR,16);
				I3S(MOV,R1,NONE,R1,ROR,16);
				I3S(MOV,R2,NONE,R2,ROR,16);
				I3S(MOV,R3,NONE,R3,ROR,16);
			}

			I3(STR_POST,R0,R11,R12);
			I2C(STR_POST,R2,R11,4*p->DirX);
			I3(STR_POSTSUB,R3,R11,R12);
			I2C(STR_POST,R1,R11,4*p->DirX);
		}
	}

	if (p->RealOnlyDiff)
		InstPost(p->Skip);
}

void RGB_Read(blit_soft* p,reg RGB,reg Src,int Inc)
{
	int Type[4];
	int IncByte[4];
	reg Tmp = R6;
	int i,j;
	bool_t First;

	if (p->Src.BitCount==p->Dst.BitCount && (p->Src.BitCount!=8 || !p->LookUp) &&
		p->Src.BitMask[0] == p->Dst.BitMask[0] &&
		p->Src.BitMask[1] == p->Dst.BitMask[1] &&
		p->Src.BitMask[2] == p->Dst.BitMask[2])
	{
		switch (p->Src.BitCount)
		{
		case 8:
			Byte(); I2C(LDR_POST,RGB,Src,Inc); 
			break;
		case 16:
			Half(); I2C(LDR_POST,RGB,Src,Inc*2); 
			break;
		case 32:
			I2C(LDR_POST,RGB,Src,Inc*4); 
			break;
		}
	}
	else
	{
		switch (p->Src.BitCount)
		{
		case 8:
			if (p->LookUp)
			{
				I1P(MOV,Tmp,p->LookUp,0);
				Byte(); I2C(LDR_POST,RGB,Src,Inc);
				switch (p->Dst.BitCount)
				{
				case 8:
					Byte(); I3(LDR,RGB,Tmp,RGB); 
					break;
				case 16:
					I3(ADD,RGB,RGB,RGB);
					Half(); I3(LDR,RGB,Tmp,RGB);
					break;
				case 32:
					I3S(LDR,RGB,Tmp,RGB,LSL,2);
					break;
				}
			}
			break;

		case 16:
			Half(); I2C(LDR_POST,RGB,Src,Inc*2);

			for (i=0;i<3;++i)
			{
				j = min(p->DstSize[i],p->SrcSize[i]);
				I2C(AND,Tmp,RGB,((1<<j)-1) << (p->SrcSize[i]+p->SrcPos[i]-j));
				I3S(ORR,RGB,RGB,Tmp,LSL,16-p->SrcSize[i]-p->SrcPos[i]+p->DstSize[i]+p->DstPos[i]);
			}

			I3S(MOV,RGB,NONE,RGB,LSR,16);
			break;

		case 32:
		case 24:

			for (j=0;j<3;++j)
				IncByte[j] = 1;
			IncByte[3] = (Inc-1) * (p->SrcBPP/8) + (p->Src.BitCount==32);

			for (j=0;j<4;++j)
			{
				Type[j] = -1;
				for (i=0;i<3;++i)
					if (p->SrcPos[i]==j*8)
						Type[j] = i;

				if (Type[j]<0)
				{
					// merge IncByte to previous
					for (i=j-1;i>=0;--i)
						if (Type[i]>=0 || IncByte[i])
						{
							IncByte[i] += IncByte[j];
							IncByte[j] = 0;
							break;
						}
				}
			}

			First = 1;
			for (j=0;j<4;++j)
			{
				i = Type[j];
				if (i>=0)
				{
					Byte(); I2C(LDR_POST,Tmp,Src,IncByte[j]);
					I2C(AND,Tmp,Tmp,((1 << p->DstSize[i])-1) << (8-p->DstSize[i]));
					if (First)
					{
						I3S(MOV,RGB,NONE,Tmp,LSR,8-p->DstPos[i]-p->DstSize[i]);
						First = 0;
					}
					else
						I3S(ORR,RGB,RGB,Tmp,LSR,8-p->DstPos[i]-p->DstSize[i]);
				}
				else if (IncByte[j]>0)
					I2C(ADD,Src,Src,IncByte[j]);
			}

			break;
		}
	}
}

void RGB_4X2S(blit_soft* p,int* Inc)
{
	if (p->SwapXY)
	{
		RGB_Read(p,R0,R8,Inc[0]);
		RGB_Read(p,R1,R14,Inc[0]);
		RGB_Read(p,R2,R8,Inc[1]);
		RGB_Read(p,R3,R14,Inc[1]);

		if (p->DirX>0)
		{
			I3S(ORR,R4,R0,R1,LSL,16);
			I3S(ORR,R5,R2,R3,LSL,16);
		}
		else
		{
			I3S(ORR,R4,R1,R0,LSL,16);
			I3S(ORR,R5,R3,R2,LSL,16);
		}

		RGB_Read(p,R0,R8,Inc[2]);
		RGB_Read(p,R1,R14,Inc[2]);
		RGB_Read(p,R2,R8,Inc[3]);
		RGB_Read(p,R3,R14,Inc[3]);

		if (p->DirX>0)
		{
			I3S(ORR,R1,R0,R1,LSL,16);
			I3S(ORR,R3,R2,R3,LSL,16);
		}
		else
		{
			I3S(ORR,R1,R1,R0,LSL,16);
			I3S(ORR,R3,R3,R2,LSL,16);
		}

		I3(STR_POST,R4,R11,R12);
		I3(STR_POST,R5,R11,R12);
		I3(STR_POST,R1,R11,R12);
		I3(STR_POST,R3,R11,R12);
	}
	else
	{
		RGB_Read(p,R0,R8,Inc[0]);
		RGB_Read(p,R1,R8,Inc[1]);
		RGB_Read(p,R2,R8,Inc[2]);
		RGB_Read(p,R3,R8,Inc[3]);

		if (p->DirX>0)
		{
			I3S(ORR,R4,R0,R1,LSL,16);
			I3S(ORR,R5,R2,R3,LSL,16);
		}
		else
		{
			I3S(ORR,R4,R1,R0,LSL,16);
			I3S(ORR,R5,R3,R2,LSL,16);
		}

		RGB_Read(p,R0,R14,Inc[0]);
		RGB_Read(p,R1,R14,Inc[1]);
		RGB_Read(p,R2,R14,Inc[2]);
		RGB_Read(p,R3,R14,Inc[3]);

		if (p->DirX>0)
		{
			I3S(ORR,R1,R0,R1,LSL,16);
			I3S(ORR,R3,R2,R3,LSL,16);
		}
		else
		{
			I3S(ORR,R1,R1,R0,LSL,16);
			I3S(ORR,R3,R3,R2,LSL,16);
		}

		I3(STR_POST,R4,R11,R12);
		I2C(STR_POST,R1,R11,4*p->DirX);
		I3(STR_POSTSUB,R3,R11,R12);
		I2C(STR_POST,R5,R11,4*p->DirX);
	}
}

void Any_RGB_RGB(blit_soft* p)
{
	dyninst* LoopX;
	dyninst* LoopY;
	dyninst* EndOfLine;
	bool_t Stretch;
	bool_t Same = 
		p->Src.BitCount==p->Dst.BitCount && 
		p->Src.BitMask[0] == p->Dst.BitMask[0] &&
		p->Src.BitMask[1] == p->Dst.BitMask[1] &&
		p->Src.BitMask[2] == p->Dst.BitMask[2];

	Stretch = p->RScaleX!=16 || p->RScaleY!=16 || !Same;

	p->RealOnlyDiff = (boolmem_t)(p->OnlyDiff && !Stretch);
	p->Caps = 0;
	p->DstAlignSize = 4;

	if (p->SrcDoubleX && !Stretch)
		p->DstAlignSize = 8;

	p->LookUp = NULL;
	if (p->Src.Palette)
		CalcPalRGBLookUp(p);

	if (p->LookUp_Data)
	{
		p->LookUp = InstCreate(p->LookUp_Data,p->LookUp_Size,NONE,NONE,NONE,0,0);
		free(p->LookUp_Data);
		p->LookUp_Data = NULL;
	}

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I2C(LDR,R11,R1,0);//Dst[0]
	I2C(LDR,R9,R2,0); //Src[0]

	if (p->DirX<0)
	{
		//adjust reversed destination for block size
		I2C(SUB,R11,R11,2);
	}

	I3(MOV,R12,NONE,R3); //DstPitch

	I2C(LDR,R2,SP,OFS(stack,Height));
	I2C(LDR,R1,SP,OFS(stack,Width));

	if (!Stretch)
	{
		//SrcNext = 2*Src->Pitch - SrcWidth << (SrcBPP2+SrcDoubleX)
		I2C(LDR,R14,SP,OFS(stack,SrcPitch));
		I3S(MOV,R3,NONE,R1,LSL,p->SrcDoubleX+p->SrcBPP2);
		I3S(RSB,R0,R3,R14,LSL,1); 
		I2C(STR,R0,SP,OFS(stack,SrcNext));
	}

	if (p->SwapXY)
	{
		//EndOfRect = Dst + Height*2*DirX
		I3S(p->DirX<0?SUB:ADD,R0,R11,R2,LSL,p->DstBPP2);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = 2*(DstBPP/8)*DirX - Width*Dst->Pitch
		I3(MUL,R3,R1,R12);
		I2C(MOV,R0,NONE,2*p->DirX*(p->DstBPP/8));
		I3(SUB,R0,R0,R3);
		I2C(STR,R0,SP,OFS(stack,DstNext));
	}
	else
	{
		I3(MUL,R0,R12,R2); //DstPitch * Height
		I3(ADD,R0,R11,R0);
		I2C(STR,R0,SP,OFS(stack,EndOfRect));

		//DstNext = 2*Dst->Pitch - DirX*Width*(DstBPP/8)
		I3(ADD,R3,R1,R1);
		I3S(p->DirX<0?ADD:RSB,R0,R3,R12,LSL,p->DstBPP2); 
		I2C(STR,R0,SP,OFS(stack,DstNext));
	}

	if (p->RealOnlyDiff)
		I2C(LDR,R8,SP,OFS(stack,Src2SrcLast));

	if (Stretch)
		I2C(MOV,R7,NONE,0); //Pos

	I2C(LDR,R0,SP,OFS(stack,Width));
	LoopY = Label(1);

	//R0 width
	if (p->SwapXY)
	{
		I3(MUL,R10,R12,R0); //R12=DstPitch
		I3(ADD,R10,R11,R10);
	}
	else
		I3S(p->DirX>0?ADD:SUB,R10,R11,R0,LSL,1);

	if (Stretch)
	{
		int Col,i,ColCount;

		I3(MOV,R8,NONE,R9); //Src0

		I2C(LDR,R6,SP,OFS(stack,SrcPitch));
		I3S(MOV,R4,NONE,R7,LSR,4);
		I2C(ADD,R0,R7,p->RScaleY);
		I3S(RSB,R4,R4,R0,LSR,4);
		I3(MUL,R4,R6,R4);
		I3(ADD,R14,R9,R4); //Src1

		EndOfLine = Label(0);
		LoopX = Label(1);

		ColCount = 16;
		for (i=1;i<4 && !(p->RScaleX & i);i<<=1)
			ColCount >>= 1;

		for (Col=0;Col<ColCount;Col+=4)
		{
			int Inc[4];
			for (i=0;i<4;++i)
				Inc[i] = (((Col+i+1) * p->RScaleX) >> 4) - (((Col+i) * p->RScaleX) >> 4);

			RGB_4X2S(p,Inc);
			
			I3(CMP,NONE,R10,R11);
			if (Col+4 >= ColCount) 
				I0P(B,NE,LoopX);
			else
				I0P(B,EQ,EndOfLine);
		}

		InstPost(EndOfLine);

		I2C(LDR,R6,SP,OFS(stack,SrcPitch));
		I3S(MOV,R4,NONE,R7,LSR,4);
		I2C(ADD,R7,R7,2*p->RScaleY);
		I3S(RSB,R4,R4,R7,LSR,4);
		I3(MUL,R4,R6,R4);
		I3(ADD,R9,R9,R4); //add 2 lines
	}
	else
	{
		LoopX = Label(1);
		RGB_4X2(p);
		I3(CMP,NONE,R10,R11);
		I0P(B,NE,LoopX);
		I2C(LDR,R0,SP,OFS(stack,SrcNext));
		I3(ADD,R9,R9,R0);
	}

	I2C(LDR,R1,SP,OFS(stack,DstNext));
	I2C(LDR,R2,SP,OFS(stack,EndOfRect));
	I2C(LDR,R0,SP,OFS(stack,Width)); // needed for next EndOfLine
	I3(ADD,R11,R11,R1);
	I3(CMP,NONE,R2,R11);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();

	if (p->LookUp)
	{
		Align(16);
		InstPost(p->LookUp);
	}
}

#endif
