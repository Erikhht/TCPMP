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
 * $Id: dyncode_mips.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "dyncode.h"

#if defined(MIPS) && defined(CONFIG_DYNCODE)

//format coded in 'code' at 8bit position
//00: rs,rt,offset(s16<<2) (branch)
//01: rs,rt,rd
//02: rs,rt,immd(s16) write rt
//03: rs,offset(s16<<2)
//04: rs,rt ->lo,hi
//05: rt,rd,sa
//06: rt,rs,rd
//07: addr(s26<<2)
//08: rs,rt (branch)
//09: rs	(branch)
//0A: rs,immd(s16)
//0B: rd <-> lo,hi
//0C: rs,rt,immd(s16) read rt

#define LO	1
#define HI	2

static const reg SaveRegs[11] =
{
	R16,R17,R18,R19,R20,R21,R22,R23,
	R30,R28,R31
};

void InstInit()
{
}

static INLINE int Format(int i) { return (i >> 8) & 15; }
static INLINE int C(int i) { return i & ~0xFF00; }

void I3(int Code, reg Dest, reg Op1, reg Op2)
{
	dyninst* p = NULL;
	if (Format(Code)==1)
		p = InstCreate32(C(Code) | (Dest << 11) | (Op1 << 21) | (Op2 << 16),Dest,Op1,Op2,0,0);
	if (Format(Code)==6)
		p = InstCreate32(C(Code) | (Dest << 11) | (Op2 << 21) | (Op1 << 16),Dest,Op1,Op2,0,0);
	InstPost(p);
}

void I2C(int Code, reg Dest, reg Op1, int Const)
{
	dyninst* p = NULL;

	if (Code == DSLL && Const < 0)
	{
		Code = DSRL;
		Const = -Const;
	}
	if (Code == DSRL && Const < 0)
	{
		Code = DSLL;
		Const = -Const;
	}
	if (Code == SLL && Const < 0)
	{
		Code = SRL;
		Const = -Const;
	}
	if (Code == SRL && Const < 0)
	{
		Code = SLL;
		Const = -Const;
	}
	if (Code == SRA && Const < 0)
	{
		Code = SLL;
		Const = -Const;
	}
	if (Code == DSLL && Const >= 32)
	{
		Code = DSLL32;
		Const -= 32;
	}
	if (Code == DSRA && Const >= 32)
	{
		Code = DSRA32;
		Const -= 32;
	}
	if (Code == DSRL && Const >= 32)
	{
		Code = DSRL32;
		Const -= 32;
	}

	assert(Format(Code)!=4 || Const==0);
	if (Format(Code)==4 && Const == 0)
	{
		p = InstCreate32(C(Code) | (Dest << 21) | (Op1 << 16),NONE,Dest,Op1,0,LO|HI);
	}
	assert(Format(Code)!=8 || Const==0);
	if (Format(Code)==8 && Const == 0)
	{
		p = InstCreate32(C(Code) | (Dest << 11) | (Op1 << 21),Dest,Op1,NONE,0,0);
		if (p)
			p->Branch = 1;
	}
	assert(Format(Code)!=5 || (Const >= 0 && Const < 32));
	if (Format(Code)==5 && Const >= 0 && Const < 32)
	{
		p = InstCreate32(C(Code) | (Dest << 11) | (Op1 << 16) | (Const << 6),Dest,Op1,NONE,0,0);
	}
	if ((Code == ANDI || Code == ORI || Code == XORI) && Const >= 32768)
		Const -= 65536;
	assert(Format(Code)!=2 || (Const >= -32768 && Const < 32768));
	if (Format(Code)==2 && Const >= -32768 && Const < 32768)
	{
		p = InstCreate32(C(Code) | (Dest << 16) | (Op1 << 21) | (Const & 0xFFFF),Dest,Op1,NONE,0,0);
	}
	assert(Format(Code)!=12 || (Const >= -32768 && Const < 32768));
	if (Format(Code)==12 && Const >= -32768 && Const < 32768)
	{
		p = InstCreate32(C(Code) | (Dest << 16) | (Op1 << 21) | (Const & 0xFFFF),NONE,Op1,Dest,0,0);
	}
	InstPost(p);
}

void I2(int Code, reg Rm, reg Rn)
{
	I2C(Code,Rm,Rn,0);
}

void I2P(int Code, reg Op1, reg Op2, dyninst* Block)
{
	dyninst* p = NULL;
	if (Format(Code)==0)
	{
		p = InstCreate32(C(Code) | (Op2 << 16) | (Op1 << 21),NONE,Op1,Op2,0,0);
		if (p)
		{
			p->Tag = Format(Code);
			p->ReAlloc = Block;
			p->Branch = 1;
		}
	}
	InstPost(p);
}

