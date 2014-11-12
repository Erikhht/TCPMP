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
;* $Id: mcomp_mmx.asm 323 2005-11-01 20:52:32Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

BITS 32
%if 0

SECTION .rdata
round7:		dd 07070707h,07070707h
round8:		dd 08080808h,08080808h

SECTION .text

%macro cglobal 2
%define %1 _%1@%2
global %1
%endmacro

cglobal AddBlock4x4_00,12
cglobal AddBlock4x4_01,12
cglobal AddBlock4x4_02,12
cglobal AddBlock4x4_03,12
cglobal AddBlock4x4_10,12
cglobal AddBlock4x4_11,12
cglobal AddBlock4x4_12,12
cglobal AddBlock4x4_13,12
cglobal AddBlock4x4_20,12
cglobal AddBlock4x4_21,12
cglobal AddBlock4x4_22,12
cglobal AddBlock4x4_23,12
cglobal AddBlock4x4_30,12
cglobal AddBlock4x4_31,12
cglobal AddBlock4x4_32,12
cglobal AddBlock4x4_33,12
cglobal CopyBlock4x4,16
cglobal CopyBlock4x4_01,16
cglobal CopyBlock4x4_02,16
cglobal CopyBlock4x4_03,16 
cglobal CopyBlock4x4_10,16
cglobal CopyBlock4x4_11,16 
cglobal CopyBlock4x4_12,16
cglobal CopyBlock4x4_13,16
cglobal CopyBlock4x4_20,16
cglobal CopyBlock4x4_21,16 
cglobal CopyBlock4x4_22,16
cglobal CopyBlock4x4_23,16
cglobal CopyBlock4x4_20,16
cglobal CopyBlock4x4_21,16 
cglobal CopyBlock4x4_22,16
cglobal CopyBlock4x4_23,16
cglobal CopyBlock4x4_01R,16
cglobal CopyBlock4x4_02R,16
cglobal CopyBlock4x4_03R,16 
cglobal CopyBlock4x4_10R,16
cglobal CopyBlock4x4_11R,16 
cglobal CopyBlock4x4_12R,16
cglobal CopyBlock4x4_13R,16
cglobal CopyBlock4x4_20R,16
cglobal CopyBlock4x4_21R,16 
cglobal CopyBlock4x4_22R,16
cglobal CopyBlock4x4_23R,16
cglobal CopyBlock4x4_20R,16
cglobal CopyBlock4x4_21R,16 
cglobal CopyBlock4x4_22R,16
cglobal CopyBlock4x4_23R,16

%macro loadparam 1
	mov		esi,[esp+12]		;src
	mov		edi,[esp+12+4]		;dst
	mov		eax,[esp+12+8]		;src pitch
%if %1>0
	mov		edx,8				;dst pitch (fixed for AddBlock)
%else	
	mov		edx,[esp+12+12]		;dst pitch
%endif
%endmacro

%macro loadmask1 0
	mov		ecx,0x01010101
	movd	mm6,ecx
	pcmpeqb mm7,mm7
	punpckldq mm6,mm6
	pxor	mm7,mm6
%endmacro

%macro loadmask4 0
	mov		ecx,0x03030303
	movd	mm6,ecx
	pcmpeqb mm7,mm7
	punpckldq mm6,mm6
	pxor	mm7,mm6
%endmacro

%macro loadmask16 0
	mov		ecx,0x0F0F0F0F
	movd	mm6,ecx
	pcmpeqb mm7,mm7
	punpckldq mm6,mm6
	pxor	mm7,mm6
%endmacro

%macro load1 2
	movd	mm0,[esi+%1]
%if %2>0
	add		esi,eax
%endif
	movq	mm1,mm0
	pand	mm1,mm7
	psrlq	mm1,1
%endmacro

%macro load2 2
	movd	mm2,[esi+%1]
%if %2>0
	add		esi,eax
%endif
	movq	mm3,mm2
	pand	mm3,mm7
	psrlq	mm3,1
%endmacro

