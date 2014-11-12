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
;* $Id: blit_mmx.asm 323 2005-11-01 20:52:32Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

BITS 32
SECTION .data

OFFSET_COL	equ 8

align 8
const_b254		dd	0xfefefefe,0xfefefefe

SECTION .text

%macro func 2
%define %1 _%1@%2
global %1
align 16
%1:
%endmacro

%macro _prefetch 2
%if s%1=smmx2
	prefetchnta [%2+256]
%elif s%1=s3dnow
	prefetch [%2+256]
%endif
%endmacro

; caps, in, src, in2, src2, tmp1, tmp2
%macro avg 7
%if s%1=smmx
	movq	%6,%3
	movq	%7,%5
	pxor	%6,%2
	pxor	%7,%4
	por		%2,%6
	por		%4,%7
	pand	%6,[const_b254]
	pand	%7,[const_b254]
	psrlq	%6,1
	psrlq	%7,1
	psubb	%2,%6
	psubb	%4,%7

%elif s%1=smmx2
	pavgb	%2,%3
	pavgb	%4,%5
%elif s%1=s3dnow
	pavgusb	%2,%3	
	pavgusb	%4,%5
%else
	%error	not supported caps %1
%endif
%endmacro

%macro save 3
%if s%1=smmx2
 	movntq %2,%3
%else
	movq %2,%3
%endif
%endmacro

%define savg 2	
%define smmx2 3
%define s3dnow 4
%define snone 5
%define smmx 6
%define srgb32 7
%define srgb24 8
%define sbgr32 9
%define sbgr24 10
%define syuy2 11
%define srgb 12
%define srgb16 13
%define sbgr16 14
%define syuy2_color 15

;eax tmp/Col
;ebx SrcPitch/2
;ecx SrcPtr[1]
;edx StrPtr[2]
;esi SrcPtr[0]
;edi DstPtr
;ebp Width/2

;---------------------------------------------------------------------

; caps, type, mode
%macro blit_pack_single_yuy2 3
	mov al,[esi]
	add esi,2
	mov [edi],al
	mov al,[ecx]
%if %2=2
	add al,[ecx+ebx]
	rcr al,1
%endif	
	add ecx,1
	mov [edi+1],al
	mov al,[esi-1]
	add edi,4
	mov [edi-2],al
	mov al,[edx]
%if %2=2
	add al,[edx+ebx]
	rcr al,1
%endif	
	add edx,1
	mov [edi-1],al
%endmacro

; caps, type, mode, i
%macro blit_pack_block_yuy2 4
	movq mm0,[esi-16]
	movq mm2,[ecx]
	movq mm3,[edx]
	movq mm1,[esi-8]

%if %2=2
	avg %1,mm2,[ecx+ebx],mm3,[edx+ebx],mm4,mm5
%endif

	_prefetch %1,esi

	movq mm5,mm0
	movq mm4,mm2
	punpcklbw mm2,mm3
	punpckhbw mm4,mm3
	movq mm3,mm1
    punpcklbw mm0,mm2
    punpcklbw mm1,mm4
    punpckhbw mm5,mm2
    punpckhbw mm3,mm4

%if %2=2
	_prefetch %1,ecx+ebx
%endif

	save %1,[edi],mm0
	add ecx,8
	save %1,[edi+8],mm5
	add edx,8
	save %1,[edi+16],mm1
	add esi,16
	save %1,[edi+24],mm3
	add edi,32

%if %2=2
	_prefetch %1,edx+ebx
%endif
%endmacro

;---------------------------------------------------------------------

; caps, type, mode
%macro blit_pack_single_yuy2_color 3

	movzx eax,word [esi]
	add esi,2
	movd mm0,eax
	mov al,[ecx]
%if %2=2
	add al,[ecx+ebx]
	rcr al,1
%endif	
	add ecx,1
	movd mm2,eax
	mov al,[edx]
%if %2=2
	add al,[edx+ebx]
	rcr al,1
