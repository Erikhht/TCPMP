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
@* $Id: mcomp_wmmx.s 271 2005-08-09 08:31:35Z picard $
@*
@* The Core Pocket Media Player
@* Copyright (c) 2004-2005 Gabor Kovacs
@*
@*****************************************************************************

@R0 src
@R1 dst
@R2 srcpitch
@R3 dstpitch


  .macro CopyBegin
	add ip,r2,#7
	.dc.l 0x0f7d0f002
	.dc.l 0x0f5d0f007
	stmdb sp!,{lr}
	.dc.l 0x0f7d0f00c
	.dc.l 0x0f5d0f000
  .endm

  .macro CopyEnd
	ldmia sp!,{pc}  
  .endm

  .macro PreLoad

	.dc.l 0x0f7d0f082
  .endm

  .macro PreLoad2Init

	add ip,r2,#4
  .endm

  .macro PreLoad2

	.dc.l 0x0f7d0f082 @2*pitch
	.dc.l 0x0f7d0f08c @2*pitch+8
  .endm

  .macro PrepareAlignVer Name, Height
    ands r14,r0,#7
	tmcr wcgr1,r14
	mov r14,#&Height
	beq &Name.Aligned
	bic r0,r0,#7
  .endm

  .macro PrepareAlignHor Name
    and r14,r0,#7
	tmcr wcgr1,r14
    add r14,r14,#1
	bic r0,r0,#7
	cmp r14,#8
	beq &Name.Wrap
	tmcr wcgr2,r14
	mov r14,#8
  .endm

@------------------------------------------
@COPYBLOCK
@------------------------------------------

  .macro CopyBlock Name

	.align 2
	.global &Name
&Name:
    CopyBegin
	sub r1,r1,r3
	PrepareAlignVer &Name,8

	PreLoad2Init
&Name.Loop:
	PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	add r1,r1,r3
	walignr1 wr0,wr0,wr1
	wstrd wr0,[r1]
	subs r14,r14,#1
	bne &Name.Loop
	CopyEnd

&Name.Aligned:
	PreLoad
	wldrd wr0,[r0]
	add	r0,r0,r2
	add r1,r1,r3
	wstrd wr0,[r1]
	subs r14,r14,#1
	bne &Name.Aligned
	CopyEnd

  .endm

@------------------------------------------
@COPYBLOCKM
@------------------------------------------

  .macro CopyBlockM Name

	.align 2
	.global &Name
&Name:
    CopyBegin
	sub r1,r1,r3
	PrepareAlignVer &Name,16

	PreLoad2Init
