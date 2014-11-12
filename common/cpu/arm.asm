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
;* $Id: arm.asm 271 2005-08-09 08:31:35Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

	AREA	|.text|, CODE

	EXPORT GetCpuId
	EXPORT GetCPAR
	EXPORT CPUSpeedClk
	EXPORT CheckARM5E
	EXPORT CheckARMXScale
	EXPORT IRQDisable
	EXPORT IRQEnable

IRQDisable PROC
	mrs	r0, cpsr
	orr	r0, r0, #0x80
	msr	cpsr_c, r0
	mov pc,lr
	ENDP

IRQEnable PROC
	mrs	r0, cpsr
	bic	r0, r0, #0x80
	msr	cpsr_c, r0
	mov pc,lr
	ENDP

CheckARM5E PROC

	mov r0,#2
	qadd r0,r0,r0
	qadd r0,r0,r0 
	qadd r0,r0,r0
	qadd r0,r0,r0
	cmp r0,#32
	moveq r0,#1
	movne r0,#0
	mov pc,lr

CheckARMXScale PROC
	mov r0,#0x1000000
	mov r1,#0x1000000
	mar acc0,r0,r1
	mov r0,#32
	mov r1,#32
	mra r0,r1,acc0
	cmp r0,#0x1000000
	moveq r0,#1
	movne r0,#0
	cmp r1,#0x1000000 ;64bit or just 40bit?
	moveq r0,#2
	mov pc,lr

GetCpuId PROC
	mrs	r0,cpsr
	and r0,r0,#15
	cmp r0,#15
	bne UserMode
	mrc p15,0,r0,c0,c0,0
	nop
	nop
	mrc p15,0,r2,c0,c0,1
	nop
	nop
	str r0,[r1,#0]
	str r2,[r1,#4]
UserMode
	mov	pc,lr
	ENDP

GetCPAR PROC
	mov r0,#0
	mrs	r1,cpsr
	and r1,r1,#15
	cmp r1,#15
	bne UserMode2
	mrc p15,0,r0,c15,c1,0
	nop
	nop
UserMode2
	mov	pc,lr
	ENDP

	MACRO
	speed10
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	add r3,r3,#1
	MEND

CPUSpeedClk PROC

	mov r2,r0
	mov r1,#3+100
	mul r0,r2,r1 ;loop overhead
	mov	r1,#0

CPULoop
	speed10
	speed10
	speed10
	speed10
	speed10
	speed10
	speed10
	speed10
	speed10
	speed10
	subs r2,r2,#1
	bgt CPULoop

	mov pc,lr

	ENDP

	END