%endif	
	add edx,1
	movd mm3,eax
	mov eax,[esp]

	pxor mm7,mm7
	punpcklbw mm0,mm7
	punpcklbw mm2,mm7
	punpcklbw mm3,mm7
	psllw  mm0,5
	psllw  mm2,5
	psllw  mm3,5
	pmulhw mm0,[eax+8*0]
	pmulhw mm2,[eax+8*2]
	pmulhw mm3,[eax+8*4]
	paddsw mm0,[eax+8*1]
	paddsw mm2,[eax+8*3]
	paddsw mm3,[eax+8*5]
	packuswb mm0,mm0
	packuswb mm2,mm2
	packuswb mm3,mm3

	punpcklbw mm2,mm3
    punpcklbw mm0,mm2
	movd [edi],mm0
	add edi,4

%endmacro

; caps, type, mode, i
%macro blit_pack_block_yuy2_color 4
	movq mm0,[esi-16]
	movq mm2,[ecx]
	movq mm3,[edx]
	movq mm1,[esi-8]

%if %2=2
	avg %1,mm2,[ecx+ebx],mm3,[edx+ebx],mm4,mm5
%endif

	_prefetch %1,esi

	movq mm6,[eax+8*0]
	pxor mm7,mm7
	movq mm4,mm0
	movq mm5,mm1
	punpcklbw mm0,mm7
	punpcklbw mm1,mm7
	psllw  mm0,5
	psllw  mm1,5
	pmulhw mm0,mm6
	pmulhw mm1,mm6
	punpckhbw mm4,mm7
	punpckhbw mm5,mm7
	movq mm7,[eax+8*1]
	psllw  mm4,5
	psllw  mm5,5
	pmulhw mm4,mm6
	pmulhw mm5,mm6
	paddsw mm0,mm7
	paddsw mm1,mm7
	paddsw mm4,mm7
	paddsw mm5,mm7
	packuswb mm0,mm4
	packuswb mm1,mm5

%if %2=2
	_prefetch %1,ecx+ebx
%endif

	pxor mm7,mm7
	movq mm4,mm2
	movq mm5,mm3
	punpcklbw mm2,mm7
	punpcklbw mm3,mm7
	punpckhbw mm4,mm7
	punpckhbw mm5,mm7
	movq mm6,[eax+8*2]
	movq mm7,[eax+8*4]
	psllw  mm2,5
	psllw  mm3,5
	pmulhw mm2,mm6
	pmulhw mm3,mm7
	psllw  mm4,5
	psllw  mm5,5
	pmulhw mm4,mm6
	pmulhw mm5,mm7
	movq mm6,[eax+8*3]
	movq mm7,[eax+8*5]
	paddsw mm2,mm6
	paddsw mm3,mm7
	paddsw mm4,mm6
	paddsw mm5,mm7
	packuswb mm2,mm4
	packuswb mm3,mm5

	movq mm5,mm0
	movq mm4,mm2
	punpcklbw mm2,mm3
	punpckhbw mm4,mm3
	movq mm3,mm1
    punpcklbw mm0,mm2
    punpcklbw mm1,mm4
    punpckhbw mm5,mm2
    punpckhbw mm3,mm4

%if %2=2
	_prefetch %1,edx+ebx
%endif

	save %1,[edi],mm0
	add ecx,8
	save %1,[edi+8],mm5
	add edx,8
	save %1,[edi+16],mm1
	add esi,16
	save %1,[edi+24],mm3
	add edi,32

%endmacro

;---------------------------------------------------------------------

;input
; mm0 y(8)
; mm1 u(4)
; mm2 v(4)
; mm7 0
;output
; mm0 g
; mm1 b
; mm2 r

