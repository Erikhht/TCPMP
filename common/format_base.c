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
 * $Id: format_base.c 611 2006-01-25 22:01:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

#define DROP_SHOW		8

#define ENDCHUNKS		(128*1024/BLOCKSIZE)
#define SKIPDISTANCE	64*1024

#define SYNCREAD		6

#define SEEKTRIES		4
#define SEEKMAXREAD		256*1024

#define PROCESSMINAUDIO	BLOCKSIZE
#define PROCESSMINVIDEO	128*1024

#define READER_BONUS	256*1024

int FormatBaseEnum(format_base* p,int* No,datadef* Param)
{
	if (FormatEnum(p,No,Param) == ERR_NONE)
		return ERR_NONE;

	if (*No < p->TotalCount)
	{
		memset(Param,0,sizeof(datadef));
		Param->Class = FORMAT_CLASS;
		Param->Size = sizeof(pin);
		Param->No = FORMAT_STREAM + *No;
		Param->Type = TYPE_PACKET;
		Param->Flags = DF_OUTPUT;
		*No = 0;
		return ERR_NONE;
	}
	*No -= p->TotalCount;

	if (*No < p->TotalCount)
	{
		memset(Param,0,sizeof(datadef));
		Param->Class = FORMAT_CLASS;
		Param->Size = sizeof(pin);
		Param->No = FORMAT_COMMENT + *No;
		Param->Type = TYPE_COMMENT;
		Param->Flags = DF_OUTPUT;
		*No = 0;
		return ERR_NONE;
	}
	*No -= p->TotalCount;

	return ERR_INVALID_PARAM;
}

static INLINE void Format_BufferInsert(format_reader* p,format_buffer* Buffer)
{
	Buffer->Next = NULL;
	if (p->BufferLast)
		p->BufferLast->Next = Buffer;
	else
		p->BufferFirst = Buffer;
	p->BufferLast = Buffer;
	p->BufferAvailable += Buffer->Length;
}

format_buffer* Format_BufferAlloc(format_base*,int Mode); //0:optional 1:drop 2:new alloc
format_packet* Format_PacketAlloc(format_base*);
void Format_PacketRelease(format_base*, format_packet*);
bool_t Format_ReadBuffer(format_reader*, bool_t ToRead);

int Format_Seek(format_base* p,filepos_t Pos,int Origin)
{
	int Result = p->Reader->Seek(p->Reader,Pos,Origin);
	if (Result == ERR_NONE)
		Format_AfterSeek(p);
	return Result;
}

int Format_SeekForce(format_base* p,filepos_t Pos,int Origin)
{
	int Result;
	filepos_t Save = p->Reader[0].FilePos;
	p->Reader[0].FilePos = -1;
	Result = Format_Seek(p,Pos,Origin);
	if (Result != ERR_NONE)
		p->Reader[0].FilePos = Save;
	return Result;
}

int Format_SeekByPacket(format_base* p,tick_t Time,filepos_t FilePos,bool_t PrevKey)
{
	int Try;
	int LastTime;
	int LastPos;
	format_stream* Stream = NULL;

	if (FilePos < 0)
	{
		Time -= SHOWAHEAD; // we should go a little earlier
		if (Time > 0)
		{
			if (p->FileSize<0 || p->Duration<0)
				return ERR_NOT_SUPPORTED;

			FilePos = Scale(p->FileSize,Time,p->Duration);
		}
		else
			FilePos = 0;
	}

	if (Time>0)
		Stream = Format_DefSyncStream(p,p->Reader);

	p->GlobalOffset = 0;
	p->InSeek = 1;

	LastPos = -1;
	LastTime = -1;

	for (Try=0;Try<SEEKTRIES;++Try) // maximum iterations
	{
		if (FilePos < p->SeekByPacket_DataStart)
			FilePos = p->SeekByPacket_DataStart;
		if (p->SeekByPacket_BlockAlign > 1)
			FilePos -= (FilePos - p->SeekByPacket_DataStart) % p->SeekByPacket_BlockAlign;

		if (Format_Seek(p,FilePos,SEEK_SET) != ERR_NONE)
		{
			p->InSeek = 0;
			return ERR_NOT_SUPPORTED;
		}

		if (Stream)
		{
			// check if really seeked to the right time (and adjust if needed)

			format_reader* Reader = p->Reader;
			int SeekRead = FilePos + SEEKMAXREAD;

			while (Reader->FilePos<SeekRead)
			{
				Format_ReadBuffer(Reader,0); // simulate input thread

				if (p->FillQueue(p,Reader) == ERR_END_OF_FILE)
					break;

				if (Stream->LastTime >= 0)
				{
					DEBUG_MSG3(DEBUG_FORMAT,T("SeekByPacket to:%d current:%d filepos:%d"),Time,Stream->LastTime,FilePos);

					if (!p->TimeStamps)
					{
						// adjust GlobalOffset
						p->GlobalOffset = Time - Stream->LastTime;
						if (p->GlobalOffset>0)
						{
							// need to adjust all stream packets
							int No;
							for (No=0;No<p->StreamCount;++No)
							{
								format_packet* i;
								format_stream* s = p->Streams[No];
								if (s->LastTime >= 0)
									s->LastTime += p->GlobalOffset;
								for (i=s->PacketFirst;i;i=i->Next)
									if (i->RefTime >= 0)
										i->RefTime += p->GlobalOffset;
							}
						}
						else
							p->GlobalOffset=0; // can't be negative
						SeekRead = -1; // accept
						break;
					}

					if (Time>=Stream->LastTime && (Time-Stream->LastTime) <= (TICKSPERSEC/2))
						FilePos = -1; // accept
					else
					{
						int Move;
						
						if (LastPos >= 0)
 							Move = Scale(FilePos-LastPos,Time-Stream->LastTime,Stream->LastTime-LastTime);
						else
 							Move = Scale(p->FileSize,Time-Stream->LastTime,p->Duration);

						if (Move < 0)
							Move = (Move*9)/8;
						else
							Move = (Move*15)/16;

						if (Move > 64*1024 || Move < -64*1024)
						{
							LastTime = Stream->LastTime;
							LastPos = FilePos;
						}
						else
							LastPos = -1;

						FilePos += Move;
					}

					if (FilePos < 0 || FilePos >= p->FileSize)
						SeekRead = -1; // accept
					break;
				}

				if (!Reader->BufferAvailable && !Reader->ReadBuffer && Reader->NoMoreInput)
					break; // end of file
			}

			if (Reader->FilePos<SeekRead)
				continue; // try again
		}
		break;
	}

	p->InSeek = 0;

	return ERR_NONE;
}

format_buffer* Format_BufferRemoveLast(format_reader* Reader)
{
	format_buffer *Buffer,*Tail;
	LockEnter(Reader->Format->BufferLock);
	Buffer = Reader->BufferLast;
	if (Buffer)
	{
		Tail = Reader->BufferFirst;
		if (Tail == Buffer)
			Reader->BufferFirst = Tail = NULL;
		else
		{
			while (Tail && Tail->Next != Buffer)
				Tail = Tail->Next;
			if (Tail)
				Tail->Next = NULL;
		}
		Reader->BufferLast = Tail;
		Reader->BufferAvailable -= Buffer->Length;
		assert(Reader->BufferAvailable >= 0);
	}
	LockLeave(Reader->Format->BufferLock);
	return Buffer;
}

int Format_Hibernate(format_base* p, int Mode)
{
	bool_t Changed = 0;

	if (p->InputLock)
	{
		int No;
		format_buffer *Buffer;

		LockEnter(p->InputLock);
		LockEnter(p->BufferLock);

#ifndef TARGET_PALMOS

		if (Mode == HIBERNATE_HARD)
		{
			format_reader *Reader = p->Reader;
			filepos_t Seek = 0;

			if (Reader->InputBuffer)
				Seek += Reader->InputBuffer->Length;

			for (Buffer = Reader->BufferFirst;Buffer;Buffer = Buffer->Next)
				Seek += Buffer->Length;

			if (Seek && Reader->Input && Reader->Input->Seek(Reader->Input,-Seek,SEEK_CUR)>=0)
			{
				Reader->NoMoreInput = 0;

				if (Reader->InputBuffer)
				{
					Format_BufferRelease(p,Reader->InputBuffer);
					Reader->InputBuffer = NULL;
				}

				while ((Buffer = Format_BufferRemoveLast(Reader)) != NULL)
					Format_BufferRelease(p,Buffer);
			}
		}
#endif

		while (p->FreeBuffers)
		{
			Buffer = p->FreeBuffers;
			p->FreeBuffers = Buffer->Next;
			FreeBlock(&Buffer->Block);
			free(Buffer);
			Changed = 1;
		}

		if (!Changed)
			for (No=0;No<MAXREADER;++No)
				for (Buffer = p->Reader[No].BufferFirst;Buffer;Buffer = Buffer->Next)
					if (SetHeapBlock(BLOCKSIZE,&Buffer->Block,HEAP_STORAGE))
						Changed = 1;

		LockLeave(p->BufferLock);
		LockLeave(p->InputLock);
	}

	return Changed ? ERR_NONE : ERR_OUT_OF_MEMORY;
}

static bool_t DropBuffer(format_base* p)
{
	bool_t Dropped = 0;
	int No=0;

	for (No=0;No<p->StreamCount;++No)
	{
		format_stream* Stream = p->Streams[No];
		if (Stream->PacketFirst)
		{
			format_packet* Packet = Stream->PacketFirst;
			Stream->PacketFirst = Packet->Next;
			if (Stream->PacketLast == Packet)
				Stream->PacketLast = NULL;

			Format_PacketRelease(p,Packet);

			Stream->Pending = 0;
			Stream->State.DropLevel = 2;
			Stream->DropCount = DROP_SHOW;
			Dropped = 1;
		}
	}

	return Dropped;
}

format_buffer* Format_BufferAlloc(format_base* p,int Mode)
{
	format_buffer* Buffer;

retry:
	Buffer = p->FreeBuffers;
	if (!Buffer)
	{
		if (p->BufferUsed < p->BufferSize || Mode==2)
		{
			block Block;
			if (AllocBlock(BLOCKSIZE,&Block,Mode!=2,HEAP_ANY))
			{
				Buffer = (format_buffer*) malloc(sizeof(format_buffer));
				if (Buffer)
					Buffer->Block = Block;
				else
					FreeBlock(&Block);
			}
		}
		else
		if (Mode==0)
			return NULL;

		if (!Buffer)
		{
			if (Mode==1 && DropBuffer(p))
				goto retry;
			return NULL;
		}
	}
	else
	{
		assert(Buffer->RefCount==0);
		p->FreeBuffers = Buffer->Next;
	}

	Buffer->Length = 0;
	Buffer->Id = p->BufferId++;
	Buffer->RefCount = 1;
	Buffer->Next = NULL;
	++p->BufferUsed;

	return Buffer;
}

