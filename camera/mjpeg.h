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
 * $Id: mjpeg.h 277 2005-08-24 16:41:02Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#ifndef __MJPEG_H
#define __MJPEG_H

#define MJPEG_ID		FOURCC('M','J','P','G')
#define JPEG_ID			FOURCC('R','J','P','G')
#define WEB_MJPEG_ID	FOURCC('W','J','P','G')

#define MJPEG_NOT_SUPPORTED	0x200
#define MJPEG_PROGRESSIVE   0x201

extern void MJPEG_Init();
extern void MJPEG_Done();

#endif