%macro yuv_rgb 0

	movq mm5,mm0
	movq mm3,[eax+8]
	punpcklbw mm0,mm7
	movq mm4,[eax+16+8]
	punpckhbw mm5,mm7
	movq mm6,[eax+32+8]
	punpcklbw mm1,mm7
	paddsw mm0,mm3
	punpcklbw mm2,mm7
	paddsw mm5,mm3
	movq mm7,[eax]
	psllw mm0,7
	paddsw mm1,mm4
	psllw mm5,7
	paddsw mm2,mm6
	psllw mm1,7
	psllw mm2,7
	pmulhw mm0,mm7
	movq mm6,[eax+16]
	pmulhw mm5,mm7
	movq mm7,[eax+32]
	movq mm3,mm1
	movq mm4,mm2
	punpckldq mm1,mm1
	punpckldq mm2,mm2
	pmulhw mm1,mm6
	pmulhw mm2,mm7
	punpckhdq mm3,mm3
	punpckhdq mm4,mm4
	pmulhw mm3,mm6
	pmulhw mm4,mm7
	movq mm6,mm1
	movq mm7,mm2
	punpcklwd mm1,mm1
	punpcklwd mm2,mm2
	punpckhwd mm6,mm6
	punpckhwd mm7,mm7
	paddsw mm1,mm0		;y*y_mul+u*u_mul_lo
	paddsw mm2,mm0      ;y*y_mul           +v*v_mul_lo
	paddsw mm0,mm6
	paddsw mm0,mm7      ;y*y_mul+u*u_mul_hi+v*v_mul_hi
	psraw mm1,4
	psraw mm2,4
	psraw mm0,4
	movq mm6,mm3
	movq mm7,mm4
	punpcklwd mm3,mm3
	punpcklwd mm4,mm4
	punpckhwd mm6,mm6
	punpckhwd mm7,mm7
	paddsw mm3,mm5		;y*y_mul+u*u_mul_lo
	paddsw mm4,mm5      ;y*y_mul           +v*v_mul_lo
	paddsw mm5,mm6
	paddsw mm5,mm7      ;y*y_mul+u*u_mul_hi+v*v_mul_hi
	psraw mm3,4
	psraw mm4,4
	psraw mm5,4
	packuswb mm1,mm3    ;b
	packuswb mm2,mm4	;r
	packuswb mm0,mm5    ;g

%endmacro

; caps, type, mode
%macro blit_pack_single_rgb 3
	movzx eax,word [esi]
	add esi,2
	pxor mm7,mm7
	movd mm0,eax
%if %2=2
	mov al,[ecx]
	add al,[ecx+ebx]
	rcr al,1
%else
	movzx eax,byte [ecx]
%endif	
	add ecx,1
	movd mm1,eax
%if %2=2
	mov al,[edx]
	add al,[edx+ebx]
	rcr al,1
%else
	movzx eax,byte [edx]
%endif	
	add edx,1
	movd mm2,eax
	mov eax,[esp]
	yuv_rgb

	pxor mm7,mm7
%if (s%3=srgb32) || (s%3=srgb24)
	punpcklbw mm2,mm7	;0r0r0r0r (0)
	punpcklbw mm1,mm0	;gbgbgbgb (0)
	punpcklwd mm1,mm2	;0rgb0rgb (0)
%else
	punpcklbw mm1,mm7	;0b0b0b0b (0)
	punpcklbw mm2,mm0	;grgrgrgr (0)
	punpcklwd mm2,mm1	;0bgr0bgr (0)
%endif
	
%if s%3=srgb32
	save %1,[edi],mm1
	add edi,8
%endif
%if s%3=sbgr32
	save %1,[edi],mm2
	add edi,8
%endif
%if (s%3=srgb24) || (s%3=sbgr24)
%if s%3=srgb24
	movq mm2,mm1
%else
	movq mm1,mm2
%endif
	punpckhdq mm2,mm7	;00000bgr (1)
	punpckldq mm1,mm7	;00000bgr (0)
	psllq mm2,24		;00bgr000 (1)
	por mm2,mm1			;00bgrbgr
	movd eax,mm2
	psrlq mm2,32
	mov [edi],eax
	movd eax,mm2
	mov [edi+4],ax
	add edi,6
%endif

%endmacro

; caps, type, mode, i
%macro blit_pack_block_rgb 4
	movq mm0,[esi-16]
	movd mm1,[ecx]
	pxor mm7,mm7
	movd mm2,[edx]
%if %2=2
	avg %1,mm1,[ecx+ebx],mm2,[edx+ebx],mm4,mm5
%endif
%if (%4=0)
	_prefetch %1,esi
%endif
	yuv_rgb

