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
 * $Id: benchresult.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __BENCHRESULT_H
#define __BENCHRESULT_H

void BenchResult_Init();
void BenchResult_Done();

#define BENCHRESULT_ID		FOURCC('B','E','N','C')

#define BENCHRESULT_FRAMES		0x100
#define BENCHRESULT_SAMPLES		0x106
#define BENCHRESULT_TIME		0x101
#define BENCHRESULT_FPS			0x102
#define BENCHRESULT_SRATE		0x107
#define BENCHRESULT_ORIG_TIME	0x103
#define BENCHRESULT_ORIG_FPS	0x104
#define BENCHRESULT_ORIG_SRATE	0x108
#define BENCHRESULT_SPEED		0x105
#define BENCHRESULT_MSG			0x10A
#define BENCHRESULT_SAVE		0x10B
#define BENCHRESULT_BYTES		0x110
#define BENCHRESULT_BANDWIDTH	0x111
#define BENCHRESULT_ORIG_BANDWIDTH 0x112
#define BENCHRESULT_ZOOM		0x113
#define BENCHRESULT_SAVED		0x10E
#define BENCHRESULT_LOG			0x120
#define BENCHRESULT_URL			0x121
#define BENCHRESULT_FILESIZE	0x122
#define BENCHRESULT_CUSTOM		0x123
#define BENCHRESULT_INVALID		0x124

#endif
