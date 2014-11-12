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
 * $Id: color.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __COLOR_H
#define __COLOR_H

#define COLOR_ID		FOURCC('C','O','L','O')

// dithering enabled (bool_t)
#define COLOR_DITHER		0x34
// brightness (int -128..127)
#define COLOR_BRIGHTNESS	0x37
// contrast (int -128..127)
#define COLOR_CONTRAST		0x38
// saturation (int -128..127)
#define COLOR_SATURATION	0x62
// red adjust (int -32..32)
#define COLOR_R_ADJUST		0x64
// green adjust (int -32..32)
#define COLOR_G_ADJUST		0x65
// blue adjust (int -32..32)
#define COLOR_B_ADJUST		0x66

#define COLOR_RESET			0x63
#define COLOR_CAPS			0x70

void Color_Init();
void Color_Done();

#endif
