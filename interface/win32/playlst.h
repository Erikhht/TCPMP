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
 * $Id: playlist.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PLAYLIST_WIN32_H
#define __PLAYLIST_WIN32_H

#define PLAYLIST_ID	FOURCC('P','L','S','T')

void PlaylistWin_Init();
void PlaylistWin_Done();

#define PLAYLIST_WIDTH_NAME		0x107
#define PLAYLIST_WIDTH_LENGTH	0x108
#define PLAYLIST_WIDTH_URL		0x109

#define PLAYLIST_ADD_FILES		0x200
#define PLAYLIST_DELETE			0x201
#define PLAYLIST_DELETE_ALL		0x202
#define	PLAYLIST_SORTBYNAME		0x203
#define PLAYLIST_UP				0x204
#define PLAYLIST_DOWN			0x205
#define PLAYLIST_LOAD			0x206	
#define PLAYLIST_SAVE			0x207	
#define PLAYLIST_FILE			0x208
#define PLAYLIST_PLAY			0x209

#define PLAYLIST_LABEL_NAME		0x300
#define PLAYLIST_LABEL_LENGTH	0x301
#define PLAYLIST_LABEL_URL		0x302

#endif