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
 * $Id: format_base.h 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __FORMAT_BASE_H
#define __FORMAT_BASE_H

// base format implementation

#define FORMATBASE_CLASS			FOURCC('F','M','T','B')

#define MAXREADER	2
#define MAXSTREAM	64
#define MINBUFFER	(256*1024)
#define HEADERLOAD	(1024*1024)

#define VIDEO_BURST		1
#define AUDIO_BURST		6

typedef struct format_buffer
{
	int Id;
	int Length;
	int RefCount;
	struct format_buffer* Next;
	block Block;
#ifndef NDEBUG
	filepos_t FilePos;
#endif

} format_buffer;

typedef struct format_ref
{
	format_buffer* Buffer;
	int Begin;
	int Length;
	struct format_ref* Next;

} format_ref;

struct format_stream;
struct format_base;

typedef struct format_packet
{
	format_ref* Data;
	tick_t RefTime;
	tick_t EndRefTime; // optional (only for subtitle)
	struct format_stream* Stream;
	struct format_packet* Next;
	bool_t Key;
} format_packet;

typedef struct format_reader
{
	int (*Seek)(struct format_reader*,filepos_t Pos,int Origin);
	bool_t (*Eof)(struct format_reader*);
	void (*Skip)(struct format_reader*,int);
	int (*Read)(struct format_reader*,void*,int);
	int (*Read8)(struct format_reader*);
	int (*ReadBE16)(struct format_reader*);
	int (*ReadLE16)(struct format_reader*);
	int (*ReadBE32)(struct format_reader*);
	int (*ReadLE32)(struct format_reader*);
	int64_t (*ReadBE64)(struct format_reader*);
	int64_t (*ReadLE64)(struct format_reader*);
	format_ref* (*ReadAsRef)(struct format_reader*, int Length);
	bool_t (*GetReadBuffer)(struct format_reader*);

	struct format_base* Format;
	int Ratio;

	stream* Input;
	bool_t NoMoreInput;
	bool_t OutOfSync;

	int BufferAvailable; // number of bytes in BufferFirst..BufferLast chain
	format_buffer* BufferFirst; 
	format_buffer* BufferLast; 

	format_buffer* InputBuffer; 
	format_buffer* ReadBuffer; 
	int ReadPos; 
	int ReadLen; 
	filepos_t FilePos;

	struct format_stream* Current;
	filepos_t OffsetMin;
	filepos_t OffsetMax;

	int Reserved[10];

} format_reader;

typedef struct format_stream
{
	int No;
	int Id;
	format_reader* Reader;
	pin Pin;
	pin Comment;
	packetprocess Process;
	packetformat Format;
	bool_t Fragmented; //don't bother to merge buffers
	bool_t ForceMerge; 
	bool_t DisableDrop; // no dropping allowed
	int PacketBurst;

	// buffer for merging fragmented packets 
	// to a single continous memory
	buffer BufferMem; // faster system mem
	block BufferBlock; // using storage mem
	int BufferBlockLength;
	int BufferBlockId;

	bool_t Pending; // 1:full packet 2:only first reference
	tick_t LastTime; // last known refrence time (-1 out of sync)
	packet Packet;  // pending packet
	flowstate State;
	int DropCount;

	// buffered packets waiting for processing
	format_packet* PacketFirst;
	format_packet* PacketLast;

	int Priority;
	int Reserved[9];

} format_stream;

typedef int (*fmtfunc)(void* This);
typedef void (*fmtvoid)(void* This);
typedef int (*fmtfill)(void* This,format_reader* Reader);
typedef int (*fmtseek)(void* This,tick_t,filepos_t,bool_t);
typedef int (*fmtreadpacket)(void* This,format_reader* Reader,format_packet* Packet);
typedef void (*fmtstream)(void* This, void* Stream);
typedef int (*fmtstreamprocess)(void* This, void* Stream);
typedef void (*fmtbackup)(void* This, int Begin);

