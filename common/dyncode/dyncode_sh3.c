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
 * $Id: dyncode_sh3.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "dyncode.h"

#if defined(SH3) && defined(CONFIG_DYNCODE)

//format coded in 'code' at 16bit position
//00: rn,imm
//01: rn,label,disp8*2
//02: rn,label,disp8*4
//03: rm,rn
//04: rn,disp4*1
//05: rn,disp4*2
//06: rm,rn,disp4*4
//07: rn
//08: imm
//09: -
//0A: label8
//0B: label12
//0C: rn (branch)
//0D: - (branch)

//read at 20bit / write at 24bit
//01: r1
//02: r2
//04: R0
//08: T

//spec at 28bit
//01: read mac
//02: read pr
//04: write mac
//08: write pr

static INLINE int Format(int i) { return (i >> 16) & 15; }
static INLINE int C(int i) { return i & 0xFFFF; }

void InstInit()
{
}

void InstPost(dyninst* p)
{
	context* c = Context();
	InstAdd(p,c);
}

void PostAdj(dyninst* p,int Code,reg Rn,reg Rm)
{
	if (p)
	{
		if (Code & (1<<20)) p->RdRegs |= 1 << Rn;
		if (Code & (2<<20)) p->RdRegs |= 1 << Rm;
		if (Code & (4<<20)) p->RdRegs |= 1 << R0;
		if (Code & (1<<28)) p->RdRegs |= 1 << MACL;
		if (Code & (2<<28)) p->RdRegs |= 1 << PR;
		p->RdFlags = Code & (8<<20) ? 1:0;

		if (Code & (1<<24)) p->WrRegs |= 1 << Rn;
		if (Code & (2<<24)) p->WrRegs |= 1 << Rm;
		if (Code & (4<<24)) p->WrRegs |= 1 << R0;
		if (Code & (4<<28)) p->WrRegs |= 1 << MACL;
		if (Code & (8<<28)) p->WrRegs |= 1 << PR;
		p->WrFlags = Code & (8<<24) ? 1:0;
	}
	InstPost(p);
}

void IShift(reg Rn, reg Tmp, int Left)
{
	switch (Left)
	{
	default:
		if (Tmp != NONE)
		{
			I1C(MOVI,Tmp,Left);
			I2(SHLD,Tmp,Rn);
			break;
		}
		// no break;
	case 16+8:
	case 16+2:
	case 16+1:
	case 16:
	case 8+2:
	case 8+1:
	case 8:
	case 2+1:
	case 2:
	case 1:
	case 0:
	case -16-8:
	case -16-2:
	case -16-1:
	case -16:
	case -8-2:
	case -8-1:
	case -8:
	case -2-1:
	case -2:
	case -1:
		while (Left <= -16) { I1(SHLR16,Rn); Left += 16; }
		while (Left >= 16) { I1(SHLL16,Rn); Left -= 16; }
		while (Left <= -8) { I1(SHLR8,Rn); Left += 8; }
		while (Left >= 8) { I1(SHLL8,Rn); Left -= 8; }
		while (Left <= -2) { I1(SHLR2,Rn); Left += 2; }
		while (Left >= 2) { I1(SHLL2,Rn); Left -= 2; }
		while (Left <= -1) { I1(SHLR,Rn); Left += 1; }
		while (Left >= 1) { I1(SHLL,Rn); Left -= 1; }
		break;
	}
}

void I0C(int Code, int Const)
{
	dyninst* p = NULL;
	if (Format(Code)==8 && Const >= -128 && Const <= 127)
		p = InstCreate16(C(Code) | (Const & 255),NONE,NONE,NONE,0,0);
	if (Format(Code)==9 && Const == 0)
		p = InstCreate16(C(Code),NONE,NONE,NONE,0,0);
	if (Format(Code)==13 && Const == 0)
	{
		p = InstCreate16(C(Code),NONE,NONE,NONE,0,0);
		if (p)
			p->Branch = 1;
	}
	PostAdj(p,Code,NONE,NONE);
}

void I0(int Code)
{
	I0C(Code,0);
}

void I1C(int Code, reg Rn, int Const)
{
	dyninst* p = NULL;
	if (Format(Code)==0 && Const >= -128 && Const <= 127)
		p = InstCreate16(C(Code) | (Rn << 8) | (Const & 255),NONE,NONE,NONE,0,0);
	if (Format(Code)==4 && Const >= -8 && Const <= 7)
		p = InstCreate16(C(Code) | (Rn << 4) | (Const & 15),NONE,NONE,NONE,0,0);
	if (Format(Code)==5 && !(Const & 1) && Const >= -16 && Const <= 14)
		p = InstCreate16(C(Code) | (Rn << 4) | ((Const >> 1) & 15),NONE,NONE,NONE,0,0);
	if (Format(Code)==7 && Const == 0)
		p = InstCreate16(C(Code) | (Rn << 8),NONE,NONE,NONE,0,0);
	if (Format(Code)==12 && Const == 0)
	{
		p = InstCreate16(C(Code) | (Rn << 8),NONE,NONE,NONE,0,0);
		if (p)
			p->Branch = 1;
	}
	PostAdj(p,Code,Rn,NONE);
}

