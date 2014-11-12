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
 * $Id: mcomp_mips32.c 323 2005-11-01 20:52:32Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#if defined(MIPS32)

// $8		src end pointer
// $4		src pointer
// $5		dst pointer
// $6		src stride
// $7		dst stride
// $2,$9	first item lower 4 bytes (in two forms)
// $10,$11	first item upper 4 bytes (in two forms)
// $12,$13	second item lower 4 bytes (in two forms)
// $14,$15	second item upper 4 bytes (in two forms)
// $24		0x0101 0101 - for non horver
// $25		0xFEFE FEFE - for non horver
// $24		rounding	- for horver
// $25		temporary	- for horver
// $3		0x0303 0303 - for horver
// $1		0xFCFC FCFC - for horver
// $16		temporary	- for horver (must be saved/restored)

#define SET_SRCEND8					\
		"sll	$8,$6,3;"			\
		"addu	$8,$4,$8;"			

#define SET_SRCEND16				\
		"sll	$8,$6,4;"			\
		"addu	$8,$4,$8;"			

#define SET_MASKS					\
	   	"li		$24,0x01010101;"	\
		"nor	$25,$24,$0;"		

#define SET_MASKS2					\
		".set noat;"				\
	   	"li		$3,0x03030303;"		\
		"nor	$1,$3,$0;"			

#define LOAD_FIRST8(ofs)			\
		"ulw	$2, " #ofs "($4);"	\
		"ulw	$10," #ofs "+4($4);"\
		"and	$9,$2,$25;"			\
		"and	$11,$10,$25;"		\
		"srl	$9,$9,1;"			\
		"srl	$11,$11,1;"

#define LOAD_SECOND8(ofs)			\
		"ulw	$12," #ofs "($4);"	\
		"ulw	$14," #ofs "+4($4);"\
		"and	$13,$12,$25;"		\
		"and	$15,$14,$25;"		\
		"srl	$13,$13,1;"			\
		"srl	$15,$15,1;"

#define LOAD_FIRST8_HV				\
		"ulw	$2,0($4);"			\
		"ulw	$9,1($4);"			\
		"and	$16,$2,$1;"			\
		"and	$25,$9,$1;"			\
		"and	$2,$2,$3;"			\
		"and	$9,$9,$3;"			\
		"srl	$16,$16,2;"			\
		"srl	$25,$25,2;"			\
		"addu	$2,$2,$9;"			\
		"addu	$9,$16,$25;"		\
									\
		"ulw	$10,4($4);"			\
		"ulw	$11,5($4);"			\
		"and	$16,$10,$1;"		\
		"and	$25,$11,$1;"		\
		"and	$10,$10,$3;"		\
		"and	$11,$11,$3;"		\
		"srl	$16,$16,2;"			\
		"srl	$25,$25,2;"			\
		"addu	$10,$10,$11;"		\
		"addu	$11,$16,$25;"

#define LOAD_SECOND8_HV				\
		"ulw	$12,0($4);"			\
		"ulw	$13,1($4);"			\
		"and	$16,$12,$1;"		\
		"and	$25,$13,$1;"		\
		"and	$12,$12,$3;"		\
		"and	$13,$13,$3;"		\
		"srl	$16,$16,2;"			\
		"srl	$25,$25,2;"			\
		"addu	$12,$12,$13;"		\
		"addu	$13,$16,$25;"		\
									\
		"ulw	$14,4($4);"			\
		"ulw	$15,5($4);"			\
		"and	$16,$14,$1;"		\
		"and	$25,$15,$1;"		\
		"and	$14,$14,$3;"		\
		"and	$15,$15,$3;"		\
		"srl	$16,$16,2;"			\
		"srl	$25,$25,2;"			\
		"addu	$14,$14,$15;"		\
		"addu	$15,$16,$25;"

#define AVG8						\
		"or		$2,$2,$12;"			\
		"or		$10,$10,$14;"		\
		"and	$2,$2,$24;"			\
		"and	$10,$10,$24;"		\
		"addu	$2,$2,$9;"			\
		"addu	$10,$10,$11;"		\
		"addu	$2,$2,$13;"			\
		"addu	$10,$10,$15;"

#define AVGROUND8					\
		"and	$2,$2,$12;"			\
		"and	$10,$10,$14;"		\
		"and	$2,$2,$24;"			\
		"and	$10,$10,$24;"		\
		"addu	$2,$2,$9;"			\
		"addu	$10,$10,$11;"		\
		"addu	$2,$2,$13;"			\
		"addu	$10,$10,$15;"

#define SWAPSET8						\
		"move	$2,$12;"			\
		"move	$9,$13;"			\
		"move	$10,$14;"			\
		"move	$11,$15;"	

