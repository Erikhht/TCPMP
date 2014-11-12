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
 * $Id: subtitle.h 432 2005-12-28 16:39:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __SUBTITLE_H
#define __SUBTITLE_H

#define SUBTITLE_ASCII		FOURCC('A','S','C',' ')
#define SUBTITLE_OEM		FOURCC('O','E','M',' ')
#define SUBTITLE_UTF8		FOURCC('U','T','F','8')
#define SUBTITLE_UTF16		FOURCC('U','T','F','1')
#define SUBTITLE_SSA		FOURCC('S','S','A',' ')
#define SUBTITLE_ASS		FOURCC('A','S','S',' ')
#define SUBTITLE_USF		FOURCC('U','S','F',' ')

typedef struct subtitle
{
	uint32_t FourCC;

} subtitle;

#endif

