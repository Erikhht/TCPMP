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
@* $Id: arm.s 271 2005-08-09 08:31:35Z picard $
@*
@* The Core Pocket Media Player
@* Copyright (c) 2004-2005 Gabor Kovacs
@*
@*****************************************************************************

.global GetCpuId
.global GetCPAR
.global CPUSpeedClk
.global CPUSpeedClkEnd
.global FlushCache
.global setjmp
.global longjmp

setjmp: 
	mov r12,#0
	stmia r0,{r4-r12,sp,lr}
	mov r0,#0
	mov	pc,lr

longjmp:
	ldmia r0!,{r4-r12,sp,lr}
	movs r0,r1
	moveq r0,#1
	mov pc,lr

GetCpuId:
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
UserMode:
	mov	pc,lr

GetCPAR:
	mov r0,#0
	mrs	r1,cpsr
	and r1,r1,#15
	cmp r1,#15
	bne UserMode2
	mrc p15,0,r0,c15,c1,0
	nop
	nop
UserMode2:
	mov	pc,lr

.macro	speed10
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
.endm

CPUSpeedClk:

	mov r2,r0
	mov r1,#3+100
	mul r0,r2,r1
	mov r1,#0

CPULoop:
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


