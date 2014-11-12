/*****************************************************************************
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: block_wmmx.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

#ifdef ARM

#define IDCT_Const8x8 WMMXIDCT_Const8x8 
#define Statistics(a,b,c,d)
#define Intra8x8 WMMXIntra8x8
#define Inter8x8Back WMMXInter8x8Back
#define Inter8x8BackFwd WMMXInter8x8BackFwd
#define Inter8x8QPEL WMMXInter8x8QPEL
#define Inter8x8GMC WMMXInter8x8GMC
#include "block.h"
#undef Intra8x8
#undef Inter8x8Back
#undef Inter8x8BackFwd

#ifdef CONFIG_IDCT_SWAP
#define SWAPXY
#define SWAP8X4
#define Intra8x8 WMMXIntra8x8Swap
#define Inter8x8Back WMMXInter8x8BackSwap
#define Inter8x8BackFwd WMMXInter8x8BackFwdSwap
#include "block.h"
#endif

#endif