#define WRITE8						\
		"sw		$2,0($5);"			\
		"sw		$10,4($5);"			\
		"addu	$5,$5,$7;" 

#define SAVE						\
		"addiu	$sp,$sp,-4;"		\
		"sw		$16,0(sp);"			

#define RESTORE						\
		"lw		$16,0(sp);"			\
		"addiu	$sp,$sp,4;"			

void CopyBlock(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND8

		"loop:"
		"ulw	$2,0($4);"
		"ulw	$10,4($4);" 
		"addu	$4,$4,$6;"
		"sw		$2,0($5);"	
		"sw		$10,4($5);"	
		"addu	$5,$5,$7;" 
		"bne	$4,$8,loop;"); 
} 

void CopyBlockHor(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND8
		SET_MASKS

		"loophor:"
		LOAD_FIRST8(0)
		LOAD_SECOND8(1)
		"addu	$4,$4,$6;" 

		AVG8
		WRITE8

		"bne	$4,$8,loophor;");
} 

void CopyBlockHorRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND8
		SET_MASKS

		"loophorround:"
		LOAD_FIRST8(0)
		LOAD_SECOND8(1)

		"addu	$4,$4,$6;" 
		
		AVGROUND8
		WRITE8

		"bne	$4,$8,loophorround;");
} 

void CopyBlockVer(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND8
		SET_MASKS

		LOAD_FIRST8(0)

		"loopver:"
		"addu	$4,$4,$6;" 
		
		LOAD_SECOND8(0)

		AVG8
		WRITE8
		SWAPSET8

		"bne	$4,$8,loopver;");
} 

void CopyBlockVerRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND8
		SET_MASKS

		LOAD_FIRST8(0)

		"loopverround:"
		"addu	$4,$4,$6;" 
		
		LOAD_SECOND8(0)

		AVGROUND8
		WRITE8
		SWAPSET8

		"bne	$4,$8,loopverround;");
}

void CopyBlockHorVer(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SAVE
	    SET_SRCEND8
		SET_MASKS2

		"sll	$24,$3,1;"		 
		"and	$24,$24,$3;"	// 0x0202 0202

		//preprocessing
		
		LOAD_FIRST8_HV

		"loophorver:"
		"addu	$4,$4,$6;"
	
		LOAD_SECOND8_HV);

__asm (	"addu	$2,$2,$12;"
		"addu	$9,$9,$13;"
		"addu	$10,$10,$14;"
		"addu	$11,$11,$15;"

		"addu	$2,$2,$24;"
		"addu	$10,$10,$24;"

		"and	$2,$2,$1;"
		"and	$10,$10,$1;"
		"srl	$2,$2,2;"
		"srl	$10,$10,2;"
		"addu	$2,$2,$9;"
		"addu	$10,$10,$11;"
		
		WRITE8
		SWAPSET8

		"bne	$4,$8,loophorver;"
		RESTORE);
} 

void CopyBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SAVE
	    SET_SRCEND8
		SET_MASKS2

		"srl	$24,$3,1;"		 
		"and	$24,$24,$3;"	// 0x0101 0101

		//preprocessing
		
		LOAD_FIRST8_HV

		"loophorverround:"
		"addu	$4,$4,$6;"
	
		LOAD_SECOND8_HV);

__asm (	"addu	$2,$2,$12;"
		"addu	$9,$9,$13;"
		"addu	$10,$10,$14;"
		"addu	$11,$11,$15;"

		"addu	$2,$2,$24;"
		"addu	$10,$10,$24;"

		"and	$2,$2,$1;"
		"and	$10,$10,$1;"
		"srl	$2,$2,2;"
		"srl	$10,$10,2;"
		"addu	$2,$2,$9;"
		"addu	$10,$10,$11;"

		WRITE8
		SWAPSET8

		"bne	$4,$8,loophorverround;"
		RESTORE);
}

void AddBlock8x8(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND8
		SET_MASKS

		"loopadd:"
		"lw		$2,0($4);" 
		"lw		$10,4($4);" 
		"addu	$4,$4,$6;" 

		"lw		$9,0($5);"	
		"and	$11,$2,$25;"
		"or		$2,$2,$9;"	
		"and	$2,$2,$24;"	
		"srl	$11,$11,1;"	
		"addu	$2,$2,$11;"	
		"and	$9,$9,$25;"	
		"srl	$9,$9,1;"	
		"addu	$2,$2,$9;"	
							
		"lw		$11,4($5);"	
		"and	$9,$10,$25;"
		"or		$10,$10,$11;"
		"and	$10,$10,$24;"
		"srl	$9,$9,1;"	
		"addu	$10,$10,$9;"
		"and	$11,$11,$25;"
		"srl	$11,$11,1;"	
		"addu	$10,$10,$11;"
							
		"sw		$2,0($5);"	
		"sw		$10,4($5);"	
		"addu	$5,$5,$7;" 

		"bne	$4,$8,loopadd;"); 
} 