static int SumByteRate(format_base* p)
{
	// try to sum stream byterates
	int No;
	int ByteRate = 0;

	for (No=0;No<p->StreamCount;++No)
	{
		packetformat Format;
		p->Format.Get(p,(FORMAT_STREAM|PIN_FORMAT)+No,&Format,sizeof(Format));
		if (Format.ByteRate<=0)
		{
			if (p->Streams[No]->Pin.Node && p->ProcessTime == TIME_SYNC)
			{
				p->ProcessTime = 0;
				p->Process(p,p->Streams[No]); // try processing
				p->ProcessTime = TIME_SYNC;
			}
			p->Format.Get(p,(FORMAT_STREAM|PIN_FORMAT)+No,&Format,sizeof(Format));
			if (Format.ByteRate<=0)
				return p->SumByteRate; // failed
		}
		ByteRate += Format.ByteRate;
	}

	return ByteRate;
}

static bool_t IsVideo(format_base* p)
{
	int No;
	for (No=0;No<p->StreamCount;++No)
		if (p->Streams[No]->Format.Type == PACKET_VIDEO)
			return 1;
	return 0;
}

static void FindCoverArt(format_base* p);

static void CalcDuration(format_base* p)
{
	if (p->Duration<0 && p->FileSize>=0)
	{
		if (p->TimeStamps)
		{
			// try to read the last packets and figure out Duration
			filepos_t SavePos;
			format_reader* Reader = p->Reader;

			LockEnter(p->InputLock);
			LockEnter(p->BufferLock);

			SavePos = Reader->Input->Seek(Reader->Input,0,SEEK_CUR);
			if (SavePos >= 0)
			{
				format_buffer* Buffer[ENDCHUNKS];
				int No,i;
				int Total = 0;
				tick_t Max=-1;
				format_reader Backup = *Reader;

				memset(Buffer,0,sizeof(Buffer));
		
				Reader->NoMoreInput = 1; //disable reading during ReadPacket
		
				for (No=0;No<ENDCHUNKS;++No)
				{
					filepos_t Pos = p->FileSize - (No+1)*BLOCKSIZE;
					if (Pos<0)
					{
						if (No)
							break;
						Pos = 0;
					}

					Buffer[No] = Format_BufferAlloc(p,2);
					if (!Buffer[No])
						break;

					if (Reader->Input->Seek(Reader->Input,Pos,SEEK_SET)<0)
						break;

					Buffer[No]->Length = Reader->Input->ReadBlock(Reader->Input,&Buffer[No]->Block,0,BLOCKSIZE);
					if (Buffer[No]->Length < 0)
						break;

					Total += Buffer[No]->Length;
					if (No)
						Buffer[No]->Next = Buffer[No-1];

					// setup temporary buffer
					Reader->InputBuffer = NULL;
					Reader->BufferAvailable = Total;
					Reader->BufferLast = Buffer[0];
					Reader->BufferFirst = Buffer[No];
					Reader->ReadPos = 0;
					Reader->ReadBuffer = NULL;
					Reader->FilePos = Pos;
					for (i=0;i<=No;++i)
						Buffer[i]->RefCount++;

					if (p->SeekByPacket_BlockAlign > 1)
					{
						int Rem = (Pos - p->SeekByPacket_DataStart) % p->SeekByPacket_BlockAlign;
						if (Rem)
							Reader->Skip(Reader,p->SeekByPacket_BlockAlign - Rem);
					}
					if (p->BackupPacketState)
						p->BackupPacketState(p,1);

					while (Reader->BufferLast || Reader->ReadBuffer)
					{
						int Result;
						format_packet* Packet = Format_PacketAlloc(p);
						if (!Packet)
							break;

						Result = p->ReadPacket(p,Reader,Packet);

						if (Result != ERR_NONE && Result != ERR_DATA_NOT_FOUND)
						{
							Reader->BufferLast = NULL;
							Reader->ReadBuffer = NULL;
						}
						else
						if (Result == ERR_NONE && Packet->Stream && Packet->RefTime>Max)
							Max = Packet->RefTime;

						Format_PacketRelease(p,Packet);
					}

					if (p->BackupPacketState)
						p->BackupPacketState(p,0);

					for (i=0;i<=No;++i)
						Buffer[i]->RefCount=1; // ugly

					if (Max >= 0)
					{
						p->Duration = Max;
						break;
					}
				}

				for (No=0;No<ENDCHUNKS;++No)
					if (Buffer[No])
						Format_BufferRelease(p,Buffer[No]);

				Reader->Input->Seek(Reader->Input,SavePos,SEEK_SET);
				*Reader = Backup;
			}

			LockLeave(p->BufferLock);
			LockLeave(p->InputLock);
		}

		if (p->StreamCount)
		{
			int ByteRate = SumByteRate(p);
			if (ByteRate>0)
			{
				tick_t Duration = Scale(p->FileSize,TICKSPERSEC,ByteRate);
				if (p->Duration<0 || p->Duration+3*TICKSPERSEC<Duration)
				{
					p->Duration = Duration;
					p->TimeStamps = 0;
				}
			}
		}
	}

#if !defined(MULTITHREAD)
	if (IsVideo(p))
		p->AutoReadSize = 1; // force auto readsize
#endif

	if (p->StreamCount && p->ProcessMinBuffer)
	{
		int ByteRate = SumByteRate(p);
		if (ByteRate<=0 && p->Duration>0)
		{
			int FileSize = p->FileSize;
			if (FileSize < 0 && p->Reader->Input)
			{
				LockEnter(p->InputLock);
				p->Reader->Input->Get(p->Reader->Input,STREAM_LENGTH,&FileSize,sizeof(FileSize));
				LockLeave(p->InputLock);
			}
			ByteRate = Scale(FileSize,TICKSPERSEC,p->Duration);
		}

		if (ByteRate>0)
		{
			while (ByteRate>p->ProcessMinBuffer*2 && p->ProcessMinBuffer < p->BufferSize*BLOCKSIZE/4)
				p->ProcessMinBuffer += BLOCKSIZE;

			if (p->AutoReadSize)
			{
				int No;
				int Rate = ByteRate / 16;
				
				// find video framerate
				for (No=0;No<p->StreamCount;++No)
				{
					packetformat Format;
					p->Format.Get(p,(FORMAT_STREAM|PIN_FORMAT)+No,&Format,sizeof(Format));
					if (Format.Type == PACKET_VIDEO && Format.PacketRate.Num>0)
					{
						Rate = Scale(ByteRate,Format.PacketRate.Den,Format.PacketRate.Num);
						break;
					}
				}

				p->ReadSize = 16384;
				while (p->ReadSize>1024 && Rate<5000)
				{
					Rate <<= 1;
					p->ReadSize >>= 1;
				}
			}
		}
	}

#if defined(TARGET_SYMBIAN)
	if (p->ReadSize > 4096) // large reads interfere with audio (until multithreaded is not added...)
		p->ReadSize = 4096;
#endif

	FindCoverArt(p);
}

void Format_BufferRelease(format_base* p, format_buffer* Buffer)
{
	//DEBUG_MSG4(DEBUG_FORMAT,"Buffer:%d release %d->%d (%d)",Buffer->Id,Buffer->RefCount,Buffer->RefCount-1,p->BufferUsed);

	LockEnter(p->BufferLock);

	assert(Buffer->RefCount>0 && Buffer->RefCount<1000);

	if (--Buffer->RefCount <= 0)
	{
		assert(Buffer->RefCount==0);
		--p->BufferUsed;
		if (p->BufferUsed <= p->BufferSize)
		{
			Buffer->Next = p->FreeBuffers;
			p->FreeBuffers = Buffer;
		}
		else
		{
			FreeBlock(&Buffer->Block);
			free(Buffer);
		}
	}

	LockLeave(p->BufferLock);
}

format_buffer* Format_BufferRemove(format_reader* Reader)
{
	format_buffer* Buffer;
	LockEnter(Reader->Format->BufferLock);
	Buffer = Reader->BufferFirst;
	if (Buffer)
	{
		Reader->BufferFirst = Buffer->Next;
		if (Reader->BufferLast == Buffer)
			Reader->BufferLast = NULL;
		Reader->BufferAvailable -= Buffer->Length;
		assert(Reader->BufferAvailable >= 0);
	}
	LockLeave(Reader->Format->BufferLock);
	return Buffer;
}

void Format_SingleRefRelease(format_base* p, format_ref* Ref)
{
	if (Ref->Buffer)
		Format_BufferRelease(p,Ref->Buffer);
	Ref->Next = p->FreeRefs;
	p->FreeRefs = Ref;
}

void Format_ReleaseRef(format_base* p, format_ref* Ref)
{
	while (Ref)
	{
		format_ref* Next = Ref->Next;
		Format_SingleRefRelease(p,Ref);
		Ref = Next;
	}
}

void Format_PacketRelease(format_base* p, format_packet* Packet)
{
	Format_ReleaseRef(p,Packet->Data);
	Packet->Data = NULL;
	Packet->Next = p->FreePackets;
	p->FreePackets = Packet;
}

static NOINLINE void ReleaseStream(format_base* p,format_stream* Stream)
{
	format_packet* Packet;

	Stream->State.DropLevel = 0;
	Stream->DropCount = 0;
	Stream->Pending = 0;
	Stream->LastTime = TIME_SYNC;

	while (Stream->PacketFirst)
	{
		Packet = Stream->PacketFirst;
		Stream->PacketFirst = Packet->Next;
		if (Stream->PacketLast == Packet)
			Stream->PacketLast = NULL;

		Format_PacketRelease(p,Packet);
	}

	if (p->ReleaseStream && Stream != p->SubTitle && Stream != p->CoverArt)
		p->ReleaseStream(p,Stream);
}

static int UpdateStreamPin(format_base* p,format_stream* Stream)
{
	if (!Stream->Pin.Node)
		ReleaseStream(p,Stream);
	return ERR_NONE;
}

void Format_AfterSeek(format_base* p)
{
	int No;

	// release stream packets
	for (No=0;No<p->StreamCount;++No)
		ReleaseStream(p,p->Streams[No]);

	if (p->SubTitle)
		ReleaseStream(p,p->SubTitle);

	p->LastRead = NULL;
	if (p->AfterSeek)
		p->AfterSeek(p);
}

void Format_ReleaseBuffers(format_reader* Reader)
{
	format_buffer* Buffer;

	// release read buffers
	Reader->NoMoreInput = 0;

	Reader->ReadPos = 0;
	if (Reader->ReadBuffer)
	{
		Format_BufferRelease(Reader->Format,Reader->ReadBuffer);
		Reader->ReadBuffer = NULL;
	}

	if (Reader->InputBuffer)
	{
		Format_BufferRelease(Reader->Format,Reader->InputBuffer);
		Reader->InputBuffer = NULL;
	}

	while ((Buffer = Format_BufferRemove(Reader))!=NULL)
		Format_BufferRelease(Reader->Format,Buffer);
}

