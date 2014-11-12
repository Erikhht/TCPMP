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
 * $Id: asf.h 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __ASF_H
#define __ASF_H

#define ASF_ID		FOURCC('R','A','S','F')
#define ASF_NAME	0x100

#define ASF_HTTP_ID	FOURCC('R','A','S','H')

#define ASF_CHUNK_MASK		0xFF7F

#define ASF_CHUNK_DATA		0x4424
#define ASF_CHUNK_END		0x4524
#define ASF_CHUNK_HEADER	0x4824

extern void ASF_Init();
extern void ASF_Done();

#endif