void I1C(int Code, reg Reg, int Const)
{
	dyninst* p = NULL;
	if (Format(Code)==10 && Const >= -32768 && Const < 32768)
	{
		p = InstCreate32(C(Code) | (Reg << 16) | (Const & 0xFFFF),Reg,NONE,NONE,0,0);
	}
	if (Format(Code)==11 && Const==0)
	{
		if (Code == MFHI) p = InstCreate32(C(Code) | (Reg << 11),Reg,NONE,NONE,HI,0);
		if (Code == MFLO) p = InstCreate32(C(Code) | (Reg << 11),Reg,NONE,NONE,LO,0);
		if (Code == MTHI) p = InstCreate32(C(Code) | (Reg << 21),NONE,Reg,NONE,0,HI);
		if (Code == MTLO) p = InstCreate32(C(Code) | (Reg << 21),NONE,Reg,NONE,0,LO);
	}
	if (Format(Code)==9 && Const==0)
	{
		p = InstCreate32(C(Code) | (Reg << 21),NONE,Reg,NONE,0,0);
		if (p)
			p->Branch = 1;
	}
	InstPost(p);
}

void I1(int Code, reg Rn)
{
	I1C(Code,Rn,0);
}

void I1P(int Code, reg Reg, dyninst* Block, int Ofs)
{
	dyninst* p = NULL;
	if (Format(Code)==3)
	{
		p = InstCreate32(C(Code) | (Reg << 21),NONE,Reg,NONE,0,0);
		if (p)
		{
			p->Tag = Format(Code);
			p->ReAlloc = Block;
		}
	}
	InstPost(p);
}

bool_t InstReAlloc(dyninst* p,dyninst* ReAlloc)
{
	int Diff = ReAlloc->Address - (p->Address+4);
	int* Code = (int*) InstCode(p);
	
	if (p->CodeSize > 4)
	{
		// address table
		int i;
		for (i=0;i<p->CodeSize;i+=4,++Code)
			*Code += (int)ReAlloc->Address;
		return 1;
	}

	Diff >>= 2;
	assert(Diff <= 32767 && Diff >= -32768);
	if (Diff > 32767 || Diff < -32768)
	{
		DEBUG_MSG1(-1,T("Realloc failed %d"),Diff);
		return 0;
	}

	assert(p->Tag==0 || p->Tag==3);
	if (p->Tag == 0)
	{
		*Code &= ~0xFFFF;
		*Code |= (uint16_t)Diff;
		return 1;
	}
	else
	if (p->Tag == 3)
	{
		*Code &= ~0xFFFF;
		*Code |= (uint16_t)Diff;
		return 1;
	}

	DEBUG_MSG(-1,T("Realloc failed unknown tag"));
	return 0;
}

void CodeBegin(int Save,int Local,bool_t Align64)
{
	int i;
	assert(Save <= 11);
	if (Align64)
	{
		I3(OR,R2,SP,SP);
		I2C(ADDIU,SP,SP,-8);
		I2C(ANDI,R8,SP,4);
		I3(ADDU,SP,SP,R8);
		I2C(SW,R8,SP,0);
	}
	if (Save || Local)
	{
		I2C(ADDIU,SP,SP,-Save*4-Local);
		for (i=0;i<Save;++i)
			I2C(SW,SaveRegs[i],SP,i*4+Local);
	}
}

void CodeEnd(int Save,int Local,bool_t Align64)
{
	if (Save || Local)
	{
		int i;
		assert(Save <= 11);
		for (i=Save-1;i>=0;--i)
			I2C(LW,SaveRegs[i],SP,i*4+Local);
		I2C(ADDIU,SP,SP,Save*4+Local);
	}
	if (Align64)
	{
		I2C(LW,R8,SP,0);
		I3(SUBU,SP,SP,R8);
		I2C(ADDIU,SP,SP,8);
	}
	DS(); I1(JR,RA);
}

void NOP()
{
	dyninst* p = InstCreate32(0,NONE,NONE,NONE,0,0);
	if (p)
		p->Branch = 1;
	InstPost(p);
}

void Break()
{
	dyninst* p = InstCreate32(0x0000000D,NONE,NONE,NONE,0,0);
	if (p)
		p->Branch = 1;
	InstPost(p);
}

void InstPost(dyninst* p)
{
	context* c = Context();
	InstAdd(p,c);
}

void IConst(reg Dst,int Const)
{
	I1C(LUI,Dst,Const >> 16);
	I2C(ORI,Dst,Dst,(int16_t)Const);
}

