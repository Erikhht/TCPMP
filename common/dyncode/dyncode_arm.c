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
 * $Id: dyncode_arm.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "dyncode.h"

#if defined(ARM) && defined(CONFIG_DYNCODE)

void InstInit()
{
	context* c = Context();
	c->NextCond = AL;
	c->NextSet = 0;
	c->NextByte = 0;
	c->NextHalf = 0;
	c->NextSign = 0;
}

void InstPost(dyninst* p)
{
	context* c = Context();
	InstAdd(p,c);

	c->NextCond = AL;
	c->NextSet = 0;
	c->NextByte = 0;
	c->NextHalf = 0;
	c->NextSign = 0;
}

void Byte()
{
	context* c = Context();
	c->NextByte = 1;
	c->NextSign = 0;
}

void Half()
{
	context* c = Context();
	c->NextHalf = 1;
	c->NextSign = 0;
}

void SByte()
{
	context* c = Context();
	c->NextByte = 1;
	c->NextSign = 1;
}

void SHalf()
{
	context* c = Context();
	c->NextHalf = 1;
	c->NextSign = 1;
}

void C(int i)
{
	context* c = Context();
	c->NextCond = i;
}

void S()
{
	context* c = Context();
	c->NextSet = 1;
}

void IPLD(int* Code, reg* Dest)
{
	context* c = Context();
	switch (*Code)
	{
	case PLD: c->NextCond=15; c->NextByte=1; *Code = LDR; *Dest=PC; break;
	case PLD_PRE: c->NextCond=15; c->NextByte=1; *Code = LDR_PRE; *Dest=PC; break;
	case PLD_POST: c->NextCond=15; c->NextByte=1; *Code = LDR_POST; *Dest=PC; break;
	case PLD_PRESUB: c->NextCond=15; c->NextByte=1; *Code = LDR_PRESUB; *Dest=PC; break;
	case PLD_POSTSUB: c->NextCond=15; c->NextByte=1; *Code = LDR_POSTSUB; *Dest=PC; break;
	}
}

#define MODE(x) ((uint32_t)(x) >> 28)
#define MODEMASK ~0xF0000000

void I3C(int Code, reg Dest, reg Op1, reg Op2,int Const)
{
	context* c = Context();
	dyninst* p = NULL;

	if (MODE(Code) == 8 && Const>=0 && Const<8)
	{
		p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 12) | ((Op1 & 15) << 16) | ((Op2 & 15) << 0) | (Const << 20),
				Dest, Op1, Op2, (c->NextCond != AL)?1:0, 0);
	}

	InstPost(p);
}

void I3(int Code, reg Dest, reg Op1, reg Op2)
{
	context* c = Context();
	if (MODE(Code) == 4)
	{
		dyninst* p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 0) | ((Op1 & 15) << 12) | ((Op2 & 15) << 16),
				Dest, Op1, Op2, (c->NextCond != AL)?1:0, 0);
		InstPost(p);
	}
	else
	if (MODE(Code) == 7)
	{
		dyninst* p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 12) | ((Op1 & 15) << 16) | ((Op2 & 15) << 0),
				Dest, Op1, Op2, (c->NextCond != AL)?1:0, 0);
		InstPost(p);
	}
	else
		I3S(Code,Dest,Op1,Op2,LSL,0);
}

