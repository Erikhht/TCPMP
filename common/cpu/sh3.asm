;*****************************************************************************
;*
;* This program is free software ; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
;*
;* $Id: sh3.asm 271 2005-08-09 08:31:35Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

	.section	.text, code, align=4
    .export     `_CPUSpeedClk`

	.macro Speed10
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	add.l #1,r1
	.endm
	
    .align 4
_CPUSpeedClk:	.entry
    .prolog

	mov.b #104,r0
	mul.l r0,r5

loopn:
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	Speed10
	dt	r5
    bf loopn

	sts macl,r0
	mov.l r0,@r4
    mov.b #0,r0
    mov.l r0,@(4,r4)
    rts
	mov.l r4,r0

    .endf
    .end