void AddBlock4x4(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_MASKS

		"lw		$2,0($4);" 
		"addu	$4,$4,$6;" 
		"lw		$10,0($4);" 
		"addu	$4,$4,$6;" 

		"lw		$9,0($5);"	
		"and	$11,$2,$25;"
		"or		$2,$2,$9;"	
		"and	$2,$2,$24;"	
		"srl	$11,$11,1;"	
		"addu	$2,$2,$11;"	
		"and	$9,$9,$25;"	
		"srl	$9,$9,1;"	
		"addu	$2,$2,$9;"	
		"sw		$2,0($5);"	
		"addu	$5,$5,$7;"
		
		"lw		$11,0($5);"	
		"and	$9,$10,$25;"
		"or		$10,$10,$11;"
		"and	$10,$10,$24;"
		"srl	$9,$9,1;"	
		"addu	$10,$10,$9;"
		"and	$11,$11,$25;"
		"srl	$11,$11,1;"	
		"addu	$10,$10,$11;"
		"sw		$10,0($5);"	
		"addu	$5,$5,$7;" 
		
		"lw		$2,0($4);" 
		"addu	$4,$4,$6;" 
		"lw		$10,0($4);" 
		"addu	$4,$4,$6;" 

		"lw		$9,0($5);"	
		"and	$11,$2,$25;"
		"or		$2,$2,$9;"	
		"and	$2,$2,$24;"	
		"srl	$11,$11,1;"	
		"addu	$2,$2,$11;"	
		"and	$9,$9,$25;"	
		"srl	$9,$9,1;"	
		"addu	$2,$2,$9;"	
		"sw		$2,0($5);"	
		"addu	$5,$5,$7;"
		
		"lw		$11,0($5);"	
		"and	$9,$10,$25;"
		"or		$10,$10,$11;"
		"and	$10,$10,$24;"
		"srl	$9,$9,1;"	
		"addu	$10,$10,$9;"
		"and	$11,$11,$25;"
		"srl	$11,$11,1;"	
		"addu	$10,$10,$11;"
		"sw		$10,0($5);"	
		"addu	$5,$5,$7;" ); 
} 

void CopyBlock16x16(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND16

		"loopcopy16:"
		"lw		$2,0($4);" 
		"lw		$10,4($4);" 
		"lw		$12,8($4);" 
		"lw		$14,12($4);" 
		"addu	$4,$4,$6;"
		"sw		$2,0($5);"	
		"sw		$10,4($5);"	
		"sw		$12,8($5);"	
		"sw		$14,12($5);"	
		"addu	$5,$5,$7;" 
		"bne	$4,$8,loopcopy16;"); 
} 

void CopyBlock8x8(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND8

		"loopcopy8:"
		"lw		$2,0($4);" 
		"lw		$10,4($4);" 
		"addu	$4,$4,$6;"
		"lw		$12,0($4);" 
		"lw		$14,4($4);" 
		"addu	$4,$4,$6;"
		"sw		$2,0($5);"	
		"sw		$10,4($5);"	
		"addu	$5,$5,$7;" 
		"sw		$12,0($5);"	
		"sw		$14,4($5);"	
		"addu	$5,$5,$7;" 
		"bne	$4,$8,loopcopy8;"); 
} 

void CopyBlock4x4(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	"lw		$2,0($4);" 
		"addu	$4,$4,$6;"
		"lw		$10,0($4);" 
		"addu	$4,$4,$6;"
		"lw		$12,0($4);" 
		"addu	$4,$4,$6;"
		"lw		$14,0($4);" 
		"sw		$2,0($5);"	
		"addu	$5,$5,$7;" 
		"sw		$10,0($5);"	
		"addu	$5,$5,$7;" 
		"sw		$12,0($5);"	
		"addu	$5,$5,$7;" 
		"sw		$14,0($5);"); 
} 

void CopyBlockM(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND16

		"loopm:"
		"ulw	$2,0($4);"
		"ulw	$10,4($4);" 
		"ulw	$12,8($4);"
		"ulw	$14,12($4);" 
		"addu	$4,$4,$6;"
		"sw		$2,0($5);"	
		"sw		$10,4($5);"	
		"sw		$12,8($5);"	
		"sw		$14,12($5);"	
		"addu	$5,$5,$7;" 
		"bne	$4,$8,loopm;"); 
} 

#endif