%if (%2=2) 
%if (%4=0)
	_prefetch %1,edx+ebx
%else
	_prefetch %1,ecx+ebx
%endif
%endif

	pxor mm7,mm7
%if (s%3=srgb32) || (s%3=srgb24)
	movq mm3,mm2
	movq mm4,mm1
	punpcklbw mm2,mm7	;0r0r0r0r (0)
	punpckhbw mm3,mm7	;0r0r0r0r (1)
	punpcklbw mm1,mm0	;gbgbgbgb (0)
	punpckhbw mm4,mm0	;gbgbgbgb (1)
	movq mm7,mm1
	movq mm0,mm4
	punpcklwd mm1,mm2	;0rgb0rgb (0)
	punpckhwd mm7,mm2	;0rgb0rgb (1)
	punpcklwd mm4,mm3	;0rgb0rgb (2)
	punpckhwd mm0,mm3	;0rgb0rgb (3)
%else
	movq mm3,mm1
	movq mm4,mm2
	punpcklbw mm1,mm7	;0b0b0b0b (0)
	punpckhbw mm3,mm7	;0b0b0b0b (1)
	punpcklbw mm2,mm0	;grgrgrgr (0)
	punpckhbw mm4,mm0	;grgrgrgr (1)
	movq mm7,mm2
	movq mm0,mm4
	punpcklwd mm2,mm1	;0bgr0bgr (0)
	punpckhwd mm7,mm1	;0bgr0bgr (1)
	punpcklwd mm4,mm3	;0bgr0bgr (2)
	punpckhwd mm0,mm3	;0bgr0bgr (3)
	movq mm1,mm2
%endif

%if (s%3=srgb32) || (s%3=sbgr32)
	save %1,[edi],mm1
	add ecx,4
	save %1,[edi+8],mm7
	add edx,4
	save %1,[edi+16],mm4
	add esi,8
	save %1,[edi+24],mm0
	add edi,32
%else
	pxor mm6,mm6
	movq mm3,mm7		;0ddd0ccc
	movq mm5,mm4		;0fff0eee
	pslld mm0,8 		;hhh0ggg0
	psrld mm5,8			;00ffxxxx
	punpckhdq mm6,mm1	;0bbb0000
	punpckhdq mm5,mm0	;hhh000ff
	psllq mm0,32		;ggg00000
	pslld mm3,16		;xxxxcc00
	psrlq mm6,8			;00bbb000
	punpckldq mm1,mm3	;cc000aaa
	punpckldq mm3,mm4	;0eeexxxx
	pslld mm4,24		;f000xxxx
	psrlq mm0,24		;000ggg00
	por mm1,mm6			;ccbbbaaa
	punpckldq mm6,mm7	;0cccxxxx
	pslld mm7,8			;ddd0xxxx
	por mm5,mm0			;hhhgggff
	psrld mm6,16		;000cxxxx
	punpckhdq mm7,mm3	;0eeeddd0
	punpckhdq mm6,mm4	;f000000c
	save %1,[edi],mm1
	por mm7,mm6			;feeedddc
	add ecx,4
	add edx,4
	save %1,[edi+8],mm7
	add esi,8
	save %1,[edi+16],mm5
	add edi,24
%endif

%endmacro

;---------------------------------------------------------------------

;caps, mode, type, align, repeat, name
%macro blit_pack_line 6

%%small:
	add ebp,ebp
%%smallstart:
%if (s%6=srgb) || (s%6=syuy2_color)
	push eax
%endif
%%smallloop:
	blit_pack_single_%6 %1,%3,%2
	sub ebp,2
	jne %%smallloop
%if (s%6=srgb) || (s%6=syuy2_color)
	pop eax
%endif
	ret

blit_i420_%2_%3_%1:
	cmp ebp,8+%4
	jl %%small

%%large4:
	lea ebp,[esi+ebp*2]
%if %4==4
	test edi,4
	je %%noalign4
%if (s%6=srgb) || (s%6=syuy2_color)
	push eax
%endif
	blit_pack_single_%6 %1,%3,%2
%if (s%6=srgb) || (s%6=syuy2_color)
	pop eax
