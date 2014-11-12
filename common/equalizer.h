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
 * $Id: equalizer.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __EQUALIZER_H
#define __EQUALIZER_H

#define EQUALIZER_ID		FOURCC('E','Q','U','A')

#define EQUALIZER_RESET		0x18E
#define EQUALIZER_ENABLED	0x18F
#define EQUALIZER_ATTENUATE	0x190
#define EQUALIZER_PREAMP	0x180
#define EQUALIZER_1			0x181
#define EQUALIZER_2			0x182
#define EQUALIZER_3			0x183
#define EQUALIZER_4			0x184
#define EQUALIZER_5			0x185
#define EQUALIZER_6			0x186
#define EQUALIZER_7			0x187
#define EQUALIZER_8			0x188
#define EQUALIZER_9			0x189
#define EQUALIZER_10		0x18A

void Equalizer_Init();
void Equalizer_Done();

#endif