typedef struct format_base
{
	format Format;

	fmtfunc Init;
	fmtvoid Done;
	fmtseek Seek;
	fmtfill FillQueue;
	fmtreadpacket ReadPacket;
	fmtstream FreeStream;
	fmtstream ReleaseStream;
	fmtvoid AfterSeek;
	fmtstreamprocess Process;
	fmtstream Sended;
	fmtbackup BackupPacketState;

	bool_t AutoReadSize;
	bool_t TimeStamps; // CalcDuration will try to load last packets
	bool_t FirstSync;
	bool_t NeedSubtitles;
	bool_t DisableReader;
	bool_t UseBufferBlock;
	tick_t Duration;
	tick_t StartTime;
	filepos_t FileSize;
	tick_t DropTolerance;
	tick_t SkipTolerance;
	tick_t AVOffset;
	tick_t GlobalOffset;
	notify UpdateStreams;
	notify ReleaseStreams;
	int FileAlign;
	int MinHeaderLoad;

	int SeekByPacket_DataStart;
	int SeekByPacket_BlockAlign;

	int StreamCount;
	int ActiveStreams;
	format_stream* Streams[MAXSTREAM];
	format_stream* SubTitle;
	int ProcessMinBuffer;
	pin Comment;

	bool_t SyncMode;
	int SyncRead; // number of blocks the processing thread allowed to read by it's own after sync
	format_stream* SyncStream;
	tick_t SyncTime;

	int BufferId;
	format_packet* FreePackets;
	format_buffer* FreeBuffers; 
	format_ref*	   FreeRefs;

	void* BufferLock; // buffer handling must be multithread safe
	int BufferUsed; // number of buffers allocated (referenced)
	int BufferSize; // number of buffers allowed
	int ReadSize; // BLOCKSIZE/(2^n)

	format_reader Reader[MAXREADER];
	format_reader* LastRead;

	tick_t ProcessTime;
	bool_t Bench;
	bool_t HeaderLoaded;
	bool_t InSeek;

	void* InputLock;
	int SumByteRate;

	bool_t Timing;
	int TotalCount;
	format_stream* CoverArt;
	int Reserved[6];

} format_base;

void FormatBase_Init();
void FormatBase_Done();

DLL int FormatBaseEnum(format_base* p,int* No,datadef* Param);
DLL int FormatBaseGet(format_base* p,int No,void* Data,int Size);
DLL int FormatBaseSet(format_base* p,int No,const void* Data,int Size);

DLL int Format_Send(format_base* p,format_stream* Stream);
DLL int Format_CheckEof(format_base* p,format_stream* Stream);
DLL format_stream* Format_AddStream(format_base*, int Length); 
DLL void Format_RemoveStream(format_base*); 
DLL void Format_PrepairStream(format_base*, format_stream*);
DLL format_stream* Format_DefSyncStream(format_base*,format_reader*);
DLL int Format_SeekByPacket(format_base*,tick_t Time,filepos_t FilePos,bool_t PrevKey);

DLL int Format_WaveFormatMem(format_stream*,void*,int Length);
DLL int Format_BitmapInfoMem(format_stream*,void*,int Length);

DLL void Format_BufferRelease(format_base*, format_buffer*);

DLL int Format_Seek(format_base*,filepos_t Pos,int Origin);
DLL int Format_SeekForce(format_base*,filepos_t Pos,int Origin);
DLL void Format_AfterSeek(format_base* p);

DLL format_reader* Format_FindReader(format_base* p,filepos_t Min,filepos_t Max);
DLL format_buffer* Format_BufferRemove(format_reader*);
DLL bool_t Format_ReadBuffer(format_reader* Reader, bool_t ToRead);

DLL void Format_WaveFormat(format_reader*,format_stream*,int Length);
DLL void Format_BitmapInfo(format_reader*,format_stream*,int Length);

DLL format_ref* Format_DupRef(format_base*, format_ref*, int Offset, int Length);
DLL void Format_ReleaseRef(format_base*, format_ref*);
DLL format_ref* Format_RefAlloc(format_base* p, format_buffer* To, int Begin, int Length);

format_stream* Format_LoadSubTitle(format_base*, stream* Input);
void Format_FreeSubTitle(format_base*, format_stream*);
void Format_ProcessSubTitle(format_base*, format_stream*);

DLL bool_t Format_AllocBufferBlock(format_base* p,format_stream* Stream,int Size);
DLL void Format_TimeStampRestarted(format_base* p,format_stream* Stream,tick_t* RefTime);

#endif
