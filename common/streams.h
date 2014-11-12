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
 * $Id: streams.h 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __STREAM_H
#define __STREAM_H

#define STREAM_CLASS		FOURCC('S','T','R','M')

#define STREAM_URL			0x91
#define STREAM_LENGTH		0x92
#define STREAM_SILENT		0x93
#define STREAM_CREATE		0x94
#define STREAM_CONTENTTYPE	0x95
#define STREAM_COMMENT		0x96
#define STREAM_PRAGMA_SEND	0x97
#define STREAM_PRAGMA_GET	0x98
#define STREAM_METAPROCESSOR 0x99
#define STREAM_BASE			0x9A

#ifndef SEEK_SET
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#endif

#ifndef EOF
#define EOF				(-1)
#endif

typedef struct streamdir
{
	tchar_t FileName[MAXPATH];
	tchar_t DisplayName[MAXPATH];
	int Type;					// from Exts
	filepos_t Size;				// -1 for directory
	int64_t Date;
	void* Private;

} streamdir;

// read from stream
typedef	int (*streamread)(void* This,void* Data,int Size);
// read from stream to block
typedef	int (*streamreadblock)(void* This,block* Block,int Ofs,int Size);
// seek
typedef	filepos_t (*streamseek)(void* This,filepos_t Pos,int SeekMode);
// write to stream
typedef	int (*streamwrite)(void* This,const void* Data,int Size);
// list directory 
typedef	int (*streamenumdir)(void* This,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item);
// data available
typedef	int (*streamdataavail)(void* This);

typedef struct stream
{
	VMT_NODE
	streamread		Read;
	streamreadblock	ReadBlock;
	streamseek		Seek;
	streamenumdir	EnumDir;
	streamwrite		Write;
	streamdataavail DataAvailable;

} stream;

void Stream_Init();
void Stream_Done();
DLL int StreamEnum(void* p, int* No, datadef* Param);

DLL const tchar_t* GetMime(const tchar_t* URL, tchar_t* Mime, int MimeLen, bool_t* HasHost);
DLL stream* GetStream(const tchar_t* URL, bool_t Silent);

DLL stream* StreamOpen(const tchar_t*, bool_t Write); 
DLL int StreamRead(stream*, void*, int);
DLL int StreamWrite(stream*, const void*, int);
DLL filepos_t StreamSeek(stream*, filepos_t, int);
DLL int StreamClose(stream*);
DLL void StreamPrintf(stream* Stream, const tchar_t* Msg,...);
DLL void StreamPrintfEx(stream* Stream, bool_t UTF8, const tchar_t* Msg,...);
DLL void StreamText(stream* Stream, const tchar_t*, bool_t UTF8);

DLL stream* StreamOpenMem(const void*,int); 
DLL int StreamCloseMem(stream*);

//--------------------------------------------------------------------------
 
#define STREAMPROCESS_CLASS		FOURCC('S','T','R','P')

#define STREAMPROCESS_INPUT		0xB0

//---------------------------------------------------------------------------

#define MEMSTREAM_ID		FOURCC('M','E','M','S')
#define MEMSTREAM_DATA		0x100


#endif
