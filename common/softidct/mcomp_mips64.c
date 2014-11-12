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
 * $Id: mcomp_mips64.c 284 2005-10-04 08:54:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "softidct.h"

#if defined(MIPS64)

// important: disable interrupts before using 64bit registers (but not too long, it could freeze)
// important: $8 can't be used as 64bit register (trashed by some kernel routine)

// $8		src end pointer
// $4		src pointer
// $5		dst pointer
// $6		src stride
// $7		dst stride
// $2,$9	first item lower 8 bytes (in two forms)
// $10,$11	first item upper 8 bytes (in two forms) - for 16x16 macroblocks
// $12,$13	second item lower 8 bytes (in two forms)
// $14,$15	second item upper 8 bytes (in two forms) - for 16x16 macroblocks
// $24		0x0101 0101 0101 0101 - for non horver
// $25		0xFEFE FEFE FEFE FEFE - for non horver
// $24		rounding			  - for horver
// $25		temporary			  - for 16x16 horver
// $3		0x0303 0303 0303 0303 - for horver
// $1		0xFCFC FCFC FCFC FCFC - for horver
// $16		temporary			  - for 16x16 horver (must be saved/restored)

#define SET_SRCEND8					\
		"sll	$8,$6,3;"			\
		"addu	$8,$4,$8;"			

#define SET_SRCEND16				\
		"sll	$8,$6,4;"			\
		"addu	$8,$4,$8;"			

#define SET_MASKS					\
	   	"li		$24,0x01010101;"	\
		"dsll	$25,$24,32;"		\
		"or		$24,$24,$25;"		\
		"nor	$25,$24,$0;"		

#define SET_MASKS2					\
		".set noat;"				\
	   	"li		$3,0x03030303;"		\
		"dsll	$1,$3,32;"			\
		"or		$3,$3,$1;"			\
		"nor	$1,$3,$0;"			

#define LOAD_FIRST8(ofs)			\
		"uld	$2, " #ofs "($4);"	\
		"and	$9,$2,$25;"			\
		"dsrl	$9,$9,1;"

#define LOAD_FIRST16(ofs)			\
		"uld	$2, " #ofs "($4);"	\
		"uld	$10," #ofs "+8($4);"\
		"and	$9,$2,$25;"			\
		"and	$11,$10,$25;"		\
		"dsrl	$9,$9,1;"			\
		"dsrl	$11,$11,1;"

#define LOAD_SECOND8(ofs)			\
		"uld	$12," #ofs "($4);"	\
		"and	$13,$12,$25;"		\
		"dsrl	$13,$13,1;"

#define LOAD_SECOND16(ofs)			\
		"uld	$12," #ofs "($4);"	\
		"uld	$14," #ofs "+8($4);"\
		"and	$13,$12,$25;"		\
		"and	$15,$14,$25;"		\
		"dsrl	$13,$13,1;"			\
		"dsrl	$15,$15,1;"

#define LOAD_FIRST8_HV				\
		"uld	$2,0($4);"			\
		"uld	$9,1($4);"			\
		"and	$10,$2,$1;"			\
		"and	$11,$9,$1;"			\
		"and	$2,$2,$3;"			\
		"and	$9,$9,$3;"			\
		"dsrl	$10,$10,2;"			\
		"dsrl	$11,$11,2;"			\
		"daddu	$2,$2,$9;"			\
		"daddu	$9,$10,$11;"

#define LOAD_FIRST16_HV				\
		"uld	$2,0($4);"			\
		"uld	$9,1($4);"			\
		"and	$16,$2,$1;"			\
		"and	$25,$9,$1;"			\
		"and	$2,$2,$3;"			\
		"and	$9,$9,$3;"			\
		"dsrl	$16,$16,2;"			\
		"dsrl	$25,$25,2;"			\
		"daddu	$2,$2,$9;"			\
		"daddu	$9,$16,$25;"		\
									\
		"uld	$10,8($4);"			\
		"uld	$11,9($4);"			\
		"and	$16,$10,$1;"		\
		"and	$25,$11,$1;"		\
		"and	$10,$10,$3;"		\
		"and	$11,$11,$3;"		\
		"dsrl	$16,$16,2;"			\
		"dsrl	$25,$25,2;"			\
		"daddu	$10,$10,$11;"		\
		"daddu	$11,$16,$25;"

#define LOAD_SECOND8_HV				\
		"uld	$12,0($4);"			\
		"uld	$13,1($4);"			\
		"and	$14,$12,$1;"		\
		"and	$15,$13,$1;"		\
		"and	$12,$12,$3;"		\
		"and	$13,$13,$3;"		\
		"dsrl	$14,$14,2;"			\
		"dsrl	$15,$15,2;"			\
		"daddu	$12,$12,$13;"		\
		"daddu	$13,$14,$15;"