%macro load1hv 1
	movd	mm0,[esi]
	movd	mm4,[esi+1]
	add		esi,eax
	movq	mm1,mm0
	movq	mm5,mm4
	pand	mm0,mm6
	pand	mm4,mm6
	pand	mm1,mm7
	pand	mm5,mm7
	psrlq	mm1,%0
	psrlq	mm5,%0
	paddb	mm0,mm4
	paddb	mm1,mm5
%endmacro

%macro load2hv 1
	movd	mm2,[esi]
	movd	mm4,[esi+1]
	add		esi,eax
	movq	mm3,mm2
	movq	mm5,mm4
	pand	mm2,mm6
	pand	mm4,mm6
	pand	mm3,mm7
	pand	mm5,mm7
	psrlq	mm3,%0
	psrlq	mm5,%0
	paddb	mm2,mm4
	paddb	mm3,mm5
%endmacro

%macro avg1 0
	por		mm0,mm2
	pand	mm0,mm6
	paddb	mm0,mm1
	paddb	mm0,mm3
%endmacro

%macro avg2 0
	por		mm2,mm0
	pand	mm2,mm6
	paddb	mm2,mm3
	paddb	mm2,mm1
%endmacro

%macro avground1 0
	pand	mm0,mm2
	pand	mm0,mm6
	paddb	mm0,mm1
	paddb	mm0,mm3
%endmacro

%macro avground2 0
	pand	mm2,mm0
	pand	mm2,mm6
	paddb	mm2,mm3
	paddb	mm2,mm1
%endmacro

%macro save1 0
	movd	[edi],mm0
	add		edi,edx
%endmacro

%macro save2 0
	movd	[edi],mm2
	add		edi,edx
%endmacro

%macro saveadd1 0
	movd	mm4,[edi]
	movq	mm1,mm0
	pand	mm0,mm7
	por		mm1,mm4
	pand	mm4,mm7
	pand	mm1,mm6
	psrlq	mm0,1
	psrlq	mm4,1
	paddb	mm1,mm0
	paddb	mm1,mm4
	movd	[edi],mm1
	add		edi,edx
%endmacro

%macro saveadd2 0
	movd	mm4,[edi]
	movq	mm3,mm2
	pand	mm2,mm7
	por		mm3,mm4
	pand	mm4,mm7
	pand	mm3,mm6
	psrlq	mm2,1
	psrlq	mm4,1
	paddb	mm3,mm2
	paddb	mm3,mm4
	movd	[edi],mm3
	add		edi,edx
%endmacro

%macro CopyBlock4x4_NN 4
ALIGN 16
CopyBlock4x4_%0%1%2:
	push	esi
	push	edi

	loadparam 0
	loadmask16
	
	load1hv 4 
%rep 2
	load2hv 4

	movq	mm4,[%3]
	paddb	mm0,mm2
	paddb	mm1,mm3
	paddb	mm0,mm4		;+7
	pand	mm0,mm7
	psrlq	mm0,2
	paddb	mm0,mm1

	save1

	load1hv 4

	movq	mm4,[%3]
	paddb	mm2,mm0
	paddb	mm3,mm1
	paddb	mm2,mm4		;+7
	pand	mm2,mm7
	psrlq	mm2,2
	paddb	mm2,mm3

	save2
%endrep

	pop		edi
	pop		esi 
	ret 16
%endmacro

ALIGN 16
CopyBlock4x4:
	push	esi
	push	edi
	loadparam 0
	push	ebx
	push	ecx

	mov		ebx,[esi]
	mov		ecx,[esi+eax]
	add		esi,eax
	mov		[edi],ebx
	mov		[edi+edx],ecx
	add		edi,edx
	mov		ebx,[esi+eax]
	mov		ecx,[esi+eax*2]
	mov		[edi+edx],ebx
	mov		[edi+edx*2],ecx

	pop		ecx
	pop		ebx
	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_02:
	push	esi
	push	edi

	loadparam 0
	loadmask1
	
%rep 4
	load1	0,0
	load2	1,1
	avg1
	save1
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_20:
	push	esi
	push	edi

	loadparam 0
	loadmask1
	load1	0,1
%rep 2 
	load2	0,1
	avg1
	save1
	load1	0,1
	avg2
	save2
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_22:
	push	esi
	push	edi

	loadparam 0
	loadmask4
	
	load1hv 2
