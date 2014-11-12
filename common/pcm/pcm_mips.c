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
 * $Id: pcm_mips.c 304 2005-10-20 11:02:59Z picard $
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "pcm_soft.h"

#if defined(MIPS)

typedef struct stack
{
	void* this;   // R4
	char* Dst;	  // R5
	char* Src;	  // R6
	int DstLength;// R7
	pcmstate* State;
	int Volume;

} stack;

// R13 tmp
// R24,R25 dither 
// R2,R3 value
// R4 Min
// R5 Max
// R7 Step
// R8 Pos
// R9 SrcLeft
// R10 SrcRight
// R11 DstLeft-1
// R12 DstRight-1
// R14 DstLeftEnd-1

void PCMLoop(pcm_soft* p,bool_t Speed)
{
	bool_t DstPacked = !(p->Dst.Flags & PCM_PLANES) && p->Dst.Channels==2 && (p->Dst.Bits<=16);
	dyninst* Loop;
	reg Left;
	reg Right;

	Loop = Label(1);

	Left = R2;
	if (p->UseLeft)
		Right = R3;
	else
		Right = R2;

	if (Speed)
	{
		I2C(SRL,R3,R8,8);
		if (p->SrcShift) I2C(SLL,R3,R3,p->SrcShift);
		if (p->UseLeft)
			I3(ADDU,R2,R3,R9);
		if (p->UseRight)
			I3(ADDU,R3,R3,R10);

		switch (p->Src.Bits)
		{
		case 8:
			if (p->UseLeft)
				I2C(p->SrcUnsigned ? LBU:LB,R2,R2,0);
			if (p->UseRight)
				I2C(p->SrcUnsigned ? LBU:LB,R3,R3,0);
			break;

		case 16:
			if (p->UseLeft)
				I2C(p->SrcUnsigned ? LHU:LH,R2,R2,0);
			if (p->UseRight)
				I2C(p->SrcUnsigned ? LHU:LH,R3,R3,0);
			break;

		case 32:
			if (p->UseLeft)
				I2C(LW,R2,R2,0);
			if (p->UseRight)
				I2C(LW,R3,R3,0);
			break;
		}
		I3(ADDU,R8,R8,R7);
	}
	else
	{
		switch (p->Src.Bits)
		{
		case 8:
			if (p->UseLeft)
				I2C(p->SrcUnsigned ? LBU:LB,R2,R9,0);
			if (p->UseRight)
				I2C(p->SrcUnsigned ? LBU:LB,R3,R10,0);
			break;

		case 16:
			if (p->UseLeft)
				I2C(p->SrcUnsigned ? LHU:LH,R2,R9,0);
			if (p->UseRight)
				I2C(p->SrcUnsigned ? LHU:LH,R3,R10,0);
			break;

		case 32:
			if (p->UseLeft)
				I2C(LW,R2,R9,0);
			if (p->UseRight)
				I2C(LW,R3,R10,0);
			break;
		}

		if (p->UseLeft)
			I2C(ADDIU,R9,R9,1<<p->SrcShift);
		if (p->UseRight)
			I2C(ADDIU,R10,R10,1<<p->SrcShift);
	}

	if (p->SrcUnsigned && p->SrcUnsigned <= 0x8000) 
	{
		I2C(ADDIU,R2,R2,-p->SrcUnsigned);
		if (p->SrcChannels>1)
			I2C(ADDIU,R3,R3,-p->SrcUnsigned);
	}

	if (p->Stereo)
	{
		if ((p->Src.Flags ^ p->Dst.Flags) & PCM_SWAPPEDSTEREO)
		{
			Left = R3;
			Right = R2;
		}
		else
		{
			Left = R2;
			Right = R3;
		}
	}
	else
	{
		if (p->Join)
			I3(ADDU,R2,R2,R3);
		Right = Left = R2;
	}

	if (p->ActualDither)
	{
		I3(ADDU,R2,R2,R24);
		I3(ADDU,R24,R2,ZERO);
		if (p->Stereo) 
		{
			I3(ADDU,R3,R3,R25);
			I3(ADDU,R25,R3,ZERO);
		}
	}

	if (p->UseVolume)
	{
		if (p->Src.Bits >= 23)
			I2C(SRA,R2,R2,8); 

		I2(MULT,R2,R24);
		I1(MFLO,R2);

		if (p->Stereo)
		{
			if (p->Src.Bits >= 23)
				I2C(SRA,R3,R3,8);
			else
				NOP();
			NOP(); // MULT and MFLO too close

			I2(MULT,R3,R24);
			I1(MFLO,R3);
		}
	}

	if (p->Clip>0)
	{
		dyninst* ClipMin = Label(0);
		dyninst* ClipMax = Label(0);

		I3(SUBU,R13,R2,R4);
		I1P(BGEZ,R13,ClipMin,0);
		I3(SUBU,R13,R5,R2); // delay slot
		I3(ADDU,R2,R4,ZERO);
		InstPost(ClipMin);

		I1P(BGEZ,R13,ClipMax,0);
		I2C(ADDIU,R11,R11,1<<p->DstShift); // delay slot
		I3(ADDU,R2,R5,ZERO);
		InstPost(ClipMax);

		if (p->Stereo)
		{
			dyninst* ClipMin = Label(0);
			dyninst* ClipMax = Label(0);

			I3(SUBU,R13,R3,R4);
			I1P(BGEZ,R13,ClipMin,0);
			I3(SUBU,R13,R5,R3); // delay slot
			I3(ADDU,R3,R4,ZERO);
			InstPost(ClipMin);

			I1P(BGEZ,R13,ClipMax,0);
			I2C(ADDIU,R12,R12,1<<p->DstShift); // delay slot
			I3(ADDU,R3,R5,ZERO);
			InstPost(ClipMax);
		}
		else
		if (p->Dst.Channels>1 && !DstPacked)
			I2C(ADDIU,R12,R12,1<<p->DstShift); // delay slot
	}
	else
	{
		I2C(ADDIU,R11,R11,1<<p->DstShift);
		if (p->Dst.Channels>1 && !DstPacked)
			I2C(ADDIU,R12,R12,1<<p->DstShift);
	}

	if (p->Shift)
	{
		I2C(SRA,R2,R2,-p->Shift);
		if (p->Stereo) I2C(SRA,R3,R3,-p->Shift);
	}

	if (p->ActualDither)
	{
		I2C(SRA,R13,R2,p->Shift);
		I3(SUBU,R24,R24,R13);
		if (p->Stereo)
		{
			I2C(SRA,R13,R3,p->Shift);
			I3(SUBU,R25,R25,R13);
		}
	}

	if (p->DstUnsigned && p->DstUnsigned < 0x8000) 
	{
		I2C(ADDIU,Left,Left,p->DstUnsigned);
		if (Left != Right)
			I2C(ADDIU,Right,Right,p->DstUnsigned);
	}

	switch (p->Dst.Bits)
	{
	case 8:
		if (DstPacked)
		{
			I2C(SLL,Right,Right,8);
			I2C(ANDI,Left,Left,0xFF);
			I3(OR,Left,Left,Right);
			I2C(SH,Left,R11,0);
		}
		else
		{
			I2C(SB,Left,R11,0);
			if (p->Dst.Channels>1)
				I2C(SB,Right,R12,0);
		}
		break;

	case 16:
		if (DstPacked)
		{
			I2C(SLL,Right,Right,16);
			I2C(ANDI,Left,Left,0xFFFF);
			I3(OR,Left,Left,Right);
			I2C(SW,Left,R11,0);
		}
		else
		{
			I2C(SH,Left,R11,0);
			if (p->Dst.Channels>1)
				I2C(SH,Right,R12,0);
		}
		break;

	case 32:
		I2C(SW,Left,R11,0);
		if (p->Dst.Channels>1)
			I2C(SW,Right,R12,0);
		break;
	}

	DS(); I2P(BNE,R11,R14,Loop);
	
	if (p->ActualDither)
	{
		I2C(LW,R4,SP,OFS(stack,State));
		I2C(SW,R24,R4,OFS(pcmstate,Dither[0]));
		I2C(SW,R25,R4,OFS(pcmstate,Dither[1]));
	}
	else
		I3(ADDU,R4,ZERO,ZERO); // for the delay slot

	CodeEnd(0,0,0);
}

