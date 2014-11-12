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
 * $Id: dyncode_sh3.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __DYNCODE_SH3_H
#define __DYNCODE_SH3_H

#ifdef SH3

#define DYNCODE

//R0				result
//R4-R7				a0-a3 argument
//R15				sp stack pointer

#define STACKFRAME	10 

#define R0	0
#define R1	1
#define R2	2
#define R3	3
#define R4	4
#define R5	5
#define R6	6
#define R7	7
#define R8	8
#define R9	9
#define R10	10
#define R11	11
#define R12	12
#define R13	13
#define R14	14
#define R15	15
#define	MACL 16
#define MACH 17
#define PR	18
#define PC	19
#define GBR 20
#define SP 15

#define MOVI		0x0100E000
#define MOVW_PC		0x01019000
#define MOVL_PC		0x0102D000
#define MOV			0x02136003
#define MOVB_ST		0x00332000 
#define MOVW_ST		0x00332001
#define MOVL_ST		0x00332002
#define MOVB_LD		0x02136000
#define MOVW_LD		0x02136001
#define MOVL_LD		0x02136002
#define MOVB_STSUB	0x02332004
#define MOVW_STSUB	0x02332005
#define MOVL_STSUB	0x02332006
#define MOVB_LDADD	0x03136004
#define MOVW_LDADD	0x03136005
#define MOVL_LDADD	0x03136006
#define MOVB_STOFS	0x00548000 /*R0*/
#define MOVW_STOFS	0x00558100 /*R0*/
#define MOVL_STOFS	0x00361000
#define MOVB_LDOFS	0x04148400 /*R0*/
#define MOVW_LDOFS	0x04158500 /*R0*/
#define MOVL_LDOFS	0x02165000
#define MOVB_STR0	0x00730004
#define MOVW_STR0	0x00730005
#define MOVL_STR0	0x00730006
#define MOVB_LDR0	0x0253000C
#define MOVW_LDR0	0x0253000D
#define MOVL_LDR0	0x0253000E
#define MOVT		0x01870029
#define SWAPB		0x02136008
#define SWAPW		0x02136009
#define XTRCT		0x0233200D
#define ADD			0x0233300C
#define ADDI		0x01107000
#define ADDC		0x0AB3300E
#define ADDV		0x0A33300F
#define CMPEQI		0x08488800 /*R0*/
#define CMPEQ		0x08333000
#define CMPHS		0x08333002
#define CMPGE		0x08333003
#define CMPHI		0x08333006
#define CMPGT		0x08333007
#define CMPPZ		0x08174011
#define CMPPL		0x08174015
#define CMPSTR		0x0833200C
#define DMULSL		0x4033300D 
#define DMULUL		0x40333005
#define DT			0x09174010
#define EXTSB		0x0213600E
#define EXTSW		0x0213600F 
#define EXTUB		0x0213600C
#define EXTUW		0x0213600D
#define MAC_L		0x5333000F
#define MAC_W		0x5333400F
#define MULL		0x40330007
#define MULSW		0x4033200F
#define MULUW		0x4033200E
#define NEG			0x0213600B
#define NEGC		0x0A93600A
#define SUB			0x02333008
#define SUBC		0x0AB3300A
#define SUBV		0x0A33300B
#define AND			0x02332009
#define ANDI		0x0448C900 /*R0*/
#define NOT			0x02136007
#define OR			0x0233200B
#define ORI			0x0448CB00 /*R0*/
#define TST			0x08332008
#define TSTI		0x0848C800 /*R0*/
#define XOR			0x0233200A
#define XORI		0x0448CA00 /*R0*/
#define ROTL		0x09174004
#define ROTR		0x09174005
#define ROTCL		0x09974024
#define ROTCR		0x09974025
#define SHAD		0x0233400C
#define SHAL		0x09174020
#define SHAR		0x09174021
#define SHLD		0x0233400D
#define SHLL		0x09174000
#define SHLR		0x09174001
#define SHLL2		0x01174008
#define SHLR2		0x01174009
#define SHLL8		0x01174018
#define SHLR8		0x01174019
#define SHLL16		0x01174028
#define SHLR16		0x01174029

#define BF			0x008A8B00
#define BFS			0x008A8F00
#define BT			0x008A8900
#define BTS			0x008A8D00
#define BRA			0x008BA000
#define BRAF		0x008C0023
#define BSR			0x808BB000
#define BSRF		0x808C0003
#define JMP			0x000C402B
#define JSR			0x800C400B
#define RTS			0x200D000B

#define CLRMAC		0x40090028
#define CLRT		0x08090048
#define PREF		0x00070083
#define SETT		0x08090018

#define LDS_MACH		0x4017400A
#define LDS_MACL		0x4017401A
#define LDS_PR			0x8017402A
#define LDS_LDADD_MACH	0x41174006
#define LDS_LDADD_MACL	0x41174016
#define LDS_LDADD_PR	0x81174026

#define STS_MACH		0x1107000A
#define STS_MACL		0x1107001A
#define STS_PR			0x2107002A
#define STS_LDADD_MACH	0x11174002
#define STS_LDADD_MACL	0x11174012
#define STS_LDADD_PR	0x21174022

void IShift(reg Rn, reg Tmp, int Left);
void I0(int Code);
void I0C(int Code, int Const);
void I1(int Code, reg Rn);
void I1C(int Code, reg Rn, int Const);
void I2(int Code, reg Rm, reg Rn);
void I2C(int Code, reg Rm, reg Rn, int Const);
void I0P(int Code, dyninst* Block);
void I1P(int Code, reg Reg, dyninst* Block, int Ofs);
dyninst* ICode(dyninst* Block, int Ofs);

void CodeBegin(int Save,int Local);
void CodeEnd(int Save,int Local);

void NOP();
void Break();

#endif
#endif