void I3S(int Code, reg Dest, reg Op1, reg Op2, int ShiftType, int Shift)
{
	context* c = Context();
	dyninst* p = NULL;

	if (Shift == 0)
		ShiftType = LSL;

	if (ShiftType == LSL && Shift < 0)
	{
		ShiftType = LSR;
		Shift = -Shift;
	}
	if ((ShiftType == LSR || ShiftType == ASR) && Shift < 0)
	{
		ShiftType = LSL;
		Shift = -Shift;
	}
	if (ShiftType == ROR && Shift < 0)
		Shift = Shift & 31;

	if (Shift >= 0 && Shift < 32)
	{
		if (Code >= 0 && Code < 16)
		{
			if (Code == CMP || Code == TST || Code == CMN || Code == TEQ) 
				c->NextSet = 1;

			p = InstCreate32((c->NextCond << 28) | (Code << 21) | ((c->NextSet?1:0)<<20) |
				((Op1==NONE?R0:Op1) << 16) | ((Dest==NONE?R0:Dest) << 12) | (Shift << 7) | (ShiftType << 5) | (Op2),
				Dest, Op1, Op2, (c->NextCond != AL)?1:0, c->NextSet?1:0);
		}
		if (Shift == 0)
		{
			if (Code == MUL && Dest != Op1)
			{
				p = InstCreate32((c->NextCond << 28) | ((c->NextSet?1:0)<<20) |
					(Dest << 16) | (Op2 << 8) | 0x90 | (Op1),
					Dest, Op1, Op2, (c->NextCond != AL)?1:0, c->NextSet?1:0);
			}
			if (Code >= QADD && Code <= QDSUB)
			{
				p = InstCreate32((c->NextCond << 28) | 
					(Dest << 12) | (Op1) | (Op2 << 16) | (0x5 << 4) | (1<<24) | ((Code-QADD)<<21),
					Dest, Op1, Op2, (c->NextCond != AL)?1:0, c->NextSet?1:0);
			}
		}

		IPLD(&Code,&Dest);

		if (Code == LDR || Code == STR || 
			Code == LDR_PRE || Code == STR_PRE || 
			Code == LDR_POST|| Code == STR_POST ||
			Code == LDR_PRESUB || Code == STR_PRESUB || 
			Code == LDR_POSTSUB || Code == STR_POSTSUB)
		{
			bool_t Pre = (Code != LDR_POST) && (Code != STR_POST) && (Code != LDR_POSTSUB) && (Code != STR_POSTSUB);
			bool_t PreWrite = (Code == LDR_PRE) || (Code == STR_PRE) || (Code == LDR_PRESUB) || (Code == STR_PRESUB);
			bool_t Load = (Code == LDR) || (Code == LDR_PRE) || (Code == LDR_POST) || (Code == LDR_PRESUB) || (Code == LDR_POSTSUB);
			bool_t Unsigned = (Code != LDR_PRESUB) && (Code != STR_PRESUB) && (Code != LDR_POSTSUB) && (Code != STR_POSTSUB);

			if (c->NextHalf || c->NextSign)
			{
				if (Shift==0 && (c->NextHalf || c->NextByte))
					p = InstCreate32((c->NextCond << 28) | 
						((Pre?1:0)<<24) | (Unsigned<<23) | 
						((PreWrite?1:0)<<21) | (Load<<20) | (c->NextSign << 6) | (c->NextHalf << 5) |
						(Op1 << 16) | (Dest << 12) | (9 << 4) | Op2,
						NONE, Op1, Op2, (c->NextCond != AL)?1:0, 0);
			}
			else
				p = InstCreate32((c->NextCond << 28) | (3 << 25) |
					((Pre?1:0)<<24) | (Unsigned<<23) | (c->NextByte<<22) |
					((PreWrite?1:0)<<21) | (Load<<20) |
					(Op1 << 16) | (Dest << 12) | (Shift << 7) | (ShiftType << 5) | Op2,
					NONE, Op1, Op2, (c->NextCond != AL)?1:0, 0);

			if (p)
			{
				if (Load)
					p->WrRegs |= 1 << Dest;
				else
					p->RdRegs |= 1 << Dest;

				if (!Pre || PreWrite)
					p->WrRegs |= 1 << Op1;
			}
		}
	}
	InstPost(p);
}

void I4(int Code, reg Dest, reg Op1, reg Op2, reg Op3)
{
	context* c = Context();
	dyninst* p = NULL;
	if (Code == MLA)
	{
		p = InstCreate32((c->NextCond << 28) | (1 << 21) | ((c->NextSet?1:0)<<20) |
			(Dest << 16) | (Op3 << 12) | (Op2 << 8) | 0x90 | (Op1),
			Dest, Op1, Op2, (c->NextCond != AL)?1:0, c->NextSet?1:0);

		if (p) p->RdRegs |= 1 << Op3;

	}
	InstPost(p);
}

