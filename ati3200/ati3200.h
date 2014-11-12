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
 * $Id: ati3200.h 624 2006-02-01 17:48:42Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __ATI3200_H
#define __ATI3200_H

#define AHI_ID			FOURCC('A','H','I','_')
#define AHI_IDCT_ID		VOUT_IDCT_CLASS(AHI_ID)

#define AHI_NAME				0x100
#define AHI_VERSION				0x101
#define AHI_REVISION			0x102
#define AHI_CHIPID				0x103
#define AHI_REVISIONID			0x104
#define AHI_TOTALMEMORY			0x105
#define AHI_INTERNALTOTALMEM	0x106
#define AHI_EXTERNALTOTALMEM	0x107
#define AHI_INTERNALFREEMEM		0x10E
#define AHI_EXTERNALFREEMEM		0x10F
#define AHI_CAPS1				0x108
#define AHI_CAPS2				0x109
#define AHI_CAPS3				0x10A
#define AHI_CAPS4				0x10B
#define AHI_UNMAP				0x10C
#define AHI_IDCTBUG				0x110
#define AHI_DEVICEBITMAP		0x111
#define AHI_PRIMARYREUSE		0x113
#define AHI_LOWBITDEPTH			0x114
#define AHI_STRETCHFLIP			0x115
#define AHI_NORELEASE			0x116

#define AHI_OUT_OF_MEMORY		0x200
#define AHI_OPEN_ERROR			0x201
#define AHI_OLDVERSION			0x202

void ATI3200_Init();
void ATI3200_Done();

#endif
