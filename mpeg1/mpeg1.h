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
 * $Id: mpeg1.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __MPEG1_H
#define __MPEG1_H

#define MPEG1_ID			FOURCC('M','P','G','1')
#define MPEG1_INPUT			0x100
#define MPEG1_IDCT			0x101
#define MPEG2_NOT_SUPPORTED	0x201

#define MVES_ID				FOURCC('M','V','E','S')

extern void MVES_Init();
extern void MVES_Done();

extern void MPEG1_Init();
extern void MPEG1_Done();

#endif