%endif
%%noalign4:
%endif

	add esi,16
%%move4:
%assign i 0
%rep %5
	blit_pack_block_%6 %1,%3,%2,i
%assign i i+1 
%endrep
	cmp esi,ebp
	jbe %%move4
	sub esi,16
	sub ebp,esi
	je  %%end
	jmp %%smallstart
%%end:
	ret

%endmacro

%macro blit_pack_inner 3

	sub dword [esp+24+28],2
	jle %%noloop3

%%loop3:
	call blit_i420_%2_1_%1
	mov ebp,[esp] ;-Width/2
%if %3==6
	lea edi,[edi+ebp*4]
	lea edi,[edi+ebp*2]
%else
	lea edi,[edi+ebp*%3]
%endif
	lea esi,[esi+ebp*2]
	add ecx,ebp
	add edx,ebp
	mov ebp,[esp+4] ;Width/2

	add edi,[esp+12+28] ;DstPitch
	lea esi,[esi+ebx*2]

	call blit_i420_%2_2_%1
	mov ebp,[esp] ;-Width/2
%if %3==6
	lea edi,[edi+ebp*4]
	lea edi,[edi+ebp*2]
%else
	lea edi,[edi+ebp*%3]
%endif
	lea esi,[esi+ebp*2]
	add ecx,ebp
	add edx,ebp
	mov ebp,[esp+4] ;Width/2

	add edi,[esp+12+28] ;DstPitch
	lea esi,[esi+ebx*2] ;
	add ecx,ebx
	add edx,ebx
	sub dword [esp+24+28],2
	jg %%loop3

%%noloop3
	call blit_i420_%2_1_%1
	mov ebp,[esp] ;-Width/2
%if %3==6
	lea edi,[edi+ebp*4]
	lea edi,[edi+ebp*2]
%else
	lea edi,[edi+ebp*%3]
%endif
	lea esi,[esi+ebp*2]
	add ecx,ebp
	add edx,ebp
	mov ebp,[esp+4] ;Width/2

	add edi,[esp+12+28] ;DstPitch
	lea esi,[esi+ebx*2]

	cmp dword [esp+24+28],0
	jne %%nolast3
	call blit_i420_%2_1_%1
%%nolast3:

	add esp,8
	emms
	pop ebp
	pop ebx
	pop	edi
	pop esi
	ret 32

%endmacro

;caps, mode, align, block, name, color
%macro blit_pack 6

blit_pack_line %1,%2,1,%3,%4,%5
blit_pack_line %1,%2,2,%3,%4,%5

%if %6
blit_pack_line %1,%2_color,1,%3,%4,%5_color
blit_pack_line %1,%2_color,2,%3,%4,%5_color
%endif

func blit_i420_%2_%1,32
	push esi
	push edi
	push ebx
	push ebp

	mov ebx,[esp+16+20] ;SrcPitch
	mov edi,[esp+4+20]  ;DstPtr
	mov esi,[esp+8+20]  ;SrcPtr
	mov ebp,[esp+20+20] ;Width
	mov edi,[edi]
	sar ebx,1
	sar ebp,1
	xor eax,eax
	push ebp
	sub eax,ebp
	push eax
	mov eax,[esp+0+28]	;this
	mov ecx,[esi+4]		;u
	mov edx,[esi+8]		;v
	mov esi,[esi]		;y
	add eax,OFFSET_COL

%if %6
	cmp dword [eax],2048+(2048<<16)
	jne %%color
	cmp dword [eax+8],0
	jne %%color
	cmp dword [eax+8*2],2048+(2048<<16)
	jne %%color
	cmp dword [eax+8*3],0
	jne %%color
	cmp dword [eax+8*4],2048+(2048<<16)
	jne %%color
	cmp dword [eax+8*5],0
	jne %%color
	jmp %%nocolor
%%color:
	blit_pack_inner %1,%2_color,%3
%%nocolor
%endif

	blit_pack_inner %1,%2,%3

%endmacro

%macro blit_plane_rgb 3

