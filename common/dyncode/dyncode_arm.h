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
 * $Id: dyncode_arm.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __DYNCODE_ARM_H
#define __DYNCODE_ARM_H

#ifdef ARM

#define DYNCODE

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
#define SP	13
#define R14	14
#define PC	15
#define LR	14

#define EQ	0
#define	NE	1
#define CS	2
#define CC	3
#define MI	4
#define PL	5
#define VS	6
#define VC	7
#define HI	8
#define LS	9
#define GE	10
#define LT	11
#define GT	12
#define LE	13
#define AL	14

#define LSL	0
#define LSR 1
#define ASR	2
#define ROR 3

#define AND				0
#define EOR				1
#define SUB				2
#define RSB				3
#define ADD				4
#define ADC				5
#define SBC				6
#define RSC				7
#define TST				8
#define TEQ				9
#define CMP				10
#define CMN				11
#define ORR				12
#define MOV				13
#define BIC				14
#define MVN				15
#define MUL				16
#define MLA				17
#define QADD			18
#define QSUB			19
#define QDADD			20
#define QDSUB			21
#define LDR				22
#define STR				23
#define LDR_POST		24
#define STR_POST		25
#define LDR_PRE			26
#define STR_PRE			27
#define LDR_POSTSUB		28
#define STR_POSTSUB		29
#define LDR_PRESUB		30
#define STR_PRESUB		31
#define B				32
#define BL				33
#define PLD				34
#define PLD_POST		35
#define PLD_PRE			36
#define PLD_POSTSUB		37
#define PLD_PRESUB		38

#define WR0		(0+16)
#define WR1		(1+16)
#define WR2		(2+16)
#define WR3		(3+16)
#define WR4		(4+16)
#define WR5		(5+16)
#define WR6		(6+16)
#define WR7		(7+16)
#define WR8		(8+16)
#define WR9		(9+16)
#define WR10	(10+16)
#define WR11	(11+16)
#define WR12	(12+16)
#define WR13	(13+16)
#define WR14	(14+16)
#define WR15	(15+16)

#define WCID	(0+32)
#define WCSSF	(2+32)
#define WCASF	(3+32)
#define WCGR0	(8+32)
#define WCGR1	(9+32)
#define WCGR2	(10+32)
#define WCGR3	(11+32)

//mode
//1: I2 rd:16,rn:12
//2: I2C rd:16,rn:12,imm[3]:0
//4: I3 rd:0,rnlo:12,rnhi:16
//6: I2 rd:12,rn:16
//7: I3 rd:12,rn:16,rm:0
//8: I3C rd:12,rn:16,rm:0,imm[3]:20
//9: I2C rd:12,rn:16,imm[8]:0|20
//F: I2C,I1P rd:12,rn:16,imm[8]:0 (w or q *4)