format_reader* Format_FindReader(format_base* p,filepos_t Min,filepos_t Max)
{
	int No;
	filepos_t LimitMax;

	if (Max < Min+16)
		Max = Min+16;

	LimitMax = p->BufferSize*BLOCKSIZE/2;
	if (p->Duration>0)
	{
		// example a 55Mb/sec mjpeg with pcm audio can have large differences at end of file
		LimitMax = Scale(Max-Min,3*TICKSPERSEC,p->Duration);
		if (LimitMax < 512*1024)
			LimitMax = 512*1024;
	}

	for (No=0;No<MAXREADER;++No)
	{
		format_reader* Reader = p->Reader+No;
		filepos_t Limit = (Reader->OffsetMax - Reader->OffsetMin)/4;
		if (Limit<128*1024)
			Limit=128*1024;
		if (Limit>LimitMax)
			Limit=LimitMax;

		if (!Reader->OffsetMax ||
			(Reader->OffsetMin < Max && Reader->OffsetMax > Min && 
			 abs(Reader->OffsetMin-Min)<Limit && abs(Reader->OffsetMax-Max)<Limit))
		{	
			stream* Orig = p->Reader[0].Input;

			if (Reader->OffsetMin > Min)
				Reader->OffsetMin = Min;
			if (Reader->OffsetMax < Max)
				Reader->OffsetMax = Max;

			Reader->Ratio = Scale(MAX_INT,1,Reader->OffsetMax - Reader->OffsetMin);

			if (!Reader->Input && Orig)
			{
				bool_t Silent = 1;
				tchar_t URL[MAXPATH];
				stream* Input;
				
				if (Orig->Get(Orig,STREAM_URL,URL,sizeof(URL)) != ERR_NONE)
					break;
				
				Input = (stream*)NodeCreate(Orig->Class);
				if (!Input)
					break;

				Input->Set(Input,STREAM_SILENT,&Silent,sizeof(bool_t));

				if (Input->Set(Input,STREAM_URL,URL,sizeof(URL))!=ERR_NONE)
				{
					NodeDelete((node*)Input);
					break;
				}

				Reader->ReadPos = 0;
				Reader->FilePos = 0;
				Reader->Input = Input;
			}

			return Reader;
		}
	}

	return NULL;
}

static NOINLINE void Format_FreeStream(format_base* p,format_stream* Stream)
{
	PacketFormatClear(&Stream->Format);
	BufferClear(&Stream->BufferMem);
	FreeBlock(&Stream->BufferBlock);
	free(Stream);
}

int Format_Reset(format_base* p)
{
	int Result = ERR_NONE;
	format_packet* Packet;
	format_buffer* Buffer;
	format_reader* Reader;
	format_ref*	Ref;
	int No;

	if (p->BufferLock) // only if it was inited
	{
		Format_AfterSeek(p);
		for (No=0;No<MAXREADER;++No)
			Format_ReleaseBuffers(p->Reader+No);
	}

	if (p->ReleaseStreams.Func)
		p->ReleaseStreams.Func(p->ReleaseStreams.This,0,0);

	if (p->Done)
		p->Done(p);

	while (p->StreamCount)
		Format_RemoveStream(p);

	while (p->FreePackets)
	{
		Packet = p->FreePackets;
		p->FreePackets = Packet->Next;
		free(Packet);
	}

	while (p->FreeBuffers)
	{
		Buffer = p->FreeBuffers;
		p->FreeBuffers = Buffer->Next;
		FreeBlock(&Buffer->Block);
		free(Buffer);
	}

	while (p->FreeRefs)
	{
		Ref = p->FreeRefs;
		p->FreeRefs = Ref->Next;
		free(Ref);
	}

	if (p->SubTitle)
	{
		Format_FreeSubTitle(p,p->SubTitle);
		p->SubTitle = NULL;
	}

	if (p->CoverArt)
	{
		Format_FreeStream(p,p->CoverArt);
		p->CoverArt = NULL;
	}

	p->TotalCount = 0;
	p->StreamCount = 0;
	p->ActiveStreams = 0;

	LockDelete(p->BufferLock);
	LockDelete(p->InputLock);
	p->BufferLock = NULL;
	p->InputLock = NULL;
	p->LastRead = NULL;

	for (No=0;No<MAXREADER;++No)
	{
		Reader = p->Reader+No;
		if (No>0 && Reader->Input)
		{
			Reader->Input->Set(Reader->Input,STREAM_URL,NULL,0);
			NodeDelete((node*)Reader->Input);
			Reader->Input = NULL;
		}
		Reader->Ratio = -1;
		Reader->OffsetMin = MAX_INT;
		Reader->OffsetMax = 0;
		Reader->Current = NULL;
		Reader->OutOfSync = 0;
	}

	Reader = p->Reader;
	if (Reader->Input)
	{
		if (!NodeIsClass(Reader->Input->Class,STREAM_CLASS) &&
			!NodeIsClass(Reader->Input->Class,STREAMPROCESS_CLASS))
			return ERR_INVALID_PARAM;

		if (Reader->Input->Get(Reader->Input,STREAM_LENGTH,&p->FileSize,sizeof(p->FileSize)) != ERR_NONE)
			p->FileSize = -1;

		p->InputLock = LockCreate();
		p->BufferLock = LockCreate();
		p->Duration = TIME_UNKNOWN;
		p->FirstSync = 1;
		p->SyncMode = 1;			// start with syncmode
		p->SyncTime = TIME_SYNC;
		p->SyncStream = NULL;
		p->HeaderLoaded = 0;
		p->ProcessMinBuffer = PROCESSMINVIDEO;
		p->ReadSize = 16384;
		p->GlobalOffset = 0;
		p->SumByteRate = 0;

		Reader->FilePos = 0;
		Reader->NoMoreInput = 0;

		p->SeekByPacket_DataStart = 0;
		p->SeekByPacket_BlockAlign = 1;

		if (p->Init)
		{
			Result = p->Init(p);
			if (Result != ERR_NONE)
			{
				Reader->Input = NULL;
				Format_Reset(p);
			}
			else
				p->SyncRead = SYNCREAD;
		}	
	}
	return Result;
}

int FormatBaseGet(format_base* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	if (No >= FORMAT_COMMENT && No < FORMAT_COMMENT+p->TotalCount)  
		GETVALUE(p->Streams[No-FORMAT_COMMENT]->Comment,pin)
	else
	if (No >= FORMAT_STREAM && No < FORMAT_STREAM+p->TotalCount)
		GETVALUE(p->Streams[No-FORMAT_STREAM]->Pin,pin)
	else
	if (No >= FORMAT_STREAM_PRIORITY && No < FORMAT_STREAM_PRIORITY+p->TotalCount)
		GETVALUE(p->Streams[No-FORMAT_STREAM_PRIORITY]->Priority,int)
	else
	if (No >= (FORMAT_STREAM|PIN_PROCESS) && No < (FORMAT_STREAM|PIN_PROCESS)+p->TotalCount)
		GETVALUE(p->Streams[No-(FORMAT_STREAM|PIN_PROCESS)]->Process,packetprocess)
	else
	if (No >= (FORMAT_STREAM|PIN_FORMAT) && No < (FORMAT_STREAM|PIN_FORMAT)+p->TotalCount)
	{
		packetformat CodecFormat;
		format_stream* s = p->Streams[No-(FORMAT_STREAM|PIN_FORMAT)];
		assert(Size == sizeof(packetformat));
		if (s->Pin.Node && s->Pin.Node->Get(s->Pin.Node,s->Pin.No|PIN_FORMAT,&CodecFormat,sizeof(CodecFormat))==ERR_NONE)
			PacketFormatCombine(&s->Format,&CodecFormat);
		*(packetformat*)Data = s->Format;
		Result = ERR_NONE;
	}
	else
	switch (No)
	{
	case FORMAT_BUFFERSIZE: GETVALUE(p->BufferSize,int); break;
	case FORMAT_INPUT: GETVALUE(p->Reader[0].Input,stream*); break;
	case FORMAT_DURATION: GETVALUECOND(p->Duration,tick_t,p->Duration>=0); break;
	case FORMAT_FILEPOS: GETVALUECOND(p->Reader[0].FilePos,int,p->Timing && p->Reader[0].FilePos>=0); break;
	case FORMAT_FILEALIGN: GETVALUE(p->FileAlign,int); break;
	case FORMAT_STREAM_COUNT: GETVALUE(p->TotalCount,int); break;
	case FORMAT_FILESIZE: GETVALUECOND(p->FileSize/1024,int,p->FileSize>=0); break;
	case FORMAT_FIND_SUBTITLES: GETVALUE(p->NeedSubtitles,bool_t); break;
	case FORMAT_GLOBAL_COMMENT: GETVALUE(p->Comment,pin); break;
	case FORMAT_AUTO_READSIZE: GETVALUE(p->AutoReadSize,bool_t); break;
	case FORMAT_TIMING: GETVALUE(p->Timing,bool_t); break;
	case FORMAT_COVERART: GETVALUECOND(p->StreamCount+(p->SubTitle!=NULL),int,p->CoverArt!=NULL); break;
	case FORMAT_MIN_BUFFER: GETVALUE(p->ProcessMinBuffer/BLOCKSIZE,int); break;
	}
	return Result;
}