func blit_%2_%3_%1,32
	push esi
	push edi
	push ebx
	push ebp

	mov eax,[esp+12+20] ;DstPitch
	mov ebx,[esp+16+20] ;SrcPitch

	mov edi,[esp+4+20]  ;DstPtr
	mov esi,[esp+8+20]  ;SrcPtr
	mov ebp,[esp+20+20] ;Width
	mov edx,[esp+24+20] ;Height
	mov edi,[edi]		
	mov esi,[esi]
%if (s%2=srgb32) || (s%2=sbgr32)
	shl ebp,2
%endif
%if (s%2=srgb24) || (s%2=sbgr24)
	lea ebp,[ebp+ebp*2]
%endif
%if (s%2=srgb16) || (s%2=sbgr16)
	shl ebp,2
%endif

%%loopy6:
	_prefetch %1,esi
	mov ecx,ebp
	cmp ebp,32+4
	jl %%small6

	push ebp
	add ebp,esi
%if (s%2=srgb16) || (s%2=sbgr16)
	test edi,2
	je %%noalignw6
	movsw
%%noalignw6:
%endif
	test edi,4
	je %%noalignd6
	movsd
%%noalignd6:
	add esi,32
%%move6:
	movq mm0,[esi-32]
	movq mm1,[esi-24]
	movq mm2,[esi-16]
	movq mm3,[esi-8]
	add esi,32
	_prefetch %1,esi
	save %1,[edi+0],mm0
	save %1,[edi+8],mm1
	save %1,[edi+16],mm2
	save %1,[edi+24],mm3
	add edi,32
	cmp esi,ebp
	jbe %%move6

	sub esi,32
	mov ecx,ebp
	pop ebp
	sub ecx,esi

%%small6:
	rep movsb
	add edi,eax
	add esi,ebx
	sub edi,ebp
	sub esi,ebp
	dec edx
	jne %%loopy6

	emms
	pop ebp
	pop ebx
	pop	edi
	pop esi
	ret 32

%endmacro

%macro blit 1

;This,DstPtr,SrcPtr,DstPitch,SrcPitch,Width,Height,Src2SrcLast

; edi dst
; esi src
; eax dstpitch
; ebx srcpitch
; ebp width
; edx height
; ecx mul,ofs

blit_plane_yuv_%1:
	cmp word [ecx],2048
	jne %%jmpcolor
	cmp word [ecx+8],0
	je  %%loopy
%%jmpcolor:
	jmp %%color

%%loopy:
	_prefetch %1,esi
	mov ecx,ebp
	cmp ebp,32+4
	jl %%small

	push ebp
	add ebp,esi
	test edi,2
	je %%noalignw
	movsw
%%noalignw:
	test edi,4
	je %%noalignd
	movsd
%%noalignd:
	add esi,32
%%move:
	movq mm0,[esi-32]
	movq mm1,[esi-24]
	movq mm2,[esi-16]
	movq mm3,[esi-8]
	add esi,32
	_prefetch %1,esi
	save %1,[edi+0],mm0
	save %1,[edi+8],mm1
	save %1,[edi+16],mm2
	save %1,[edi+24],mm3
	add edi,32
	cmp esi,ebp
	jbe %%move

	sub esi,32
	mov ecx,ebp
	pop ebp
	sub ecx,esi

%%small:
	rep movsb
	add edi,eax
	add esi,ebx
	sub edi,ebp
	sub esi,ebp
	dec edx
	jne %%loopy
	ret

%%color:
	movq mm5,[ecx]
	movq mm6,[ecx+8]
	pxor mm7,mm7

%%loopy2:
	_prefetch %1,esi
	cmp ebp,16+4
	jge %%large2
	mov ecx,ebp
	jmp %%small2

%%large2:
	push ebp
	add ebp,esi
	test edi,4
	je %%noalignd2

	movd mm0,[esi]
	add esi,4
	punpcklbw mm0,mm7
	psllw  mm0,5
	pmulhw mm0,mm5
	paddsw mm0,mm6
	packuswb mm0,mm0
	movd [edi],mm0
	add edi,4

%%noalignd2:
	add esi,16
