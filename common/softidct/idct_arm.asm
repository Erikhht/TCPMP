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
;* $Id: idct_arm.asm 284 2005-10-04 08:54:26Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

	AREA	|.text|, CODE

	EXPORT IDCT_Block4x8
	EXPORT IDCT_Block8x8
	EXPORT IDCT_Block4x8Swap
	EXPORT IDCT_Block8x8Swap

; r6 Block
; r7,r8 must be saved

	macro
	MCol8 $Name,$Rotate,$Pitch

$Name PROC

; r10 = x0
; r4  = x1
; r2  = x2
; r1  = x3
; r3  = x4
; r12 = x5
; r0  = x6
; r5  = x7
; r11 = x8  
; r9  = tmp (x567)

	ldrsh     r4, [r6, #4*$Pitch]
	ldrsh     r0, [r6, #5*$Pitch]
	ldrsh     r12,[r6, #7*$Pitch]
	ldrsh     r5, [r6, #3*$Pitch]
	ldrsh     r2, [r6, #6*$Pitch]
	ldrsh     r1, [r6, #2*$Pitch]
	ldrsh     r3, [r6, #1*$Pitch]
	ldrsh     r10,[r6]
	if $Rotate
	add		  r6,r6,r9
	endif

	orr       r9, r12, r0
	orr       r9, r9, r5
	orr       r11, r9, r2
	orr       r11, r11, r4
	orrs      r11, r11, r1

	bne       $Name.Mode2 
 	cmp       r3, #0
	bne       $Name.Mode1
	if $Rotate=0
	cmp       r10, #0
	beq       $Name.Zero
	endif
	mov       r10, r10, lsl #3
	strh      r10, [r6]
	strh      r10, [r6, #0x10]
	strh      r10, [r6, #0x20]
	strh      r10, [r6, #0x30]
	strh      r10, [r6, #0x40]
	strh      r10, [r6, #0x50]
	strh      r10, [r6, #0x60]
	strh      r10, [r6, #0x70]
$Name.Zero
	mov		pc,lr

$Name.Mode1							;x0,x4
	mov       r11, r3
	mov       r2, #0x8D, 30  ; 0x234 = 564
	orr       r2, r2, #1
	mov       r9, r3
	mul       r2, r11, r2
	mov       r11, #0xB1, 28  ; 0xB10 = 2832
	orr       r11, r11, #9
	mul       r4, r9, r11
	mov       r11, #0x96, 28  ; 0x960 = 2400
	orr       r11, r11, #8
	mul       r5, r9, r11
	mov       r11, #0x19, 26  ; 0x640 = 1600
	mov       r1, r10, lsl #11
	orr       r11, r11, #9
	mul       r0, r3, r11
	add       r1, r1, #0x80  ; 0x80 = 128

	add       r3, r4, r1
	add       r11, r5, r1
	mov       r3, r3, asr #8
	mov       r11, r11, asr #8
	strh      r3, [r6]
	strh      r11, [r6, #0x10]  ; 0x10 = 16

	add       r3, r0, r1
	add       r11, r2, r1
	mov       r3, r3, asr #8
	mov       r11, r11, asr #8
	strh      r3, [r6, #0x20]  ; 0x20 = 32
	strh      r11, [r6, #0x30]  ; 0x30 = 48

	sub       r3, r1, r2
	sub       r11, r1, r0
	mov       r3, r3, asr #8
	mov       r11, r11, asr #8
	strh      r3, [r6, #0x40]  ; 0x40 = 64
	strh      r11, [r6, #0x50]  ; 0x50 = 80

	sub       r3, r1, r5
	sub       r11, r1, r4
	mov       r3, r3, asr #8
	mov       r11, r11, asr #8
	strh      r3, [r6, #0x60]  ; 0x60 = 96
	strh      r11, [r6, #0x70]  ; 0x70 = 112
	mov		pc,lr

$Name.Mode2						;x0,x1,x2,x3
	orrs      r11, r9, r3
	bne       $Name.Mode3
	mov       r3, r10, lsl #11
	add       r3, r3, #128
	mov       r9, #0x45, 28  ; 0x450 = 1104
	add       r5, r3, r4, lsl #11
	add       r11, r2, r1
	orr       r9, r9, #4
	sub       r3, r3, r4, lsl #11
	mul       r4, r11, r9
	mov       r11, #0x3B, 26  ; 0xEC0 = 3776
	orr       r11, r11, #8
	mul       r11, r2, r11
	sub       r2, r4, r11
	mov       r11, #0x62, 28  ; 0x620 = 1568
	mul       r11, r1, r11
	add       r0, r2, r3
	add       r1, r11, r4
	add       r4, r5, r1
	sub       r3, r3, r2
	sub       r5, r5, r1
	mov       r1, r4, asr #8
	mov       r3, r3, asr #8
	mov       r2, r0, asr #8
	mov       r4, r5, asr #8
	strh      r1, [r6,#0x00]
	strh      r2, [r6,#0x10]
	strh      r3, [r6,#0x20]
	strh      r4, [r6,#0x30]
	strh      r4, [r6,#0x40] 
	strh      r3, [r6,#0x50] 
	strh      r2, [r6,#0x60] 
	strh      r1, [r6,#0x70] 
	mov		pc,lr

$Name.Mode3						;x0,x1,x2,x3,x4,x5,x6,x7

	mov     r9, #0x8D, 30  
	orr     r9, r9, #1			;W7
	add     r11, r12, r3
	mul     r11, r9, r11		;x8 = W7 * (x5 + x4)

	mov     r9, #0x8E, 28  
	orr     r9, r9, #4			;W1-W7
	mla     r3, r9, r3, r11		;x4 = x8 + (W1-W7) * x4

	mvn     r9, #0xD40
	eor     r9, r9, #0xD		;-W1-W7
	mla     r12, r9, r12, r11	;x5 = x8 + (-W1-W7) * x5

	mov     r9, #0x96, 28		;
	orr     r9, r9, #8			;W3
	add     r11, r0, r5
	mul     r11, r9, r11		;x8 = W3 * (x6 + x7)
								
	mvn     r9, #0x310
	eor     r9, r9, #0xE		;W5-W3
	mla     r0, r9, r0, r11		;x6 = x8 + (W5-W3) * x6

	mvn     r9, #0xFB0			;-W3-W5
	mla     r5, r9, r5, r11		;x7 = x8 + (-W3-W5) * x7

	mov     r10, r10, lsl #11
	add     r10, r10, #128		;x0 = (x0 << 11) + 128
	add		r11, r10,r4,lsl #11 ;x8 = x0 + (x1 << 11)
	sub		r10, r10,r4,lsl #11 ;x0 = x0 - (x1 << 11)

	mov     r9, #0x45, 28  
	orr     r9, r9, #4			;W6
	add		r4, r1, r2
	mul		r4, r9, r4			;x1 = W6 * (x3 + x2)

	mvn     r9, #0xEC0
	eor     r9, r9, #0x7		;-W2-W6
	mla     r2, r9, r2, r4		;x2 = x1 + (-W2-W6) * x2

	mov     r9, #0x620			;W2-W6
	mla     r1, r9, r1, r4		;x3 = x1 + (W2-W6) * x3

	add		r4, r3, r0			;x1 = x4 + x6
	sub		r3, r3, r0			;x4 -= x6
	add		r0, r12,r5			;x6 = x5 + x7
	sub		r12,r12,r5			;x5 -= x7
	add		r5, r11,r1			;x7 = x8 + x3
	sub		r11,r11,r1			;x8 -= x3
	add		r1, r10,r2			;x3 = x0 + x2
	sub		r10,r10,r2			;x0 -= x2

	add		r9, r3, r12			;x4 + x5
	sub		r3, r3, r12			;x4 - x5
	mov		r12, #181
	mul		r2, r9, r12			;181 * (x4 + x5)
	mul		r9, r3, r12			;181 * (x4 - x5)
	add		r2, r2, #128		;x2 = 181 * (x4 + x5) + 128
	add		r3, r9, #128		;x4 = 181 * (x4 - x5) + 128

	add		r9,r5,r4			
	sub		r5,r5,r4			
	mov		r9,r9,asr #8		;(x7 + x1) >> 8
	mov		r5,r5,asr #8		;(x7 - x1) >> 8
	strh	r9,[r6,#0x00]
	strh	r5,[r6,#0x70]

	add		r9,r1,r2,asr #8
	sub		r1,r1,r2,asr #8			
	mov		r9,r9,asr #8		;(x3 + x2) >> 8
	mov		r1,r1,asr #8		;(x3 - x2) >> 8
	strh	r9,[r6,#0x10]
	strh	r1,[r6,#0x60]

	add		r9,r10,r3,asr #8			
	sub		r10,r10,r3,asr #8			
	mov		r9,r9,asr #8		;(x0 + x4) >> 8
	mov		r10,r10,asr #8		;(x0 - x4) >> 8
	strh	r9,[r6,#0x20]
	strh	r10,[r6,#0x50]

	add		r9,r11,r0			
	sub		r11,r11,r0			
	mov		r9,r9,asr #8		;(x8 + x6) >> 8
	mov		r11,r11,asr #8		;(x8 - x6) >> 8
	strh	r9,[r6,#0x30]
	strh	r11,[r6,#0x40]

	mov		pc,lr
	mend

	MCol8 Col8,0,16
	MCol8 Col8Swap,1,2

; r0 Block[0]
; r6 Block
; r7 Src
; r8 Dst

	ALIGN 16
RowConst PROC

	add     r0, r0, #0x20  ; 0x20 = 32
	cmp     r7, #0
	mov     r3, r0, asr #6
	beq     RowConst_NoSrc
	cmp     r3, #0
	beq		RowConst_Zero
	blt     RowConst_Sub

RowConst_Add
	ldr     r0, CarryMask
	ldr     r2, [r7]
	orr     r3, r3, r3, lsl #8
	orr     r3, r3, r3, lsl #16
	add     r4, r2, r3
	eor     r11, r2, r3
	and     r2, r3, r2
	bic     r11, r11, r4
	orr     r11, r11, r2
	and     r5, r11, r0
	mov     r12, r5, lsl #1
	sub     r10, r4, r12
	sub     r11, r12, r5, lsr #7
	ldr     r2, [r7, #4]
	orr     r11, r11, r10
	str     r11, [r8]
	add     r4, r2, r3
	eor     r11, r2, r3
	and     r2, r3, r2
	bic     r11, r11, r4
	orr     r11, r11, r2
	and     r5, r11, r0
	mov     r12, r5, lsl #1
	sub     r10, r4, r12
	sub     r11, r12, r5, lsr #7
	orr     r11, r11, r10
	str     r11, [r8, #4]
	add		r7, r7, #8			;source stride
	mov		pc,lr

RowConst_Sub
	ldr     r0, CarryMask
	ldr     r2, [r7]
	rsb     r3, r3, #0
	orr     r3, r3, r3, lsl #8
	orr     r3, r3, r3, lsl #16
	mvn		r2, r2
	add     r4, r2, r3
	eor     r11, r2, r3
	and     r2, r3, r2
	bic     r11, r11, r4
	orr     r11, r11, r2
	and     r5, r11, r0
	mov     r12, r5, lsl #1
	sub     r10, r4, r12
	sub     r11, r12, r5, lsr #7
	ldr     r2, [r7, #4]
	orr     r11, r11, r10
	mvn		r11, r11
	str     r11, [r8]
	mvn		r2, r2
	add     r4, r2, r3
	eor     r11, r2, r3
	and     r2, r3, r2
	bic     r11, r11, r4
	orr     r11, r11, r2
	and     r5, r11, r0
	mov     r12, r5, lsl #1
	sub     r10, r4, r12
	sub     r11, r12, r5, lsr #7
	orr     r11, r11, r10
	mvn		r11, r11
	str     r11, [r8, #4]
	add		r7, r7, #8			;source stride
	mov		pc,lr

RowConst_Zero
	ldr     r1, [r7]
	ldr     r2, [r7, #4]
	str     r1, [r8]
	str     r2, [r8, #4]
	add		r7, r7, #8			;source stride
	mov		pc,lr

RowConst_NoSrc
	cmp     r3, #0
	movmi   r3, #0
	cmppl   r3, #255
	movgt   r3, #255
	orr     r3, r3, r3, lsl #8
	orr     r3, r3, r3, lsl #16
	str     r3, [r8]
	str     r3, [r8, #4]
	mov		pc,lr

	ENDP

CarryMask	DCD 0x80808080
W1			DCW	2841                 ; 2048*sqrt(2)*cos(1*pi/16) 
W3			DCW 2408                 ; 2048*sqrt(2)*cos(3*pi/16) 
nW5			DCW 0xF9B7 ;-1609        ; 2048*sqrt(2)*cos(5*pi/16) 
W6			DCW 1108                 ; 2048*sqrt(2)*cos(6*pi/16) 
W7			DCW 565                  ; 2048*sqrt(2)*cos(7*pi/16) 
W2			DCW 2676                 ; 2048*sqrt(2)*cos(2*pi/16) 

; r6 Block
; r7 Src
; r8 Dst

	ALIGN 16
IDCT_Block4x8Swap PROC

	add		r0, r0, #256
	stmdb   sp!, {r0, r2, r4 - r12, lr}  ; r0=BlockEnd r2=DstStride
	sub		r6, r0, #256	;Block
	mov		r7, r3			;Src
	mov	    r8, r1			;Dst

	mov		r9,#128-0*16+0*2
	bl      Col8Swap  
	mov		r9,#128-1*16+1*2
	add     r6, r6, #1*16-0*2-128
	bl      Col8Swap  
	mov		r9,#128-2*16+2*2
	add     r6, r6, #2*16-1*2-128
	bl      Col8Swap  
	mov		r9,#128-3*16+3*2
	add     r6, r6, #3*16-2*2-128
	bl      Col8Swap 
	sub     r6, r6, #6
	b		Row4_Loop

	ALIGN 16
IDCT_Block4x8 PROC

	add		r0, r0, #128
	stmdb   sp!, {r0, r2, r4 - r12, lr}  ; r0=BlockEnd r2=DstStride
	sub		r6, r0, #128	;Block
	mov		r7, r3			;Src
	mov	    r8, r1			;Dst

	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8 
	sub     r6, r6, #6

Row4_Loop

	ldrsh   r4, [r6, #4]		;x3
	ldrsh   r5, [r6, #6]		;x7
	ldrsh   r3, [r6, #2]		;x4
	ldrsh   r0, [r6]			;x0

	orr     r11, r5, r4
	orrs    r11, r11, r3
	bne     Row4_NoConst

	bl		RowConst
	b		Row4_Next

Row4_NoConst
	cmp     r7, #0

	ldrsh	r10, W7
	ldrsh	r11, W1
	mov		r2, #4
	add     r0, r0, #32
	mov     r0, r0, lsl #8		;x0
	mla     r14, r3, r10, r2	;x5 = x4 * W7 + 4
	ldrsh	r10, W3
	mla     r3, r11, r3, r2		;x4 = x4 * W1 + 4
	mov     r14, r14, asr #3	;x5 >>= 3
	ldrsh	r11, nW5
	mla     r12, r5, r10, r2	;x6 = x7 * W3 + 4
	mov     r3, r3, asr #3		;x4 >>= 3
	ldrsh	r10, W6
	mla     r5, r11, r5, r2		;x7 = x7 * -W5 + 4
	ldrsh	r11, W2
	add     r9, r3, r12, asr #3	;x1 = x4 + (x6 >> 3)
	sub     r3, r3, r12, asr #3 ;x4 = x4 - (x6 >> 3)
	mla     r12, r4, r10, r2	;x2 = x3 * W6 + 4
	mla     r4, r11, r4, r2		;x3 = x3 * W2 + 4
	add     r2, r14, r5, asr #3	;x6 = x5 + (x7 >> 3)
	sub     r5, r14, r5, asr #3 ;x5 = x5 - (x7 >> 3)
	add     r14, r0, r4, asr #3 ;x7 = x0 + (x3 >> 3)
	sub     r4, r0, r4, asr #3	;x8 = x0 - (x3 >> 3)
	add     r10, r0, r12, asr #3;x3 = x0 + (x2 >> 3)
	sub     r0, r0, r12, asr #3	;x0 = x0 - (x2 >> 3)
	add     r1, r5, r3			
	mov     r11, #181
	mul     r12, r1, r11		;x2 = 181 * (x5 + x4)
	sub     r3, r3, r5
	mul     r1, r3, r11			;x4 = 181 * (x4 - x5)
	add     r12, r12, #128		;x2 += 128
	add     r3, r1, #128		;x4 += 128
	add     r1, r14, r9			;x5 = x7 + x1
	sub     r5, r14, r9			;x1 = x7 - x1
	add     r11, r10, r12, asr #8 ;x7 = x3 + (x2 >> 8)
	sub     r14, r10, r12, asr #8 ;x2 = x3 - (x2 >> 8)
	add     r9, r0, r3, asr #8	;x3 = x0 + (x4 >> 8)
	sub     r3, r0, r3, asr #8  ;x4 = x0 - (x4 >> 8)
	add     r12, r4, r2			;x0 = x8 + x6
	sub     r4,  r4, r2			;x6 = x8 - x6

	beq     Row4_NoSrc

	ldrb    r0, [r7]
	ldrb    r2, [r7, #7]
	ldrb    r10, [r7, #1]
	add     r1, r0, r1, asr #14
	add     r5, r2, r5, asr #14
	add     r11, r10, r11, asr #14
	ldrb    r2, [r7, #6]
	ldrb    r0, [r7, #2]
	ldrb    r10, [r7, #5]
	add     r14, r2, r14, asr #14
	add     r9, r0, r9, asr #14
	ldrb    r0, [r7, #3]
	ldrb    r2, [r7, #4]
	add     r3, r10, r3, asr #14
	add     r12, r0, r12, asr #14
	add     r4, r2, r4, asr #14
	add		r7, r7, #8			;source stride

Row4_Sat
	orr     r0, r5, r14
	orr     r0, r0, r4
	orr     r0, r0, r1
	orr     r0, r0, r12
	orr     r0, r0, r11
	orr     r0, r0, r9
	orr     r0, r0, r3
	bics    r0, r0, #0xFF  ; 0xFF = 255
	beq     Row4_Write

	mov		r0, #0xFFFFFF00

	tst     r1, r0
	movne	r1, #0xFF
	movmi	r1, #0x00

	tst     r11, r0
	movne	r11, #0xFF
	movmi	r11, #0x00

	tst     r9, r0
	movne	r9, #0xFF
	movmi	r9, #0x00

	tst     r12, r0
	movne	r12, #0xFF
	movmi	r12, #0x00

	tst     r4, r0
	movne	r4, #0xFF
	movmi	r4, #0x00

	tst     r3, r0
	movne	r3, #0xFF
	movmi	r3, #0x00

	tst     r14, r0
	movne	r14, #0xFF
	movmi	r14, #0x00

	tst     r5, r0
	movne	r5, #0xFF
	movmi	r5, #0x00

Row4_Write
	strb    r1, [r8]
	strb    r11,[r8, #1]
	strb    r9, [r8, #2]
	strb    r12,[r8, #3]
	strb    r4, [r8, #4]
	strb    r3, [r8, #5]
	strb    r14,[r8, #6]
	strb    r5, [r8, #7]

Row4_Next
	ldr		r2, [sp, #4]	;DstStride
	ldr		r1, [sp, #0]	;BlockEnd

	add		r6,r6,#16		;Block += 16
	add		r8,r8,r2		;Dst += DstStride

	cmp		r6,r1
	bne		Row4_Loop

	ldmia   sp!, {r0,r2,r4 - r12, pc}  

Row4_NoSrc

	mov     r5, r5, asr #14
	mov     r14, r14, asr #14
	mov     r12, r12, asr #14
	mov     r1, r1, asr #14
	mov     r11, r11, asr #14
	mov     r9, r9, asr #14
	mov     r3, r3, asr #14
	mov     r4, r4, asr #14

	b		Row4_Sat
	ENDP

; r6 Block
; r7 Src
; r8 Dst

	ALIGN 16
IDCT_Block8x8Swap PROC

	add		r0, r0, #256
	stmdb   sp!, {r0, r2, r4 - r12, lr}  ; r0=BlockEnd r2=DstStride
	sub		r6, r0, #256	;Block
	mov		r7, r3			;Src
	mov	    r8, r1			;Dst

	mov		r9,#128-0*16+0*2
	bl      Col8Swap
	mov		r9,#128-1*16+1*2
	add     r6, r6, #1*16-0*2-128
	bl      Col8Swap  
	mov		r9,#128-2*16+2*2
	add     r6, r6, #2*16-1*2-128
	bl      Col8Swap 
	mov		r9,#128-3*16+3*2
	add     r6, r6, #3*16-2*2-128
	bl      Col8Swap 
	mov		r9,#128-4*16+4*2
	add     r6, r6, #4*16-3*2-128
	bl      Col8Swap 
	mov		r9,#128-5*16+5*2
	add     r6, r6, #5*16-4*2-128
	bl      Col8Swap 
	mov		r9,#128-6*16+6*2
	add     r6, r6, #6*16-5*2-128
	bl      Col8Swap 
	mov		r9,#128-7*16+7*2
	add     r6, r6, #7*16-6*2-128
	bl      Col8Swap 
	sub     r6, r6, #14
	b		Row8_Loop

	ALIGN 16
IDCT_Block8x8 PROC

	add		r0, r0, #128
	stmdb   sp!, {r0, r2, r4 - r12, lr}  ; r0=BlockEnd r2=DstStride
	sub		r6, r0, #128	;Block
	mov		r7, r3			;Src
	mov	    r8, r1			;Dst

	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8  
	add     r6, r6, #2
	bl      Col8 
	sub     r6, r6, #14

Row8_Loop

	ldrsh   r0, [r6]			;x0
	ldrsh   r3, [r6, #2]		;x4
	ldrsh   r4, [r6, #4]		;x3
	ldrsh   r5, [r6, #6]		;x7
	ldrsh   r9, [r6, #8]		;x1
	ldrsh   r2, [r6, #10]		;x6
	ldrsh   r14,[r6, #12]		;x2
	ldrsh   r1, [r6, #14]		;x5

	orr     r11, r3, r4
	orr	    r11, r11, r5
	orr     r11, r11, r9
	orr     r11, r11, r2
	orr     r11, r11, r14
	orrs    r11, r11, r1
	bne     Row8_NoConst

	bl		RowConst
	b		Row8_Next

_W3			DCW 2408                 ; 2048*sqrt(2)*cos(3*pi/16) 
_W6			DCW 1108                 ; 2048*sqrt(2)*cos(6*pi/16) 
_W7			DCW 565                  ; 2048*sqrt(2)*cos(7*pi/16) 

_W1_nW7			DCW 2276
_nW1_nW7		DCW 0xF2B2 ;-3406
_W5_nW3			DCW 0xFCE1 ;-799
_nW2_nW6		DCW 0xF138 ;-3784

	ALIGN	4

Row8_NoConst
	cmp     r7, #0

	add     r0, r0, #32
	ldrsh	r10, _W7
	mov     r0, r0, lsl #11		;x0 = (x0 + 32) << 11
	ldrsh	r12, _W1_nW7
	add		r11,r3,r1			
	mul		r11,r10,r11			;x8 = W7 * (x4 + x5)
	ldrsh	r10, _nW1_nW7
	mla     r3, r12, r3, r11	;x4 = x8 + (W1-W7) * x4
	ldrsh	r12, _W3
	mla     r1, r10, r1, r11	;x5 = x8 + (-W1-W7) * x5
	ldrsh	r10, _W5_nW3	
	add		r11,r2,r5			;x6 + x7
	mul		r11,r12,r11			;x8 = W3 * (x6 + x7)
	mvn     r12, #0xFB0			;-W3-W5
	mla		r2,r10,r2,r11		;x6 = x8 + (W5-W3) * x6
	ldrsh	r10, _W6
	mla		r5,r12,r5,r11		;x7 = x8 + (-W3-W5) * x7
	ldrsh	r12, _nW2_nW6 
	add		r11, r0, r9, lsl #11;x8 = x0 + (x1 << 11)
	sub		r0, r0, r9, lsl #11	;x0 = x0 - (x1 << 11)
	add		r9, r4, r14
	mul		r9, r10, r9			;x1 = W6 * (x3 + x2)
	mov     r10, #0x620			;W2-W6
	mla		r14, r12, r14, r9	;x2 = x1 + (-W2-W6) * x2
	mov		r12, #181
	mla		r4, r10, r4, r9		;x3 = x1 + (W2-W6) * x3
	add		r9, r3, r2			;x1 = x4 + x6
	sub		r3, r3, r2			;x4 = x4 - x6
	add		r2, r1, r5			;x6 = x5 + x7
	sub		r1, r1, r5			;x5 = x5 - x7
	add		r5, r11, r4			;x7 = x8 + x3
	sub		r11, r11, r4		;x8 = x8 - x3
	add		r4, r0, r14			;x3 = x0 + x2
	sub		r0, r0, r14			;x0 = x0 - x2
	add		r3, r3, #4			;
	add		r14, r3, r1			;x2 = x4 + x5 + 4
	sub		r3, r3, r1			;x4 = x4 - x5 + 4
	mov		r10, #16
	mov		r14, r14, asr #3
	mov		r3, r3, asr #3
	mla		r14, r12, r14, r10	;x2 = 181 * ((x4 + x5 + 4) >> 3) + 16
	mla		r3, r12, r3, r10	;x4 = 181 * ((x4 - x5 + 4) >> 3) + 16

	add		r1, r5, r9			;x5 = x7 + x1
	sub		r9, r5, r9			;x1 = x7 - x1
	add		r5, r4, r14, asr #5	;x7 = x3 + (x2 >> 5)
	sub		r14,r4, r14, asr #5	;x2 = x3 - (x2 >> 5)
	add		r4, r0, r3, asr #5	;x3 = x0 + (x4 >> 5)
	sub		r3, r0, r3, asr #5	;x4 = x0 - (x4 >> 5)
	add		r0, r11, r2			;x0 = x8 + x6
	sub		r2, r11, r2			;x6 = x8 - x6

	beq     Row8_NoSrc

	ldrb    r10, [r7]
	ldrb    r12, [r7, #7]
	ldrb    r11, [r7, #1]
	add     r1, r10, r1, asr #17
	add     r9, r12, r9, asr #17
	add     r5, r11, r5, asr #17
	ldrb    r10, [r7, #6]
	ldrb    r12, [r7, #2]
	ldrb    r11, [r7, #5]
	add     r14, r10, r14, asr #17
	add     r4, r12, r4, asr #17
	ldrb    r10, [r7, #3]
	ldrb    r12, [r7, #4]
	add     r3, r11, r3, asr #17
	add     r0, r10, r0, asr #17
	add     r2, r12, r2, asr #17
	add		r7, r7, #8			;source stride

Row8_Sat
	orr     r10, r1, r9
	orr     r10, r10, r5
	orr     r10, r10, r14
	orr     r10, r10, r4
	orr     r10, r10, r3
	orr     r10, r10, r0
	orr     r10, r10, r2
	bics    r10, r10, #0xFF  ; 0xFF = 255
	beq     Row8_Write

	mov		r10, #0xFFFFFF00

	tst     r1, r10
	movne	r1, #0xFF
	movmi	r1, #0x00

	tst     r9, r10
	movne	r9, #0xFF
	movmi	r9, #0x00

	tst     r5, r10
	movne	r5, #0xFF
	movmi	r5, #0x00

	tst     r14, r10
	movne	r14, #0xFF
	movmi	r14, #0x00

	tst     r4, r10
	movne	r4, #0xFF
	movmi	r4, #0x00

	tst     r3, r10
	movne	r3, #0xFF
	movmi	r3, #0x00

	tst     r0, r10
	movne	r0, #0xFF
	movmi	r0, #0x00

	tst     r2, r10
	movne	r2, #0xFF
	movmi	r2, #0x00

Row8_Write
	strb    r1, [r8]
	strb    r5, [r8, #1]
	strb    r4, [r8, #2]
	strb    r0, [r8, #3]
	strb    r2, [r8, #4]
	strb    r3, [r8, #5]
	strb    r14,[r8, #6]
	strb    r9, [r8, #7]

Row8_Next
	ldr		r2, [sp, #4]	;DstStride
	ldr		r1, [sp, #0]	;BlockEnd

	add		r6,r6,#16		;Block += 16
	add		r8,r8,r2		;Dst += DstStride

	cmp		r6,r1
	bne		Row8_Loop

	ldmia   sp!, {r0,r2,r4 - r12, pc}  

Row8_NoSrc

	mov     r1, r1, asr #17
	mov     r9, r9, asr #17
	mov     r5, r5, asr #17
	mov     r14, r14, asr #17
	mov     r4, r4, asr #17
	mov     r3, r3, asr #17
	mov     r0, r0, asr #17
	mov     r2, r2, asr #17

	b		Row8_Sat
	ENDP

	END
