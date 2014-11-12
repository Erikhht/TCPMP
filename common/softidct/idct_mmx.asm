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
;* $Id: idct_mmx.asm 432 2005-12-28 16:39:13Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

;******************
;*  NOT FINISHED  *
;******************

BITS 32

ROW_SHIFT		equ	11
COL_SHIFT		equ 6

SECTION .data
ALIGN 16

SECTION .text

%macro cglobal 2
%define %1 _%1@%2
global %1
%endmacro

cglobal IDCT_Const8x8,16
cglobal IDCT_Const4x4,16
;cglobal IDCT_Block8x8,16
;cglobal IDCT_Block8x4,16

; ecx:block
%macro Row 1
	movq	mm0,[ecx+%1*16]
	movq	mm1,[ecx+%1*16+8]

; x0 x4 x3 x7 x1 x6 x2 x5

;		x4' = W7 * x5 + W1 * x4;
;		x5' = W7 * x4 - W1 * x5;

;		x6' = W3 * x7 + W5 * x6;
;		x7' = W3 * x6 - W5 * x7;

;		x6' = x4 + x6;
;		x4' = x4 - x6;

;		x7' = x5 + x7;
;		x5' = x5 - x7;

;		x5' = (181 * (x4 + x5) + 128) >> 8;
;		x4' = (181 * (x4 - x5) + 128) >> 8;

;		x3' = W6 * x2 + W2 * x3;
;		x2' = W6 * x3 - W2 * x2;

;		x1 <<= 11;
;		x0 <<= 11;
		   
;		x1' = x0 + x1;
;		x0' = x0 - x1;

;		x3' = x1 + x3;
;		x1' = x1 - x3;

;		x2' = x0 + x2;
;		x0' = x0 - x2;

	movq	[ecx+%1*16],mm0
	movq	[ecx+%1*16+8],mm1
%endmacro

; ecx:block
; edi:dest   edx:dest pitch
; esi:src    eax:src pitch

%macro Col4x4 2


%endmacro

%macro Col4x8 2


%endmacro

%if 0
ALIGN 16
IDCT_Block8x8:
	push	esi
	push	edi

	mov		ecx,[esp+12]		;block
	mov		edi,[esp+12+4]		;dst
	mov		edx,[esp+12+8]		;dst pitch
	mov		esi,[esp+12+12]		;src 
	mov		eax,8				;src pitch

	Row		0
	Row		1
	Row		2
	Row		3
	Row		4
	Row		5
	Row		6
	Row		7

	or		esi,esi
	jne		.Add

	Col4x8	0,0
	Col4x8	8,0
	pop		edi
	pop		esi 
	ret 16

.Add:
	Col4x8	0,1
	Col4x8	8,1
	pop		edi
	pop		esi 
	ret 16

ALIGN 16
IDCT_Block8x4:
	push	esi
	push	edi

	mov		ecx,[esp+12]		;src
	mov		edi,[esp+12+4]		;dst
	mov		edx,[esp+12+8]		;dst pitch
	mov		esi,[esp+12+12]		;src 
	mov		eax,8				;src pitch

	Row		0
	Row		1
	Row		2
	Row		3

	or		esi,esi
	jne		.Add

	Col4x4	0,0
	Col4x4	8,0
	pop		edi
	pop		esi 
	ret 16

.Add:
	Col4x4	0,1
	Col4x4	8,1
	pop		edi
	pop		esi 
	ret 16
%endif

ALIGN 16
IDCT_Const8x8:
	push	esi
	push	edi

	mov		ecx,[esp+12]		;v
	mov		edi,[esp+12+4]		;dst
	mov		edx,[esp+12+8]		;dst pitch
	mov		esi,[esp+12+12]		;src
	mov		eax,8				;src pitch

	or		ecx,ecx
	js		.Sub

.Add:
	movd	mm7,ecx
	punpcklbw mm7,mm7
	punpcklwd mm7,mm7
	punpckldq mm7,mm7
	
%rep 4
	movq	mm0,[esi]
	movq	mm1,[esi+eax]
	paddusb mm0,mm7
	lea		esi,[esi+eax*2]
	paddusb mm1,mm7
	movq	[edi],mm0
	movq	[edi+edx],mm1
	lea		edi,[edi+edx*2]
%endrep

	pop		edi
	pop		esi 
	ret 16

.Sub:
	neg		ecx
	movd	mm7,ecx
	punpcklbw mm7,mm7
	punpcklwd mm7,mm7
	punpckldq mm7,mm7
	
%rep 4
	movq	mm0,[esi]
	movq	mm1,[esi+eax]
	psubusb mm0,mm7
	lea		esi,[esi+eax*2]
	psubusb mm1,mm7
	movq	[edi],mm0
	movq	[edi+edx],mm1
	lea		edi,[edi+edx*2]
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
IDCT_Const4x4:
	push	esi
	push	edi

	mov		ecx,[esp+12]		;v
	mov		edi,[esp+12+4]		;dst
	mov		edx,[esp+12+8]		;dst pitch
	mov		esi,[esp+12+12]		;src
	mov		eax,8				;src pitch

	or		ecx,ecx
	js		.Sub

.Add:
	movd	mm7,ecx
	punpcklbw mm7,mm7
	punpcklwd mm7,mm7
	punpckldq mm7,mm7
	
%rep 2
	movd	mm0,[esi]
	movd	mm1,[esi+eax]
	paddusb mm0,mm7
	lea		esi,[esi+eax*2]
	paddusb mm1,mm7
	movd	[edi],mm0
	movd	[edi+edx],mm1
	lea		edi,[edi+edx*2]
%endrep

	pop		edi
	pop		esi 
	ret 16

.Sub:
	neg		ecx
	movd	mm7,ecx
	punpcklbw mm7,mm7
	punpcklwd mm7,mm7
	punpckldq mm7,mm7
	
%rep 2
	movd	mm0,[esi]
	movd	mm1,[esi+eax]
	psubusb mm0,mm7
	lea		esi,[esi+eax*2]
	psubusb mm1,mm7
	movd	[edi],mm0
	movd	[edi+edx],mm1
	lea		edi,[edi+edx*2]
%endrep

	pop		edi
	pop		esi 
	ret 16

