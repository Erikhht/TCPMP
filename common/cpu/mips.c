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
 * $Id: mips.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(MIPS)

void STDCALL GetCpuId(int Id,uint32_t* p)
{
	__asm(	".set noreorder;"
			"mfc0	$2, $15;"	//prid
			"nop;"
			"nop;"
			"nop;"
			"nop;"
			"sw		$2,0($4);"
			".set reorder;", p);
}

#define SPEED10			\
	__asm(				\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	"addi	$2,$2,1;"	\
	);

int64_t STDCALL CPUSpeedClk(int n)
{
	__asm(	"add $3,$4,zero;"
			"loopn:");
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
			SPEED10
	__asm(	"addiu   $3,$3,-1;"
			"bne	$3,zero,loopn;");

	return n*(3+100)+500;
}

#endif