&Name.Loop:
	PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	wldrd wr2,[r0,#16]
	add	r0,r0,r2
	add r1,r1,r3
	walignr1 wr0,wr0,wr1
	walignr1 wr1,wr1,wr2
	wstrd wr0,[r1]
	wstrd wr1,[r1,#8]
	subs r14,r14,#1
	bne &Name.Loop
	CopyEnd

&Name.Aligned:
	PreLoad
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	add r1,r1,r3
	wstrd wr0,[r1]
	wstrd wr1,[r1,#8]
	subs r14,r14,#1
	bne &Name.Aligned
	CopyEnd

  .endm

@------------------------------------------
@ADDBLOCK
@------------------------------------------

  .macro AddBlock Name
	.align 2
	.global &Name
&Name:
	CopyBegin
	PrepareAlignVer &Name,8

	PreLoad2Init
&Name.Loop:
	PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	wldrd wr5,[r1]
	walignr1 wr0,wr0,wr1
	wavg2br wr0,wr0,wr5
	wstrd wr0,[r1],#8
	subs r14,r14,#1
	bne &Name.Loop
	CopyEnd

&Name.Aligned:
	PreLoad
	wldrd wr0,[r0]
	add	r0,r0,r2
	wldrd wr5,[r1]
	wavg2br wr0,wr0,wr5
	wstrd wr0,[r1],#8
	subs r14,r14,#1
	bne &Name.Aligned
	CopyEnd

  .endm

@------------------------------------------
@ COPYBLOCKHOR
@------------------------------------------

  .macro CopyHorRow Round, Add, Wrap
    PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
    .if &Add > 0
	  wldrd wr5,[r1]
    .else
	  add r1,r1,r3
    .endif
    walignr1 wr2,wr0,wr1
    .if &Wrap == 0
	  walignr2 wr1,wr0,wr1
    .endif
    .if &Round > 0
	  wavg2b wr0,wr1,wr2
    .else
	  wavg2br wr0,wr1,wr2
    .endif
    .if &Add > 0
	  wavg2br wr0,wr0,wr5
  	  wstrd wr0,[r1],#8
    .else
	  wstrd wr0,[r1]
	.endif
  .endm

  .macro CopyBlockHor Name, Round, Add
	.align 2
	.global &Name
&Name:
	CopyBegin
    .if &Add == 0
	  sub r1,r1,r3
	.endif
	PreLoad2Init
	PrepareAlignHor &Name

&Name.Loop:
	CopyHorRow &Round,&Add,0
	subs r14,r14,#1
	bne &Name.Loop
	CopyEnd

&Name.Wrap:
	CopyHorRow &Round,&Add,1
	subs r14,r14,#1
	bne &Name.Wrap
	CopyEnd

  .endm

@------------------------------------------
@ COPYBLOCKVER
@------------------------------------------

  .macro SetVerRow Round, Add
  .if &Add > 0
    wldrd wr5,[r1]
  .else
    add r1,r1,r3
  .endif
  .if &Round > 0
	wavg2b wr1,wr0,wr2
  .else
 	wavg2br wr1,wr0,wr2
  .endif
  .if &Add > 0
    wavg2br wr1,wr1,wr5
    wstrd wr1,[r1],#8
  .else
    wstrd wr1,[r1]
  .endif
  .endm

  .macro CopyBlockVer Name, Round, Add
	.align 2
	.global &Name
&Name:
	CopyBegin
    .if &Add == 0
	  sub r1,r1,r3
	.endif
	PrepareAlignVer &Name,8

	PreLoad2Init
	PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	walignr1 wr0,wr0,wr1

&Name.Loop:
	PreLoad2
	wldrd wr2,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	walignr1 wr2,wr2,wr1
	SetVerRow &Round,&Add
	PreLoad2
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	walignr1 wr0,wr0,wr1
	SetVerRow &Round,&Add
	subs r14,r14,#2
	bne &Name.Loop
	CopyEnd

&Name.Aligned:

	PreLoad	
	wldrd wr0,[r0]
	add	r0,r0,r2

&Name.Loop2:
	PreLoad	
	wldrd wr2,[r0]
	add	r0,r0,r2
	SetVerRow &Round,&Add
	PreLoad	
	wldrd wr0,[r0]
	add	r0,r0,r2
	SetVerRow &Round,&Add
	subs r14,r14,#2
	bne &Name.Loop2
	CopyEnd
  .endm

@------------------------------------------
@ COPYBLOCKHORVER
@------------------------------------------

@ wr6 0x03
@ wr7 ~0x03

  .macro LoadHorVerRow Parity, Wrap
  PreLoad2
  .if &Parity
	wldrd wr0,[r0]
	wldrd wr1,[r0,#8]
	add	r0,r0,r2
	walignr1 wr2,wr0,wr1
    .if &Wrap == 0
	  walignr2 wr1,wr0,wr1
    .endif

	wand wr0,wr2,wr6
	wand wr3,wr1,wr6
	wand wr2,wr2,wr7
	wand wr1,wr1,wr7
	wsrldg wr2,wr2,wcgr0  
	wsrldg wr1,wr1,wcgr0  
	waddb wr2,wr2,wr1
	waddb wr1,wr3,wr0 
  .else
	wldrd wr3,[r0]
	wldrd wr4,[r0,#8]
	add	r0,r0,r2
	walignr1 wr5,wr3,wr4
    .if &Wrap == 0
	  walignr2 wr4,wr3,wr4
    .endif

	wand wr0,wr5,wr6
	wand wr3,wr4,wr6
	wand wr5,wr5,wr7
	wand wr4,wr4,wr7
	wsrldg wr5,wr5,wcgr0  
	wsrldg wr4,wr4,wcgr0  
	waddb wr5,wr5,wr4
	waddb wr4,wr3,wr0
  .endif
  .endm

  .macro SetHorVerRow Add
  .if &Add > 0
    wldrd wr9,[r1]
  .else
    add r1,r1,r3
  .endif
	waddb wr0,wr1,wr4
	waddb wr0,wr0,wr8   @rounding
	wand wr0,wr0,wr7
	waddb wr3,wr2,wr5   
	wsrldg wr0,wr0,wcgr0  
	waddb wr0,wr0,wr3 
  .if &Add > 0
    wavg2br wr0,wr0,wr9
    wstrd wr0,[r1],#8
  .else
    wstrd wr0,[r1]
  .endif
  .endm

  .macro CopyBlockHorVer Name, Round, Add
	.align 2
	.global &Name
&Name:
	CopyBegin
    .if &Add == 0
	  sub r1,r1,r3
	.endif
    .if &Round > 0
	  mov r14,#1
    .else
	  mov r14,#2
    .endif
	tbcstb wr8,r14
	mov r14,#3 
	tbcstb wr6,r14
	mvn r14,#3 
	tbcstb wr7,r14
	mov r14,#2
	tmcr wcgr0,r14
	PreLoad2Init
	PrepareAlignHor &Name

	LoadHorVerRow 1,0

&Name.Loop:
	LoadHorVerRow 0,0
	SetHorVerRow &Add
	LoadHorVerRow 1,0
	SetHorVerRow &Add
	subs r14,r14,#2
	bne &Name.Loop
	CopyEnd

&Name.Wrap:
	LoadHorVerRow 1,1

&Name.Loop2:
	LoadHorVerRow 0,1
	SetHorVerRow &Add
	LoadHorVerRow 1,1
	SetHorVerRow &Add
	subs r14,r14,#2
	bne &Name.Loop2
	CopyEnd
  .endm

	CopyBlock WMMXCopyBlock
	CopyBlockVer WMMXCopyBlockVer,0,0
	CopyBlockHor WMMXCopyBlockHor,0,0
	CopyBlockHorVer WMMXCopyBlockHorVer,0,0

	CopyBlockVer WMMXCopyBlockVerRound,1,0
	CopyBlockHor WMMXCopyBlockHorRound,1,0
	CopyBlockHorVer WMMXCopyBlockHorVerRound,1,0

	AddBlock WMMXAddBlock
	CopyBlockVer WMMXAddBlockVer,0,1
	CopyBlockHor WMMXAddBlockHor,0,1
	CopyBlockHorVer WMMXAddBlockHorVer,0,1

	CopyBlockM WMMXCopyBlockM


