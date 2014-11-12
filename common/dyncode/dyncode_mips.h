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
 * $Id: dyncode_mips.h 341 2005-11-16 00:15:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __DYNCODE_MIPS_H
#define __DYNCODE_MIPS_H

#ifdef MIPS

#define DYNCODE

//R0				zero
//R1				at
//R2-R3				v0-v1 result
//R4-R7				a0-a3 argument
//R8-R15,R24,R25	t0-t9 temporary (not preserved)
//R16-R23,R30		s0-s8 saved 
//R26-R27			k0-k1 kernel
//R28				gp global pointer
//R29				sp stack pointer
//R31				ra return address

#define	ZERO	0
#define	R1		1
#define	R2		2
#define	R3		3
#define	R4		4
#define	R5		5
#define	R6		6
#define	R7		7
#define	R8		8
#define	R9		9
#define	R10		10
#define	R11		11
#define	R12		12
#define	R13		13
#define	R14		14
#define	R15		15
#define	R16		16
#define	R17		17
#define	R18		18
#define	R19		19
#define	R20		20
#define	R21		21
#define	R22		22
#define	R23		23
#define	R24		24
#define	R25		25
#define	K0		26
#define	K1		27
#define	R28		28
#define	SP		29
#define	R30		30
#define	R31		31

#define	GP		28
#define	RA		31

#define	ADDIU	0x24000200
#define	ADDU	0x00000121
#define	AND 	0x00000124
#define	ANDI 	0x30000200
#define	BEQ		0x10000000 
#define	BEQL	0x50000000
#define	BGEZ	0x04010300
#define	BGEZAL	0x04110300
#define	BGEZALL 0x04130300
#define	BGEZL	0x04030300
#define	BGTZ	0x1C000300
#define	BGTZL	0x5C000300
#define	BLEZ	0x18000300
#define	BLEZL	0x58000300
#define	BLTZ	0x04000300
#define	BLTZAL	0x04100300
#define	BLTZALL 0x04120300
#define	BLTZL   0x04020300
#define	BNE		0x14000000
#define	BNEL	0x54000000
#define	DADDIU  0x64000200
#define	DADDU   0x0000012D
#define	DDIV	0x0000041E
#define	DDIVU	0x0000041F
#define	DIV		0x0000041A
#define	DIVU	0x0000041B
#define	DMULT	0x0000041C
#define	DMULTU	0x0000041D
#define	DSLL	0x00000538
#define	DSLL32	0x0000053C
#define	DSLLV	0x00000614
#define	DSRA	0x0000053B
#define	DSRA32	0x0000053F
#define	DSRAV	0x00000617
#define	DSRL	0x0000053A
#define	DSRL32	0x0000053E
#define	DSRLV	0x00000616
#define	DSUBU	0x0000012F
#define	JALR	0x00000809
#define	JR		0x00000908
#define	LB		0x80000200
#define	LBU		0x90000200
#define	LD		0xDC000200
#define	LDL		0x68000200
#define	LDR		0x6C000200
#define	LH		0x84000200
#define	LHU		0x94000200
#define	LUI		0x3C000A00
#define	LW		0x8C000200
#define	LWL		0x88000200
#define	LWR		0x98000200
#define	LWU		0x9C000200
#define	MFHI	0x00000B10
#define	MFLO	0x00000B12
#define	MTHI	0x00000B11
#define	MTLO	0x00000B13
#define	MULT	0x00000418
#define	MULTU	0x00000419
#define	NOR		0x00000127
#define	OR		0x00000125
#define	ORI		0x34000200
#define	SB		0xA0000C00
#define	SD		0xFC000C00
#define	SDL		0xB0000C00
#define	SDR		0xB4000C00
#define	SH		0xA4000C00
#define	SLL		0x00000500
#define	SLLV	0x00000604
#define	SLT		0x0000012A
#define	SLTI	0x28000200
#define	SLTIU	0x2C000200
#define	SLTU	0x0000012B
#define	SRA		0x00000503
#define	SRAV	0x00000607
#define	SRL		0x00000502
#define	SRLV	0x00000606
#define	SUBU	0x00000123
#define	SW		0xAC000C00
#define	SWL		0xA8000C00
#define	SWR		0xB8000C00
#define	XOR		0x00000126
#define	XORI	0x38000200

void I1(int, reg Reg);
void I1C(int, reg Reg, int Const);
void I2(int, reg Dest, reg Op1);
void I2C(int, reg Dest, reg Op1, int Const);
void I3(int, reg Dest, reg Op1, reg Op2);
void I1P(int, reg Reg, dyninst* Block, int Ofs);
void I2P(int, reg Op1, reg Op2, dyninst*);
void IConst(reg, int Value);
reg IMul(reg Op1, int Mul, reg Tmp1, reg Tmp2, reg Tmp3, bool_t* Neg);

void NOP();
void Break();

void CodeBegin(int Save,int Local,bool_t Align64);
void CodeEnd(int Save,int Local,bool_t Align64);

#endif
#endif