#define TBCSTB	0x1E400010
#define TBCSTH	0x1E400050
#define TBCSTW	0x1E400090
#define TINSRB	0x2E600010
#define TINSRH	0x2E600050
#define TINSRW	0x2E600090
#define TMCR	0x1E000110
#define TMCRR	0x4C400000
#define TMRC	0x6E100110
#define TMRRC	0x7C500000
#define WACCB	0x6E0001C0
#define WACCH	0x6E4001C0
#define WACCW	0x6E8001C0
#define WADDB  	0x7E000180
#define WADDH  	0x7E400180
#define WADDW  	0x7E800180
#define WADDBUS	0x7E100180
#define WADDHUS	0x7E500180
#define WADDWUS	0x7E900180
#define WADDBSS	0x7E300180
#define WADDHSS	0x7E700180
#define WADDWSS	0x7EB00180
#define WALIGNI	0x8E000020
#define WALIGNR0 0x7E800020
#define WALIGNR1 0x7E900020
#define WALIGNR2 0x7EA00020
#define WALIGNR3 0x7EB00020
#define WAND	0x7E200000
#define WANDN	0x7E300000
#define WAVG2B	0x7E800000
#define WAVG2H	0x7EC00000
#define WAVG2BR	0x7E900000
#define WAVG2HR	0x7ED00000
#define WCMPEQB	0x7E000060
#define WCMPEQH	0x7E400060
#define WCMPEQW	0x7E800060
#define WCMPGTUB 0x7E100060
#define WCMPGTUH 0x7E500060
#define WCMPGTUW 0x7E900060
#define WCMPGTSB 0x7E300060
#define WCMPGTSH 0x7E700060
#define WCMPGTSW 0x7EB00060
#define WLDRB	0xFD900000
#define WLDRH	0xFDD00000
#define WLDRW	0xFD900100 
#define WLDRD   0xFDD00100
#define WSTRB	0xFD800000
#define WSTRH	0xFDC00000
#define WSTRW	0xFD800100 
#define WSTRD	0xFDC00100
#define WLDRB_PRE 0xFDB00000
#define WLDRH_PRE 0xFDF00000
#define WLDRW_PRE 0xFDB00100 
#define WLDRD_PRE 0xFDF00100
#define WSTRB_PRE 0xFDA00000
#define WSTRH_PRE 0xFDE00000
#define WSTRW_PRE 0xFDA00100 
#define WSTRD_PRE 0xFDE00100
#define WLDRB_PRESUB 0xFD300000
#define WLDRH_PRESUB 0xFD700000
#define WLDRW_PRESUB 0xFD300100 
#define WLDRD_PRESUB 0xFD700100
#define WSTRB_PRESUB 0xFD200000
#define WSTRH_PRESUB 0xFD600000
#define WSTRW_PRESUB 0xFD200100 
#define WSTRD_PRESUB 0xFD600100
#define WLDRB_POST 0xFCB00000
#define WLDRH_POST 0xFCF00000
#define WLDRW_POST 0xFCB00100 
#define WLDRD_POST 0xFCF00100
#define WSTRB_POST 0xFCA00000
#define WSTRH_POST 0xFCE00000
#define WSTRW_POST 0xFCA00100 
#define WSTRD_POST 0xFCE00100
#define WLDRB_POSTSUB 0xFC300000
#define WLDRH_POSTSUB 0xFC700000
#define WLDRW_POSTSUB 0xFC300100 
#define WLDRD_POSTSUB 0xFC700100
#define WSTRB_POSTSUB 0xFC200000
#define WSTRH_POSTSUB 0xFC600000
#define WSTRW_POSTSUB 0xFC200100 
#define WSTRD_POSTSUB 0xFC600100
#define WMACU	0x7E400100
#define WMACS	0x7E600100
#define WMACUZ	0x7E500100
#define WMACSZ	0x7E700100
#define WMADDU	0x7E800100
#define WMADDS	0x7EA00100
#define WMAXUB	0x7E000160
#define WMAXUH	0x7E400160
#define WMAXUW	0x7E800160
#define WMAXSB	0x7E200160
#define WMAXSH	0x7E600160
#define WMAXSW	0x7EA00160
#define WMINUB	0x7E100160
#define WMINUH	0x7E500160
#define WMINUW	0x7E900160
#define WMINSB	0x7E300160
#define WMINSH	0x7E700160
#define WMINSW	0x7EB00160
#define WMULUL	0x7E000100
#define WMULUM	0x7E100100
#define WMULSL	0x7E200100
#define WMULSM	0x7E300100
#define WOR		0x7E000000
#define WPACKHUS 0x7E500080
#define WPACKWUS 0x7E900080
#define WPACKDUS 0x7ED00080
#define WPACKHSS 0x7E700080
#define WPACKWSS 0x7EB00080
#define WPACKDSS 0x7EF00080
#define WRORH	0x7E700040
#define WRORW	0x7EB00040
#define WRORD	0x7EF00040
#define WRORHG	0x7E700140
#define WRORWG	0x7EB00140
#define WRORDG	0x7EF00140
#define WSADB	0x7E000120
#define WSADBZ	0x7E100120
#define WSADH	0x7E400120
#define WSADHZ	0x7E500120
#define WSHUFH	0x9E0001E0
#define WSLLH	0x7E500040
#define WSLLW	0x7E900040
#define WSLLD	0x7ED00040
#define WSLLHG	0x7E500140
#define WSLLWG	0x7E900140
#define WSLLDG	0x7ED00140
#define WSRAH	0x7E400040
#define WSRAW	0x7E800040
#define WSRAD	0x7EC00040
#define WSRAHG	0x7E400140
#define WSRAWG	0x7E800140
#define WSRADG	0x7EC00140
#define WSRLH	0x7E600040
#define WSRLW	0x7EA00040
#define WSRLD	0x7EE00040
#define WSRLHG	0x7E600140
#define WSRLWG	0x7EA00140
#define WSRLDG	0x7EE00140
#define WSUBB  	0x7E0001A0
#define WSUBH  	0x7E4001A0
#define WSUBW  	0x7E8001A0
#define WSUBBUS	0x7E1001A0
#define WSUBHUS	0x7E5001A0
#define WSUBWUS	0x7E9001A0
#define WSUBBSS	0x7E3001A0
#define WSUBHSS	0x7E7001A0
#define WSUBWSS	0x7EB001A0
#define WUNPCKEHUB 0x6E0000C0
#define WUNPCKEHUH 0x6E4000C0
#define WUNPCKEHUW 0x6E8000C0
#define WUNPCKEHSB 0x6E2000C0
#define WUNPCKEHSH 0x6E6000C0
#define WUNPCKEHSW 0x6EA000C0
#define WUNPCKIHB  0x7E1000C0
#define WUNPCKIHH  0x7E5000C0
#define WUNPCKIHW  0x7E9000C0
#define WUNPCKELUB 0x6E0000E0
#define WUNPCKELUH 0x6E4000E0
#define WUNPCKELUW 0x6E8000E0
#define WUNPCKELSB 0x6E2000E0
#define WUNPCKELSH 0x6E6000E0
#define WUNPCKELSW 0x6EA000E0
#define WUNPCKILB  0x7E1000E0
#define WUNPCKILH  0x7E5000E0
#define WUNPCKILW  0x7E9000E0
#define WXOR	0x7E100000

void C(int);
void S();
void Byte();
void Half();
void SByte();
void SHalf();

void IConst(reg Dest, int Const);
void IMul(reg Dest, reg Op1, int Mul);

void I3(int, reg Dest, reg Op1, reg Op2);
void I3C(int, reg Dest, reg Op1, reg Op2, int Const);
void I3S(int, reg Dest, reg Op1, reg Op2, int ShiftType, int Shift);
void I4(int, reg Dest, reg Op1, reg Op2, reg Op3);
void I2(int, reg Dest, reg Op1);
void I2C(int, reg Dest, reg Op1, int Const);

void I1P(int, reg Dest, dyninst* Block, int Ofs);
void I0P(int, int Cond, dyninst*);

void Break();

void CodeBegin();
void CodeEnd();

#endif
#endif