%rep 2
	load2hv 2

	pcmpeqb mm4,mm4		;-1
	paddb	mm0,mm2
	paddb	mm4,mm4		;-2
	paddb	mm1,mm3
	psubb	mm0,mm4		;+2
	pand	mm0,mm7
	psrlq	mm0,2
	paddb	mm0,mm1

	save1

	load1hv 2

	pcmpeqb mm4,mm4		;-1
	paddb	mm2,mm0
	paddb	mm4,mm4		;-2
	paddb	mm3,mm1
	psubb	mm2,mm4		;+2
	pand	mm2,mm7
	psrlq	mm2,2
	paddb	mm2,mm3

	save2
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_02R:
	push	esi
	push	edi

	loadparam 0
	loadmask1
	
%rep 4
	load1	0,0
	load2	1,1
	avground1
	save1
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_20R:
	push	esi
	push	edi

	loadparam 0
	loadmask1
	load1	0,1
%rep 2 
	load2	0,1
	avground1
	save1
	load1	0,1
	avground2
	save2
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
CopyBlock4x4_22R:
	push	esi
	push	edi

	loadparam 0
	loadmask4
	
	load1hv 2
%rep 2
	load2hv 2

	pcmpeqb mm4,mm4		;-1
	paddb	mm0,mm2
	paddb	mm1,mm3
	psubb	mm0,mm4		;+1
	pand	mm0,mm7
	psrlq	mm0,2
	paddb	mm0,mm1

	save1

	load1hv 2

	pcmpeqb mm4,mm4		;-1
	paddb	mm2,mm0
	paddb	mm3,mm1
	psubb	mm2,mm4		;+1
	pand	mm2,mm7
	psrlq	mm2,2
	paddb	mm2,mm3

	save2
%endrep

	pop		edi
	pop		esi 
	ret 16

ALIGN 16
AddBlock4x4:
	push	esi
	push	edi

	loadparam 1
	loadmask1

%rep 4
	movd	mm0,[esi]
	add		esi,eax
	saveadd1
%endrep

	pop		edi
	pop		esi 
	ret 12

ALIGN 16
AddBlock4x4_02:
	push	esi
	push	edi

	loadparam 1
	loadmask1
	
%rep 4
	load1	0,0
	load2	1,1
	avg1
	saveadd1
%endrep

	pop		edi
	pop		esi 
	ret 12

ALIGN 16
AddBlock4x4_20:
	push	esi
	push	edi

	loadparam 1
	loadmask1
	load1	0,1
%rep 2 
	load2	0,1
	avg1
	saveadd1
	load1	0,1
	avg2
	saveadd2
%endrep

	pop		edi
	pop		esi 
	ret 12

ALIGN 16
AddBlock4x4_22:
	push	esi
	push	edi

	loadparam 1
	loadmask4
	
	load1hv 2
%rep 2
	load2hv 2

	pcmpeqb mm5,mm5		;-1
	paddb	mm0,mm2
	paddb	mm5,mm5		;-2
	paddb	mm1,mm3
	psubb	mm0,mm5		;+=2
	pand	mm0,mm7
	psrlq	mm0,2
	paddb	mm0,mm1

	paddb	mm6,mm5		;0x03-2=0x01
	psubb	mm7,mm5		;0xFD+2=0xFF
	saveadd1
	psubb	mm6,mm5		;restore mask
	paddb	mm7,mm5		;restore mask

	load1hv 2

	pcmpeqb mm5,mm5		;-1
	paddb	mm2,mm0
	paddb	mm5,mm5		;-2
	paddb	mm3,mm1
	psubb	mm2,mm5		;+=2
	pand	mm2,mm7
	psrlq	mm2,2
	paddb	mm2,mm3

	paddb	mm6,mm5		;0x03-2=0x01
	psubb	mm7,mm5		;0xFD+2=0xFF
	saveadd2
	psubb	mm6,mm5		;restore mask
	paddb	mm7,mm5		;restore mask

%endrep

	pop		edi
	pop		esi 
	ret 12

%endif
