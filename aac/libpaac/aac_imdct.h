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
 * $Id: aac_imdct.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __AAC_IMDCT_H
#define __AAC_IMDCT_H

typedef void (*aac_filter)(void* p, int Sequence, int Shape, int PrevShape, 
						   real_t *Data, real_t *Overlap, real_t *Tmp);

typedef struct
{
	aac_filter Filter;
	const real_t *LongWindow[2];
	const real_t *ShortWindow[2];

} libpaac;

void AAC_Filter_Init(libpaac* p,int FrameSize);

#endif
