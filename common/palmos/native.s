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
@* $Id: native.s 292 2005-10-14 20:30:00Z picard $
@*
@* The Core Pocket Media Player
@* Copyright (c) 2004-2005 Gabor Kovacs
@*
@*****************************************************************************

.globl	SonyInvalidateDCache
.globl	SonyCleanDCache
.globl	SysGetEntryAddresses
.globl	SysFindModule
.globl  HALDelay
.globl  HALDisplayWake
.globl  HALDisplayOff_TREO650
.globl	SysLoadModule
.globl	SysUnloadModule
.globl  PceCall
.globl  PalmCall
.globl  PalmCall2

AddrPceCall: .DC.L PceCall

@r0,r1
SonyCleanDCache:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	swi	0xb5
	ldmia sp!, {r9, pc}  

@r0,r1
SonyInvalidateDCache:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	swi	0xb4
	ldmia sp!, {r9, pc}  

@r0,r1,r2,r3
SysGetEntryAddresses:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-8]
	mov lr, pc
	ldr	pc, [ip, #2104]
	ldmia sp!, {r9, pc}  

@r0,r1,r2,r3,[sp]
SysFindModule:
	ldr ip, [sp, #0]
	stmdb sp!, {r9, lr}
	stmdb sp!, {ip}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-8]
	mov lr, pc
	ldr	pc, [ip, #2092]
	add sp, sp, #4
	ldmia sp!, {r9, pc}  

@r0,r1,r2,r3,[sp]
SysLoadModule:
	ldr ip, [sp, #0]
	stmdb sp!, {r9, lr}
	stmdb sp!, {ip}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-8]
	mov lr, pc
	ldr	pc, [ip, #2176]
	add sp, sp, #4
	ldmia sp!, {r9, pc}  

@r0
SysUnloadModule:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-8]
	mov lr, pc
	ldr	pc, [ip, #2312]
	ldmia sp!, {r9, pc}  

@r0
HALDelay:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-4]
	mov lr, pc
	ldr	pc, [ip, #0x28]
	ldmia sp!, {r9, pc}  

HALDisplayWake:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-4]
	mov lr, pc
	ldr	pc, [ip, #0x40]
	ldmia sp!, {r9, pc}  

HALDisplayOff_TREO650:
	stmdb sp!, {r9, lr}
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	ldr	ip, [r9, #-4]
	mov lr, pc
	ldr	pc, [ip, #0x358]
	ldmia sp!, {r9, pc}  

PalmCall:
	stmdb sp!, {r9, lr}
	mov ip, r0
	mov r0, r1
	mov r1, r2
	mov r2, r3
	ldr r3, [sp, #8]
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	mov lr, pc
	mov pc, ip
	ldmia sp!, {r9, pc}  

PalmCall2:
	stmdb sp!, {r9, lr}

	sub sp, sp, #16
	ldr ip, [sp, #8+16+4]
	ldr r9, [sp, #8+16+8]
	str ip, [sp, #0]
	str r9, [sp, #4]
	ldr ip, [sp, #8+16+12]
	ldr r9, [sp, #8+16+16]
	str ip, [sp, #8]
	str r9, [sp, #12]

	mov ip, r0
	mov r0, r1
	mov r1, r2
	mov r2, r3
	ldr r3, [sp, #8+16]
	ldr r9, AddrPceCall
	ldr r9, [r9, #0]
	mov lr, pc
	mov pc, ip

	add sp, sp, #16
	ldmia sp!, {r9, pc}  
