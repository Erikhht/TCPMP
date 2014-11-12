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
 * $Id: mediainfo.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __MEDIAINFO_H
#define __MEDIAINFO_H

void MediaInfo_Init();
void MediaInfo_Done();

#define MEDIAINFO_ID		FOURCC('M','E','D','I')

#define MEDIAINFO_FORMAT		0x100
#define MEDIAINFO_CODEC			0x101
#define MEDIAINFO_FPS			0x104
#define MEDIAINFO_SIZE			0x106
#define MEDIAINFO_BITRATE		0x109
#define MEDIAINFO_VIDEO_TOTAL	0x10A
#define MEDIAINFO_VIDEO_DROPPED	0x10B
#define MEDIAINFO_AVG_FPS		0x10C
#define MEDIAINFO_AUDIO_FORMAT	0x10D
#define MEDIAINFO_AUDIO_MONO	0x10E
#define MEDIAINFO_AUDIO_STEREO	0x10F

#endif