void IMul(reg Dest, reg Op1, int Mul)
{
	if (Dest == Op1)
		InstPost(NULL); //assert

	switch (Mul)
	{
	case 0: I2C(MOV,Dest,NONE,0); break;
	case 1: I3(MOV,Dest,NONE,Op1); break;
	case 2: I3S(MOV,Dest,NONE,Op1,LSL,1); break;
	case 3: I3S(ADD,Dest,Op1,Op1,LSL,1); break;
	case 4: I3S(MOV,Dest,NONE,Op1,LSL,2); break;
	case 5: I3S(ADD,Dest,Op1,Op1,LSL,2); break;
	case 6: I3S(MOV,Dest,NONE,Op1,LSL,1); I3S(ADD,Dest,Dest,Dest,LSL,1); break; //2*3
	case 7: I3S(RSB,Dest,Op1,Op1,LSL,3); break;
	case 8: I3S(MOV,Dest,NONE,Op1,LSL,3); break;
	case 9: I3S(ADD,Dest,Op1,Op1,LSL,3); break;
	case 10: I3S(MOV,Dest,NONE,Op1,LSL,1); I3S(ADD,Dest,Dest,Dest,LSL,2); break; //2*5
	case 11: I3S(RSB,Dest,Op1,Op1,LSL,3); I3S(ADD,Dest,Dest,Op1,LSL,2); break;  //7+4
	case 12: I3S(ADD,Dest,Op1,Op1,LSL,1); I3S(MOV,Dest,NONE,Dest,LSL,2); break; //3*4
	case 13: I3S(ADD,Dest,Op1,Op1,LSL,3); I3S(ADD,Dest,Dest,Op1,LSL,2); break;  //9+4
	case 14: I3S(RSB,Dest,Op1,Op1,LSL,3); I3(ADD,Dest,Dest,Dest); break;  //7*2
	case 15: I3S(RSB,Dest,Op1,Op1,LSL,4); break;
	case 16: I3S(MOV,Dest,NONE,Op1,LSL,4); break;
	case 17: I3S(ADD,Dest,Op1,Op1,LSL,4); break;
	default: I2C(MOV,Dest,NONE,Mul); I3(MUL,Dest,Op1,Dest); break;
	}
}

void IConst(reg Dest, int Const)
{
	context* c = Context();
	dyninst* p = NULL;
	int Shift,Code;

	if (Const < 0)
	{
		Code = MVN;
		Const = -Const-1;
	}
	else
		Code = MOV;

	Const = (Const << 8) | ((Const >> 24) & 255);
	for (Shift=8;Shift<=32;Shift+=2)
	{
		if (Const & 0xC0)
			break;
		Const = (Const << 2) | ((Const >> 30) & 3);
	}
	Shift &= 31;
	Const &= 255;

	p = InstCreate32((c->NextCond << 28) | (1<<25) | (Code << 21) | ((c->NextSet?1:0)<<20) |
		(Dest << 12) | (Shift << 7) | Const,
		Dest, NONE, NONE, (c->NextCond != AL)?1:0, c->NextSet?1:0);

	InstPost(p);
}

void I2(int Code, reg Dest, reg Op1)
{
	context* c = Context();
	if (MODE(Code) == 1)
	{
		dyninst* p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 16) | ((Op1 & 15) << 12),
				Dest, Op1, NONE, (c->NextCond != AL)?1:0, 0);
		InstPost(p);
	}
	else
	if (MODE(Code) == 6)
	{
		dyninst* p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 12) | ((Op1 & 15) << 16),
				Dest, Op1, NONE, (c->NextCond != AL)?1:0, 0);
		InstPost(p);
	}
	else
		I2C(Code,Dest,Op1,0);
}