%%move2:
	movq mm0,[esi-16]
	movq mm2,[esi-8]
	_prefetch %1,esi
	add esi,16
	movq mm1,mm0
	movq mm3,mm2
	punpcklbw mm0,mm7
	punpcklbw mm2,mm7
	psllw  mm0,5
	psllw  mm2,5
	pmulhw mm0,mm5
	pmulhw mm2,mm5
	punpckhbw mm1,mm7
	punpckhbw mm3,mm7
	psllw  mm1,5
	psllw  mm3,5
	pmulhw mm1,mm5
	pmulhw mm3,mm5
	paddsw mm0,mm6
	paddsw mm2,mm6
	paddsw mm1,mm6
	paddsw mm3,mm6
	packuswb mm0,mm1
	packuswb mm2,mm3
	save %1,[edi+0],mm0
	save %1,[edi+8],mm2
	add edi,16
	cmp esi,ebp
	jbe %%move2

	sub esi,16
	mov ecx,ebp
	pop ebp
	sub ecx,esi

%%small2:
	or ecx,ecx
	jle %%nosmall2
	push eax
%%smallloop2:
	movzx eax,byte [esi]
	add esi,1
	movd mm0,eax
	punpcklbw mm0,mm7
	psllw  mm0,5
	pmulhw mm0,mm5
	paddsw mm0,mm6
	packuswb mm0,mm0
	movd eax,mm0
	mov [edi],al
	add edi,1
	dec ecx
	jne %%smallloop2
	pop eax
%%nosmall2:

	add edi,eax
	add esi,ebx
	sub edi,ebp
	sub esi,ebp
	dec edx
	jne %%loopy2
	ret

func blit_i420_i420_%1,32
	push esi
	push edi
	push ebx
	push ebp

	mov eax,[esp+12+20] ;DstPitch
	mov ebx,[esp+16+20] ;SrcPitch

	mov ecx,[esp+0+20]  ;this
	mov edi,[esp+4+20]  ;DstPtr
	mov esi,[esp+8+20]  ;SrcPtr
	mov ebp,[esp+20+20] ;Width
	mov edx,[esp+24+20] ;Height
	mov edi,[edi]		;y
	mov esi,[esi]		;y
	add ecx,OFFSET_COL
	call blit_plane_yuv_%1

	sar eax,1
	sar ebx,1

	mov ecx,[esp+0+20]  ;this
	mov edi,[esp+4+20]  ;DstPtr
	mov esi,[esp+8+20]  ;SrcPtr
	mov ebp,[esp+20+20] ;Width
	mov edx,[esp+24+20] ;Height
	sar ebp,1
	sar edx,1
	mov edi,[edi+4]		;u
	mov esi,[esi+4]		;u
	add ecx,OFFSET_COL+8*2
	call blit_plane_yuv_%1

	mov ecx,[esp+0+20]  ;this
	mov edi,[esp+4+20]  ;DstPtr
	mov esi,[esp+8+20]  ;SrcPtr
	mov ebp,[esp+20+20] ;Width
	mov edx,[esp+24+20] ;Height
	sar ebp,1
	sar edx,1
	mov edi,[edi+8]		;v
	mov esi,[esi+8]		;v
	add ecx,OFFSET_COL+8*4
	call blit_plane_yuv_%1

	emms
	pop ebp
	pop ebx
	pop	edi
	pop esi
	ret 32

blit_plane_rgb %1,rgb32,rgb32
blit_plane_rgb %1,rgb24,rgb24
blit_plane_rgb %1,rgb16,rgb16
;blit_plane_rgb %1,rgb32,rgb24
;blit_plane_rgb %1,rgb24,rgb32
;blit_plane_rgb %1,rgb32,bgr32
;blit_plane_rgb %1,rgb24,bgr24
;blit_plane_rgb %1,rgb32,bgr24
;blit_plane_rgb %1,rgb24,bgr32

blit_pack %1,yuy2,4,1,yuy2,1
blit_pack %1,rgb32,8,2,rgb,0
blit_pack %1,rgb24,6,2,rgb,0
blit_pack %1,bgr32,8,2,rgb,0
blit_pack %1,bgr24,6,2,rgb,0

%endmacro

blit mmx2
blit mmx
blit 3dnow