int FormatBaseSet(format_base* p,int No,const void* Data,int Size)
{
	node* Advanced;
	uint8_t* Ptr;
	int Result = ERR_INVALID_PARAM;

	if (No >= FORMAT_COMMENT && No < FORMAT_COMMENT+p->TotalCount)
		SETVALUE(p->Streams[No-FORMAT_COMMENT]->Comment,pin,ERR_NONE)
	else
	if (No >= FORMAT_STREAM && No < FORMAT_STREAM+p->TotalCount)
	{
		format_stream* s = p->Streams[No-FORMAT_STREAM];
		if (s->Pin.Node && No-FORMAT_STREAM<p->StreamCount) p->ActiveStreams--;
		SETVALUE(s->Pin,pin,UpdateStreamPin(p,s));
		if (s->Pin.Node && No-FORMAT_STREAM<p->StreamCount) p->ActiveStreams++;
	}
	else
	if (No >= (FORMAT_STREAM|PIN_PROCESS) && No < (FORMAT_STREAM|PIN_PROCESS)+p->TotalCount)
		SETVALUE(p->Streams[No-(FORMAT_STREAM|PIN_PROCESS)]->Process,packetprocess,ERR_NONE)
	else
	switch (No)
	{
	case FORMAT_INPUT: SETVALUENULL(p->Reader[0].Input,stream*,Format_Reset(p),p->Reader[0].Input=NULL); break;
	case FORMAT_UPDATESTREAMS: SETVALUE(p->UpdateStreams,notify,ERR_NONE); break;
	case FORMAT_RELEASESTREAMS: SETVALUE(p->ReleaseStreams,notify,ERR_NONE); break;
	case FORMAT_FIND_SUBTITLES: SETVALUE(p->NeedSubtitles,bool_t,ERR_NONE); break;
	case FORMAT_GLOBAL_COMMENT: SETVALUE(p->Comment,pin,ERR_NONE); break;
	case FORMAT_FILEALIGN: SETVALUE(p->FileAlign,int,ERR_NONE); break;
	case FORMAT_AUTO_READSIZE: SETVALUE(p->AutoReadSize,bool_t,ERR_NONE); break;

	case NODE_HIBERNATE:
		assert(Size == sizeof(int));
		Result = Format_Hibernate(p,*(int*)Data);
		break;

	case NODE_SETTINGSCHANGED:
		Advanced = Context()->Advanced;
		if (Advanced)
		{
			Advanced->Get(Advanced,ADVANCED_DROPTOL,&p->DropTolerance,sizeof(tick_t));
			Advanced->Get(Advanced,ADVANCED_SKIPTOL,&p->SkipTolerance,sizeof(tick_t));
			Advanced->Get(Advanced,ADVANCED_AVOFFSET,&p->AVOffset,sizeof(tick_t));
		}
		break;

	case FORMAT_DATAFEED:

		Ptr = (uint8_t*) Data;
		while (Size)
		{
			format_buffer* Buffer;

			Buffer = Format_BufferAlloc(p,2);
			if (Buffer)
			{
				Buffer->Length = Size;
				if (Buffer->Length > BLOCKSIZE)
					Buffer->Length = BLOCKSIZE;

				WriteBlock(&Buffer->Block,0,Ptr,Buffer->Length);

				Ptr += Buffer->Length;
				Size -= Buffer->Length;

				Format_BufferInsert(p->Reader,Buffer);
			}
		}

		break;

	case FORMAT_BUFFERSIZE: SETVALUE(p->BufferSize,int,ERR_NONE); break;
	}
	return Result;
}

format_packet* Format_PacketAlloc(format_base* p)
{
	format_packet* Packet = p->FreePackets;

	if (!Packet)
	{
		Packet = (format_packet*) malloc(sizeof(format_packet));
		if (!Packet)
			return NULL;
	}
	else
		p->FreePackets = Packet->Next;

	Packet->Data = NULL;
	Packet->RefTime = TIME_UNKNOWN;
	Packet->EndRefTime = TIME_UNKNOWN;
	Packet->Stream = NULL;
	Packet->Next = NULL;
	Packet->Key = 1;

	return Packet;
}

static int Format_Sync(format_base* p,tick_t Time,int FilePos,bool_t PrevKey)
{
	if (!p->Seek || !p->Reader[0].Input)
		return ERR_NOT_SUPPORTED;

	if (!p->HeaderLoaded)
		return ERR_LOADING_HEADER;

	p->SyncMode = 1;
	p->SyncRead = SYNCREAD;
	p->SyncTime = TIME_SYNC; // set before p->Seek
	p->SyncStream = NULL;

	if (Time>=0 || FilePos>=0)
	{
		int Result = p->Seek(p,Time,FilePos,PrevKey);
		if (Result != ERR_NONE)
		{
			p->SyncMode = 0;
			return Result;
		}
	}

	return ERR_NONE;
}

int Format_CheckEof(format_base* p,format_stream* Stream)
{
	int Result;

	if (!p->Timing && p->ProcessTime>=0 && p->ProcessTime<5*TICKSPERSEC)
		return ERR_BUFFER_FULL;

	Stream->State.CurrTime = p->ProcessTime;
	Stream->State.DropLevel = 0;

	Result = Stream->Process(Stream->Pin.Node,NULL,&Stream->State);
	if (Result != ERR_BUFFER_FULL)
		Result = ERR_END_OF_FILE;

	return Result;
}

static NOINLINE int Synced(format_base* p,format_stream* Stream)
{
	p->SyncMode = 0;

	if (p->SyncTime == TIME_SYNC)
	{
		p->SyncTime = Stream->Packet.RefTime;
		if (p->SyncTime < 0)
			p->SyncTime = Stream->LastTime;
	}

	if (p->FirstSync)
	{
		// some codecs may need the first few packets as headers
		int No;
		p->ProcessTime = 0;
		for (No=0;No<p->StreamCount;++No)
			if (p->Streams[No]->Pin.Node && p->Streams[No]->PacketFirst)
				p->Process(p,p->Streams[No]); // try processing
		p->ProcessTime = TIME_SYNC;
	}

	return ERR_SYNCED;
}

int Format_Send(format_base* p,format_stream* Stream)
{
	int Result;
	tick_t CurrTime = p->ProcessTime;
	
	Stream->State.CurrTime = CurrTime;

	if (Stream->Packet.RefTime >= 0 && !Stream->DisableDrop)
	{
		Stream->State.DropLevel = 0;

		if (CurrTime >= 0)
		{
			tick_t Diff = CurrTime - Stream->Packet.RefTime;

			if (Diff < -TICKSPERSEC/2 && Stream->Format.Type == PACKET_VIDEO)
			{
				// make sure buffers and pending packets are processed
				Stream->Process(Stream->Pin.Node,NULL,&Stream->State);
				return ERR_BUFFER_FULL;
			}

			if (Diff > p->DropTolerance)
			{
				if (Diff > p->SkipTolerance + p->DropTolerance)
				{
					Stream->State.DropLevel = 2;
					Stream->DropCount = DROP_SHOW;
				}
				else
				{
					if (!Stream->State.DropLevel)
						Stream->DropCount = 0;

					if (Stream->DropCount >= DROP_SHOW)
					{
						Stream->State.DropLevel = 0; // show one packet
						Stream->DropCount = 0;
					}
					else
					{
						Stream->State.DropLevel = 1;
						++Stream->DropCount;
					}
				}
			}
		}
	}

	DEBUG_MSG5(DEBUG_FORMAT,T("Send stream:%d size:%d time:%d curr:%d drop:%d"),Stream->No,Stream->Packet.Length,Stream->Packet.RefTime,CurrTime,Stream->State.DropLevel);

	Result = Stream->Process(Stream->Pin.Node,&Stream->Packet,&Stream->State);

	// packet not processed?
	if (Result == ERR_BUFFER_FULL)
	{
		DEBUG_MSG1(DEBUG_FORMAT,T("Send stream:%d BufferFull"),Stream->No);
		return Result;
	}

	if (p->Sended)
		p->Sended(p,Stream);

	Stream->Pending = 0;

	// sync: found a valid frame?
	if (p->SyncMode && (Result == ERR_NONE || Result == ERR_NOT_SUPPORTED))
		Result = Synced(p,Stream);

	return Result;
}

void Format_TimeStampRestarted(format_base* p,format_stream* Stream,tick_t* RefTime)
{
	if (!p->InSeek)
	{
		format_packet* i;
		*RefTime -= p->GlobalOffset;

		p->GlobalOffset = Stream->LastTime; // last processed time
		for (i=Stream->PacketFirst;i;i=i->Next)
			if (i->RefTime>=0)
				p->GlobalOffset = i->RefTime; // last packet time

		*RefTime += p->GlobalOffset;
		p->TimeStamps = 0;
	}
}

int Format_FillQueue(format_base* p,format_reader* Reader)
{
	int Result;
	format_packet* Packet;
	
	if (!Reader->BufferAvailable && !Reader->ReadBuffer && p->ProcessMinBuffer!=0 &&
		(!p->SyncMode || p->SyncRead<=0 || Reader->NoMoreInput))
		return ERR_NEED_MORE_DATA;

	Packet = Format_PacketAlloc(p);
	if (!Packet)
		return ERR_OUT_OF_MEMORY;

	Result = p->ReadPacket(p,Reader,Packet);

	if (Result == ERR_NONE && Packet->Stream && Packet->Stream->Pin.Node)
	{
		format_stream* s = Packet->Stream;

		if (Packet->RefTime>=0)
		{
			Packet->RefTime += p->GlobalOffset;

			if (s->Format.Type == PACKET_AUDIO)
			{
				Packet->RefTime += p->AVOffset;
				if (Packet->RefTime < 0)
					Packet->RefTime = 0;
			}

			if (s->LastTime<0)
				s->LastTime = Packet->RefTime; // found reftime
			else
			if (s->LastTime > Packet->RefTime+TICKSPERSEC  && Packet->RefTime-p->GlobalOffset<TICKSPERSEC)
				Format_TimeStampRestarted(p,s,&Packet->RefTime);
		}

		if (s->LastTime >= TIME_UNKNOWN)
		{
			// add packet to stream
			if (!s->PacketFirst)
				s->PacketFirst = Packet;
			else
				s->PacketLast->Next = Packet;
			s->PacketLast = Packet;

			if (Packet->EndRefTime>=0 && s->Format.Type == PACKET_SUBTITLE)
			{
				// add empty packet
				format_packet* Tail = Format_PacketAlloc(p);
				if (Tail)
				{
					Tail->Stream = Packet->Stream;
					Tail->RefTime = Packet->EndRefTime;

					if (!s->PacketFirst)
						s->PacketFirst = Tail;
					else
						s->PacketLast->Next = Tail;
					s->PacketLast = Tail;
				}
			}
		}
		else
			Format_PacketRelease(p,Packet);
	}
	else
		Format_PacketRelease(p,Packet);

	return Result;
}

NOINLINE bool_t Format_AllocBufferBlock(format_base* p,format_stream* Stream,int Size)
{
	FreeBlock(&Stream->BufferBlock);
	if (Size>0)
	{
		Size = (Size + SAFETAIL + 0x7FFF) & ~0x7FFF;
		if (AllocBlock(Size,&Stream->BufferBlock,0,HEAP_STORAGE))
			Size -= SAFETAIL;
		else
			Size = 0;
	}
	Stream->BufferBlockLength = Size;
	return Size>0;
}

/*
static bool_t ReleasePacket(format_base* p)
{
	int No;
	format_stream* Min = NULL;
	tick_t MinTime = MAX_TICK;

	for (No=0;No<p->StreamCount;++No)
	{
		format_stream* Stream = p->Streams[No];
		if (Stream->PacketFirst && Stream->PacketFirst->RefTime < MinTime)
		{
			Min = Stream;
			MinTime = Stream->PacketFirst->RefTime;
		}
	}

	if (Min)
	{
		format_packet* Packet = Min->PacketFirst;

		Min->PacketFirst = Packet->Next;
		if (Min->PacketLast == Packet)
			Min->PacketLast = NULL;

		Format_PacketRelease(p,Packet);
		return 1;
	}

	return 0;
}
*/