void I1(int Code, reg Rn)
{
	I1C(Code,Rn,0);
}

void I2C(int Code, reg Rm, reg Rn, int Const)
{
	dyninst* p = NULL;
	if (Format(Code)==3 && Const == 0)
		p = InstCreate16(C(Code) | (Rn << 8) | (Rm << 4),NONE,NONE,NONE,0,0);
	if (Format(Code)==6 && !(Const & 3) && Const >= -32 && Const <= 28)
		p = InstCreate16(C(Code) | (Rn << 8) | (Rm << 4) | ((Const >> 2) & 15),NONE,NONE,NONE,0,0);
	PostAdj(p,Code,Rm,Rn);
}

void I2(int Code, reg Rm, reg Rn)
{
	I2C(Code,Rm,Rn,0);
}

dyninst* ICode(dyninst* Block, int Ofs)
{
	dyninst* p = InstCreate32(Ofs,NONE,NONE,NONE,0,0);
	if (p)
	{
		p->Tag = -1;
		p->ReAlloc = Block;
		p->Branch = 1;
	}
	return p;
}

void I1P(int Code, reg Rn, dyninst* Block, int Ofs)
{
	dyninst* p = NULL;
	if (Format(Code)==1 && !(Ofs & 1) && Ofs >= -128*2 && Ofs <= 127*2)
	{
		p = InstCreate16(C(Code) | (Rn << 8) | ((Ofs >> 1) & 255),Rn,NONE,NONE,0,0);
		if (p)
		{
			p->Tag = Format(Code);
			p->ReAlloc = Block;
			p->Branch = 1;
		}
	}
	if (Format(Code)==2 && !(Ofs & 3) && Ofs >= -128*4 && Ofs <= 127*4)
	{
		p = InstCreate16(C(Code) | (Rn << 8) | ((Ofs >> 2) & 255),Rn,NONE,NONE,0,0);
		if (p)
		{
			p->Tag = Format(Code);
			p->ReAlloc = Block;
			p->Branch = 1;
		}
	}
	PostAdj(p,Code,Rn,NONE);
}

void I0P(int Code, dyninst* Block)
{
	dyninst* p = NULL;
	if (Format(Code)==10 || Format(Code)==11)
	{
		p = InstCreate16(C(Code),NONE,NONE,NONE,0,0);
		if (p)
		{
			p->Tag = Format(Code);
			p->ReAlloc = Block;
			p->Branch = 1;
		}
	}
	PostAdj(p,Code,NONE,NONE);
}

bool_t InstReAlloc(dyninst* p,dyninst* ReAlloc)
{
	int Diff = ReAlloc->Address - (p->Address+4);
	uint16_t* Code = (uint16_t*) InstCode(p);
	
	Diff >>= 1;

	assert(p->Tag == 10 || p->Tag == 1 || p->Tag == 2 || p->Tag == 11 || p->Tag == -1);

	if (p->Tag == -1) // 32bit code address
	{
		*(uint32_t*)Code += (uint32_t)ReAlloc->Address;
		return 1;
	}
	else
	if (p->Tag == 10 || p->Tag == 1 || p->Tag == 2) //8bit
	{
		if (p->Tag == 2)
		{
			assert(((int)ReAlloc->Address & 3)==0);
			Diff = ((int)ReAlloc->Address >> 2) - (((int)p->Address+4) >> 2);
		}
		Diff += *(int8_t*)Code;
		assert(Diff <= 127 && Diff >= -128);
		if (Diff > 127 || Diff < -128)
			return 0;
		*Code &= 0xFF00;
		*Code |= (uint16_t)(Diff & 0xFF);
		return 1;
	}
	else
	if (p->Tag == 11) //12bit
	{
		assert(Diff <= 2047 && Diff >= -2048);
		if (Diff > 2047 || Diff < -2048)
			return 0;
		*Code &= 0xF000;
		*Code |= (uint16_t)(Diff & 0xFFF);
		return 1;
	}
	return 0;
}

void CodeBegin(int Save,int Local)
{
	int i;
	assert(Save <= 7);
	for (i=0;i<Save;++i)
		I2(MOVL_STSUB,(reg)(R8+i),SP);
	if (Local)
		I1C(ADDI,SP,-Local);
}

void CodeEnd(int Save,int Local)
{
	int i;
	assert(Save <= 7);
	if (Local)
		I1C(ADDI,SP,Local);
	for (i=Save-1;i>=0;--i)
		I2(MOVL_LDADD,SP,(reg)(R8+i));
	DS(); I0(RTS);
}

void NOP()
{
	dyninst* p = InstCreate16(0x0009,NONE,NONE,NONE,0,0);
	if (p)
		p->Branch = 1;
	InstPost(p);
}

void Break()
{
	dyninst* p = InstCreate16(0xC300,NONE,NONE,NONE,0,0);
	if (p)
		p->Branch = 1;
	InstPost(p);
}

#endif