void I2C(int Code, reg Dest, reg Op1, int Const)
{
	int Shift;
	context* c = Context();
	dyninst* p = NULL;

	if (MODE(Code) == 2 && Const>=0 && Const<8)
	{
		p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 16) | ((Op1 & 15) << 12) | (Const << 0),
				Dest, Op1, NONE, (c->NextCond != AL)?1:0, 0);
	}

	if (MODE(Code) == 9 && Const>=0 && Const<256)
	{
		p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 12) | ((Op1 & 15) << 16) | ((Const & 15) << 0) | ((Const & 0xF0) << 16),
				Dest, Op1, NONE, (c->NextCond != AL)?1:0, 0);
	}

	if (MODE(Code) == 15)
	{
		if (Const<0)
		{
			Code ^= (1<<23);
			Const = -Const;
		}
		if (Code & 256) // dword or qword
		{
			if (Const & 3)
				Const = -1;
			else
				Const >>= 2;
		}
		if (Const>=0 && Const<256)
		{
			p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | ((Dest & 15) << 12) | ((Op1 & 15) << 16) | (Const << 0),
					NONE, Op1, NONE, (c->NextCond != AL)?1:0, 0);

			if (Code & (1<<20))
				p->WrRegs |= 1 << Dest;
			else
				p->RdRegs |= 1 << Dest;

			if (!(Code & (1<<24)) || (Code & (1<<21)))
				p->WrRegs |= 1 << Op1;
		}
	}

	if (Code >= 0 && Code < 16)
	{
		if (Code == MOV && Const < 0)
		{
			Code = MVN;
			Const = -Const-1;
		}
		else
		if (Code == ADD && Const < 0)
		{
			Code = SUB;
			Const = -Const;
		}
		else
		if (Code == SUB && Const < 0)
		{
			Code = ADD;
			Const = -Const;
		}

		if (Code == CMP || Code == TST || Code == CMN || Code == TEQ) 
			c->NextSet = 1;

		for (Shift = 0;Shift<32;Shift+=2)
		{
			if (Const >= 0 && Const <= 255)
				break;
			Const = (Const << 2) | ((Const >> 30) & 3);
		}
		if (Const >= 0 && Const <= 255)
			p = InstCreate32((c->NextCond << 28) | (1<<25) | (Code << 21) | ((c->NextSet?1:0)<<20) |
				((Op1==NONE?R0:Op1) << 16) | ((Dest==NONE?R0:Dest) << 12) | (Shift << 7) | Const,
				Dest, Op1, NONE, (c->NextCond != AL)?1:0, c->NextSet?1:0);
	}
	
	IPLD(&Code,&Dest);

	if (Code == LDR || Code == STR || 
		Code == LDR_PRE || Code == STR_PRE || 
		Code == LDR_POST|| Code == STR_POST ||
		Code == LDR_PRESUB || Code == STR_PRESUB || 
		Code == LDR_POSTSUB || Code == STR_POSTSUB)
	{
		bool_t Pre = (Code != LDR_POST) && (Code != STR_POST) && (Code != LDR_POSTSUB) && (Code != STR_POSTSUB);
		bool_t PreWrite = (Code == LDR_PRE) || (Code == STR_PRE) || (Code == LDR_PRESUB) || (Code == STR_PRESUB);
		bool_t Load = (Code == LDR) || (Code == LDR_PRE) || (Code == LDR_POST) || (Code == LDR_PRESUB) || (Code == LDR_POSTSUB);
		bool_t Unsigned = (Code != LDR_PRESUB) && (Code != STR_PRESUB) && (Code != LDR_POSTSUB) && (Code != STR_POSTSUB);

		if (Const == 0)
		{
			Pre = 1;
			PreWrite = 0;
		}
		if (Const < 0)
		{
			Const = -Const;
			Unsigned = !Unsigned;
		}

		if (c->NextHalf || c->NextSign)
		{
			if (Const >= 0 && Const < 256 && (c->NextHalf || c->NextByte))
				p = InstCreate32((c->NextCond << 28) | 
					((Pre?1:0)<<24) | (Unsigned<<23) | (1 << 22) |
					((PreWrite?1:0)<<21) | (Load<<20) | (c->NextSign << 6) | (c->NextHalf << 5) |
					(Op1 << 16) | (Dest << 12) | ((Const >> 4) << 8) | (9 << 4) | (Const & 15),
					NONE, Op1, NONE, (c->NextCond != AL)?1:0, 0);
		}
		else
			if (Const >= 0 && Const < 4096)
				p = InstCreate32((c->NextCond << 28) | (1 << 26) |
					((Pre?1:0)<<24) | (Unsigned<<23) | (c->NextByte<<22) |
					((PreWrite?1:0)<<21) | (Load<<20) |
					(Op1 << 16) | (Dest << 12) | Const,
					NONE, Op1, NONE, (c->NextCond != AL)?1:0, 0);

		if (p)
		{
			if (Load)
				p->WrRegs |= 1 << Dest;
			else
				p->RdRegs |= 1 << Dest;

			if (!Pre || PreWrite)
				p->WrRegs |= 1 << Op1;
		}
	}

	InstPost(p);
}


void I1P(int Code, reg Dest, dyninst* Block, int Ofs)
{
	context* c = Context();
	dyninst* p = NULL;

	if (MODE(Code)==15)
	{
		p = InstCreate32((c->NextCond << 28) | (Code & MODEMASK) | (PC << 16) | ((Dest & 15) << 12),
			Dest, PC, NONE, (c->NextCond != AL)?1:0, 0);

		if (p)
		{
			p->Tag = Ofs;
			p->ReAlloc = Block;
		}
	}

	if (Code == LDR || Code == STR)
	{
		int Load = (Code == LDR);

		p = InstCreate32((c->NextCond << 28) | (1 << 26) |
			(1<<24) | (c->NextByte<<22) | (Load<<20) |
			(PC << 16) | (Dest << 12),
			Dest, PC, NONE, (c->NextCond != AL)?1:0, 0);

		if (p)
		{
			p->Tag = Ofs;
			p->ReAlloc = Block;
		}
	}
	else
	if (Code == MOV) // ADD|SUB,Dst,R15,Ofs
	{
		p = InstCreate32((c->NextCond << 28) |
			(1<<25) | (PC << 16) | (Dest << 12),
			Dest, PC, NONE, (c->NextCond != AL)?1:0, 0);

		if (p)
		{
			p->Tag = Ofs;
			p->ReAlloc = Block;
		}
	}
	InstPost(p);
}