reg IMul(reg Op1, int Mul, reg Tmp1, reg Tmp2, reg Tmp3, bool_t* Neg)
{
	reg Result = Tmp3;

	if (Neg) 
		*Neg = 0;

	if (Mul>=0)
	switch (Mul)
	{
	case 0: Result=ZERO; break;
	case 1: Result=Op1; break;
	case 2: I3(ADDU,Result,Op1,Op1); break;
	case 3: I2C(SLL,Tmp1,Op1,1); I3(ADDU,Result,Tmp1,Op1); break;
	case 4: I2C(SLL,Result,Op1,2); break;
	case 5: I2C(SLL,Tmp1,Op1,2); I3(ADDU,Result,Tmp1,Op1); break;
	case 6: I2C(SLL,Tmp1,Op1,2); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Result,Tmp1,Tmp2); break;
	case 7: I2C(SLL,Tmp1,Op1,3); I3(SUBU,Result,Tmp1,Op1); break;
	case 8: I2C(SLL,Result,Op1,3); break;
	case 9: I2C(SLL,Tmp1,Op1,3); I3(ADDU,Result,Tmp1,Op1); break;
	case 10: I2C(SLL,Tmp1,Op1,3); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Result,Tmp1,Tmp2); break;
	case 11: I2C(SLL,Tmp1,Op1,3); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 12: I2C(SLL,Tmp1,Op1,3); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Result,Tmp1,Tmp2); break; 
	case 13: I2C(SLL,Tmp1,Op1,3); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 14: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,1); I3(SUBU,Result,Tmp1,Tmp2); break; 
	case 15: I2C(SLL,Tmp1,Op1,4); I3(SUBU,Result,Tmp1,Op1); break;
	case 16: I2C(SLL,Result,Op1,4); break;
	case 17: I2C(SLL,Tmp1,Op1,4); I3(ADDU,Result,Tmp1,Op1); break;
	case 18: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Result,Tmp1,Tmp2); break;
	case 19: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 20: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Result,Tmp1,Tmp2); break;
	case 21: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 22: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); I3(ADDU,Result,Result,Op1); break;
	case 23: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,3); I3(ADDU,Result,Tmp1,Tmp2); I3(SUBU,Result,Result,Op1); break;
	case 24: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,3); I3(ADDU,Result,Tmp1,Tmp2); break;
	case 25: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,3); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 26: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,3); I3(ADDU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); I3(ADDU,Result,Result,Op1); break;
	case 27: I2C(SLL,Tmp1,Op1,5); I2C(SLL,Tmp2,Op1,2); I3(SUBU,Result,Tmp1,Tmp2); I3(SUBU,Result,Result,Op1); break;
	case 28: I2C(SLL,Tmp1,Op1,5); I2C(SLL,Tmp2,Op1,2); I3(SUBU,Result,Tmp1,Tmp2); break;
	case 29: I2C(SLL,Tmp1,Op1,5); I2C(SLL,Tmp2,Op1,2); I3(SUBU,Result,Tmp1,Tmp2); I3(ADDU,Result,Result,Op1); break;
	case 30: I2C(SLL,Tmp1,Op1,5); I2C(SLL,Tmp2,Op1,1); I3(SUBU,Result,Tmp1,Tmp2); break;
	case 31: I2C(SLL,Tmp1,Op1,5); I3(SUBU,Result,Tmp1,Op1); break;
	default: I2C(SLL,Result,Op1,5); break;
	}
	else
	switch (Mul)
	{
	case -1: I3(SUBU,Result,ZERO,Op1); break;
	case -3: I2C(SLL,Tmp1,Op1,2); I3(SUBU,Result,Op1,Tmp1); break;
	case -6: I2C(SLL,Tmp1,Op1,1); I2C(SLL,Tmp2,Op1,3); I3(SUBU,Result,Tmp1,Tmp2); break;
	case -7: I2C(SLL,Tmp1,Op1,3); I3(SUBU,Result,Op1,Tmp1); break;
	case -11: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(ADDU,Tmp2,Tmp2,Op1); I3(SUBU,Result,Tmp2,Tmp1); break;
	case -12: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(SUBU,Result,Tmp2,Tmp1); break; 
	case -13: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,1); I3(ADDU,Tmp2,Tmp2,Op1); I3(SUBU,Result,Tmp2,Tmp1); break;
	case -14: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,1); I3(SUBU,Result,Tmp2,Tmp1); break; 
	case -15: I2C(SLL,Tmp1,Op1,4); I3(SUBU,Result,Op1,Tmp1); break;
	case -19: I2C(SLL,Tmp1,Op1,4); I2C(SLL,Tmp2,Op1,2); I3(SUBU,Result,Op1,Tmp1); I3(SUBU,Result,Result,Tmp2); break;

	default: 
		Result = IMul(Op1,-Mul,Tmp1,Tmp2,Tmp3,NULL);
		if (Neg)
			 *Neg = 1;
		else
			I3(SUBU,Result,ZERO,Result); 
		break;
	}

	return Result;
}

#endif
