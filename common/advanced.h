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
 * $Id: advanced.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __ADVANCED_H
#define __ADVANCED_H

#define ADVANCED_ID		FOURCC('A','D','V','P')

#define ADVANCED_OLDSHELL		0x1F
#define ADVANCED_NOBACKLIGHT	0x20
#define ADVANCED_NOWMMX			0x28
#define ADVANCED_SLOW_VIDEO		0x25
#define ADVANCED_IDCTSWAP		0x29
#define ADVANCED_COLOR_LOOKUP	0x26
#define ADVANCED_HOMESCREEN		0x2D
#define ADVANCED_BENCHFROMPOS	0x30
#define ADVANCED_PRIORITY		0x36
#define ADVANCED_SYSTEMVOLUME	0x34
#define ADVANCED_AVIFRAMERATE	0x35
#define ADVANCED_VR41XX			0x37
#define ADVANCED_KEYFOLLOWDIR	0x38
#define ADVANCED_MEMORYOVERRIDE	0x39
#define ADVANCED_WIDCOMMAUDIO	0x3A
#define ADVANCED_WIDCOMMDLL		0x3B
#define ADVANCED_NOBATTERYWARNING 0x3C
#define ADVANCED_NOEVENTCHECKING 0x3D
#define ADVANCED_CARDPLUGINS	0x3E
#define ADVANCED_WAVEOUTPRIORITY 0x3F
#define ADVANCED_NODEBLOCKING	0x41
#define ADVANCED_BLINKLED		0x42

// these are not boolean (should not use QueryAdvanced)
#define ADVANCED_DROPTOL		0x2A
#define ADVANCED_SKIPTOL		0x2B
#define ADVANCED_AVOFFSET		0x2C

void Advanced_Init();
void Advanced_Done();

// helper for getting boolean advanced setting values
DLL bool_t QueryAdvanced(int Param);

#endif