void I0P(int Code, int Cond, dyninst* Target)
{
	dyninst* p = NULL;
	if (Code == B || Code == BL)
	{
		p = InstCreate32((Cond << 28) | (5 << 25) | ((Code == BL?1:0)<<24),
			PC, NONE, NONE, (Cond != AL)?1:0, 0);
		if (p)
		{	
			if (Code == BL)
				p->WrRegs |= 1 << LR;
			p->ReAlloc = Target;
			p->Branch = 1;
		}
	}
	InstPost(p);
}

void Break()
{
	context* c = Context();
	dyninst* p = InstCreate32((c->NextCond << 28) | (15 << 24),NONE,NONE,NONE,0,0);
	if (p)
		p->Branch = 1;
	InstPost(p);
}

void CodeBegin()
{
	int i;
	dyninst* p = InstCreate32(0xE92D5FF0,SP,NONE,NONE,0,0);
	if (p)
		for (i=4;i<16;++i)
			p->RdRegs |= 1 << i;
	InstPost(p);
}

void CodeEnd()
{
	int i;
	dyninst* p = InstCreate32(0xE8BD9FF0,SP,NONE,NONE,0,0);
	if (p)
		for (i=4;i<16;++i)
			p->WrRegs |= 1 << i;
	InstPost(p);
}

bool_t InstReAlloc(dyninst* p,dyninst* ReAlloc)
{
	int Diff = ReAlloc->Address - (p->Address+8);
	int* Code = (int*) InstCode(p);

	if (((*Code >> 25) & 7) == 6) //wldr,wstr
	{
		int Ofs = Diff + p->Tag;
		int OfsUnsigned = 1;
		if (Ofs < 0)
		{
			Ofs = -Ofs;
			OfsUnsigned = 0;
		}
		if (*Code & 256)
		{
			if (Ofs & 3)
				Ofs = 256;
			else
				Ofs >>= 2;
		}
		if (Ofs < 256)
		{
			*Code &= ~(1<<23);
			*Code |= OfsUnsigned<<23;
			*Code &= ~255;
			*Code |= Ofs;
			return 1;
		}

		DEBUG_MSG1(-1,T("Realloc failed for wldr,wstr %d"),Ofs);
	}
	else
	if (((*Code >> 25) & 7) == 5) //branch
	{
		*Code &= 0xFF000000;
		*Code |= (Diff >> 2) & ~0xFF000000;
		return 1;
	}
	else
	if (((*Code >> 25) & 7) == 1) //add dest,pc,#const
	{
		int Shift;
		int Ofs = Diff + p->Tag;

		*Code &= ~((15 << 21)|4095);

		if (Ofs < 0)
		{
			Ofs = -Ofs;
			*Code |= (SUB << 21);
		}
		else
			*Code |= (ADD << 21);

		for (Shift = 0;Shift<32;Shift+=2)
		{
			if (Ofs >= 0 && Ofs <= 255)
				break;
			Ofs = (Ofs << 2) | ((Ofs >> 30) & 3);
		}

		if (Ofs >= 0 && Ofs <= 255)
		{
			*Code |= (Shift << 7) | Ofs;
			return 1;
		}

		DEBUG_MSG1(-1,T("Realloc failed for add dest,pc,#const %d"),Ofs);
	}
	else
	if (((*Code >> 25) & 7) == 2) //ldr,str
	{
		int Ofs = Diff + p->Tag;
		int OfsUnsigned = 1;
		if (Ofs < 0)
		{
			Ofs = -Ofs;
			OfsUnsigned = 0;
		}
		if (Ofs < 4096)
		{
			*Code &= ~(1<<23);
			*Code |= OfsUnsigned<<23;
			*Code &= ~4095;
			*Code |= Ofs;
			return 1;
		}

		DEBUG_MSG1(-1,T("Realloc failed for ldr,str %d"),Ofs);
	}
	return 0;
}

#endif
