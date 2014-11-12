@*****************************************************************************
@*
@* This program is free software ; you can redistribute it and/or modify
@* it under the terms of the GNU General Public License as published by
@* the Free Software Foundation; either version 2 of the License, or
@* (at your option) any later version.
@*
@* This program is distributed in the hope that it will be useful,
@* but WITHOUT ANY WARRANTY; without even the implied warranty of
@* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
@* GNU General Public License for more details.
@*
@* You should have received a copy of the GNU General Public License
@* along with this program; if not, write to the Free Software
@* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
@*
@* $Id: idct_wmmx.s 324 2005-11-02 04:49:29Z picard $
@*
@* The Core Pocket Media Player
@* Copyright (c) 2004-2005 Gabor Kovacs
@*
@*****************************************************************************


	.global WMMXIDCT_Const8x8

	.align 2
@WMMXIDCT_Const8x8(int v,uint8_t * Dst,int DstPitch, uint8_t * Src)
WMMXIDCT_Const8x8:
	cmp r0,#0
	bgt const8x8add
	blt const8x8sub
	cmp r1,r3
	beq const8x8done

	.macro const8x8copyrow
	wldrd   wr1,[r3]
	add		r3,r3,#8
	wldrd   wr2,[r3]
	add		r3,r3,#8
	wstrd   wr1,[r1]
	add		r1,r1,r2
	wstrd   wr2,[r1]
	add		r1,r1,r2
	.endm
	
	const8x8copyrow
	const8x8copyrow
	const8x8copyrow
	const8x8copyrow

const8x8done:
	mov pc,lr

const8x8add:	
	.macro const8x8addrow
	wldrd   wr1,[r3]
	add		r3,r3,#8
	wldrd   wr2,[r3]
	add		r3,r3,#8
	waddbus wr1,wr1,wr0
	waddbus wr2,wr2,wr0
	wstrd   wr1,[r1]
	add		r1,r1,r2
	wstrd   wr2,[r1]
	add		r1,r1,r2
	.endm

	tbcstb  wr0,r0
	const8x8addrow
	const8x8addrow
	const8x8addrow
	const8x8addrow
	mov pc,lr

const8x8sub:
	.macro const8x8subrow
	wldrd   wr1,[r3]
	add		r3,r3,#8
	wldrd   wr2,[r3]
	add		r3,r3,#8
	wsubbus wr1,wr1,wr0
	wsubbus wr2,wr2,wr0
	wstrd   wr1,[r1]
	add		r1,r1,r2
	wstrd   wr2,[r1]
	add		r1,r1,r2
	.endm

	rsb r0,r0,#0
	tbcstb  wr0,r0
	const8x8subrow
	const8x8subrow
	const8x8subrow
	const8x8subrow
	mov pc,lr


