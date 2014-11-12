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
 * $Id: openfile_win32.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __OPENFILE_WIN32_H
#define __OPENFILE_WIN32_H

#define OPENFILE_ID	FOURCC('O','P','E','N')

// multiple selection (bool_t)
#define OPENFILE_MULTIPLE		0x101
// class filter (int)
#define OPENFILE_CLASS			0x121
// flags
#define OPENFILE_FLAGS			0x122
// path history 
#define OPENFILE_HISTORYCOUNT	0x120
// path history 
#define OPENFILE_HISTORY		0x1000

// setup
#define OPENFILE_FILETYPE		0x103
#define OPENFILE_PATH			0x104
#define OPENFILE_SORTCOL		0x105
#define OPENFILE_SORTDIR		0x106
#define OPENFILE_WIDTH_NAME		0x107
#define OPENFILE_WIDTH_TYPE		0x108
#define OPENFILE_WIDTH_SIZE		0x109
#define OPENFILE_WIDTH_DATE		0x10A
#define OPENFILE_ONLYNAME		0x110
#define OPENFILE_FILETYPE_SINGLE 0x123
#define OPENFILE_FILETYPE_SAVE	0x124

void OpenFile_Init();
void OpenFile_Done();

#define OPENFILE_FILTER			0x210
#define OPENFILE_ALL_FILES		0x201
#define OPENFILE_DIR			0x202
#define OPENFILE_UP				0x206
#define OPENFILE_LABEL_NAME		0x203
#define OPENFILE_LABEL_TYPE		0x204
#define OPENFILE_LABEL_SIZE		0x205
#define OPENFILE_LABEL_DATE		0x20E
#define OPENFILE_MORE			0x20A
#define OPENFILE_SORT			0x20B
#define OPENFILE_OPTIONS		0x20C
#define OPENFILE_SELECTALL		0x211
#define OPENFILE_ENTERURL		0x212
#define OPENFILE_GO				0x213
#define OPENFILE_HISTORYMENU	0x214
#define OPENFILE_EMPTY			0x215
#define OPENFILE_EMPTYNAME		0x216
#define OPENFILE_SAVE_NAME		0x217
#define OPENFILE_SAVE_TYPE		0x218
#define OPENFILE_SAVE			0x219
#define OPENFILE_ENTERNAME		0x21A
#define OPENFILE_SELECTDIR		0x21B
#define OPENFILE_SELECTDIR2		0x21C
#define OPENFILE_URL			0x21D
#define OPENFILE_LIST			0x21E

#define OPENFLAG_SAVE	0x01
// single mode (no multiple possible)
#define OPENFLAG_SINGLE	0x02
// add or set playlist
#define OPENFLAG_ADD	0x04

#endif