#define LOAD_SECOND16_HV			\
		"uld	$12,0($4);"			\
		"uld	$13,1($4);"			\
		"and	$16,$12,$1;"		\
		"and	$25,$13,$1;"		\
		"and	$12,$12,$3;"		\
		"and	$13,$13,$3;"		\
		"dsrl	$16,$16,2;"			\
		"dsrl	$25,$25,2;"			\
		"daddu	$12,$12,$13;"		\
		"daddu	$13,$16,$25;"		\
									\
		"uld	$14,8($4);"			\
		"uld	$15,9($4);"			\
		"and	$16,$14,$1;"		\
		"and	$25,$15,$1;"		\
		"and	$14,$14,$3;"		\
		"and	$15,$15,$3;"		\
		"dsrl	$16,$16,2;"			\
		"dsrl	$25,$25,2;"			\
		"daddu	$14,$14,$15;"		\
		"daddu	$15,$16,$25;"

#define AVG8						\
		"or		$2,$2,$12;"			\
		"and	$2,$2,$24;"			\
		"daddu	$2,$2,$9;"			\
		"daddu	$2,$2,$13;"

#define AVG16						\
		"or		$2,$2,$12;"			\
		"or		$10,$10,$14;"		\
		"and	$2,$2,$24;"			\
		"and	$10,$10,$24;"		\
		"daddu	$2,$2,$9;"			\
		"daddu	$10,$10,$11;"		\
		"daddu	$2,$2,$13;"			\
		"daddu	$10,$10,$15;"

#define AVGROUND8					\
		"and	$2,$2,$12;"			\
		"and	$2,$2,$24;"			\
		"daddu	$2,$2,$9;"			\
		"daddu	$2,$2,$13;"

#define AVGROUND16					\
		"and	$2,$2,$12;"			\
		"and	$10,$10,$14;"		\
		"and	$2,$2,$24;"			\
		"and	$10,$10,$24;"		\
		"daddu	$2,$2,$9;"			\
		"daddu	$10,$10,$11;"		\
		"daddu	$2,$2,$13;"			\
		"daddu	$10,$10,$15;"

#define SWAPSET8					\
		"move	$2,$12;"			\
		"move	$9,$13;"			

#define SWAPSET16					\
		"move	$2,$12;"			\
		"move	$9,$13;"			\
		"move	$10,$14;"			\
		"move	$11,$15;"	

#define WRITE8						\
		"sdr	$2,0($5);"			\
		"addu	$5,$5,$7;" 

#define WRITE16						\
		"sdr	$2,0($5);"			\
		"sdr	$10,8($5);"			\
		"addu	$5,$5,$7;" 

#define SAVE						\
		"addiu	$sp,$sp,-4;"		\
		"sw		$16,0(sp);"			

#define RESTORE						\
		"lw		$16,0(sp);"			\
		"addiu	$sp,$sp,4;"			

#ifdef MIPSVR41XX
//cache without loading
#define CACHE16						\
		".set noreorder;"			\
		"cache	13,0($5);"			\
		".set reorder;"
#else
#define CACHE16
#endif

void CopyBlock(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND8

		"loop:"
		"uld	$2,0($4);" 
		"addu	$4,$4,$6;"
		"sdr	$2,0($5);"	
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
__asm (	SET_SRCEND8
		SET_MASKS2

		"dsll	$24,$3,1;"		 
		"and	$24,$24,$3;"	// 0x0202 0202 0202 0202

		//preprocessing
		
		LOAD_FIRST8_HV

		"loophorver:"
		"addu	$4,$4,$6;"
	
		LOAD_SECOND8_HV

		"daddu	$2,$2,$12;"
		"daddu	$9,$9,$13;"

		"daddu	$2,$2,$24;"

		"and	$2,$2,$1;"
		"dsrl	$2,$2,2;"
		"daddu	$2,$2,$9;"

		WRITE8
		SWAPSET8

		"bne	$4,$8,loophorver;");
} 

void CopyBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND8
		SET_MASKS2

		"dsrl	$24,$3,1;"		 
		"and	$24,$24,$3;"	// 0x0101 0101 0101 0101

		//preprocessing
		
		LOAD_FIRST8_HV

		"loophorverround:"
		"addu	$4,$4,$6;"
	
		LOAD_SECOND8_HV

		"daddu	$2,$2,$12;"
		"daddu	$9,$9,$13;"

		"daddu	$2,$2,$24;"

		"and	$2,$2,$1;"
		"dsrl	$2,$2,2;"
		"daddu	$2,$2,$9;"

		WRITE8
		SWAPSET8

		"bne	$4,$8,loophorverround;");
}

void CopyMBlock(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND16

		"loopm:"
		CACHE16

		"uld	$2,0($4);" 
		"uld	$10,8($4);" 
		"addu	$4,$4,$6;" 
		
		WRITE16

		"bne	$4,$8,loopm;"); 
} 

void CopyMBlockHor(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND16
		SET_MASKS

		"loopmhor:"
		LOAD_FIRST16(0)
		LOAD_SECOND16(1)
		"addu	$4,$4,$6;" 

		CACHE16
		AVG16
		WRITE16

		"bne	$4,$8,loopmhor;");
} 

void CopyMBlockHorRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND16
		SET_MASKS

		"loopmhorround:"
		LOAD_FIRST16(0)
		LOAD_SECOND16(1)
		"addu	$4,$4,$6;" 

		CACHE16
		AVGROUND16
		WRITE16

		"bne	$4,$8,loopmhorround;");
} 

void CopyMBlockVer(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND16
		SET_MASKS

		LOAD_FIRST16(0)

		"loopmver:"
		"addu	$4,$4,$6;" 
		
		LOAD_SECOND16(0)

		CACHE16
		AVG16
		WRITE16
		SWAPSET16

		"bne	$4,$8,loopmver;"
		
		);
} 

void CopyMBlockVerRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SET_SRCEND16
		SET_MASKS

		LOAD_FIRST16(0)

		"loopmverround:"
		"addu	$4,$4,$6;" 
		
		LOAD_SECOND16(0)

		CACHE16
		AVGROUND16
		WRITE16
		SWAPSET16

		"bne	$4,$8,loopmverround;");
} 

void CopyMBlockHorVer(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SAVE
	    SET_SRCEND16
		SET_MASKS2

		"dsll	$24,$3,1;"		 
		"and	$24,$24,$3;"		// 0x0202 0202 0202 0202

		//preprocessing
		
		LOAD_FIRST16_HV

		"loopmhorver:"
		"addu	$4,$4,$6;"
	
		LOAD_SECOND16_HV 
		CACHE16);

__asm (	"daddu	$2,$2,$12;"
		"daddu	$9,$9,$13;"
		"daddu	$10,$10,$14;"
		"daddu	$11,$11,$15;"

		"daddu	$2,$2,$24;"
		"daddu	$10,$10,$24;"

		"and	$2,$2,$1;"
		"and	$10,$10,$1;"
		"dsrl	$2,$2,2;"
		"dsrl	$10,$10,2;"
		"daddu	$2,$2,$9;"
		"daddu	$10,$10,$11;"

		WRITE16
		SWAPSET16

		"bne	$4,$8,loopmhorver;"
		
		RESTORE);
} 

void CopyMBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm (	SAVE
	    SET_SRCEND16
		SET_MASKS2

		"dsrl	$24,$3,1;"		 
		"and	$24,$24,$3;"		// 0x0101 0101 0101 0101

		//preprocessing
		
		LOAD_FIRST16_HV

		"loopmhorverround:"
		"addu	$4,$4,$6;");
	
__asm (	LOAD_SECOND16_HV
		CACHE16

		"daddu	$2,$2,$12;"
		"daddu	$9,$9,$13;"
		"daddu	$10,$10,$14;"
		"daddu	$11,$11,$15;"

		"daddu	$2,$2,$24;"
		"daddu	$10,$10,$24;"

		"and	$2,$2,$1;"
		"and	$10,$10,$1;"
		"dsrl	$2,$2,2;"
		"dsrl	$10,$10,2;"
		"daddu	$2,$2,$9;"
		"daddu	$10,$10,$11;"

		WRITE16
		SWAPSET16

		"bne	$4,$8,loopmhorverround;"
	
		RESTORE);
}

void AddBlock8x8(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND8
		SET_MASKS

		"loopadd:"
		"ldr	$2,0($4);"
		"addu	$4,$4,$6;"

		"ldr	$9,0($5);"
		"and	$11,$2,$25;"
		"or		$2,$2,$9;"
		"and	$2,$2,$24;"
		"dsrl	$11,$11,1;"
		"daddu	$2,$2,$11;"
		"and	$9,$9,$25;"
		"dsrl	$9,$9,1;"
		"daddu	$2,$2,$9;"

		"sdr	$2,0($5);"
		"addu	$5,$5,$7;" 
		"bne	$4,$8,loopadd;"); 
} 

void AddBlock16x16(unsigned char * Src, unsigned char * Dst, int SrcStride, int DstStride) 
{ 
__asm(	SET_SRCEND16
		SET_MASKS

		"loopadd16:"
		"ldr	$2,0($4);" 
		"ldr	$10,8($4);" 
#ifdef MIPSVR41XX
		".set noreorder;"
		"cache	17,0($4);" // hit invalidate (lose changes)
		".set reorder;"	
#endif
		"addu	$4,$4,$6;" 

		"ldr	$9,0($5);"	
		"and	$11,$2,$25;"
		"or		$2,$2,$9;"	
		"and	$2,$2,$24;"	
		"dsrl	$11,$11,1;"	
		"daddu	$2,$2,$11;"	
		"and	$9,$9,$25;"	
		"dsrl	$9,$9,1;"	
		"daddu	$2,$2,$9;"	
							
		"ldr	$11,8($5);"	
		"and	$9,$10,$25;"
		"or		$10,$10,$11;"
		"and	$10,$10,$24;"
		"dsrl	$9,$9,1;"	
		"daddu	$10,$10,$9;"
		"and	$11,$11,$25;"
		"dsrl	$11,$11,1;"	
		"daddu	$10,$10,$11;"
							
		"sdr	$2,0($5);"	
		"sdr	$10,8($5);"	
		"addu	$5,$5,$7;" 

		"bne	$4,$8,loopadd16;"); 
} 

#endif
