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
 * $Id: format.h 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __FORMAT_H
#define __FORMAT_H

#define FTYPE_PLAYLIST		'P'
#define FTYPE_VIDEO			'V'
#define FTYPE_AUDIO			'A'

#define BLOCKSIZE				32768

//---------------------------------------------------------------
// format and playlist 

#define MEDIA_CLASS				FOURCC('F','M','T','M')

//---------------------------------------------------------------
// format params

#define FORMAT_CLASS			FOURCC('F','M','T','_')

// input stream (stream*)
#define FORMAT_INPUT			0x32
// input stream (stream*)
#define FORMAT_OUTPUT			0x3B
// duration (tick_t)
#define FORMAT_DURATION			0x33
// approx file position (int)
#define FORMAT_FILEPOS			0x36
// file size if available (int)
#define FORMAT_FILESIZE			0x44
// file reader alignment (int)
#define FORMAT_FILEALIGN		0x49
// buffer size (int)
#define FORMAT_BUFFERSIZE		0x3A
// update streams notify
#define FORMAT_UPDATESTREAMS	0x41
// release streams notify
#define FORMAT_RELEASESTREAMS	0x4C
// manual data feed (for probe data)
#define FORMAT_DATAFEED			0x42
// auto find subtitle files (bool)
#define FORMAT_FIND_SUBTITLES	0x45
// covertart stream no (int)
#define FORMAT_COVERART			0x46
// global comments (comment) (optional)
#define FORMAT_GLOBAL_COMMENT	0x47
// number of streams (int)
#define FORMAT_STREAM_COUNT		0x48
// use auto readsize (bool) 
#define FORMAT_AUTO_READSIZE	0x4B
// media has timing (bool)
#define FORMAT_TIMING			0x4E
// minimum buffer needed
#define FORMAT_MIN_BUFFER		0x4F

#define FORMAT_UPDATEMASK		0x50

// output data streams (packet) 0x1000,0x1001,0x1002...
#define FORMAT_STREAM			0x1000
// comments by streams (comment) (optional)
#define FORMAT_COMMENT			0x2000
#define FORMAT_STREAM_PRIORITY	0x3000

extern DLL int FormatEnum(void*, int* EnumNo, datadef* Out);

typedef struct processstate
{
	tick_t Time;
	bool_t Fill;
	int BufferUsedBefore;
	int BufferUsedAfter;

} processstate;

// sync or seek by time or by file position (-1 for no seek)
typedef int (*fmtsync)(void* This,tick_t Time,int FilePos,bool_t PrevKey);
// read input (only one chunk) to buffer
typedef int (*fmtread)(void* This,int BufferMax,int* BufferUsed);
// read packets and send them time syncorinzed to codecs 
// time is updated after syncronization (return value ERR_SYNCED)
typedef int (*fmtprocess)(void* This,processstate* State);

typedef struct format
{
	VMT_NODE

	fmtsync			Sync;
	fmtread			Read;
	fmtprocess		Process;

} format;

void Format_Init();
void Format_Done();

#endif