void PCMCompile(pcm_soft* p)
{
	dyninst* Speed;

	CodeBegin(0,0,0);

	// dst pointers
	I2C(LW,R11,R5,0);
	if (p->Dst.Channels > 1)
	{
		if (p->Dst.Flags & PCM_PLANES)
		{
			I2C(LW,R12,R5,4);
		}
		else
			I2C(ADDIU,R12,R11,1<<(p->DstShift-1));
	}

	I2C(ADDIU,R11,R11,-(1<<p->DstShift)); // back one step
	if (p->Dst.Channels > 1)
		I2C(ADDIU,R12,R12,-(1<<p->DstShift)); // back one step
	I3(ADDU,R14,R11,R7); // dstend-1

	// src pointers
	I2C(LW,R9,R6,0);
	if (p->Src.Channels > 1)
	{
		if (p->Src.Flags & PCM_PLANES)
			I2C(LW,R10,R6,4);
		else
			I2C(ADDIU,R10,R9,p->Src.Bits>>3);
	}

	I2C(LW,R4,SP,OFS(stack,State));

	I2C(LW,R7,R4,OFS(pcmstate,Step));
	I2C(LW,R8,R4,OFS(pcmstate,Pos));

	if (p->UseVolume)
		I2C(LW,R24,SP,OFS(stack,Volume));

	if (p->ActualDither)
	{
		I2C(LW,R24,R4,OFS(pcmstate,Dither[0]));
		I2C(LW,R25,R4,OFS(pcmstate,Dither[1]));
	}

	if (p->Clip>0)
	{
		IConst(R4,p->MinLimit);
		IConst(R5,p->MaxLimit);
	}

	Speed = Label(0);

	I2C(ADDIU,R13,ZERO,256);
	I2P(BNE,R13,R7,Speed);

	PCMLoop(p,0);

	InstPost(Speed);

	PCMLoop(p,1);
}


#endif