static int Format_ProcessStream(format_base* p,format_stream* Stream)
{
	format_packet* Packet;
	format_ref* Ref;
	int Result = ERR_NONE;
	int No;
	int Burst;

	if (Stream->Pending)
	{
		Result = Format_Send(p,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
		{
			if (p->SyncMode && Stream == p->SyncStream) // can happen when sync without seek 
				Result = Synced(p,Stream);
			return Result;
		}
	}

	// process a limited number of packets at once (other streams need cpu time too)

	Burst = Stream->PacketBurst;

	for (No=0;No<Burst;++No)
	{
		while (!Stream->PacketFirst)
		{
			format_reader* Reader = Stream->Reader;

			/*
			if (p->BufferUsed >= p->BufferSize && p->SyncMode && Stream == p->SyncStream)
			{
				// try dropping other stream packets to make buffer for sync stream
				while (p->BufferUsed >= p->BufferSize)
					if (!ReleasePacket(p))
						break;
			}
			*/

			// prevent audio from going too much ahead
			if (Reader->BufferAvailable < p->ProcessMinBuffer && !Reader->NoMoreInput && 
				(!p->SyncMode || p->SyncRead<=0 || Stream != p->SyncStream))
				return ERR_NEED_MORE_DATA;

			Result = Format_FillQueue(p,Reader);

			if (Reader->NoMoreInput && (Result == ERR_END_OF_FILE || Result == ERR_NEED_MORE_DATA))
				Result = Format_CheckEof(p,Stream);

			if (Result != ERR_NONE || (p->Bench && (No || !Stream->PacketFirst)))
				return Result;
		}

		Packet = Stream->PacketFirst;

		// sync: all non SyncStream packets are stored (or dropped)
		if (p->SyncMode && Stream != p->SyncStream && p->SyncStream)
		{
			// catch up to syncstream (drop packets)
			tick_t LastTime = p->SyncStream->LastTime;
			if (p->SyncStream->Fragmented)
				LastTime -= TICKSPERSEC/2; // LastTime may not be the correct sync time

			while (Packet && Packet->RefTime < LastTime)
			{
				Stream->PacketFirst = Packet->Next;
				if (Stream->PacketLast == Packet)
					Stream->PacketLast = NULL;

				Format_PacketRelease(p,Packet);

				Packet = Stream->PacketFirst;
			}
			return ERR_NEED_MORE_DATA; // allow SyncStream to need more data
		}

		//DEBUG_MSG3(DEBUG_FORMAT,T("Packet stream:%d time:%d pos:%d"),Stream->No,Packet->RefTime,Packet->FilePos);

		// build output packet

		if (Packet->RefTime >= 0)
			Stream->LastTime = Packet->RefTime;

		Stream->Packet.RefTime = Packet->RefTime;
		Stream->Packet.Key = Packet->Key;
		Ref = Packet->Data;

		if (!Ref)
		{
			Stream->Packet.Data[0] = NULL;
			Stream->Packet.Length = 0;
			Stream->Pending = 1;
		}
		else
		if (!Stream->Fragmented)
		{
			if (Ref->Next || (Ref->Length + Ref->Begin + SAFETAIL > BLOCKSIZE) || Stream->ForceMerge)
			{
				// merge references
				if (p->UseBufferBlock)
				{
					int Total = 0;
					format_ref* i;
					for (Total=0,i=Ref;i;i=i->Next)
						Total += i->Length;

					if (Stream->BufferBlockLength < Total && !Format_AllocBufferBlock(p,Stream,Total))
						Ref = NULL;

					for (Total=0,i=Ref;i;i=i->Next)
					{
						WriteBlock(&Stream->BufferBlock,Total,i->Buffer->Block.Ptr + i->Begin,i->Length);
						Total += i->Length;
					}

					Stream->Packet.Data[0] = Stream->BufferBlock.Ptr;
					Stream->Packet.Length = Total;
				}
				else
				{
					BufferDrop(&Stream->BufferMem);
					for (;Ref;Ref=Ref->Next)
						BufferWrite(&Stream->BufferMem,Ref->Buffer->Block.Ptr + Ref->Begin, Ref->Length,16384);

					Stream->Packet.Data[0] = Stream->BufferMem.Data;
					Stream->Packet.Length = Stream->BufferMem.WritePos;
				}
			}
			else
			{
				// single reference
				Stream->Packet.Data[0] = Ref->Buffer->Block.Ptr + Ref->Begin;
				Stream->Packet.Length = Ref->Length;
			}
			Stream->Pending = 1;
		}
		else
		{
			// send first reference (doesn't matter how long)
			Stream->Packet.Data[0] = Ref->Buffer->Block.Ptr + Ref->Begin;
			Stream->Packet.Length = Ref->Length;
			Stream->Pending = 2; // indicate parital sending
		}

		Result = Format_Send(p,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			break;

		if (Stream->State.DropLevel==2 && Stream->PacketFirst && Burst<10 && No==Burst-1)
			++Burst; // try to catch up
	}

	// buffer full: there was possibly some processing so don't allow sleeping
	// need data: burst count exceeded, but it doesn't mean there is no data left in buffers
	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}

static void Format_Sended(format_base* p, format_stream* Stream)
{
	format_packet* Packet = Stream->PacketFirst;
	if (Packet)
	{
		if (Stream->Pending == 2 && Packet->Data)
		{
			// release sended references
			format_ref* Ref = Packet->Data;
			Packet->Data = Ref->Next;
			Format_SingleRefRelease(p,Ref);

			if (Packet->Data)
			{
				// packet still not finished
				Packet->RefTime = TIME_UNKNOWN;
				return;
			}
		}

		// full packet is released
		Stream->PacketFirst = Packet->Next;
		if (Stream->PacketLast == Packet)
			Stream->PacketLast = NULL;

		Format_PacketRelease(p,Packet);
	}
}

NOINLINE format_stream* Format_DefSyncStream(format_base* p, format_reader* Reader)
{
	format_stream* Found = NULL;
	format_stream* Stream;
	int No;

	// prefer video streams
	for (No=0;No<p->StreamCount;++No)
	{
		Stream = p->Streams[No];
		if (Stream->Pin.Node && (!Reader || Stream->Reader==Reader))
		{
			if (Stream->Format.Type == PACKET_VIDEO)
				return Stream;

			if (!Found)
				Found = Stream;
		}
	}

	return Found;
}

static bool_t TryReSend(node* p)
{
	if (p)
	{
		bool_t Buffered;
		if (p->Get(p,FLOW_BUFFERED,&Buffered,sizeof(Buffered))==ERR_NONE && Buffered)
			return p->Set(p,FLOW_RESEND,NULL,0) == ERR_NONE;

		if (!NodeIsClass(p->Class,OUT_CLASS))
		{
			pin Pin;
			node* Node;
			datadef DataDef;
			int No;

			for (No=0;NodeEnum(p,No,&DataDef)==ERR_NONE;++No)
				if (DataDef.Flags & DF_OUTPUT)
				{
					switch (DataDef.Type)
					{
					case TYPE_NODE:
						if (p->Get(p,DataDef.No,&Node,sizeof(Node))==ERR_NONE && TryReSend(Node))
							return 1;
						break;

					case TYPE_PACKET:
						if (p->Get(p,DataDef.No,&Pin,sizeof(Pin))==ERR_NONE && TryReSend(Pin.Node))
							return 1;
						break;
					}
				}
		}
	}
	return 0;
}

static int DummyError(void* This,int Param,int Param2)
{
	return ERR_NONE;
}

static NOINLINE int ProcessCoverArt(format_base* p,format_stream* Stream)
{
	int Result;
	notify Error;
	context* c = Context();
	node* Node = Stream->Pin.Node;
	if (!Node)
		return ERR_NONE;
	
	if (TryReSend(Node))
		return ERR_NONE;

	// suppress covert art decoding error messages
	Error = c->Error;
	c->Error.Func = DummyError;

	Stream->State.CurrTime = TIME_SYNC;
	Stream->State.DropLevel = 0;
	Result = Stream->Process(Node,&Stream->Packet,&Stream->State);

	c->Error = Error;
	return Result;
}

static int Format_Process(format_base* p,processstate* State)
{
	int No;
	int Result;
	int AllNeedMoreData = 1;
	int AllEndOfFile = 1;
	
	assert(State->Time>=0 || State->Time == TIME_BENCH);

	State->BufferUsedBefore = p->BufferUsed;
	p->ProcessTime = State->Time;
	p->Bench = p->ProcessTime < 0;

	if (!p->HeaderLoaded)
	{
		// load format begining to find streams
		format_reader* Reader = p->Reader;

		Result = p->FillQueue(p,Reader);

		if (Reader->FilePos > HEADERLOAD || (p->StreamCount && Reader->FilePos > p->MinHeaderLoad) || Reader->Eof(Reader))
			p->HeaderLoaded = 1;

		State->BufferUsedAfter = p->BufferUsed;
		return Result;
	}

	if (p->SyncMode)
	{
		p->ProcessTime = TIME_SYNC;
		// we need to choose one stream (to find a non dropped frame)
		if (!p->SyncStream)
		{
			p->SyncStream = Format_DefSyncStream(p,NULL);
			if (!p->SyncStream)
			{
				// no sync needed
				p->SyncMode = 0;

				// try to calc duration once (after first sync)
				if (p->FirstSync)
				{
					p->FirstSync = 0;
					CalcDuration(p);
				}

				if (p->CoverArt)
					ProcessCoverArt(p,p->CoverArt);

				// leave time as it is
				State->BufferUsedAfter = p->BufferUsed;
				return ERR_SYNCED;
			}
		}
	}

	// if all streams are full: caller can go to sleep
	Result = ERR_BUFFER_FULL;

	if (p->SubTitle)
		Format_ProcessSubTitle(p,p->SubTitle);

	for (No=0;No<p->StreamCount;++No)
	{
		format_stream* Stream = p->Streams[No];
		if (Stream->Pin.Node)
		{
			int i = p->Process(p,Stream);

			if (!p->SyncMode || p->SyncStream == Stream)
			{
				if (i == ERR_NEED_MORE_DATA)
				{
					if (!Stream->Reader->NoMoreInput && Stream->Format.Type != PACKET_SUBTITLE)
					{
						// if less then 1/8 sec is left and stream needs data -> force need more data mode
						tick_t LastTime = Stream->LastTime;
						if (State->Fill || (LastTime >= 0 && p->ProcessTime >= 0 && LastTime < p->ProcessTime + TICKSPERSEC/8))
							Result = ERR_NEED_MORE_DATA;
					}
				}
				else
					AllNeedMoreData = 0;

				if (i != ERR_END_OF_FILE)
					AllEndOfFile = 0;
			}

			if (i == ERR_SYNCED)
			{
				// try to calc duration once (after first sync)
				if (p->FirstSync)
				{
					p->FirstSync = 0;
					CalcDuration(p);
				}

				if (p->CoverArt)
					ProcessCoverArt(p,p->CoverArt);

				State->Time = p->SyncTime;
				State->BufferUsedAfter = p->BufferUsed;
				return i;
			}

			if (Result == ERR_BUFFER_FULL &&
				i != ERR_BUFFER_FULL && 
				i != ERR_END_OF_FILE && 
				i != ERR_NEED_MORE_DATA)
				Result = ERR_NONE; // no sleep

			if (Result == ERR_NONE && Stream->State.DropLevel)
				Result = ERR_DROPPING;
		}
	}

	if (!p->ActiveStreams)
	{
		// release buffers
		Result = ERR_END_OF_FILE;

		for (No=0;No<MAXREADER;++No)
		{
			format_buffer* Buffer;
			format_reader* Reader = p->Reader+No;
			if (!Reader->Input)
				break;
			while ((Buffer = Format_BufferRemove(Reader))!=NULL)
			{
				Reader->FilePos += Buffer->Length;
				Format_BufferRelease(p,Buffer);
			}
			if (!Reader->NoMoreInput)
				Result = ERR_BUFFER_FULL; // return buffer full so fill mode doesn't go though file
		}

		State->Time = TIME_UNKNOWN; // player should use file position
	}
	else
	{
		if (AllEndOfFile) // all end of file?
			Result = ERR_END_OF_FILE;
		else
		if (AllNeedMoreData) // all need data?
		{
			Result = ERR_END_OF_FILE;
			for (No=0;No<MAXREADER;++No)
			{
				if (!p->Reader[No].Input)
					break;
				if (!p->Reader[No].NoMoreInput)
				{
					Result = ERR_NEED_MORE_DATA;
					break;
				}
			}
		}
		else
		if (Result == ERR_BUFFER_FULL && State->Fill)
		{
			// don't allow exiting fill mode if buffer is less than ProcessMinBuffer+BLOCKSIZE
			// we might fallback to loadmode right after starting playback
			if (p->Reader->BufferAvailable < p->ProcessMinBuffer+BLOCKSIZE && !p->Reader->NoMoreInput)
				Result = ERR_NEED_MORE_DATA; 
		}
	}

	if (p->SyncMode && Result == ERR_END_OF_FILE)
	{
		p->SyncMode = 0;
		if (p->Duration<0 && p->SyncStream)
			State->Time = p->SyncStream->LastTime;
		else
			State->Time = p->Duration;
		Result = ERR_SYNCED;
	}

	State->BufferUsedAfter = p->BufferUsed;
	return Result;
}

format_ref* Format_RefAlloc(format_base* p, format_buffer* To, int Begin, int Length)
{
	format_ref* Ref = p->FreeRefs;

	if (!Ref)
	{
		Ref = (format_ref*) malloc(sizeof(format_ref));
		if (!Ref)
			return NULL;
	}
	else
		p->FreeRefs = Ref->Next;

	LockEnter(p->BufferLock);

	//DEBUG_MSG3(DEBUG_FORMAT,"Buffer:%d addref %d->%d",To->Id,To->RefCount,To->RefCount+1);
	++To->RefCount;

	LockLeave(p->BufferLock);

	Ref->Length = Length;
	Ref->Begin = Begin;
	Ref->Buffer = To;
	Ref->Next = NULL;
	return Ref;
}

bool_t Format_ReadBuffer(format_reader* Reader, bool_t ToRead)
{
	bool_t Result;

	LockEnter(Reader->Format->InputLock);
	LockEnter(Reader->Format->BufferLock);

	if (!Reader->BufferFirst && !Reader->NoMoreInput)
	{
		format_buffer* Buffer = Reader->InputBuffer;

		if (!Buffer)
			Buffer = Format_BufferAlloc(Reader->Format,1);

		if (Buffer)
		{
			// use partial buffer loading after sync for a few times (SYNCREAD)
			// but later have to use full buffer (matroska seeking example can run out of buffer...)
			int TargetLength = (ToRead && Reader->Format->SyncRead>0)? Reader->Format->ReadSize:BLOCKSIZE;
			if (Buffer->Length >= TargetLength)
			{
				Format_BufferInsert(Reader,Buffer);
				Reader->InputBuffer = NULL;
			}
			else
			{
				int Length = Reader->Input->ReadBlock(Reader->Input,&Buffer->Block,Buffer->Length,TargetLength - Buffer->Length);
				if (Length < 0)
				{
					// failed
					if (!Reader->InputBuffer)
						Format_BufferRelease(Reader->Format,Buffer);
				}
				else
				{
					Buffer->Length += Length;
					Reader->NoMoreInput = Length == 0;

					if (!Length && !Reader->InputBuffer)
						Format_BufferRelease(Reader->Format,Buffer);
					else
					{
						Format_BufferInsert(Reader,Buffer);
						Reader->InputBuffer = NULL;
					}
				}
			}
		}

		if (Reader->Format->SyncRead>0)
			Reader->Format->SyncRead--;
	}

	Result = Reader->BufferFirst != NULL;

	if (Result && ToRead)
	{
		Reader->ReadBuffer = Format_BufferRemove(Reader);
		if (Reader->ReadBuffer)
			Reader->ReadLen = Reader->ReadBuffer->Length;
	}

	LockLeave(Reader->Format->BufferLock);
	LockLeave(Reader->Format->InputLock);

	return Result;
}

format_ref* Format_DupRef(format_base* p, format_ref* Chain, int Offset, int Length)
{
	format_ref* Head = NULL;
	format_ref** Ptr = &Head;

	while (Chain && Offset >= Chain->Length)
	{
		Chain = Chain->Next;
		Offset -= Chain->Length;
	}

	while (Chain && Length > 0)
	{
		int Len = Chain->Length - Offset;
		if (Len > Length)
			Len = Length;

		*Ptr = Format_RefAlloc(p,Chain->Buffer,Chain->Begin+Offset,Len);
		if (!*Ptr)
			break;

		Chain = Chain->Next;
		Length -= Len;
		Offset = 0;

		Ptr = &(*Ptr)->Next;
	}

	return Head;
}

//---------------------------------------------------------------------------------------------

static bool_t Reader_GetReadBuffer(format_reader* Reader)
{
	if (!Reader->Input) // external reader
	{
		Reader->ReadBuffer = NULL;
		return 0;
	}

	// release old buffer
	if (Reader->ReadBuffer)
	{
		Reader->ReadPos -= Reader->ReadBuffer->Length;
		Format_BufferRelease(Reader->Format,Reader->ReadBuffer);
	}

	// get new buffer from BufferFirst..BufferLast chain
	Reader->ReadBuffer = Format_BufferRemove(Reader);
	if (Reader->ReadBuffer)
	{
		Reader->ReadLen = Reader->ReadBuffer->Length;
		return 1;
	}

	// load input now (can't wait for input thread)
	return Format_ReadBuffer(Reader,1);
}

format_ref* Reader_ReadAsRef(format_reader* p, int Length)
{
	int n;
	format_ref* Ref;
	format_ref** RefPtr;

	Ref = NULL;
	RefPtr = &Ref;

	if (Length < 0) // read as much as possible from one buffer
	{
		while (!p->ReadBuffer || p->ReadPos >= p->ReadLen)
			if (!p->GetReadBuffer(p))
				return NULL;

		n = p->ReadLen - p->ReadPos;
		if (n > -Length)
			n = -Length;
		Length = n;
	}

	while (Length > 0)
	{
		while (!p->ReadBuffer || p->ReadPos >= p->ReadLen)
			if (!p->GetReadBuffer(p))
				return Ref;

		n = p->ReadLen - p->ReadPos;
		if (n > Length)
			n = Length;

		*RefPtr = Format_RefAlloc(p->Format,p->ReadBuffer,p->ReadPos,n);
		if (!*RefPtr)
			break;

		RefPtr = &(*RefPtr)->Next;

		Length -= n;
		p->FilePos += n;
		p->ReadPos += n;
	}
	return Ref;
}

static void Reader_Skip(format_reader* p,int n)
{
	if (n > 0)
	{
		assert(n < 0x10000000);
		p->FilePos += n;
		p->ReadPos += n;
	}
}

static int Reader_Seek(format_reader* Reader,filepos_t Pos,int Origin)
{
	int Adjust = 0;

	if (Reader->FilePos >= 0 && !Reader->Format->DisableReader)
	{
		filepos_t RelPos = Pos;
		if (Origin == SEEK_SET)
			RelPos -= Reader->FilePos;
		if (Origin != SEEK_END)
		{
			// check if we can use Format_Skip
			if (RelPos >= 0 && RelPos < Reader->BufferAvailable + SKIPDISTANCE)
			{
				Reader_Skip(Reader,RelPos);
				return ERR_NONE;
			}

			// check if we are in the same ReadBuffer
			if (RelPos < 0 && Reader->ReadBuffer && Reader->ReadPos + RelPos >= 0)
			{
				Reader->FilePos += RelPos;
				Reader->ReadPos += RelPos;
				return ERR_NONE;
			}
		}
	}

	if (!Reader->Input)
		return ERR_NOT_SUPPORTED;

	if (Origin == SEEK_CUR)
	{
		// input is at a different position already, using SEEK_SET
		Origin = SEEK_SET;
		Pos += Reader->FilePos;
	}

	if (Origin == SEEK_SET)
	{
		// align reading for local files
		Adjust = Pos & (Reader->Format->FileAlign-1);
		Pos &= ~(Reader->Format->FileAlign-1);
	}

	LockEnter(Reader->Format->InputLock);
	Pos = Reader->Input->Seek(Reader->Input,Pos,Origin);
	if (Pos < 0)
	{
		LockLeave(Reader->Format->InputLock);
		return ERR_NOT_SUPPORTED;
	}

	// query again file size, because http could have reopened connection
	if (Reader->Input->Get(Reader->Input,STREAM_LENGTH,&Reader->Format->FileSize,sizeof(Reader->Format->FileSize)) != ERR_NONE)
		Reader->Format->FileSize = -1;

	Format_ReleaseBuffers(Reader);

	Reader->FilePos = Pos + Adjust;
	Reader->ReadPos += Adjust;

	LockLeave(Reader->Format->InputLock);

	return ERR_NONE;
}

static bool_t Reader_Eof(format_reader* p)
{
	return !p->BufferAvailable && !p->ReadBuffer && p->NoMoreInput;
}

static int Reader_Read(format_reader* p, void* Data, int Length0)
{
	uint8_t* Ptr = (uint8_t*)Data;
	int Length = Length0;
	int n;
	while (Length > 0)
	{
		while (!p->ReadBuffer || p->ReadPos >= p->ReadLen)
			if (!p->GetReadBuffer(p))
				return Length0 - Length;

		n = p->ReadLen - p->ReadPos;
		if (n > Length)
			n = Length;

		memcpy(Ptr,p->ReadBuffer->Block.Ptr+p->ReadPos,n);
		Ptr += n;
		Length -= n;
		p->FilePos += n;
		p->ReadPos += n;
	}

	return Length0;
}

static int Reader_Read8(format_reader* p)
{
	while (!p->ReadBuffer || p->ReadPos >= p->ReadLen)
		if (!p->GetReadBuffer(p))
			return -1;

	p->FilePos++;
	return p->ReadBuffer->Block.Ptr[p->ReadPos++];
}

static int Reader_ReadBE16(format_reader* p)
{
	int v = Reader_Read8(p) << 8;
	v |= Reader_Read8(p);
	return v;
}

static int Reader_ReadLE16(format_reader* p)
{
	int v = Reader_Read8(p);
	v |= Reader_Read8(p) << 8;
	return v;
}

static int Reader_ReadBE32(format_reader* p)
{
	int v = Reader_Read8(p) << 24;
	v |= Reader_Read8(p) << 16;
	v |= Reader_Read8(p) << 8;
	v |= Reader_Read8(p);
	return v;
}

static int Reader_ReadLE32(format_reader* p)
{
	int v = Reader_Read8(p);
	v |= Reader_Read8(p) << 8;
	v |= Reader_Read8(p) << 16;
	v |= Reader_Read8(p) << 24;
	return v;
}

static int64_t Reader_ReadBE64(format_reader* p)
{
	int64_t v = (int64_t)Reader_ReadBE32(p) << 32;
	v |= (uint32_t)Reader_ReadBE32(p);
	return v;
}

static int64_t Reader_ReadLE64(format_reader* p)
{
	int64_t v = (uint32_t)Reader_ReadLE32(p);
	v |= (int64_t)Reader_ReadLE32(p) << 32;
	return v;
}

static NOINLINE int GetRate(format_reader* p,format_reader* Last)
{
	int64_t Rate;

	if (p->NoMoreInput)
		return MAX_INT;

	if (p->BufferAvailable <= PROCESSMINVIDEO)
		return -MAX_INT;

	Rate = (int64_t)p->Ratio * (p->BufferAvailable - (p==Last?READER_BONUS:0));
	if (Rate > MAX_INT)
		return MAX_INT;
	return (int)Rate;
}

static int Format_ReadInput(format_base* p,int BufferMax,int* BufferUsed)
{
	int No;
	int Left,Length;
	int Result = ERR_NONE;
	format_buffer* Buffer;
	format_reader* Reader;
	int Used = p->BufferUsed;

	// simple estimate test without locking BufferUsed
	if (Used >= BufferMax)
	{
		*BufferUsed = Used;
		return ERR_BUFFER_FULL;
	}

	LockEnter(p->InputLock);
	LockEnter(p->BufferLock);

	// select which reader to use
	Reader = p->Reader;
	if (p->Reader[1].Input)
	{
		int Lowest = GetRate(Reader,p->LastRead);
		for (No=1;No<MAXREADER;++No)
		{
			format_reader* r = p->Reader+No;
			if (r->Ratio > 0)
			{
				int Rate = GetRate(r,p->LastRead);
				if (Lowest > Rate)
				{
					Lowest = Rate;
					Reader = r;
				}
			}
		}

		p->LastRead = Reader;
	}

	Buffer = Reader->InputBuffer;
	if (!Buffer)
	{
		// allocate input buffer
		if (p->BufferUsed < BufferMax) // check again (BufferUsed is locked)
		{
			Buffer = Format_BufferAlloc(p,0);
			if (p->BufferUsed >= BufferMax)
				Result = ERR_BUFFER_FULL;
		}

		if (!Buffer)
		{
			*BufferUsed = p->BufferUsed;
			LockLeave(p->BufferLock);
			LockLeave(p->InputLock);
			return ERR_BUFFER_FULL;
		}

#ifndef NDEBUG
		Buffer->FilePos = Reader->Input->Seek(Reader->Input,0,SEEK_CUR);
#endif
	}

	*BufferUsed = p->BufferUsed;
	LockLeave(p->BufferLock);

	Left = BLOCKSIZE - Buffer->Length;
	if (Left > p->ReadSize)
		Left = p->ReadSize;

	if (Reader->Input->DataAvailable)
	{
		int Available = Reader->Input->DataAvailable(Reader->Input);
		if (Available>=0)
		{
			if (!Available)
			{
				if (!Reader->InputBuffer)
					Format_BufferRelease(Reader->Format,Buffer);
				LockLeave(p->InputLock);
				return ERR_NEED_MORE_DATA;
			}

			if (Left > Available)
				Left = Available;
		}
	}

	Length = Reader->Input->ReadBlock(Reader->Input,&Buffer->Block,Buffer->Length,Left);

	if (Length < 0)
	{
		// failed
		if (!Reader->InputBuffer)
			Format_BufferRelease(Reader->Format,Buffer);
		Result = ERR_DEVICE_ERROR;
	}
	else
	{
		Buffer->Length += Length;
		Reader->NoMoreInput = Length == 0;

		if (!Length)
		{
			// only return end of file if all readers are at the end
			Result = ERR_END_OF_FILE;
			for (No=0;No<MAXREADER;++No)
			{
				if (!p->Reader[No].Input)
					break;
				if (!p->Reader[No].NoMoreInput)
				{
					Result = ERR_NONE;
					break;
				}
			}
		}

		if (!Length && !Reader->InputBuffer)
			Format_BufferRelease(Reader->Format,Buffer);
		else
		{
			if (!Length || Buffer->Length == BLOCKSIZE || p->ProcessMinBuffer==0)
			{
				LockEnter(p->BufferLock);
				Format_BufferInsert(Reader,Buffer);
				LockLeave(p->BufferLock);
				Buffer = NULL;
			}
			Reader->InputBuffer = Buffer;
		}
	}

	LockLeave(p->InputLock);
	return Result;
}

static NOINLINE void UpdateTotalCount(format_base* p)
{
	p->TotalCount = p->StreamCount;
	if (p->SubTitle)
		p->Streams[p->TotalCount++] = p->SubTitle;
	if (p->CoverArt)
		p->Streams[p->TotalCount++] = p->CoverArt;
}

static NOINLINE void ReadCoverArt(format_base* p,stream* Input,filepos_t Pos,int Len,const tchar_t* ContentType,const tchar_t* URL)
{
	uint8_t Probe[512];
	int ProbeLen;
	
	Input->Seek(Input,Pos,SEEK_SET);
	ProbeLen = Input->Read(Input,Probe,min(Len,sizeof(Probe)));
	if (ProbeLen>0)
	{
		int *i;
		array List;
		NodeEnumClassEx(&List,RAWIMAGE_CLASS,ContentType,URL,Probe,ProbeLen);
		for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
		{
			const tchar_t* Format = LangStr(*i,RAWIMAGE_FORMAT);
			if (tcsnicmp(Format,T("vcodec/"),7)==0)
			{
				format_stream* s = (format_stream*) malloc(sizeof(format_stream));
				if (!s)
					return;

				memset(s,0,sizeof(format_stream));
				s->Format.Type = PACKET_VIDEO;
				s->Format.Format.Video.Pixel.Flags = PF_FOURCC|PF_NOPREROTATE;
				s->Format.Format.Video.Pixel.FourCC = StringToFourCC(Format+7,1);

				if (Format_AllocBufferBlock(p,s,Len))
				{
					WriteBlock(&s->BufferBlock,0,Probe,ProbeLen);
					s->Packet.Length = ProbeLen;

					if (Len>ProbeLen)
						s->Packet.Length += Input->ReadBlock(Input,&s->BufferBlock,ProbeLen,Len-ProbeLen);
				}
				
				if (s->Packet.Length>0)
				{
					s->Packet.Data[0] = s->BufferBlock.Ptr;
					s->Packet.Key = 1;
					s->Packet.RefTime = 0;

					p->CoverArt = s;
					UpdateTotalCount(p);
					Format_PrepairStream(p,s);
				}
				else
					Format_FreeStream(p,s);
				break;
			}
		}
		ArrayClear(&List);
	}
}

static NOINLINE bool_t GetNonStreamingURL(format_base* p,tchar_t* URL,int URLLen,bool_t Local)
{
	bool_t HasHost;
	int Length;
	stream* Input = p->Reader->Input;

	if (!Input)
		return 0;

	LockEnter(p->InputLock);
	if (Input->Get(Input,STREAM_URL,URL,URLLen*sizeof(tchar_t)) != ERR_NONE)
		URL[0] = 0;
	if (Input->Get(Input,STREAM_LENGTH,&Length,sizeof(Length)) != ERR_NONE)
		Length = -1;
	LockLeave(p->InputLock);

	if (!URL[0])
		return 0; //no URL?

	GetMime(URL,NULL,0,&HasHost);
	if (HasHost && (Local || Length<0))
		return 0; //non local or streaming
	return 1;
}

static NOINLINE bool_t OpenCoverAtr(format_base* p, const tchar_t* Base, const tchar_t* Name)
{
	tchar_t Path[MAXPATH];
	int Length;
	stream* Input;

	AbsPath(Path,TSIZEOF(Path),Name,Base);
	if (FileExits(Path) && (Input = GetStream(Path,1))!=NULL)
	{
		//todo: cache global cover art...

		if (Input->Set(Input,STREAM_URL,Path,sizeof(tchar_t)*(tcslen(Path)+1)) == ERR_NONE &&
			Input->Get(Input,STREAM_LENGTH,&Length,sizeof(Length)) == ERR_NONE)
			ReadCoverArt(p,Input,0,Length,NULL,Path);

		Input->Set(Input,STREAM_URL,NULL,0);
		NodeDelete((node*)Input);
	}
	return p->CoverArt != NULL;
}

static NOINLINE void FindCoverArt(format_base* p)
{
	tchar_t Value[MAXPATH];

	if (p->Comment.Node && NodeIsClass(p->Comment.Node->Class,PLAYER_ID) &&
		((player*)p->Comment.Node)->CommentByName(p->Comment.Node,-1,PlayerComment(COMMENT_COVER),Value,TSIZEOF(Value)))
	{
		tchar_t* SPos = tcschr(Value,':');
		tchar_t* SLen = SPos?tcschr(SPos+1,':'):NULL;
		tchar_t* ContentType = SLen?tcschr(SLen+1,':'):NULL;
		int Pos = SPos ? StringToInt(SPos+1,0):0;
		int Len = SLen ? StringToInt(SLen+1,0):0;

		if (ContentType && Len>0)
		{
			stream* Input = p->Reader->Input;
			filepos_t SavePos;
			LockEnter(p->InputLock);

			SavePos = Input->Seek(Input,0,SEEK_CUR);
			if (SavePos >= 0)
			{
				if (*(++ContentType)==0)
					ContentType = NULL;

				ReadCoverArt(p,Input,Pos,Len,ContentType,NULL);
				Input->Seek(Input,SavePos,SEEK_SET);
			}
			LockLeave(p->InputLock);
		}
	}
	else if (NodeIsClass(p->Format.Class,RAWAUDIO_CLASS) && GetNonStreamingURL(p,Value,TSIZEOF(Value),1))
	{
		tchar_t Base[MAXPATH];
		SplitURL(Value,Base,TSIZEOF(Base),Base,TSIZEOF(Base),NULL,0,NULL,0);

		if (!OpenCoverAtr(p,Base,T("cover.jpg")) &&
			!OpenCoverAtr(p,Base,T("folder.jpg")) &&
			!OpenCoverAtr(p,Base,T("front.jpg")) &&
			!OpenCoverAtr(p,Base,T("coverart.jpg")));
	}
}

static void FindSubtitles(format_base* p)
{
/*
//!SUBTITLE
	tchar_t* SubExt[] = { T("sub"),T("str"),T("smi"), NULL };
	tchar_t** s;

	tchar_t URL[MAXPATH];
	if (!GetNonStreamingURL(p,URL,TSIZEOF(URL),0))
		return;

	for (s=SubExt;*s && !p->SubTitle;++s)
		if (SetFileExt(URL,TSIZEOF(URL),*s))
		{
			stream* Input = GetStream(URL,1);
			if (Input)
			{
				bool_t Silent = 1;
				Input->Set(Input,STREAM_SILENT,&Silent,sizeof(Silent));

				if (Input->Set(Input,STREAM_URL,URL,sizeof(tchar_t)*(tcslen(URL)+1)) == ERR_NONE)
				{
					p->SubTitle = Format_LoadSubTitle(p,Input);
					if (p->SubTitle)
					{
						UpdateTotalCount(p);
						Format_PrepairStream(p,(format_stream*)p->SubTitle);
					}
				}
				Input->Delete(Input);
			}
		}
*/
}

void Format_PrepairStream(format_base* p,format_stream* Stream)
{
	switch (Stream->Format.Type)
	{
	case PACKET_VIDEO: Stream->PacketBurst = VIDEO_BURST; break;
	case PACKET_AUDIO: Stream->PacketBurst = AUDIO_BURST; break;
	case PACKET_SUBTITLE: Stream->PacketBurst = 1; break;
	}

	if (p->ProcessMinBuffer)
		p->ProcessMinBuffer = IsVideo(p) ? PROCESSMINVIDEO:PROCESSMINAUDIO;

	if (Stream->Format.Type == PACKET_AUDIO && Stream->Format.Format.Audio.Format == AUDIOFMT_PCM)
	{
		// there could be alignement problems (with memory and with audio.BlockSize)
		Stream->Fragmented = 0;
		Stream->ForceMerge = 1;
	}

	if (p->UpdateStreams.Func)
		p->UpdateStreams.Func(p->UpdateStreams.This,0,0);

	if (Stream->Format.Type == PACKET_VIDEO && p->NeedSubtitles)
	{
		p->NeedSubtitles = 0;
		FindSubtitles(p);
	}
}

static NOINLINE void DefaultPalette(packetformat* p)
{
	if (p->Format.Video.Pixel.BitCount >= 1 &&
		p->Format.Video.Pixel.BitCount <= 8 &&
		p->ExtraLength >= (4 << p->Format.Video.Pixel.BitCount))
		p->Format.Video.Pixel.Palette = p->Extra;
}

int Format_BitmapInfoMem(format_stream* s,void* Data,int Length)
{
	if (Length>=4)
	{
		char* p = (char*)Data;
		int Size = INT32LE(*(int32_t*)p);
		if (Length >= Size)
		{
			PacketFormatClear(&s->Format);
			s->Format.Type = PACKET_VIDEO;
			s->Format.Format.Video.Width = INT32LE(*(int32_t*)(p+4));
			s->Format.Format.Video.Height = INT32LE(*(int32_t*)(p+8));
			s->Format.Format.Video.Pixel.BitCount = INT16LE(*(int16_t*)(p+14));
			s->Format.Format.Video.Pixel.FourCC = INT32LE(*(int32_t*)(p+16));
			s->Format.Format.Video.Aspect = ASPECT_ONE; //todo

			if (PacketFormatExtra(&s->Format,Size-40))
			{
				memcpy(s->Format.Extra,p+40,s->Format.ExtraLength);
				DefaultPalette(&s->Format);
			}

			PacketFormatDefault(&s->Format);
		}
		else
			Size = 4;
		Length -= Size;
	}
	return Length;
}

void Format_BitmapInfo(format_reader* p,format_stream* s,int Length)
{
	if (Length>=4)
	{
		int Size = p->ReadLE32(p); //size
		if (Length >= Size)
		{
			PacketFormatClear(&s->Format);
			s->Format.Type = PACKET_VIDEO;
			s->Format.Format.Video.Width = p->ReadLE32(p); 
			s->Format.Format.Video.Height = p->ReadLE32(p); 
			p->ReadLE16(p); //planes
			s->Format.Format.Video.Pixel.BitCount = p->ReadLE16(p);
			s->Format.Format.Video.Pixel.FourCC = p->ReadLE32(p);
			s->Format.Format.Video.Aspect = ASPECT_ONE; //todo
			p->Skip(p,20); //SizeImage,XPelsPerMeter,YPelsPerMeter,ClrUsed,ClrImportant

			if (PacketFormatExtra(&s->Format,Size-40))
			{
				p->Read(p,s->Format.Extra,s->Format.ExtraLength);
				DefaultPalette(&s->Format);
			}

			PacketFormatDefault(&s->Format);
		}
		else
			Size = 4;

		Length -= Size;
	}
	p->Skip(p,Length);
}

int Format_WaveFormatMem(format_stream* s,void* Data,int Length)
{
	if (Length >= 16)
	{
		char* p = (char*)Data;

		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = INT16LE(*(int16_t*)(p+0));
		s->Format.Format.Audio.Channels =INT16LE(*(int16_t*)(p+2));
		s->Format.Format.Audio.SampleRate = INT32LE(*(int32_t*)(p+4));
		s->Format.ByteRate = INT32LE(*(int32_t*)(p+8));
		s->Format.Format.Audio.BlockAlign = INT16LE(*(int16_t*)(p+12));
		s->Format.Format.Audio.Bits = INT16LE(*(int16_t*)(p+14));
		Length -= 16;

		if (Length >= 2)
		{
			int Extra = INT16LE(*(int16_t*)(p+16));
			Length -= 2;
			if (Extra > 0 && Length >= Extra)
			{
				if (PacketFormatExtra(&s->Format,Extra))
					memcpy(s->Format.Extra,p+18,s->Format.ExtraLength);
				Length -= Extra;
			}
		}

		PacketFormatDefault(&s->Format);
	}
	return Length;
}

void Format_WaveFormat(format_reader* p,format_stream* s,int Length)
{
	if (Length >= 16)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = p->ReadLE16(p);
		s->Format.Format.Audio.Channels = p->ReadLE16(p);
		s->Format.Format.Audio.SampleRate = p->ReadLE32(p);
		s->Format.ByteRate = p->ReadLE32(p);
		s->Format.Format.Audio.BlockAlign = p->ReadLE16(p);
		s->Format.Format.Audio.Bits = p->ReadLE16(p);
		Length -= 16;

		if (Length >= 2)
		{
			int Extra = p->ReadLE16(p);
			Length -= 2;
			if (Extra > 0 && Length >= Extra)
			{
				if (PacketFormatExtra(&s->Format,Extra))
					p->Read(p,s->Format.Extra,s->Format.ExtraLength);
				Length -= Extra;
			}
		}

		PacketFormatDefault(&s->Format);
	}
	p->Skip(p,Length);
}

void Format_RemoveStream(format_base* p)
{
	if (p->StreamCount)
	{
		format_stream* Stream = p->Streams[--p->StreamCount];
		UpdateTotalCount(p);
		if (p->FreeStream)
			p->FreeStream(p,Stream);
		Format_FreeStream(p,Stream);
	}
}

format_stream* Format_AddStream(format_base* p, int Length)
{
	format_stream* s;

	if (p->StreamCount >= MAXSTREAM-1-1) //subtitle,coverart
		return NULL;

	s = (format_stream*) malloc(Length);
	if (!s)
		return NULL;

	memset(s,0,Length);
	s->Reader = p->Reader; // default
	s->No = p->StreamCount;
	s->LastTime = TIME_SYNC;
	p->Streams[p->StreamCount++] = s;
	UpdateTotalCount(p);
	return s;
}

static int Create(format_base* p)
{
	int No;
	format_reader* Reader = p->Reader;
	for (No=0;No<MAXREADER;++No,++Reader)
	{
		Reader->Format = p;
		Reader->Seek = Reader_Seek;
		Reader->Eof = Reader_Eof;
		Reader->Skip = Reader_Skip;
		Reader->Read = Reader_Read;
		Reader->Read8 = Reader_Read8;
		Reader->ReadLE16 = Reader_ReadLE16;
		Reader->ReadBE16 = Reader_ReadBE16;
		Reader->ReadLE32 = Reader_ReadLE32;
		Reader->ReadBE32 = Reader_ReadBE32;
		Reader->ReadLE64 = Reader_ReadLE64;
		Reader->ReadBE64 = Reader_ReadBE64;
		Reader->ReadAsRef = Reader_ReadAsRef;
		Reader->GetReadBuffer = Reader_GetReadBuffer;
	}

	p->Format.Enum = (nodeenum)FormatBaseEnum;
	p->Format.Get = (nodeget)FormatBaseGet;
	p->Format.Set = (nodeset)FormatBaseSet;

	p->Format.Sync = (fmtsync)Format_Sync;
	p->Format.Read = (fmtread)Format_ReadInput;
	p->Format.Process = (fmtprocess)Format_Process;
	p->FillQueue = (fmtfill)Format_FillQueue;
	p->Process = (fmtstreamprocess)Format_ProcessStream;
	p->Sended = (fmtstream)Format_Sended;
	p->FileAlign = 1;
	p->Timing = 1;
	p->MinHeaderLoad = BLOCKSIZE;

#ifdef TARGET_PALMOS
	p->UseBufferBlock = Context()->LowMemory;
#endif

	return ERR_NONE;
}

static const nodedef FormatBase =
{
	sizeof(format_base)|CF_ABSTRACT,
	FORMATBASE_CLASS,
	FORMAT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void FormatBase_Init()
{
	NodeRegisterClass(&FormatBase);
}

void FormatBase_Done()
{
	NodeUnRegisterClass(FORMATBASE_CLASS);
}
