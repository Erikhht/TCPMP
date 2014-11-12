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
;* $Id: x86.asm 271 2005-08-09 08:31:35Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

BITS 32
SECTION .text

%macro cglobal 2
%define %1 _%1@%2
global %1
%endmacro

cglobal GetCpuId,8
cglobal CPUSpeedClk,4

ALIGN 16
GetCpuId:
	pushfd
	pop     eax
	mov     edx,eax
	xor     eax,00200000h
	push    eax
	popfd
	pushfd
	pop     eax
	cmp     eax,edx
	mov		eax,0
	jz      .NoCpuId
	push    ebx
	push    esi
	mov		eax,[esp+12]
	mov		esi,[esp+16]
	cpuid   
	mov		[esi],eax
	mov		[esi+4],ebx
	mov		[esi+8],ecx
	mov		[esi+12],edx
	pop     esi
	pop     ebx
.NoCpuId:
	ret 8

CPUSpeedClk:
	push	ebx
	mov		ebx,esp
	mov		edx,[esp+8]
	imul	eax,edx,1000+3

.CPULoop:
%rep 1000
	add		ecx,[ebx]
%endrep
	sub		edx,1
	jg		.CPULoop

	xor		edx,edx
	pop		ebx
	ret		4


