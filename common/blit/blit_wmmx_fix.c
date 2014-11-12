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
 * $Id: blit_wmmx_fix.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "blit_soft.h"

// RScale==16 && !SwapXY	8x2 -> 8x2
// RScale==16 && SwapXY		2x8 -> 8x2
// RScale==8  && !SwapXY	8x2 -> 16x4
// RScale==8  && SwapXY		2x8 -> 16x4
// RScale==32 && !SwapXY	16x4 -> 8x2
// RScale==32 && SwapXY		4x16 -> 8x2

#if defined(ARM) 

typedef struct stack
{
	int EndOfRect;
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

// r0 bmask (if bpos!=0)
// r1 temp
// r2 dst1 (dst+dstpitch*DoubleY or dst+8*DoubleX*DirX)
// r3 u (-=4 if !swapxy)
// r4 v (-=4 if !swapxy)
// r5..r7 temp
// r8 endofline
// r9 dst0
// r10 dstpitch
// r11 srcpitch
// r12 y0
// r14 y1 (y0+(srchalfy?2:1)*srcpitch)

// wr0..wr2 r0,g0,b0 (from uv)
// wr3..wr5 temp1 (r,g,b)
// wr6..wr8 temp2 (r,g,b)
// wr9,wr10 mask r,g
// wr11 radd (if !dither)
// wr12 gadd (if !dither)
// wr13 badd (if !dither)
// wr11 radd,gadd,badd (if dither)
// wr12,wr13 dither
// wr14 rvmul,gumul,gvmul,bumul
// wr15 ymul

// wcgr0 abs(rpos-8)
// wcgr1 abs(gpos-8)
// wcgr2 abs(bpos-8)
// wcgr3 8 (if HalfMode)

// using: wr0,wr1,wr2,wr6,wr7
static void Fix_UVSwapXY(blit_soft* p,bool_t HalfMode,bool_t Part,bool_t Row,reg Src,reg Dst)
{
	if (p->DstHalfY && Part)
	{
		Byte(); I3S(LDR_POSTSUB,R7,Src,R11,ASR,1);	//3
		Byte(); I3S(LDR_POSTSUB,R1,Src,R11,ASR,1);	//2
		Byte(); I3S(LDR_POSTSUB,R6,Src,R11,ASR,1);	//1
		Byte(); I2C(LDR_POST,R5,Src,Row?1:-1);		//0

		I2C(TINSRB,Dst,R7,p->DirX>0?7:1);
		I2C(TINSRB,Dst,R1,p->DirX>0?5:3);
		I2C(TINSRB,Dst,R6,p->DirX>0?3:5);
		I2C(TINSRB,Dst,R5,p->DirX>0?1:7);

		if (p->SrcHalfY)
			I3S(Row?SUB:ADD,Src,Src,R11,LSL,1); // 4 rows
	}
	else
	if (p->DstHalfY && !Part)
	{
		Byte(); I3S(LDR_POST,R5,Src,R11,ASR,1);		//0
		Byte(); I3S(LDR_POST,R6,Src,R11,ASR,1);		//1
		Byte(); I3S(LDR_POST,R1,Src,R11,ASR,1);		//2
		Byte(); I2C(LDR_POST,R7,Src,1);				//3

		I2C(TINSRB,Dst,R5,p->DirX>0?1:7);
		I2C(TINSRB,Dst,R6,p->DirX>0?3:5);
		I2C(TINSRB,Dst,R1,p->DirX>0?5:3);
		I2C(TINSRB,Dst,R7,p->DirX>0?7:1);
	}
	else
	{
		Byte(); I3S(LDR_POST,R5,Src,R11,ASR,1);		//0
		Byte(); I3S(LDR_POST,R6,Src,R11,ASR,0);		//1
		Byte(); I3S(LDR_POSTSUB,R7,Src,R11,ASR,1);	//3
		Byte(); I3S(LDR_POSTSUB,R1,Src,R11,ASR,0); //2

		I2C(TINSRB,Dst,R5,p->DirX>0?1:3);
		I2C(TINSRB,Dst,R6,p->DirX>0?3:1);
		I2C(TINSRB,Dst,R7,p->DirX>0?7:5);
		I2C(TINSRB,Dst,R1,p->DirX>0?5:7);

		I2C(ADD,Src,Src,1);
	}
}

static void Fix_UV(blit_soft* p,bool_t HalfMode,bool_t Part,bool_t Row)
{
	if (p->SwapXY)
	{
		Fix_UVSwapXY(p,HalfMode,Part,Row,R3,WR6);
		Fix_UVSwapXY(p,HalfMode,Part,Row,R4,WR7);
	}
	else
	{
		if (p->DstHalfY && Part)
		{
			// one uv row below
			MB(); I3S(ADD,R5,R3,R11,ASR,1);
			MB(); I3S(ADD,R6,R4,R11,ASR,1);
			MB(); I2C(WLDRW,WR6,R5,0);
			MB(); I2C(WLDRW,WR7,R6,0);
		}
		else
		{
			int Pre = (HalfMode && !p->SrcHalfX && Row)?0:4;
			MB(); I2C(WLDRW_PRE,WR6,R3,Pre);
			MB(); I2C(WLDRW_PRE,WR7,R4,Pre);
		}

		I3(WUNPCKILB,WR6,WR6,WR6);
		I3(WUNPCKILB,WR7,WR7,WR7);

		if (p->DirX<0)
		{
			I2C(WSHUFH,WR6,WR6,p->SrcHalfY?0x1B:0xB1); //swap order (2:2)
			I2C(WSHUFH,WR7,WR7,p->SrcHalfY?0x1B:0xB1); //swap order (2:2)
		}
	}

	if (HalfMode && !p->DstHalfX)
	{
		I3(Row==(p->DirX>0)?WUNPCKIHH:WUNPCKILH,WR6,WR6,WR6);
		I3(Row==(p->DirX>0)?WUNPCKIHH:WUNPCKILH,WR7,WR7,WR7);
	}

	//WR6 U
	//WR7 V

	I2C(WSHUFH,WR1,WR14,0x55); //gumul
	I3(WMULUM,WR1,WR1,WR6);
	I2C(WSHUFH,WR2,WR14,0xFF); //bumul
	I3(WMULUM,WR2,WR2,WR6);
	I2C(WSHUFH,WR6,WR14,0xAA); //gvmul
	I3(WMULUM,WR6,WR6,WR7);
	I2C(WSHUFH,WR0,WR14,0x00); //rvmul
	I3(WMULUM,WR0,WR0,WR7);
	I3((p->_GUMul<0)^(p->_GVMul<0)?WSUBH:WADDH,WR1,WR1,WR6);

	//WR0 R
	//WR1 G
	//WR2 B

	if (p->FX.Flags & BLITFX_DITHER)
	{
		I2C(WSHUFH,WR6,WR11,0x00); //radd
		I2C(WSHUFH,WR7,WR11,0x55); //gadd
		I3(p->_RVMul<0?WSUBH:WADDH,WR0,WR6,WR0);
		I2C(WSHUFH,WR6,WR11,0xAA); //badd
		I3(p->_GUMul<0?WSUBH:WADDH,WR1,WR7,WR1);
		I3(p->_BUMul<0?WSUBH:WADDH,WR2,WR6,WR2);
	}
	else
	{
		I3(p->_RVMul<0?WSUBH:WADDH,WR0,WR11,WR0);
		I3(p->_GUMul<0?WSUBH:WADDH,WR1,WR12,WR1);
		I3(p->_BUMul<0?WSUBH:WADDH,WR2,WR13,WR2);
	}
}

static void Fix_Y(blit_soft* p,bool_t Row,int Col,bool_t HalfMode)
{
	// load to upper 8bits (doesn't matter what is in the lower 8 bits)
	// load y0 wr5
	// load y1 wr8
	reg Dither;
	int AddY;

	if (p->SwapXY)
	{
		Dither = (reg)(Col?WR13:WR12);

		if (p->ArithStretch && p->SrcHalfX && p->SrcHalfY)
		{ 
			I3(LDR_POST,R5,R12,R11);
			I3(LDR_POST,R6,R12,R11);
			I3(LDR_POST,R7,R12,R11);
			I3(LDR_POST,R1,R12,R11);
			I2C(TINSRH,WR5,R5,p->DirX>0?0:3);
			I2C(TINSRH,WR6,R6,p->DirX>0?0:3);
			I2C(TINSRH,WR5,R7,p->DirX>0?1:2);
			I2C(TINSRH,WR6,R1,p->DirX>0?1:2);
			I3S(MOV,R5,NONE,R5,LSR,16);
			I3S(MOV,R6,NONE,R6,LSR,16);
			I3S(MOV,R7,NONE,R7,LSR,16);
			I3S(MOV,R1,NONE,R1,LSR,16);
			I2C(TINSRH,WR8,R5,p->DirX>0?0:3);
			I2C(TINSRH,WR7,R6,p->DirX>0?0:3);
			I2C(TINSRH,WR8,R7,p->DirX>0?1:2);
			I2C(TINSRH,WR7,R1,p->DirX>0?1:2);
			I3(LDR_POST,R5,R12,R11);
			I3(LDR_POST,R6,R12,R11);
			I3(LDR_POST,R7,R12,R11);
			I3(LDR_POST,R1,R12,R11);
			I2C(TINSRH,WR5,R5,p->DirX>0?2:1);
			I2C(TINSRH,WR6,R6,p->DirX>0?2:1);
			I2C(TINSRH,WR5,R7,p->DirX>0?3:0);
			I2C(TINSRH,WR6,R1,p->DirX>0?3:0);
			I3S(MOV,R5,NONE,R5,LSR,16);
			I3S(MOV,R6,NONE,R6,LSR,16);
			I3S(MOV,R7,NONE,R7,LSR,16);
			I3S(MOV,R1,NONE,R1,LSR,16);
			I2C(TINSRH,WR8,R5,p->DirX>0?2:1);
			I2C(TINSRH,WR7,R6,p->DirX>0?2:1);
			I2C(TINSRH,WR8,R7,p->DirX>0?3:0);
			I2C(TINSRH,WR7,R1,p->DirX>0?3:0);

			I3(WAVG2B,WR5,WR5,WR6);
			I3(WSLLHG,WR6,WR5,WCGR3);
			I3(WAVG2B,WR8,WR8,WR7);
			I3(WSLLHG,WR7,WR8,WCGR3);
			I3(WAVG2B,WR5,WR5,WR6);
			I3(WAVG2B,WR8,WR8,WR7);
		}
		else
		if (p->SrcHalfX)
		{
			reg Add = R11;
			if (p->SrcHalfY)
			{
				I3(ADD,R1,R11,R11);
				Add = R1;
			}

			I3(LDR_POST,R5,R12,Add);
			I3(LDR_POST,R6,R12,Add);
			I3(LDR_POST,R7,R12,Add);
			I3(LDR_POST,R1,R12,Add);
			I2C(TINSRH,WR5,R5,p->DirX>0?0:3);
			I2C(TINSRH,WR5,R6,p->DirX>0?1:2);
			I2C(TINSRH,WR5,R7,p->DirX>0?2:1);
			I2C(TINSRH,WR5,R1,p->DirX>0?3:0);
			I3S(MOV,R5,NONE,R5,LSR,16);
			I3S(MOV,R6,NONE,R6,LSR,16);
			I3S(MOV,R7,NONE,R7,LSR,16);
			I3S(MOV,R1,NONE,R1,LSR,16);
			I2C(TINSRH,WR8,R5,p->DirX>0?0:3);
			I2C(TINSRH,WR8,R6,p->DirX>0?1:2);
			I2C(TINSRH,WR8,R7,p->DirX>0?2:1);
			I2C(TINSRH,WR8,R1,p->DirX>0?3:0);
		}
		else
		{
			reg Add = R11;
			if (p->SrcHalfY)
			{
				I3(ADD,R1,R11,R11);
				Add = R1;
			}

			Half(); I3(LDR_POST,R5,R12,Add);
			Half(); I3(LDR_POST,R6,R12,Add);
			Half(); I3(LDR_POST,R7,R12,Add);
			Half(); I3(LDR_POST,R1,R12,Add);
			I2C(TINSRB,WR5,R5,p->DirX>0?1:7);
			I2C(TINSRH,WR8,R5,p->DirX>0?0:3);
			I2C(TINSRB,WR5,R6,p->DirX>0?3:5);
			I2C(TINSRH,WR8,R6,p->DirX>0?1:2);
			I2C(TINSRB,WR5,R7,p->DirX>0?5:3);
			I2C(TINSRH,WR8,R7,p->DirX>0?2:1);
			I2C(TINSRB,WR5,R1,p->DirX>0?7:1);
			I2C(TINSRH,WR8,R1,p->DirX>0?3:0);
		}

		if (Row)
		{
			I3S(SUB,R12,R12,R11,LSL,3+p->SrcHalfY);
			I2C(ADD,R12,R12,2 << p->SrcHalfX);
		}
	}
	else
	{
		Dither = WR12;
		if (p->ArithStretch && p->SrcHalfX && p->SrcHalfY)
		{ 
			MB(); I3(ADD,R1,R12,R11);
			MB(); I3(ADD,R5,R14,R11);
			MB(); I2C(WLDRD_POST,WR5,R12,8);
			MB(); I2C(WLDRD,WR6,R1,0);
			MB(); I2C(WLDRD_POST,WR8,R14,8);
			MB(); I2C(WLDRD,WR7,R5,0);

			I3(WAVG2B,WR5,WR5,WR6);
			I3(WSLLHG,WR6,WR5,WCGR3);
			I3(WAVG2B,WR8,WR8,WR7);
			I3(WSLLHG,WR7,WR8,WCGR3);
			I3(WAVG2B,WR5,WR5,WR6);
			I3(WAVG2B,WR8,WR8,WR7);
		}
		else
		if (p->SrcHalfX)
		{
			MB(); I2C(WLDRD_POST,WR5,R12,8); // use only every second pixel
			MB(); I2C(WLDRD_POST,WR8,R14,8); // use only every second pixel
		}
		else
		{
			MB(); I2C(WLDRW_POST,WR5,R12,4);
			MB(); I2C(WLDRW_POST,WR8,R14,4);
			I3(WUNPCKILB,WR5,WR5,WR5);
			I3(WUNPCKILB,WR8,WR8,WR8);
		}

		if (p->DirX<0)
		{
			I2C(WSHUFH,WR8,WR8,0x1B); //swap order
			I2C(WSHUFH,WR5,WR5,0x1B); //swap order
		}
	}

	MB(); I3(WMULUM,WR5,WR5,WR15);
	MB(); I3(WMULUM,WR8,WR8,WR15);

	AddY = p->_YMul>=0?WADDH:WSUBH;

	if (HalfMode)
	{
		Fix_UV(p,HalfMode,0,Row); 

		I3(AddY,WR3,WR0,WR5);
		I3(AddY,WR4,WR1,WR5);
		I3(AddY,WR5,WR2,WR5);

		if (p->DstHalfY)
			Fix_UV(p,HalfMode,1,Row);
		
		I3(AddY,WR6,WR0,WR8);
		I3(AddY,WR7,WR1,WR8);
		I3(AddY,WR8,WR2,WR8);

		I3(WPACKHUS,WR3,WR3,WR6);
		I3(WPACKHUS,WR4,WR4,WR7);
		I3(WPACKHUS,WR5,WR5,WR8);
	}
	else
	{
		// WR0,WR1,WR2 already has UV information

		I3(Row?WUNPCKIHH:WUNPCKILH,WR6,WR0,WR0);
		I3(Row?WUNPCKIHH:WUNPCKILH,WR7,WR1,WR1);

		I3(AddY,WR3,WR6,WR5);
		I3(AddY,WR4,WR7,WR5);
		I3(AddY,WR6,WR6,WR8);
		I3(AddY,WR7,WR7,WR8);
		I3(WPACKHUS,WR3,WR3,WR6);
		I3(WPACKHUS,WR4,WR4,WR7);

		MB(); I3(Row?WUNPCKIHH:WUNPCKILH,WR6,WR2,WR2);
		I3(AddY,WR5,WR6,WR5);
		I3(AddY,WR8,WR6,WR8);
		I3(WPACKHUS,WR5,WR5,WR8);
	}

	if (p->FX.Flags & BLITFX_DITHER)
	{
		if (p->DstSize[0] > p->DitherSize || p->DstSize[1] > p->DitherSize || p->DstSize[2] > p->DitherSize)
		{
			MB(); I3(WXOR,WR6,WR6,WR6);
			I3(WAVG2B,WR6,WR6,Dither);
		}

		I3(WADDBUS,WR3,WR3,(reg)(p->DstSize[0] > p->DitherSize ? WR6:Dither));
		I3(WADDBUS,WR4,WR4,(reg)(p->DstSize[1] > p->DitherSize ? WR6:Dither));
		I3(WADDBUS,WR5,WR5,(reg)(p->DstSize[2] > p->DitherSize ? WR6:Dither));
	}

	if (p->DstPos[2]==0 && p->DstPos[0]+p->DstSize[0]==16)
	{
		// special optimized case for rgb565
		// (red and blue word mask in WR9)

		I3(WAND,WR4,WR4,WR10);
		I3(WSRLDG,WR5,WR5,WCGR2);

		I2(WUNPCKEHUB,WR7,WR4);
		I3(WUNPCKIHB,WR6,WR5,WR3);
		I3(WSLLWG,WR7,WR7,WCGR1);
		I3(WAND,WR6,WR6,WR9); 
		I3(WOR,WR6,WR6,WR7);

		I2(WUNPCKELUB,WR4,WR4);
		I3(WUNPCKILB,WR3,WR5,WR3);
		I3(WSLLWG,WR4,WR4,WCGR1);
		I3(WAND,WR3,WR3,WR9);
		I3(WOR,WR3,WR3,WR4);
	}
	else
	{
		if (p->DstPos[0]!=0) I3(WAND,WR3,WR3,WR9);
		if (p->DstPos[1]!=0) I3(WAND,WR4,WR4,WR10);
		if (p->DstPos[2]!=0) 
		{
			I2(TBCSTH,WR6,R0);
			I3(WAND,WR5,WR5,WR6);
		}

		I2(WUNPCKEHUB,WR6,WR3);
		I2(WUNPCKEHUB,WR7,WR4);
		I2(WUNPCKEHUB,WR8,WR5);

		if (p->DstPos[0]+p->DstSize[0]!=8) I3(p->DstPos[0]+p->DstSize[0]>8?WSLLWG:WSRLWG,WR6,WR6,WCGR0);
		if (p->DstPos[1]+p->DstSize[1]!=8) I3(p->DstPos[1]+p->DstSize[1]>8?WSLLWG:WSRLWG,WR7,WR7,WCGR1);
		if (p->DstPos[2]+p->DstSize[2]!=8) I3(p->DstPos[2]+p->DstSize[2]>8?WSLLWG:WSRLWG,WR8,WR8,WCGR2);

		I2(WUNPCKELUB,WR3,WR3);
		I2(WUNPCKELUB,WR4,WR4);
		I2(WUNPCKELUB,WR5,WR5);

		if (p->DstPos[0]+p->DstSize[0]!=8) I3(p->DstPos[0]+p->DstSize[0]>8?WSLLWG:WSRLWG,WR3,WR3,WCGR0);
		if (p->DstPos[1]+p->DstSize[1]!=8) I3(p->DstPos[1]+p->DstSize[1]>8?WSLLWG:WSRLWG,WR4,WR4,WCGR1);
		if (p->DstPos[2]+p->DstSize[2]!=8) I3(p->DstPos[2]+p->DstSize[2]>8?WSLLWG:WSRLWG,WR5,WR5,WCGR2);

		I3(WOR,WR6,WR6,WR7);
		I3(WOR,WR3,WR3,WR4);
		I3(WOR,WR6,WR6,WR8);
		I3(WOR,WR3,WR3,WR5);
	}

	if (p->DstDoubleX)
	{
		I2C(WSHUFH,WR7,WR6,0xFA); // 7 7 6 6
		I2C(WSHUFH,WR4,WR3,0xFA); // 3 3 2 2
		I2C(WSHUFH,WR6,WR6,0x50); // 5 5 4 4
		I2C(WSHUFH,WR3,WR3,0x50); // 1 1 0 0
	}

	if (p->SwapXY)
	{
		reg Dst = (reg)(Row?R2:R9);

		MB(); I3S(ADD,R1,Dst,R10,LSL,p->DstDoubleY);

		MB(); I2C(WSTRD,WR3,Dst,0);
		if (p->DstDoubleX)
		{
			MB(); I2C(WSTRD,WR4,Dst,8);
		}

		if (p->DstDoubleY)
		{
			MB(); I3(ADD,R5,Dst,R10);

			MB(); I2C(WSTRD,WR3,R5,0);
			if (p->DstDoubleX)
			{
				MB(); I2C(WSTRD,WR4,R5,8);
			}
		}

		MB(); I2C(WSTRD,WR6,R1,0);
		if (p->DstDoubleX)
		{
			MB(); I2C(WSTRD,WR7,R1,8);
		}

		if (p->DstDoubleY)
		{
			MB(); I3(ADD,R6,R1,R10);

			MB(); I2C(WSTRD,WR6,R6,0);
			if (p->DstDoubleX)
			{
				MB(); I2C(WSTRD,WR7,R6,8);
			}
		}

		MB(); I3S(ADD,Dst,Dst,R10,LSL,1+p->DstDoubleY);
	}
	else
	{
		if (p->DstDoubleY)
		{
			MB(); I3(ADD,R1,R2,R10);
			MB(); I3(ADD,R5,R9,R10);
		}

		if (p->DstDoubleX)
		{
			MB(); I2C(WSTRD_POST,WR6,R2,8);
			MB(); I2C(WSTRD_POST,WR3,R9,8);
		}
		else
		{
			MB(); I2C(WSTRD_POST,WR6,R2,8*p->DirX);
			MB(); I2C(WSTRD_POST,WR3,R9,8*p->DirX);
		}

		if (p->DstDoubleY)
		{
			MB(); I2C(WSTRD,WR6,R1,0);
			MB(); I2C(WSTRD,WR3,R5,0);
		}

		if (p->DstDoubleX)
		{
			MB(); I2C(WSTRD_POST,WR7,R2,p->DirX>0?8:-24);
			MB(); I2C(WSTRD_POST,WR4,R9,p->DirX>0?8:-24);

			if (p->DstDoubleY)
			{
				MB(); I2C(WSTRD,WR7,R1,8);
				MB(); I2C(WSTRD,WR4,R5,8);
			}
		}
	}
}

void WMMXFix_RGB_UV(blit_soft* p)
{
	bool_t HalfMode = p->SrcHalfX || p->SrcHalfY;
	dyninst* LoopY;
	dyninst* LoopX;
	dyninst* EndLine;
	dyninst* Dither = NULL;

	p->SrcAlignPos = p->DstAlignPos = p->DstAlignSize = 8;
	if (p->RScaleX==8) p->DstAlignSize = 16;

	p->DstStepX = p->DirX * ((p->DstBPP*8) >> 3) << p->DstDoubleX;

	p->YMul = InstCreate16(abs(p->_YMul) >> 8,NONE,NONE,NONE,0,0);
	p->RVMul = InstCreate16(abs(p->_RVMul) >> 8,NONE,NONE,NONE,0,0);
	p->RAdd = InstCreate16((p->_RAdd) >> 16,NONE,NONE,NONE,0,0);
	p->GUMul = InstCreate16(abs(p->_GUMul) >> 8,NONE,NONE,NONE,0,0);
	p->GVMul = InstCreate16(abs(p->_GVMul) >> 8,NONE,NONE,NONE,0,0);
	p->GAdd = InstCreate16((p->_GAdd) >> 16,NONE,NONE,NONE,0,0);
	p->BUMul = InstCreate16(abs(p->_BUMul) >> 8,NONE,NONE,NONE,0,0);
	p->BAdd = InstCreate16((p->_BAdd) >> 16,NONE,NONE,NONE,0,0);

	CodeBegin();
	I2C(SUB,SP,SP,OFS(stack,StackFrame));

	I1P(WLDRD,WR11,p->RAdd,0);
	I1P(WLDRD,WR14,p->RVMul,0);

	I3(MOV,R10,NONE,R3); //DstPitch
	I2C(LDR,R9,R1,0); //Dst[0] RGB
	I2C(LDR,R3,R2,4); //Src[1] U
	I2C(LDR,R4,R2,8); //Src[2] V
	I2C(LDR,R12,R2,0); //Src[0] Y
	I2C(LDR,R11,SP,OFS(stack,SrcPitch));

	I2C(WSHUFH,WR15,WR11,0xFF); //ymul
	if (!(p->FX.Flags & BLITFX_DITHER))
	{
		I2C(WSHUFH,WR12,WR11,0x55); //gadd
		I2C(WSHUFH,WR13,WR11,0xAA); //badd
		I2C(WSHUFH,WR11,WR11,0x00); //radd
	}
	else
	{
		int i;
		static const uint8_t Matrix0[16] = 
		{	0,   8,  2, 10,
			12,  4, 14,  6,
			3,  11,  1,  9,
			15,  7, 13,  5 };

		uint8_t Matrix[16];
		memcpy(Matrix,Matrix0,sizeof(Matrix));
		p->DitherSize = min(p->DstSize[0],min(p->DstSize[1],p->DstSize[2]));
		if (p->DitherSize>4)
			for (i=0;i<16;++i)
				Matrix[i] >>= p->DitherSize-4;

		// dither mask
		Dither = InstCreate(Matrix,16,NONE,NONE,NONE,0,0);
		I1P(WLDRD,WR12,Dither,0);
		I1P(WLDRD,WR13,Dither,8);
	}

 	I2C(LDR,R5,SP,OFS(stack,Height));
	I2C(LDR,R6,SP,OFS(stack,Width));

	//SrcNext = 2*(SrcHalfY?2:1)*(SwapXY?4:1)*Src->Pitch - (Width*(SrcHalfY?2:1) >> SrcDoubleX)
	I3S(MOV,R1,NONE,R11,LSL,1+p->SrcHalfY+p->SwapXY*2);
	I3S(SUB,R1,R1,R6,LSR,p->SrcDoubleX-p->SrcHalfX); 
	I2C(STR,R1,SP,OFS(stack,SrcNext));

	//UVNext = (Src->Pitch >> 1)*(SrcHalfY?2:1)*(SwapXY?4:1) - (Width*(SrcHalfY?2:1) >> SrcDoubleX >> 1);
	I3S(MOV,R2,NONE,R11,ASR,1-p->SrcHalfY-p->SwapXY*2);
	I3S(SUB,R2,R2,R6,LSR,p->SrcDoubleX+1-p->SrcHalfX); 
	I2C(STR,R2,SP,OFS(stack,UVNext));

	if (p->DirX<0) //adjust reversed destination for block size
		I2C(SUB,R9,R9,-(p->DstStepX >> 1)-(p->DstBPP >> 3));

	if (p->SwapXY)
	{
		// EndOfRect = Dst + ((Height * DstBPP * DirX) >> 3)
		I2C(MOV,R1,NONE,p->DstBPP * p->DirX);
		I3(MUL,R1,R5,R1);
		I3S(ADD,R1,R9,R1,ASR,3);
		I2C(STR,R1,SP,OFS(stack,EndOfRect));

		//DstNext = DstStepX - Width*DstPitch;
		MB(); I3(MUL,R2,R10,R6);
		I2C(MOV,R1,NONE,p->DstStepX); 
		I3(SUB,R1,R1,R2); 
		I2C(STR,R1,SP,OFS(stack,DstNext));
	}
	else
	{
		// EndOfRect = Dst + DstPitch * Height
		I3(MUL,R1,R10,R5);
		I3(ADD,R1,R9,R1);
		I2C(STR,R1,SP,OFS(stack,EndOfRect));

		//DstNext = ((DstPitch*2 << DstDoubleY) - DirX * Width << DstBPP2;
		I3S(MOV,R2,NONE,R10,LSL,p->DstDoubleY+1);
		I3S(p->DirX>0?SUB:ADD,R2,R2,R6,LSL,p->DstBPP2); 
		I2C(STR,R2,SP,OFS(stack,DstNext));
	}

	// setup shift registers
	// wcgr0 abs(rpos-8)
	// wcgr1 abs(gpos-8)
	// wcgr2 abs(bpos-8)
	// wcgr3 8 (if HalfMode)

	I2C(MOV,R5,NONE,abs(p->DstPos[0]+p->DstSize[0]-8));
	I2C(MOV,R6,NONE,abs(p->DstPos[1]+p->DstSize[1]-8));
	I2C(MOV,R7,NONE,abs(p->DstPos[2]+p->DstSize[2]-8));
	I2(TMCR,WCGR0,R5);
	I2(TMCR,WCGR1,R6);
	I2(TMCR,WCGR2,R7);

	if (HalfMode)
	{
		I2C(MOV,R1,NONE,8);
		I2(TMCR,WCGR3,R1);
	}

	// setup masks
	// r0 bmask (if bpos!=0)
	// wr9,wr10 mask r,g

	I2C(MOV,R1,NONE,((1 << p->DstSize[1])-1)<<(8-p->DstSize[1]));
	I2(TBCSTB,WR10,R1);

	if (p->DstPos[2]==0 && p->DstPos[0]+p->DstSize[0]==16)
	{
		// (red and blue word mask in R9)
		I2C(MOV,R1,NONE,((1 << p->DstSize[0])-1)<<(16-p->DstSize[0]));
		I2C(ORR,R1,R1,(1 << p->DstSize[2])-1);
		I2(TBCSTH,WR9,R1);
	}
	else
	{
		I2C(MOV,R1,NONE,((1 << p->DstSize[0])-1)<<(8-p->DstSize[0]));
		I2C(MOV,R0,NONE,((1 << p->DstSize[2])-1)<<(8-p->DstSize[2]));
		I2(TBCSTB,WR9,R1);
	}

	if (p->SwapXY)
		I2C(ADD,R2,R9,(8*p->DirX) << p->DstDoubleX);
	else
		I3S(ADD,R2,R9,R10,LSL,p->DstDoubleY);

	I3S(ADD,R14,R12,R11,LSL,p->SrcHalfY);
	if (!p->SwapXY)
	{
		I2C(SUB,R3,R3,4);
		I2C(SUB,R4,R4,4);
	}

	I2C(LDR,R5,SP,OFS(stack,Width));
	
	LoopY = Label(0);
	I0P(B,AL,LoopY);

	Align(8);

	InstPost(p->RVMul);
	InstPost(p->GUMul);
	InstPost(p->GVMul);
	InstPost(p->BUMul);

	InstPost(p->RAdd);
	InstPost(p->GAdd);
	InstPost(p->BAdd);
	InstPost(p->YMul);

	if (Dither)
		InstPost(Dither);
	InstPost(LoopY);

	if (p->SwapXY)
	{
		I3(MUL,R1,R10,R5); //dstpitch * width
		I3(ADD,R8,R9,R1);
	}
	else
	{
		if (p->DirX > 0)
			I3S(ADD,R8,R9,R5,LSL,p->DstBPP2);
		else
			I3S(SUB,R8,R9,R5,LSL,p->DstBPP2);
	}

	LoopX = Label(0);

	// preload
	if (!p->Slices)
	{
		dyninst* PreLoad1;
		dyninst* PreLoad2;
		dyninst* PreLoad3;
		dyninst* PreLoad4;
		int UVAdj = p->SwapXY?0:4;

		I3S(ADD,R1,R12,R5,ASR,(p->SrcDoubleX?1:0)-(p->SrcHalfX?1:0));
		I2C(ADD,R5,R12,32);
		I3(CMP,NONE,R5,R1);
		I0P(B,CS,LoopX);

		//y0
		PreLoad1 = Label(1);
		Byte(); I2C(LDR,R6,R5,-32);
		I2C(ADD,R5,R5,64);
		I3(CMP,NONE,R5,R1);
		Byte(); I2C(LDR,R7,R5,-64);
		I0P(B,CC,PreLoad1);

		I3(SUB,R1,R1,R12);
		I3(ADD,R1,R1,R14);
		I2C(ADD,R5,R14,32);

		//y1
		PreLoad2 = Label(1);
		Byte(); I2C(LDR,R6,R5,-32);
		I2C(ADD,R5,R5,64);
		I3(CMP,NONE,R5,R1);
		Byte(); I2C(LDR,R7,R5,-64);
		I0P(B,CC,PreLoad2);

		I3(SUB,R1,R1,R14);

		I3S(ADD,R1,R3,R1,ASR,p->SrcUVX2);

		I2C(ADD,R5,R3,32);
		I3(CMP,NONE,R5,R1);
		I0P(B,CS,LoopX);

		//u
		PreLoad3 = Label(1);
		Byte(); I2C(LDR,R6,R5,-32+UVAdj);
		I2C(ADD,R5,R5,64);
		I3(CMP,NONE,R5,R1);
		Byte(); I2C(LDR,R7,R5,-64+UVAdj);
		I0P(B,CC,PreLoad3);

		I3(SUB,R1,R1,R3);

		I3(ADD,R1,R1,R4);
		I2C(ADD,R5,R4,32);

		//v
		PreLoad4 = Label(1);
		Byte(); I2C(LDR,R6,R5,-32+UVAdj);
		I2C(ADD,R5,R5,64);
		I3(CMP,NONE,R5,R1);
		Byte(); I2C(LDR,R7,R5,-64+UVAdj);
		I0P(B,CC,PreLoad4);
	}
	else
	if (p->ARM5)
	{
		//preload next
		I3S(PLD,NONE,R12,R11,LSL,p->SrcHalfY+1); 
		I3S(PLD,NONE,R14,R11,LSL,p->SrcHalfY+1);
		I3S(PLD,NONE,R3,R11,ASR,p->SrcUVPitch2);
		I3S(PLD,NONE,R4,R11,ASR,p->SrcUVPitch2);
	}

	EndLine = Label(0);
	InstPost(LoopX);
	{
		if (!HalfMode) Fix_UV(p,0,0,0);
		Fix_Y(p,0,0,HalfMode);
		Fix_Y(p,1,0,HalfMode);

		if (p->SwapXY && (p->FX.Flags & BLITFX_DITHER))
		{
			I3(CMP,NONE,R9,R8);
			I0P(B,EQ,EndLine);

			if (!HalfMode) Fix_UV(p,0,0,0);
			Fix_Y(p,0,1,HalfMode);
			Fix_Y(p,1,1,HalfMode);
		}

		I3(CMP,NONE,R9,R8);
		I0P(B,NE,LoopX);
	}
	InstPost(EndLine);

	I2C(LDR,R5,SP,OFS(stack,SrcNext));
	I2C(LDR,R6,SP,OFS(stack,DstNext));
	I2C(LDR,R7,SP,OFS(stack,UVNext));
	I2C(LDR,R8,SP,OFS(stack,EndOfRect));
	
	//increment pointers
	I3(ADD,R12,R12,R5);
	I3(ADD,R14,R14,R5);
	I3(ADD,R2,R2,R6);
	I3(ADD,R9,R9,R6);
	I3(ADD,R3,R3,R7);
	I3(ADD,R4,R4,R7);

	if (!p->SwapXY && (p->FX.Flags & BLITFX_DITHER))
	{
		//swap WR12 and WR13
		I3(WOR,WR3,WR12,WR12);
		I3(WOR,WR12,WR13,WR13);
		I3(WOR,WR13,WR3,WR3);
	}

	//prepare registers for next row
	I2C(LDR,R5,SP,OFS(stack,Width));

	I3(CMP,NONE,R9,R8);
	I0P(B,NE,LoopY);

	I2C(ADD,SP,SP,OFS(stack,StackFrame));
	CodeEnd();
}

#endif
