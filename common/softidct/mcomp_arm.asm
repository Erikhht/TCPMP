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
;* $Id: mcomp_arm.asm 327 2005-11-04 07:09:17Z picard $
;*
;* The Core Pocket Media Player
;* Copyright (c) 2004-2005 Gabor Kovacs
;*
;*****************************************************************************

;R0 src
;R1 dst
;R2 srcpitch
;R3 dstpitch

	AREA	|.text|, CODE

  macro 
  PreLoad $ARM5,$Pos
    if $ARM5>0
	  if $Pos >= 0
  	    if $Pos > 0
	      pld [r0,r2,lsl #1]
	      add r0,r0,#8
	      pld [r0,r2,lsl #1]
	      sub r0,r0,#8
	    else
	      pld [r0,r2,lsl #1]
	      add r0,r0,#4
	      pld [r0,r2,lsl #1]
	      sub r0,r0,#4
	    endif
	  else
	    pld [r0,r2,lsl #1]
	    add r0,r0,#7
	    pld [r0,r2,lsl #1]
	    sub r0,r0,#7
	  endif
	endif
  mend

  macro 
  PreLoad2Init $ARM5,$Pos
    if $ARM5>0
	  if $Pos >= 0
  	    if $Pos > 0
	      add r10,r2,r2
	      add r10,r10,#8
	    else
	      add r10,r2,r2
	      add r10,r10,#4
	    endif
	  else
	      add r10,r2,r2
	      add r10,r10,#7
	  endif
	endif
  mend

  macro 
  PreLoad2 $ARM5,$Pos
	if $ARM5>0
      pld [r0,r2,lsl #1]
      pld [r0,r10]
	endif
  mend

  macro 
  CopyBuild $Name,$Sub,$Round,$Add,$Fast,$ARM5

	align 16
	export $Name
$Name proc
	if $ARM5>0
	add ip,r2,#7
	pld [r0,r2]
	pld [r0,#7]
	endif
	stmdb	sp!, {r4 - r11, lr}
	if $ARM5>0
	pld [r0,ip]
	pld [r0]
	endif
	if $Fast>0
	movs	r4,r0,lsl #30
	beq		$Name.L00
	cmps	r4,#0x80000000
	beq		$Name.L10
	bhi		$Name.L11
$Name.L01
	bic		r0,r0,#3
	$Sub	$Name.8,8,$Round,$Add,$ARM5
$Name.L10	
	bic		r0,r0,#3
	$Sub	$Name.16,16,$Round,$Add,$ARM5
$Name.L11	
	bic		r0,r0,#3
	$Sub	$Name.24,24,$Round,$Add,$ARM5
$Name.L00	
	$Sub	$Name.0,0,$Round,$Add,$ARM5
	else
	$Sub	$Name.s,-1,$Round,$Add,$ARM5
	endif
	endp
  mend

;------------------------------------------
;COPYBLOCK
;------------------------------------------

  macro
  CopyBlockRow $Pos
	if $Pos > 0
	  ldr	r5,[r0,#8]
	  ldr	r6,[r0,#4]
	  ldr	r4,[r0],r2
		
	  mov	r5,r5,lsl #32-$Pos
	  orr	r5,r5,r6,lsr #$Pos
	  mov	r4,r4,lsr #$Pos
	  orr	r4,r4,r6,lsl #32-$Pos
	else
	  ldr	r5,[r0,#4]
	  ldr	r4,[r0],r2
	endif

	str		r5,[r1,#4]
	str		r4,[r1],r3
  mend

  macro
  CopyBlock $Id, $Pos, $Round, $Add, $ARM5

	PreLoad2Init $ARM5,$Pos
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 
    PreLoad2 $ARM5,$Pos
	CopyBlockRow $Pos 

	ldmia	sp!, {r4 - r11, pc}
  mend

;------------------------------------------
;COPYBLOCKM
;------------------------------------------

  macro
  CopyBlockMRow $Pos
	if $Pos > 0
	  ldr	r14,[r0,#16]
	  ldr	r6,[r0,#12]
	  ldr	r12,[r0,#8]
		
	  mov	r14,r14,lsl #32-$Pos
	  orr	r14,r14,r6,lsr #$Pos
	  mov	r12,r12,lsr #$Pos
	  orr	r12,r12,r6,lsl #32-$Pos

	  ldr	r5,[r0,#8]
	  ldr	r6,[r0,#4]
	  ldr	r4,[r0],r2
		
	  mov	r5,r5,lsl #32-$Pos
	  orr	r5,r5,r6,lsr #$Pos
	  mov	r4,r4,lsr #$Pos
	  orr	r4,r4,r6,lsl #32-$Pos
	else
	  ldr	r5,[r0,#4]
	  ldr	r12,[r0,#8]
	  ldr	r14,[r0,#12]
	  ldr	r4,[r0],r2
	endif

	str		r5,[r1,#4]
	str		r12,[r1,#8]
	str		r14,[r1,#12]
	str		r4,[r1],r3
  mend

  macro
  CopyBlockM $Id, $Pos, $Round, $Add, $ARM5

	PreLoad2Init $ARM5,$Pos
	mov		r11,#16
$Id.Loop
    PreLoad2 $ARM5,$Pos
	CopyBlockMRow $Pos
	subs r11,r11,#1
	bne  $Id.Loop

	ldmia	sp!, {r4 - r11, pc}
  mend

;------------------------------------------
;COPYBLOCK16x16: no aligment!, only used in Copy()
;------------------------------------------

  macro
  CopyBlock16x16Row
	ldr		r7,[r0,#12]
	ldr		r6,[r0,#8]
	ldr		r5,[r0,#4]
	ldr		r4,[r0],r2
	str		r7,[r1,#12]
	str		r6,[r1,#8]
	str		r5,[r1,#4]
	str		r4,[r1],r3
  mend

  macro
  CopyBlock16x16 $Id, $Pos, $Round, $Add, $ARM5

	mov		r11,#15

$Id.Loop
    pld [r0,r2,lsl #1]
	CopyBlock16x16Row
	subs r11,r11,#1 
	bne  $Id.Loop

	;unroll last (no preload needed)
	CopyBlock16x16Row
	ldmia	sp!, {r4 - r11, pc}
  mend

;------------------------------------------
;COPYBLOCK8x8: no aligment!, only used in Copy()
;------------------------------------------

  macro
  CopyBlock8x8 $Id, $Pos, $Round, $Add, $ARM5

	mov		r11,#3

$Id.Loop
    pld		[r0,r2,lsl #1]
	ldr		r7,[r0,#4]
	ldr		r6,[r0],r2
    pld		[r0,r2,lsl #1]
	ldr		r5,[r0,#4]
	ldr		r4,[r0],r2
	str		r7,[r1,#4]
	str		r6,[r1],r3
	str		r5,[r1,#4]
	str		r4,[r1],r3
	subs	r11,r11,#1 
	bne		$Id.Loop

	;unroll last (no preload needed)
	ldr		r7,[r0,#4]
	ldr		r6,[r0],r2
	ldr		r5,[r0,#4]
	ldr		r4,[r0],r2
	str		r7,[r1,#4]
	str		r6,[r1],r3
	str		r5,[r1,#4]
	str		r4,[r1],r3
	ldmia	sp!, {r4 - r11, pc}
  mend

;------------------------------------------
;ADDBLOCK
;------------------------------------------

  macro
  AddBlockRow $Pos

	if $Pos < 0

	ldrb	r4,[r0]
	ldrb	r6,[r0,#1]
	ldrb	r5,[r0,#4]
	ldrb	r7,[r0,#5]
	orr		r4,r4,r6,lsl #8
	ldrb	r6,[r0,#2]
	orr		r5,r5,r7,lsl #8
	ldrb	r7,[r0,#6]
	orr		r4,r4,r6,lsl #16
	ldrb	r6,[r0,#3]
	orr		r5,r5,r7,lsl #16
	ldrb	r7,[r0,#7]
	orr		r4,r4,r6,lsl #24
	add		r0,r0,r2
	orr		r5,r5,r7,lsl #24

	else
	if $Pos > 0
	  ldr	r5,[r0,#8]
	  ldr	r6,[r0,#4]
	  ldr	r4,[r0],r2
		
	  mov	r5,r5,lsl #32-$Pos
	  orr	r5,r5,r6,lsr #$Pos
	  mov	r4,r4,lsr #$Pos
	  orr	r4,r4,r6,lsl #32-$Pos
	else
	  ldr	r5,[r0,#4]
	  ldr	r4,[r0],r2
	endif
	endif

	ldr		r7,[r1,#4]
	ldr		r6,[r1]
	and		r9,r12,r5,lsr #1
	and		r8,r12,r4,lsr #1
	orr		r5,r7,r5
	orr		r4,r6,r4
	and		r7,r12,r7,lsr #1
	and		r6,r12,r6,lsr #1
	add		r7,r7,r9
	add		r6,r6,r8
	and		r5,r5,r14
	and		r4,r4,r14
	add		r7,r7,r5
	add		r6,r6,r4
	str		r7,[r1,#4]
	str		r6,[r1],#8
  mend

  macro
  AddBlock $Id, $Pos, $Round, $Add, $ARM5

	PreLoad2Init $ARM5,$Pos
	ldr		r14,$Id.Mask
	mov		r11,#8
	mvn		r12,r14,lsl #7

$Id.Loop
    PreLoad2 $ARM5,$Pos
	AddBlockRow $Pos
	subs r11,r11,#1 
	bne  $Id.Loop

	ldmia	sp!, {r4 - r11, pc}
$Id.Mask dcd 0x01010101
  mend

;------------------------------------------
; COPYBLOCKHOR
;------------------------------------------

  macro
  LoadHorRow $Id, $Pos

    ; result is r4,r5 and r8,r9 (one pixel to the right)
    ; r6,r7 can be used

	if $Pos < 0

	ldrb	r4,[r0]
	ldrb	r6,[r0,#1]
	ldrb	r5,[r0,#4]
	ldrb	r7,[r0,#5]
	orr		r4,r4,r6,lsl #8
	ldrb	r6,[r0,#2]
	orr		r5,r5,r7,lsl #8
	ldrb	r7,[r0,#6]
	orr		r4,r4,r6,lsl #16
	ldrb	r6,[r0,#3]
	orr		r5,r5,r7,lsl #16
	ldrb	r7,[r0,#7]
	orr		r4,r4,r6,lsl #24
	ldrb	r6,[r0,#8]
	orr		r5,r5,r7,lsl #24

	mov		r8,r4,lsr #8
	mov		r9,r5,lsr #8
	orr		r8,r8,r5,lsl #24
	orr		r9,r9,r6,lsl #24

	add		r0,r0,r2

	else

    ldr		r5,[r0,#4]
    ldr		r6,[r0,#8]
    ldr		r4,[r0],r2

    if $Pos+8 < 32
	  mov	r9,r5,lsr #$Pos+8
	  orr	r9,r9,r6,lsl #32-$Pos-8
	  mov	r8,r4,lsr #$Pos+8
	  orr	r8,r8,r5,lsl #32-$Pos-8
    else
	  mov	r8,r5
	  mov	r9,r6
    endif

	if $Pos > 0
	  mov	r4,r4,lsr #$Pos
	  mov	r6,r6,lsl #32-$Pos
	  orr	r4,r4,r5,lsl #32-$Pos
	  orr	r5,r6,r5,lsr #$Pos
	endif
	endif
  mend

  macro
  CopyHorRow $Id, $Pos, $Round, $Add

;r14 01010101
;r12 7f7f7f7f

	LoadHorRow	$Id,$Pos

	and		r6,r12,r4,lsr #1
	and		r7,r12,r5,lsr #1

	if $Round
	  and	r4,r4,r8
	  and	r5,r5,r9
	else
	  orr	r4,r4,r8
	  orr	r5,r5,r9
	endif

	and		r8,r12,r8,lsr #1
	and		r9,r12,r9,lsr #1

	and		r4,r4,r14
	and		r5,r5,r14

	add		r4,r4,r6
	add		r5,r5,r7
	add		r4,r4,r8
	add		r5,r5,r9
	
	if $Add
	  ldr	r7,[r1,#4]
	  ldr	r6,[r1]
	  and	r9,r12,r5,lsr #1
	  and	r8,r12,r4,lsr #1
	  orr	r5,r7,r5
	  orr	r4,r6,r4
	  and	r7,r12,r7,lsr #1
	  and	r6,r12,r6,lsr #1
	  add	r7,r7,r9
	  add	r6,r6,r8
	  and	r5,r5,r14
	  and	r4,r4,r14
	  add	r7,r7,r5
	  add	r6,r6,r4
	  str	r7,[r1,#4]
	  str	r6,[r1],#8
	else
	  str	r5,[r1,#4]
	  str	r4,[r1],r3
	endif
  mend

  macro
  CopyBlockHor $Id, $Pos, $Round, $Add, $ARM5

	PreLoad2Init $ARM5,$Pos
	ldr		r14,$Id.Mask
	mov		r11,#8
	mvn		r12,r14,lsl #7

$Id.Loop
    PreLoad2 $ARM5,$Pos
	CopyHorRow $Id,$Pos,$Round,$Add
	subs r11,r11,#1
	bne  $Id.Loop

	ldmia	sp!, {r4 - r11, pc}
$Id.Mask dcd 0x01010101
  mend

;------------------------------------------
; COPYBLOCKVER
;------------------------------------------

  macro
  LoadVerRow $Id, $Pos, $Parity
  if $Parity
    ; result is r8,r9 (r10=r8>>1,r11=r9>>1) 
    ; r10,r11 can be used

	if $Pos < 0
 	  ldrb	r8,[r0]
	  ldrb	r10,[r0,#1]
	  ldrb	r9,[r0,#4]
	  ldrb	r11,[r0,#5]
	  orr	r8,r8,r10,lsl #8 
	  ldrb	r10,[r0,#2]
	  orr	r9,r9,r11,lsl #8
	  ldrb	r11,[r0,#6]
	  orr	r8,r8,r10,lsl #16
	  ldrb	r10,[r0,#3]
	  orr	r9,r9,r11,lsl #16
	  ldrb	r11,[r0,#7]
	  orr	r8,r8,r10,lsl #24
	  add	r0,r0,r2
	  orr	r9,r9,r11,lsl #24
	else
    if $Pos > 0
	  ldr	r9,[r0,#8]
	  ldr	r10,[r0,#4]
	  ldr	r8,[r0],r2

	  mov	r9,r9,lsl #32-$Pos
	  orr	r9,r9,r10,lsr #$Pos
	  mov	r8,r8,lsr #$Pos
	  orr	r8,r8,r10,lsl #32-$Pos
    else
	  ldr	r9,[r0,#4]
	  ldr	r8,[r0],r2
    endif
	endif
	and		r11,r12,r9,lsr #1
	and		r10,r12,r8,lsr #1
  else
    ; result is r4,r5 (r6=r4>>1,r7=r5>>1) 
    ; r6,r7 can be used

	if $Pos < 0
 	  ldrb	r4,[r0]
	  ldrb	r6,[r0,#1]
	  ldrb	r5,[r0,#4]
	  ldrb	r7,[r0,#5]
	  orr	r4,r4,r6,lsl #8
	  ldrb	r6,[r0,#2]
	  orr	r5,r5,r7,lsl #8
	  ldrb	r7,[r0,#6]
	  orr	r4,r4,r6,lsl #16
	  ldrb	r6,[r0,#3]
	  orr	r5,r5,r7,lsl #16
	  ldrb	r7,[r0,#7]
	  orr	r4,r4,r6,lsl #24
	  add	r0,r0,r2
	  orr	r5,r5,r7,lsl #24
	else
    if $Pos > 0
	  ldr	r5,[r0,#8]
	  ldr	r6,[r0,#4]
	  ldr	r4,[r0],r2

	  mov	r5,r5,lsl #32-$Pos
	  orr	r5,r5,r6,lsr #$Pos
	  mov	r4,r4,lsr #$Pos
	  orr	r4,r4,r6,lsl #32-$Pos
    else
	  ldr	r5,[r0,#4]
	  ldr	r4,[r0],r2
    endif
	endif
	and		r7,r12,r5,lsr #1
	and		r6,r12,r4,lsr #1
  endif
  mend

  macro
  CopyVerRow $Id, $Pos, $Parity, $Round, $Add

;r14 01010101
;r12 7f7f7f7f

	LoadVerRow $Id,$Pos,$Parity

    if $Parity
	  if $Round
	    and	r4,r4,r8
	    and	r5,r5,r9
	  else
	    orr	r4,r4,r8
	    orr	r5,r5,r9
	  endif
	  and	r4,r4,r14
	  and	r5,r5,r14

	  add	r4,r4,r6
	  add	r5,r5,r7

  	  add	r4,r4,r10
	  add	r5,r5,r11

	  if $Add
	    ldr	r7,[r1,#4]
	    ldr	r6,[r1]
	    and	r3,r12,r5,lsr #1
		orr r5,r7,r5
	    and	r7,r12,r7,lsr #1
		add r7,r7,r3
	    and	r3,r12,r4,lsr #1
		orr r4,r6,r4
	    and	r6,r12,r6,lsr #1
		add r6,r6,r3
		and r5,r5,r14
		and r4,r4,r14
		add r5,r5,r7
		add r4,r4,r6
		ldr	r7,[sp]		;end src for loop compare
	    str	r5,[r1,#4]
	    str	r4,[r1],#8
	  else
		ldr	r7,[sp]		;end src for loop compare
	    str	r5,[r1,#4]
	    str	r4,[r1],r3
	  endif
	else
	  if $Round
	    and	r8,r8,r4
	    and	r9,r9,r5
	  else
	    orr	r8,r8,r4
	    orr	r9,r9,r5
	  endif
	  and	r8,r8,r14
	  and	r9,r9,r14

	  add	r8,r8,r10
	  add	r9,r9,r11
  	  add	r8,r8,r6
	  add	r9,r9,r7

	  if $Add
	    ldr	r11,[r1,#4]
	    ldr	r10,[r1]
	    and	r3,r12,r9,lsr #1
		orr r9,r11,r9
	    and	r11,r12,r11,lsr #1
		add r11,r11,r3
	    and	r3,r12,r8,lsr #1
		orr r8,r10,r8
	    and	r10,r12,r10,lsr #1
		add r10,r10,r3
		and r9,r9,r14
		and r8,r8,r14
		add r11,r11,r9
		add r10,r10,r8
	    str	r11,[r1,#4]
	    str	r10,[r1],#8
	  else
	    str	r9,[r1,#4]
	    str	r8,[r1],r3
	  endif
	endif
  mend


  macro
  CopyBlockVer $Id, $Pos, $Round, $Add, $ARM5

	sub		sp,sp,#4
	add		r4,r0,r2,lsl #3
	add		r4,r4,r2
	str		r4,[sp]		;end src

	ldr		r14,$Id.Mask
	mvn		r12,r14,lsl #7

    PreLoad $ARM5,$Pos
	LoadVerRow $Id,$Pos,1
$Id.Loop
    PreLoad $ARM5,$Pos
	CopyVerRow $Id,$Pos,0,$Round,$Add
    PreLoad $ARM5,$Pos
	CopyVerRow $Id,$Pos,1,$Round,$Add

	cmp		r0,r7
	bne		$Id.Loop
	add		sp,sp,#4
	ldmia	sp!, {r4 - r11, pc}
$Id.Mask dcd 0x01010101
  mend

;------------------------------------------
; COPYBLOCKHORVER
;------------------------------------------

; load needs r2,r3 for temporary (r2 is restored from stack)

  macro
  LoadHorVerRow $Id, $Pos, $Parity
  if $Parity

	;read result r4,r5 and r2,r3 (one pixel to right)
	;r6,r7 can be used

    if $Pos<0

	ldrb	r4,[r0]
	ldrb	r6,[r0,#1]
	ldrb	r5,[r0,#4]
	ldrb	r7,[r0,#5]
	orr		r4,r4,r6,lsl #8
	ldrb	r6,[r0,#2]
	orr		r5,r5,r7,lsl #8
	ldrb	r7,[r0,#6]
	orr		r4,r4,r6,lsl #16
	ldrb	r6,[r0,#3]
	orr		r5,r5,r7,lsl #16
	ldrb	r7,[r0,#7]
	orr		r4,r4,r6,lsl #24
	ldrb	r6,[r0,#8]
	orr		r5,r5,r7,lsl #24
	add		r0,r0,r2

	mov		r2,r4,lsr #8
	mov		r3,r5,lsr #8
	orr		r2,r2,r5,lsl #24
	orr		r3,r3,r6,lsl #24

	else
    ldr		r5,[r0,#4]
    ldr		r6,[r0,#8]
    ldr		r4,[r0],r2

    if $Pos+8 < 32
	  mov	r3,r5,lsr #$Pos+8
	  orr	r3,r3,r6,lsl #32-$Pos-8
	  mov	r2,r4,lsr #$Pos+8
	  orr	r2,r2,r5,lsl #32-$Pos-8
    else
	  mov	r2,r5
	  mov	r3,r6
    endif

	if $Pos > 0
	  mov	r4,r4,lsr #$Pos
	  mov	r6,r6,lsl #32-$Pos
	  orr	r4,r4,r5,lsl #32-$Pos
	  orr	r5,r6,r5,lsr #$Pos
	endif
	endif

	and		r6,r2,r14
	and		r2,r12,r2,lsr #2
	and		r7,r4,r14
	and		r4,r12,r4,lsr #2
	add		r4,r4,r2
	add		r6,r6,r7

	and		r2,r3,r14
	and		r3,r12,r3,lsr #2
	and		r7,r5,r14
	and		r5,r12,r5,lsr #2
	add		r5,r5,r3
	add		r7,r2,r7
  else
	;read result r8,r9 and r2,r3 (one pixel to right)
	;r10,r11 can be used

    if $Pos<0

	ldrb	r8,[r0]
	ldrb	r10,[r0,#1]
	ldrb	r9,[r0,#4]
	ldrb	r11,[r0,#5]
	orr		r8,r8,r10,lsl #8
	ldrb	r10,[r0,#2]
	orr		r9,r9,r11,lsl #8
	ldrb	r11,[r0,#6]
	orr		r8,r8,r10,lsl #16
	ldrb	r10,[r0,#3]
	orr		r9,r9,r11,lsl #16
	ldrb	r11,[r0,#7]
	orr		r8,r8,r10,lsl #24
	ldrb	r10,[r0,#8]
	orr		r9,r9,r11,lsl #24
	add		r0,r0,r2

	mov		r2,r8,lsr #8
	mov		r3,r9,lsr #8
	orr		r2,r2,r9,lsl #24
	orr		r3,r3,r10,lsl #24

	else
    ldr		r9,[r0,#4]
    ldr		r10,[r0,#8]
    ldr		r8,[r0],r2

    if $Pos+8 < 32
	  mov	r3,r9,lsr #$Pos+8
	  orr	r3,r3,r10,lsl #32-$Pos-8
	  mov	r2,r8,lsr #$Pos+8
	  orr	r2,r2,r9,lsl #32-$Pos-8
    else
	  mov	r2,r9
	  mov	r3,r10
    endif

	if $Pos > 0
	  mov	r8,r8,lsr #$Pos
	  mov	r10,r10,lsl #32-$Pos
	  orr	r8,r8,r9,lsl #32-$Pos
	  orr	r9,r10,r9,lsr #$Pos
	endif
	endif

	and		r10,r2,r14
	and		r2,r12,r2,lsr #2
	and		r11,r8,r14
	and		r8,r12,r8,lsr #2
	add		r8,r8,r2
	add		r10,r10,r11

	and		r2,r3,r14
	and		r3,r12,r3,lsr #2
	and		r11,r9,r14
	and		r9,r12,r9,lsr #2
	add		r9,r9,r3
	add		r11,r2,r11
  endif
	ldr		r2,[sp]
  mend

  macro
  CopyHorVerRow $Id, $Pos, $Parity, $Round, $Add

;r14 03030303
;r12 3f3f3f3f

	LoadHorVerRow $Id,$Pos,$Parity

	if $Round
	  and r3,r14,r14,lsr #1		;0x01010101
	else
	  and r3,r14,r14,lsl #1		;0x02020202
	endif
    if $Parity
	  add	r8,r8,r4
	  add	r9,r9,r5
	  add	r10,r10,r6
	  add	r11,r11,r7

	  add	r10,r10,r3
	  add	r11,r11,r3
	  and	r10,r14,r10,lsr #2
	  and	r11,r14,r11,lsr #2

	  if $Add
	    add	r8,r8,r10 
	    add	r9,r9,r11
		orr	r12,r12,r12,lsl #1  ;0x7F7F7F7F
	    ldr	r11,[r1,#4]
	    ldr	r10,[r1]
	    and	r3,r12,r9,lsr #1
		orr r9,r11,r9
	    and	r11,r12,r11,lsr #1
		add r11,r11,r3
	    and	r3,r12,r8,lsr #1
		orr r8,r10,r8
	    and	r10,r12,r10,lsr #1
		add r10,r10,r3
		and r3,r14,r14,lsr #1 ;0x01010101
		mvn	r12,r14,lsl #6    ;restore r12
		and r9,r9,r3
		and r8,r8,r3
		add r11,r11,r9
		add r10,r10,r8
	    ldr	r3,[sp,#4]	;end src for loop compare
	    str	r11,[r1,#4]
	    str	r10,[r1],#8
	  else
	    add	r8,r8,r10
	    ldr	r10,[sp,#8]  ;dstpitch
	    add	r9,r9,r11
	    ldr	r3,[sp,#4]	;end src for loop compare
	    str	r9,[r1,#4]
	    str	r8,[r1],r10
	  endif
	else
	  add	r4,r4,r8
	  add	r5,r5,r9
	  add	r6,r6,r10
	  add	r7,r7,r11

	  add	r6,r6,r3
	  add	r7,r7,r3
	  and	r6,r14,r6,lsr #2
	  and	r7,r14,r7,lsr #2

	  if $Add
	    add	r4,r4,r6
	    add	r5,r5,r7
		orr	r12,r12,r12,lsl #1  ;0x7F7F7F7F
	    ldr	r7,[r1,#4]
	    ldr	r6,[r1]
	    and	r3,r12,r5,lsr #1
		orr r5,r7,r5
	    and	r7,r12,r7,lsr #1
		add r7,r7,r3
	    and	r3,r12,r4,lsr #1
		orr r4,r6,r4
	    and	r6,r12,r6,lsr #1
		add r6,r6,r3
		and r3,r14,r14,lsr #1 ;0x01010101
		mvn	r12,r14,lsl #6    ;restore r12
		and r5,r5,r3
		and r4,r4,r3
		add r7,r7,r5
		add r6,r6,r4
	    str	r7,[r1,#4]
	    str	r6,[r1],#8
	  else
	    ldr	r3,[sp,#8]  ;dstpitch
	    add	r4,r4,r6
	    add	r5,r5,r7
	    str	r5,[r1,#4]
	    str	r4,[r1],r3
	  endif
	endif
  mend

  macro
  CopyBlockHorVer $Id, $Pos, $Round, $Add, $ARM5
	sub		sp,sp,#12
	add		r4,r0,r2,lsl #3
	add		r4,r4,r2
	str		r2,[sp]		;srcpitch
	str		r4,[sp,#4]	;end src
	str		r3,[sp,#8]	;dstpitch

	ldr		r14,$Id.Mask
	mvn		r12,r14,lsl #6
    PreLoad $ARM5,$Pos
	LoadHorVerRow $Id,$Pos,1
$Id.Loop
    PreLoad $ARM5,$Pos
	CopyHorVerRow $Id,$Pos,0,$Round,$Add
    PreLoad $ARM5,$Pos
	CopyHorVerRow $Id,$Pos,1,$Round,$Add
	cmp		r0,r3
	bne		$Id.Loop

	add		sp,sp,#12
	ldmia	sp!, {r4 - r11, pc}
$Id.Mask dcd 0x03030303
  mend

;---------------------------------------------------
; general unaligned copy (use preload)

	CopyBuild CopyBlock8x8,CopyBlock8x8,0,0,0,1
	CopyBuild CopyBlock16x16,CopyBlock16x16,0,0,0,1

;---------------------------------------------------
; smaller versions without preload
;
;	CopyBuild CopyBlock,CopyBlock,0,0,1,0
;	CopyBuild CopyBlockVer,CopyBlockVer,0,0,0,0
;	CopyBuild CopyBlockHor,CopyBlockHor,0,0,0,0
;	CopyBuild CopyBlockHorVer,CopyBlockHorVer,0,0,0,0
;	CopyBuild CopyBlockVerRound,CopyBlockVer,1,0,0,0
;	CopyBuild CopyBlockHorRound,CopyBlockHor,1,0,0,0
;	CopyBuild CopyBlockHorVerRound,CopyBlockHorVer,1,0,0,0
;
;	CopyBuild AddBlock,AddBlock,0,1,0,0
;	CopyBuild AddBlockVer,CopyBlockVer,0,1,0,0
;	CopyBuild AddBlockHor,CopyBlockHor,0,1,0,0
;	CopyBuild AddBlockHorVer,CopyBlockHorVer,0,1,0,0
;
;---------------------------------------------------
; smaller versions with preload
;
;	CopyBuild PreLoadCopyBlock,CopyBlock,0,0,1,1
;	CopyBuild PreLoadCopyBlockVer,CopyBlockVer,0,0,0,1
;	CopyBuild PreLoadCopyBlockHor,CopyBlockHor,0,0,0,1
;	CopyBuild PreLoadCopyBlockHorVer,CopyBlockHorVer,0,0,0,1
;	CopyBuild PreLoadCopyBlockVerRound,CopyBlockVer,1,0,0,1
;	CopyBuild PreLoadCopyBlockHorRound,CopyBlockHor,1,0,0,1
;	CopyBuild PreLoadCopyBlockHorVerRound,CopyBlockHorVer,1,0,0,1
;
;	CopyBuild PreLoadAddBlock,AddBlock,0,1,0,1
;	CopyBuild PreLoadAddBlockVer,CopyBlockVer,0,1,0,1
;	CopyBuild PreLoadAddBlockHor,CopyBlockHor,0,1,0,1
;	CopyBuild PreLoadAddBlockHorVer,CopyBlockHorVer,0,1,0,1
;
;---------------------------------------------------
; larger versions with preload
; (faster if there is enough intstruction cache available)

	CopyBuild CopyBlock,CopyBlock,0,0,1,1
	CopyBuild CopyBlockVer,CopyBlockVer,0,0,1,1
	CopyBuild CopyBlockHor,CopyBlockHor,0,0,1,1
	CopyBuild CopyBlockHorVer,CopyBlockHorVer,0,0,1,1
	CopyBuild CopyBlockVerRound,CopyBlockVer,1,0,1,1
	CopyBuild CopyBlockHorRound,CopyBlockHor,1,0,1,1
	CopyBuild CopyBlockHorVerRound,CopyBlockHorVer,1,0,1,1

	CopyBuild AddBlock,AddBlock,0,1,1,1
	CopyBuild AddBlockVer,CopyBlockVer,0,1,1,1
	CopyBuild AddBlockHor,CopyBlockHor,0,1,1,1
	CopyBuild AddBlockHorVer,CopyBlockHorVer,0,1,1,1

	CopyBuild CopyBlockM,CopyBlockM,0,0,1,1

	END

