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
 * $Id: avi.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "avi.h"

#define INDEXSTEP	512

#define AVI_INDEX_OF_INDEXES	0x00
#define AVI_INDEX_OF_CHUNKS		0x01
#define AVI_INDEX_KEY			0x80000000

typedef struct aviindex
{
	int32_t Id;
	int8_t Flags[4];
	int32_t Pos;
	int32_t Length;
} aviindex;

typedef struct avisuperindex
{
	int Offset;
	int Size;
	int StartTime;
	int EndTime;
	int LastData;

} avisuperindex;

typedef struct avistdindex
{
	uint32_t FourCC;
	int Size;
	int16_t LongsPerEntry;
	int8_t IndexSubType;
	int8_t IndexType;
	int Count;
	int ChunkId;
	int OffsetLow;
	int OffsetHigh;
	int Reserved;

} avistdindex;

typedef struct avistream
{
	format_stream Stream;
	
	int* Index;			// mediatime
	bool_t ByteRate;
	int RefStart;
	int MediaTime;
	int NewMediaTime;	// after seek (if seek successed)
	fraction64 MediaRate; // byterate or packetrate
	int SampleSize;
	int MaxPacket;
	int Length;
	int SuperIndexCount;
	avisuperindex* SuperIndex; //opendml index
	int PrevKey;
	int PrevMediaTime;
	int PrevKeyBuffer;

	filepos_t OffsetMin;
	filepos_t OffsetMax;
	int* ZeroChunkPos;
	array ZeroChunk;
	int LastPos;

} avistream;

typedef struct avi
{
	format_base Format;

	int FramePeriod;
	int ByteRate;
	bool_t ErrorIndex;
	bool_t UnLimited;
	bool_t CheckForZero;
	bool_t FoundStdIndex;

	int DataPos;
	int DataEnd;

	// stream header and format loading
	int HeadType;
	avistream* LastStream;

	int VideoHandler;
	int VideoFrames;
	fraction VideoFPS;
	int VideoStart;

	fraction AudioRate;
	int AudioLength;
	int AudioStart;
	int AudioSampleSize;
	
	int IndexAdjustPos;
	int IndexCount;
	int IndexPos;
	int IndexReaded;
	int IndexBufferNo;
	block IndexBuffer;

	char NameChar[256];

	int FoundTotal;
	int FoundAtEnd[8];
	int FoundLastPos[8];

} avi;

#define ALIGN2(x) (((x)+1)&~1)
#define ISDIGIT(x) ((x)>='0' && (x)<='9')
#define STREAMNO(x) ((((x) & 255)-'0')*10 + ((((x) >> 8) & 255)-'0'))
#define ISDATA(x) (ISDIGIT((x) & 255) && ISDIGIT(((x) >> 8) & 255))
#define ISIDX(x) (ISDIGIT(((x) >> 16) & 255) && ISDIGIT(((x) >> 24) & 255) && ((x) & 255)=='i' && (((x) >> 8) & 255)=='x')
#define IDXNO(x) (((((x) >> 16) & 255)-'0')*10 + ((((x) >> 24) & 255)-'0'))

static void Done(avi* p)
{
	FreeBlock(&p->IndexBuffer);
}

#define CHECKINDEX	16384

static NOINLINE int Load32LE(const uint8_t* i) { return LOAD32LE(i); }

static void PreCheckIndexODML(avi* p,avistream* Stream)
{
	int k;
	array Array = {NULL};
	avistdindex StdIndex;
	stream* Input = p->Format.Reader->Input;
	filepos_t Save = Input->Seek(Input,0,SEEK_CUR);
	if (Save>=0 && Input->Seek(Input,Stream->SuperIndex[0].Offset,SEEK_SET)>=0)
	{
		if (Input->Read(Input,&StdIndex,sizeof(StdIndex)) == sizeof(StdIndex) &&
			StdIndex.IndexType == AVI_INDEX_OF_CHUNKS &&
			StdIndex.LongsPerEntry == 2)
		{
			if (StdIndex.Count > 1024)
				StdIndex.Count = 1024;

			if (ArrayAppend(&Array,NULL,8*StdIndex.Count,1024))
			{
				if (Input->Read(Input,ARRAYBEGIN(Array,int32_t),8*StdIndex.Count) == 8*StdIndex.Count)
				{
					for (k=0;k<StdIndex.Count;++k)
						if ((ARRAYBEGIN(Array,int32_t)[k*2+1] & ~AVI_INDEX_KEY)==0)
						{
							p->CheckForZero = 1;
							break;
						}
				}
				ArrayClear(&Array);
			}
		}
		Input->Seek(Input,Save,SEEK_SET);
	}
}

static void PreCheckIndex(avi* p)
{
	block Buffer;
	if (AllocBlock(CHECKINDEX,&Buffer,0,HEAP_ANY))
	{
		format_reader* Reader = p->Format.Reader;
		filepos_t Save = Reader->Input->Seek(Reader->Input,0,SEEK_CUR);
		if (Save>=0 && Reader->Input->Seek(Reader->Input,-CHECKINDEX,SEEK_END)>=0)
		{
			if (Reader->Input->ReadBlock(Reader->Input,&Buffer,0,CHECKINDEX)==CHECKINDEX)
			{
				int Type = 0;
				int LastNo;
				int No = 24;
				const uint8_t* i = Buffer.Ptr+CHECKINDEX-12;

				while (--i>=Buffer.Ptr+16)
				{
					if (p->NameChar[*i])
					{
						Type |= *i << No;
						No -= 8;
						if (No<0)
						{
							if (Type == FOURCCLE('J','U','N','K') && ISDATA(Load32LE(i-16)))
							{
								i -= 16;
								p->FoundStdIndex = 1;
								break;
							}
							if (ISDATA(Type) && (Load32LE(i+4) & ~0x10)==0)
							{
								p->FoundStdIndex = 1;
								break;
							}
							Type <<= 8;
							No += 8;
						}
					}
					else
					{
						Type = 0;
						No = 24;
					}
				}

				LastNo = -1;
				for (;i>=Buffer.Ptr+16;i-=16)
				{
					if (Load32LE(i-8) == FOURCCLE('i','d','x','1'))
						break;

					Type = Load32LE(i);
					if (ISDATA(Type))
					{
						int Pos = Load32LE(i+8);
						int No = STREAMNO(Type);
						if (No >= 0 && No < 8)
						{
							if (No != LastNo)
							{
								if (LastNo>=0)
									++p->FoundAtEnd[LastNo];
								LastNo = No;
							}

							if (p->FoundLastPos[No] == Pos)
								p->CheckForZero = 1;

							p->FoundLastPos[No] = Pos;
							++p->FoundTotal;
						}

						if (Load32LE(i+12)==0 && Pos==Load32LE(i-16+8))
						{
							p->CheckForZero = 1;
							break;
						}
					}
				}

				if (LastNo>=0)
					++p->FoundAtEnd[LastNo];
			}
			Reader->Input->Seek(Reader->Input,Save,SEEK_SET);
		}

		FreeBlock(&Buffer);
	}
}

static int Init(avi* p)
{
	int DataLength;
	int Type;
	format_reader* Reader = p->Format.Reader;

	if (!AllocBlock(sizeof(aviindex)*INDEXSTEP,&p->IndexBuffer,0,HEAP_ANY))
		return ERR_OUT_OF_MEMORY;

	memset(p->FoundAtEnd,0,sizeof(p->FoundAtEnd));
	memset(p->FoundLastPos,0,sizeof(p->FoundLastPos));
	p->LastStream = NULL;
	p->FoundTotal = 0;
	p->FoundStdIndex = 0;
	p->CheckForZero = 0;
	p->UnLimited = 1;
	p->IndexAdjustPos = 4;
	p->IndexPos = -1;
	p->IndexCount = 0;
	p->IndexReaded = 0;
	p->IndexBufferNo = -1;
	p->DataPos = -1;
	p->DataEnd = -1;
	p->ErrorIndex = 0;
	p->AudioLength = 0;

	memset(p->NameChar,0,sizeof(p->NameChar));
	memset(p->NameChar+'a',1,'z'-'a');
	memset(p->NameChar+'A',1,'Z'-'A');
	memset(p->NameChar+'0',1,'9'-'0');
	p->NameChar['_']=1;
	p->NameChar[' ']=1;

	Type = Reader->ReadLE32(Reader);
	DataLength = Reader->ReadLE32(Reader);

	if (Type != FOURCCLE('R','I','F','F'))
		return ERR_INVALID_DATA;

	if (DataLength > 256 && (p->Format.FileSize < 0 || p->Format.FileSize > DataLength + 8))
		p->Format.FileSize = DataLength + 8;

	Reader->ReadLE32(Reader); // 'AVI ' or 'AVIX'

	PreCheckIndex(p);
	return ERR_NONE;
}

static INLINE int ChunkTime(avistream* Stream, int DataLength)
{
	if (Stream->ByteRate)
		return DataLength; //byterate

	// packetrate
	if (DataLength > Stream->MaxPacket) 
		return (DataLength + Stream->MaxPacket - 1) / Stream->MaxPacket;
	return 1;
}

static void UpdateAviFrameRate(avi* p)
{
	if (QueryAdvanced(ADVANCED_AVIFRAMERATE) && p->AudioLength && p->VideoFrames)
	{
		int i;
		for (i=0;i<p->Format.StreamCount;++i)
		{
			avistream* Stream = (avistream*) p->Format.Streams[i];
			if (Stream->Stream.Format.Type == PACKET_VIDEO && Stream->Length)
			{
				Stream->MediaRate.Num = Scale64(p->AudioLength,(int64_t)p->AudioRate.Den*TICKSPERSEC,p->AudioRate.Num);
				Stream->MediaRate.Den = p->VideoFrames;
			}
		}
	}
}

static bool_t SubtitleTiming(format_packet* Packet,format_reader* Reader,int Length)
{
	int Hour0,Hour1,Min0,Min1,Sec0,Sec1,MSec0,MSec1;
	int i;
	tchar_t s[64];
	for (i=0;i<63 && Length>0;++i)
	{
		int ch = Reader->Read8(Reader);
		--Length;
		if (ch<0 || ch==']') break;
		s[i] = (tchar_t)ch;
	}
	s[i] = 0;

	if (stscanf(s,T("[%d:%d:%d.%d-%d:%d:%d.%d"),&Hour0,&Min0,&Sec0,&MSec0,&Hour1,&Min1,&Sec1,&MSec1)!=8)
	{
		Reader->Skip(Reader,Length);
		return 0;
	}
	Packet->RefTime = Scale(((Hour0*60+Min0)*60+Sec0)*1000+MSec0,TICKSPERSEC,1000);
	Packet->EndRefTime = Scale(((Hour1*60+Min1)*60+Sec1)*1000+MSec1,TICKSPERSEC,1000);
	Packet->Data = Reader->ReadAsRef(Reader,Length);
	return 1;
}

static void SetMediaTime(avistream* p,int Time)
{
	p->MediaTime = Time;
	p->ZeroChunkPos = ARRAYBEGIN(p->ZeroChunk,int);
	if (p->ZeroChunkPos)
		while (*p->ZeroChunkPos < Time)
			++p->ZeroChunkPos;
}

static void ReadMeta(avi* p, format_reader* Reader, int* DataLength)
{
	pin* Comment = &p->LastStream->Stream.Comment;
	tchar_t Meta[512];
	char UTF8[512];
	int Len = *DataLength;
	if (Len>=512) Len=511;
	Len = Reader->Read(Reader,UTF8,Len);
	if (Len>0)
	{
		*DataLength -= Len;
		UTF8[Len] = 0;
		tcscpy_s(Meta,TSIZEOF(Meta),T("LANGUAGE="));
		UTF8ToTcs(Meta+tcslen(Meta),TSIZEOF(Meta)-tcslen(Meta),UTF8);
		if (Comment->Node)
			Comment->Node->Set(Comment->Node,Comment->No,Meta,sizeof(Meta));
	}
}

static void CheckIndex(avi* p, format_reader* Reader);

static int ReadPacket(avi* p, format_reader* Reader, format_packet* Packet)
{
	avistream* Stream;
	int No,Ch,StartPos;
	int SubType,Type,DataLength;
	tick_t Duration;
	int LongsPerEntry;
	int IndexType;
	int Count;
	int BaseOffset;

	// find a valid chunk header first

	//DEBUG_MSG1(DEBUG_FORMAT,T("AVI Packet %08x"),Reader->FilePos);

	StartPos = Reader->FilePos;

	if (!p->UnLimited && p->DataEnd >= 0 && Reader->FilePos >= p->DataEnd)
		return ERR_END_OF_FILE;

	do
	{
		bool_t PossibleAlign = Reader->FilePos & 1;
		Type = 0;
		No = 0;
		while (No<32)
		{
			Ch = Reader->Read8(Reader);

			if (Ch>=0 && p->NameChar[Ch])
			{
				Type |= Ch << No;
				No += 8;

				if (No==32 && PossibleAlign)
				{
					int Stream = STREAMNO(Type);
					if (!ISDATA(Type) || ISDIGIT((Type>>16) & 255) || ISDIGIT((Type>>24) & 255) || 
						Stream <0 || Stream>=p->Format.StreamCount)
					{
						No -= 8;
						Type >>= 8;
					}
				}
			}
			else
			{
				if (!PossibleAlign)
				{
					Reader->OutOfSync = 1;
					if ((Reader->FilePos-StartPos)>=16384)
						return ERR_DATA_NOT_FOUND; // this will force the process loop to stop, so input thread can be woken up
					if (Reader->Eof(Reader))
						return ERR_END_OF_FILE;
				}
				No = 0;
				Type = 0;
				PossibleAlign = 0;
			}
		}

		if (!p->UnLimited && p->DataEnd >= 0 && Reader->FilePos >= p->DataEnd)
			return ERR_END_OF_FILE;

		DataLength = Reader->ReadLE32(Reader);

		if (DataLength<0 || (Reader->FilePos+DataLength<0) || (p->DataPos>=0 && p->Format.FileSize>=0 && Reader->FilePos+DataLength>p->Format.FileSize))
		{
			DataLength = -1;
			Reader->OutOfSync = 1;
			continue;
		}

		if (ISDATA(Type))
		{
			No = STREAMNO(Type);
			if (No >= 0 && No < p->Format.StreamCount)
			{
				//DEBUG_MSG3(-1,"AVI Packet stream:%d pos:%08x length:%d",No,Reader->FilePos,DataLength);

				Stream = (avistream*) p->Format.Streams[No];
				if (Stream->Stream.Reader != Reader)
					break;

				if (Reader->OutOfSync)
				{
					if (p->Format.ProcessTime >= 0)
					{
						int i;
						tick_t RefTime;
	
						// this won't solve the out of sync problem
						// but at least the streams won't be dropped

						for (i=0;i<p->Format.StreamCount;++i)
						{
							avistream* s = (avistream*) p->Format.Streams[i];
							if (s->Stream.Reader == Reader)
							{
								RefTime = s->RefStart+Scale64(s->MediaTime,s->MediaRate.Num,s->MediaRate.Den);
								if (RefTime < p->Format.ProcessTime - TICKSPERSEC)
									SetMediaTime(s,Scale64(p->Format.ProcessTime-s->RefStart,s->MediaRate.Den,s->MediaRate.Num));
							}
						}
					}
					Reader->OutOfSync = 0;
				}

				Packet->Stream = &Stream->Stream;

				if (Stream->ZeroChunkPos)
					while (*Stream->ZeroChunkPos == Stream->MediaTime)
					{
						Stream->MediaTime += ChunkTime(Stream,0);
						++Stream->ZeroChunkPos;
					}

				if (Stream->Stream.Format.Type == PACKET_SUBTITLE)
				{
					if (!SubtitleTiming(Packet,Reader,DataLength))
						continue;
				}
				else
				{
					Packet->Data = Reader->ReadAsRef(Reader,DataLength);
					Packet->RefTime = Stream->RefStart+Scale64(Stream->MediaTime,Stream->MediaRate.Num,Stream->MediaRate.Den);
				}

				DEBUG_MSG5(DEBUG_FORMAT,T("AVI stream:%d time:%d mediatime:%d pos:%08x length:%08x"),No,Packet->RefTime,Stream->MediaTime,Reader->FilePos,DataLength);

				Stream->MediaTime += ChunkTime(Stream,DataLength);
				return ERR_NONE;
			}
		}

		if (ISIDX(Type))
		{
			No = IDXNO(Type);
			if (No >= 0 && No < p->Format.StreamCount)
				break; // opendml index
		}

		switch (Type)
		{
		case FOURCCLE('R','I','F','F'):
			Reader->ReadLE32(Reader); // 'AVI ' or 'AVIX'
			return ERR_NONE;

		case FOURCCLE('a','v','i','h'):
			if (p->Format.HeaderLoaded) break;
			p->FramePeriod = Reader->ReadLE32(Reader);
			p->ByteRate = Reader->ReadLE32(Reader);
			DataLength -= 8;
			break;

		case FOURCCLE('i','n','d','x'):
			if (p->Format.HeaderLoaded) break;
			// opendml index

			LongsPerEntry = Reader->ReadLE16(Reader);
			Reader->Read8(Reader); //subtype
			IndexType = Reader->Read8(Reader);
			Count = Reader->ReadLE32(Reader);
			Type = Reader->ReadLE32(Reader);
			BaseOffset = (int)Reader->ReadLE64(Reader);
			Reader->Skip(Reader,4);
			DataLength -= 6*4;

			if (ISDATA(Type))
			{
				No = STREAMNO(Type);
				if (No >= 0 && No < p->Format.StreamCount && Count)
				{
					avisuperindex* s;

					Stream = (avistream*) p->Format.Streams[No];

					if (IndexType == AVI_INDEX_OF_CHUNKS && LongsPerEntry==2)
					{
						p->UnLimited = 1;
						p->Format.FileSize = -1;

						// create fake index of indexes

						Stream->SuperIndexCount = 1;
						Stream->SuperIndex = s = (avisuperindex*) malloc(sizeof(avisuperindex));

						s->Offset = Reader->FilePos - 6*4 - 8;
						s->Size = DataLength + 6*4 + 8;
						s->StartTime = 0;
						s->EndTime = 0;
						s->LastData = 0;

						for (No=0;No<Count;++No)
						{
							int Data = BaseOffset + Reader->ReadLE32(Reader) - 8;
							int Size = Reader->ReadLE32(Reader) & ~AVI_INDEX_KEY;

							if (s->LastData < Data)
								s->LastData = Data;

							s->EndTime += ChunkTime(Stream,Size);
						}
					}
					if (IndexType == AVI_INDEX_OF_INDEXES && LongsPerEntry==4)
					{
						int Time;

						p->UnLimited = 1;
						p->Format.FileSize = -1;

						// load index of indexes

						Stream->SuperIndexCount = Count;
						Stream->SuperIndex = s = (avisuperindex*) malloc(sizeof(avisuperindex)*Count);

						Time = 0;
						for (No=0;No<Count;++No,++s)
						{
							s->Offset = (int)Reader->ReadLE64(Reader);
							s->Size = Reader->ReadLE32(Reader);
							s->StartTime = Time;
							Time += Reader->ReadLE32(Reader) * Stream->SampleSize;
  							s->EndTime = Time;
							s->LastData = -1;
							DataLength -= 16;
						}

						if (Count && !p->FoundStdIndex && !p->CheckForZero && Stream->Stream.Format.Type == PACKET_VIDEO)
						{
							LockEnter(p->Format.InputLock);
							PreCheckIndexODML(p,Stream);
							LockLeave(p->Format.InputLock);
						}
					}
				}
			}

			break;

		case FOURCCLE('s','t','r','h'):
			if (p->Format.HeaderLoaded) break;

			p->HeadType = Reader->ReadLE32(Reader);
			DataLength -= 4;

			if (p->HeadType == FOURCCLE('v','i','d','s'))
			{
				p->VideoHandler = Reader->ReadLE32(Reader);

				Reader->ReadLE32(Reader); // flags
				Reader->ReadLE16(Reader); // priority
				Reader->ReadLE16(Reader); // language
				Reader->ReadLE32(Reader); 

				p->VideoFPS.Den = Reader->ReadLE32(Reader); 
				p->VideoFPS.Num = Reader->ReadLE32(Reader); 

				if (!p->VideoFPS.Den || !p->VideoFPS.Num)
				{
					p->VideoFPS.Den = 1;
					p->VideoFPS.Num = 25;
				}
				if (p->VideoFPS.Den==1 && p->VideoFPS.Num >= 12000) //not important
					p->VideoFPS.Den = 1000;

				p->VideoStart = Reader->ReadLE32(Reader);
				p->VideoFrames = Reader->ReadLE32(Reader);

				DataLength -= 32;
			}
			else
			if (p->HeadType == FOURCCLE('a','u','d','s'))
			{
				Reader->ReadLE32(Reader); // format
				Reader->ReadLE32(Reader); // flags
				Reader->ReadLE16(Reader); // priority
				Reader->ReadLE16(Reader); // language
				Reader->ReadLE32(Reader); 
				p->AudioRate.Den = Reader->ReadLE32(Reader); 
				p->AudioRate.Num = Reader->ReadLE32(Reader); 
				p->AudioStart = Reader->ReadLE32(Reader);
				p->AudioLength = Reader->ReadLE32(Reader);

				Reader->ReadLE32(Reader); // initialframes
				Reader->ReadLE32(Reader); // suggested buffersize

				p->AudioSampleSize = Reader->ReadLE32(Reader);

				DataLength -= 44;
			}
			break;

		case FOURCCLE('s','t','r','n'):
			if (p->Format.HeaderLoaded) break;
			if (p->LastStream && DataLength)
				ReadMeta(p,Reader,&DataLength);
			break;

		case FOURCCLE('s','t','r','f'):
			if (p->Format.HeaderLoaded) break;
			if (p->HeadType == FOURCCLE('v','i','d','s'))
			{
				p->LastStream = Stream = (avistream*) Format_AddStream(&p->Format,sizeof(avistream));
				if (Stream)
				{
					Stream->Index = NULL;
					Stream->SuperIndex = NULL;
					Stream->ByteRate = 0;
					Stream->Stream.Fragmented = 0;
					Stream->SampleSize = 1;
					Stream->MaxPacket = MAX_INT;
					Format_BitmapInfo(Reader,&Stream->Stream,DataLength);

					if (Stream->Stream.Format.Format.Video.Pixel.FourCC == FOURCC('D','X','S','B'))
					{
						PacketFormatClear(&Stream->Stream.Format);
						Stream->Stream.Format.Type = PACKET_SUBTITLE;
						Stream->Stream.Format.Format.Subtitle.FourCC = FOURCC('D','X','S','B');
					}
					else
					{
						if (Stream->Stream.Format.Format.Video.Pixel.FourCC == 4)
							Stream->Stream.Format.Format.Video.Pixel.FourCC = UpperFourCC(p->VideoHandler);

						Stream->MediaRate.Den = p->VideoFPS.Num;
						Stream->MediaRate.Num = (int64_t)p->VideoFPS.Den * TICKSPERSEC;
						Stream->RefStart = Scale64(p->VideoStart,Stream->MediaRate.Num,Stream->MediaRate.Den);
						Stream->Length = p->VideoFrames;
						Stream->Stream.Format.PacketRate = p->VideoFPS;

						if (p->VideoFrames)
						{
							Duration = Scale64(p->VideoFrames,Stream->MediaRate.Num,Stream->MediaRate.Den);
							if (p->Format.Duration < Duration)
								p->Format.Duration = Duration;
						}

						UpdateAviFrameRate(p);
					}
					Format_PrepairStream(&p->Format,&Stream->Stream);
				}
				else
					Reader->Skip(Reader,DataLength);

				return ERR_NONE;
			}
			else
			if (p->HeadType == FOURCCLE('a','u','d','s'))
			{
				p->LastStream = Stream = (avistream*) Format_AddStream(&p->Format,sizeof(avistream));
				if (Stream)
				{
					Format_WaveFormat(Reader,&Stream->Stream,DataLength);

					// some encoders doesn't set AudioSampleSize, but still using CBR mode
					// detect it by insane packet rate
					if (!p->AudioSampleSize && p->AudioRate.Den*1000<=p->AudioRate.Num)
						p->AudioSampleSize = Stream->Stream.Format.Format.Audio.BlockAlign;

					Stream->Index = NULL;
					Stream->SuperIndex = NULL;
					Stream->ByteRate = p->AudioSampleSize != 0;
					Stream->Stream.Fragmented = Stream->ByteRate && p->AudioSampleSize <= 4;
					
					if (!Stream->ByteRate)
					{
						// VBR (packetrate)
						Stream->MediaRate.Den = p->AudioRate.Num;
						Stream->MediaRate.Num = (int64_t)p->AudioRate.Den * TICKSPERSEC;
						if (p->AudioRate.Num == Stream->Stream.Format.Format.Audio.SampleRate)
							Stream->MaxPacket = p->AudioRate.Den;
						else
							Stream->MaxPacket = MAX_INT;
						Stream->SampleSize = 1;
					}
					else
					{
						// CBR (byterate)
						
						// some digital cameras don't fill AudioSampleSize correctly only BlockAlign (with ADPCM)
						if (p->AudioSampleSize < Stream->Stream.Format.Format.Audio.BlockAlign)
							p->AudioSampleSize = Stream->Stream.Format.Format.Audio.BlockAlign;

						if (Stream->Stream.Format.Format.Audio.Format == 0x55 /*MP3*/ && 
							p->AudioRate.Num == 4977)
						{
							Stream->MediaRate.Den = (p->AudioRate.Num*16-7)*p->AudioSampleSize;
							Stream->MediaRate.Num = (int64_t)p->AudioRate.Den*TICKSPERSEC*16;
						}
						else
						{
							Stream->MediaRate.Den = p->AudioRate.Num*p->AudioSampleSize;
							Stream->MediaRate.Num = (int64_t)p->AudioRate.Den*TICKSPERSEC;
						}

						Stream->SampleSize = p->AudioSampleSize;
					}

					Stream->RefStart = Scale64(p->AudioStart,(int64_t)p->AudioRate.Den*TICKSPERSEC,p->AudioRate.Num);
					Stream->Length = p->AudioLength;

					if (p->AudioLength)
					{
						Duration = Scale64(p->AudioLength,(int64_t)p->AudioRate.Den*TICKSPERSEC,p->AudioRate.Num);
						if (Duration>0 && p->Format.Duration < Duration)
							p->Format.Duration = Duration;
					}

					UpdateAviFrameRate(p);
					Format_PrepairStream(&p->Format,&Stream->Stream);
				}
				else
					Reader->Skip(Reader,DataLength);
				return ERR_NONE;
			}
			else
				p->LastStream = NULL;
			break;

		case FOURCCLE('L','I','S','T'):
			SubType = Reader->ReadLE32(Reader);
			DataLength -= 4;

			switch (SubType)
			{
			case FOURCCLE('s','t','r','l'):
				return ERR_NONE;

			case FOURCCLE('h','d','r','l'):
				return ERR_NONE;

			case FOURCCLE('m','o','v','i'):
				if (p->Format.HeaderLoaded) return ERR_NONE;
				p->DataPos = Reader->FilePos;
				p->DataEnd = DataLength + Reader->FilePos;
				p->Format.HeaderLoaded = 1;

				if (p->FoundTotal >= 256 && p->Format.StreamCount>1)
				{
					for (No=0;No<p->Format.StreamCount && No<8;++No)
						if (p->FoundAtEnd[No] < 2)
							p->CheckForZero = 1; // possibly non interleaved streams
				}

				if (p->CheckForZero)
				{
					LockEnter(p->Format.InputLock);
					CheckIndex(p,Reader);
					LockLeave(p->Format.InputLock);
				}

				for (No=0;No<p->Format.StreamCount;++No)
					SetMediaTime((avistream*)p->Format.Streams[No],0);

				return ERR_NONE;

			case FOURCCLE('o','d','m','l'):
				break;

			default:
				return ERR_NONE;
			}
			break;

		case FOURCCLE('J','U','N','K'):
		case FOURCCLE('i','d','x','1'):
			break;

		default: 
			if (Reader->OutOfSync || DataLength > 256*1024) // unknown
				DataLength = -1;
		}
	}
	while (DataLength<0);

	Reader->Skip(Reader,DataLength);

	return ERR_NONE;
}

static int ErrorIndex(avi* p,stream* Input,filepos_t OldFilePos)
{
	if (OldFilePos>=0)
	{
		if (Input)
			Input->Seek(Input,OldFilePos,SEEK_SET);

		if (!p->ErrorIndex)
		{
			p->ErrorIndex = 1;
			ShowError(p->Format.Format.Class,AVI_ID,AVI_NO_INDEX);
		}
	}
	return ERR_NOT_SUPPORTED;
}

static INLINE const aviindex* IndexBuffer(avi* p,int n)
{
	return ((const aviindex*)p->IndexBuffer.Ptr)+n;
}

static int LoadIndexBuffer(avi* p,stream* Input,int No,filepos_t OldFilePos)
{
	if (p->IndexBufferNo != No)
	{
		int Bytes = (p->IndexCount - No*INDEXSTEP)*sizeof(aviindex);
		if (Bytes > INDEXSTEP*sizeof(aviindex))
			Bytes = INDEXSTEP*sizeof(aviindex);

		if (Input->Seek(Input,p->IndexPos+No*INDEXSTEP*sizeof(aviindex),SEEK_SET)<0)
			return ErrorIndex(p,Input,OldFilePos);

		if (Input->ReadBlock(Input,&p->IndexBuffer,0,Bytes) != Bytes)
			return ErrorIndex(p,Input,OldFilePos);

		p->IndexBufferNo = No;

		if (No == 0)
		{
			if (INT32LE(IndexBuffer(p,0)->Pos) == p->DataPos)
				p->IndexAdjustPos = p->DataPos;
			else
				p->IndexAdjustPos = 4;
		}
	}
	return ERR_NONE;
}

static int ReadIndex(stream* Input,avistream* Stream,avisuperindex* Index,int MediaTime,int DstPos,bool_t PrevKey)
{
	bool_t NeedKey = Stream->Stream.Format.Type == PACKET_VIDEO;
	avistdindex StdIndex;
	int No;
	int FoundPos = -1;
	int32_t* i; 
	int LookBack = 0;

	if (!NeedKey)
		PrevKey = 0;

	if (PrevKey)
		LookBack = 300;

	if (DstPos>=0 && Index->LastData>=0 && DstPos > Index->LastData)
		return -1;

	if (MediaTime>=0 && Index->EndTime + LookBack <= MediaTime)
		return -1;

	if (Input->Seek(Input,Index->Offset,SEEK_SET) < 0)
		return -2;

	if (Input->Read(Input,&StdIndex,sizeof(StdIndex)) != sizeof(StdIndex))
		return -1;

	if (StdIndex.IndexType != AVI_INDEX_OF_CHUNKS ||
		StdIndex.LongsPerEntry != 2)
		return -1;

	i = (int32_t*) malloc(8*StdIndex.Count);
	if (!i)
		return -1;

	if (Input->Read(Input,i,8*StdIndex.Count) != 8*StdIndex.Count)
	{
		free(i);
		return -1;
	}

	if (Index->LastData<0)
	{
		Index->LastData = 0;
		for (No=0;No<StdIndex.Count;++No)
		{
			int Data = StdIndex.OffsetLow + i[No*2] - 8;
			int Size = i[No*2+1] & ~AVI_INDEX_KEY;
			if (Size && Index->LastData < Data)
				Index->LastData = Data;
		}
	}
	
	Stream->NewMediaTime = Index->StartTime;

	for (No=0;No<StdIndex.Count;++No)
	{
		int Data = StdIndex.OffsetLow + i[No*2] - 8;
		int Size = i[No*2+1];

		if (MediaTime>=0)
		{
			if (Stream->NewMediaTime >= MediaTime && (!NeedKey || !(Size & AVI_INDEX_KEY))) 
			{
				if (PrevKey && Stream->NewMediaTime != MediaTime && Stream->PrevKey>=0)
				{
					Stream->NewMediaTime = Stream->PrevMediaTime;
					FoundPos = Stream->PrevKey; // use previous key
				}
				else
					FoundPos = Data; // found sample after mediatime
				break;
			}

			if (PrevKey && !(Size & AVI_INDEX_KEY))
			{
				Stream->PrevMediaTime = Stream->NewMediaTime;
				Stream->PrevKey = Data;
			}
		}

		if (DstPos>=0 && Data >= DstPos) 
		{
			FoundPos = Data; // found a sample after the destination position
			break; 
		}

		Stream->NewMediaTime = Stream->NewMediaTime + ChunkTime(Stream,Size & ~AVI_INDEX_KEY);
	}

	free(i);
	return FoundPos;
}

static int RestorePrevKey(avi* p,avistream* Stream,int No,int OldFilePos,int* Pos)
{
	int i,j;
	avistream* s;

	if (Stream->PrevKeyBuffer != No)
	{
		int Result = LoadIndexBuffer(p,Stream->Stream.Reader->Input,Stream->PrevKeyBuffer,OldFilePos);
		if (Result != ERR_NONE)
			return Result;

		No = Stream->PrevKeyBuffer;
	}

	// restart this block and recalc NewMediaTime
	for (j=0;j<p->Format.StreamCount;++j)
	{
		s = (avistream*) p->Format.Streams[j];
		s->NewMediaTime = s->Index[No];
	}

	for (i=0;i<Stream->PrevKey;++i)
	{
		j = STREAMNO(IndexBuffer(p,i)->Id);
		if (j>=0 && j<p->Format.StreamCount)
		{
			s = (avistream*) p->Format.Streams[j];
			s->NewMediaTime += ChunkTime(s,IndexBuffer(p,i)->Length);
		}
	}

	*Pos = INT32LE(IndexBuffer(p,i)->Pos) - p->IndexAdjustPos;
	return ERR_NONE;
}

static int FindIndexPos(avi* p,stream* Input)
{
	int OldFilePos = Input->Seek(Input,0,SEEK_CUR);
	int Pos = p->DataEnd;
	// try to find index block

	if (Input->Seek(Input,Pos,SEEK_SET) < 0)
		return ERR_NOT_SUPPORTED;

	for (;;)
	{
		int32_t Head[2];

		if (Pos & 1)
		{
			if (Input->Read(Input,Head,sizeof(Head)) != sizeof(Head))
				return ErrorIndex(p,Input,OldFilePos);

			Pos += sizeof(Head);

			if (Head[0] != FOURCC('i','d','x','1'))
			{
				memmove((char*)Head,(char*)Head+1,sizeof(Head)-1);
				if (Input->Read(Input,(char*)Head+sizeof(Head)-1,1) != 1)
					return ErrorIndex(p,Input,OldFilePos);
				++Pos;
			}
		}
		else
		{
			if (Input->Read(Input,Head,sizeof(Head)) != sizeof(Head))
				return ErrorIndex(p,Input,OldFilePos);
			Pos += sizeof(Head);
		}

		if (Head[0] == FOURCC('i','d','x','1'))
		{
			int i,No;

			p->IndexCount = Head[1] / sizeof(aviindex);

			i = ((p->IndexCount + INDEXSTEP - 1) / INDEXSTEP) + 1;

			for (No=0;No<p->Format.StreamCount;++No)
			{
				avistream* s = (avistream*) p->Format.Streams[No];
				s->Index = (int*) malloc(sizeof(int)*i);
				if (!s->Index)
					return ERR_OUT_OF_MEMORY;

				s->Index[0] = 0;
			}

			p->IndexPos = Pos;
			p->IndexReaded = 0;
			break;
		}

		if (Head[1]<0 || (Pos+Head[1]<0))
			return ErrorIndex(p,Input,OldFilePos);

		Pos += Head[1];

		if (Input->Seek(Input,Pos,SEEK_SET) != Pos)
			return ErrorIndex(p,Input,OldFilePos);
	}
	return ERR_NONE;
}

static NOINLINE int CheckIndexBuffer(avi* p,stream* Input,int No,filepos_t OldFilePos,int IndexCount)
{
	// load page and calc sum mediatime
	if (p->IndexReaded <= No)
	{
		int i,j;
		int Result = LoadIndexBuffer(p,Input,No,OldFilePos);
		if (Result != ERR_NONE)
			return Result;

		for (j=0;j<p->Format.StreamCount;++j)
		{
			avistream* s = (avistream*) p->Format.Streams[j];
			s->Index[No+1] = s->Index[No];
		}

		for (i=0;i<IndexCount;++i)
		{
			j = STREAMNO(IndexBuffer(p,i)->Id);
			if (j>=0 && j<p->Format.StreamCount)
			{
				avistream* s = (avistream*) p->Format.Streams[j];
				s->Index[No+1] += ChunkTime(s,INT32LE(IndexBuffer(p,i)->Length));
			}
		}

		p->IndexReaded = No+1;
	}
	return ERR_NONE;
}

static int Seek(avi* p, tick_t Time, filepos_t FilePos, bool_t PrevKey)
{
	int Result = ERR_NONE;
	int RNo;
	if (p->DataPos < 0)
	{
		// no avi header yet, only seek to the beginning supported

		if (Time>0 || FilePos>0)
			return ERR_NOT_SUPPORTED;

		p->Format.Reader[0].OutOfSync = 0;
		return Format_Seek(&p->Format,0,SEEK_SET);
	}

	for (RNo=0;RNo<MAXREADER;++RNo)
	{
		filepos_t Pos = -1;
		avistream* Stream;
		avistream* s;
		int i,j,No;
		format_reader* Reader = &p->Format.Reader[RNo];
		stream* Input = Reader->Input;
		filepos_t OldReaderPos = Reader->FilePos;
		filepos_t OldFilePos;

		if (!Input)
			break;

		OldFilePos = Input->Seek(Input,0,SEEK_CUR);
		
		if (Time <= 0 && FilePos <= 0)
		{
			// seek to the beginning (no index needed for that)

			Pos = Reader->OffsetMin<MAX_INT?Reader->OffsetMin:p->DataPos;

			for (j=0;j<p->Format.StreamCount;++j)
				if (p->Format.Streams[j]->Reader == Reader)
					((avistream*)p->Format.Streams[j])->NewMediaTime = 0;
		}
		else
		{
			Stream = (avistream*) Format_DefSyncStream(&p->Format,Reader);
			if (Stream)
			{
				bool_t NeedKey = Stream->Stream.Format.Type == PACKET_VIDEO;
				int MediaTime = Scale64(Time,Stream->MediaRate.Den,Stream->MediaRate.Num);

				Stream->PrevKey = -1;

				if (PrevKey)
					++MediaTime; // seek to same position even if there is rounding error

				if (Stream->SuperIndex)
				{
					// using OpenDML index
					if (MediaTime >= Stream->SuperIndex[Stream->SuperIndexCount-1].EndTime)
						MediaTime = Stream->SuperIndex[Stream->SuperIndexCount-1].EndTime-1;

					// find position by MediaTime
					for (No=0;No<Stream->SuperIndexCount;++No)
					{
						Pos = ReadIndex(Input,Stream,Stream->SuperIndex+No,MediaTime,-1,PrevKey);
						if (Pos == -2)
							return ERR_NOT_SUPPORTED;

						if (Pos >= 0)
							break;
					}

					if (Pos<0)
					{
						if (PrevKey && Stream->PrevKey>=0)
						{
							Stream->NewMediaTime = Stream->PrevMediaTime;
							Pos = Stream->PrevKey;
						}
						else
						{
							Stream->NewMediaTime = Stream->SuperIndex[Stream->SuperIndexCount-1].EndTime;
							Pos = Stream->SuperIndex[Stream->SuperIndexCount-1].LastData;

							if (Pos<0)
								return ErrorIndex(p,Input,OldFilePos);
						}
					}

					// calculate NewMediaTime for all other streams
					for (j=0;j<p->Format.StreamCount;++j)
					{
						s = (avistream*) p->Format.Streams[j];
						if (s->Stream.Reader==Reader && s != Stream)
						{
							// find position by Pos
							for (No=0;No<s->SuperIndexCount;++No)
								if (ReadIndex(Input,s,s->SuperIndex+No,-1,Pos,0) >= 0)
									break;

							if (No==s->SuperIndexCount)
								s->NewMediaTime = s->SuperIndex[s->SuperIndexCount-1].EndTime;
						}
					}
				}
				else
				{
					int Next;
					int IndexCount,IndexLeft;
					tick_t LookBack = 0;

					// do we have the index block?
					if (p->IndexPos < 0)
					{
						Result = FindIndexPos(p,Input);
						if (Result!=ERR_NONE)
							return Result;
					}

					IndexLeft = p->IndexCount;

					if (!NeedKey)
						PrevKey = 0;

					if (PrevKey)
						LookBack = 300;

					// find the index page

					for (No=0;IndexLeft>0;++No,IndexLeft-=INDEXSTEP)
					{
						IndexCount = IndexLeft;
						if (IndexCount > INDEXSTEP)
							IndexCount = INDEXSTEP;

						Result = CheckIndexBuffer(p,Input,No,OldFilePos,IndexCount);
						if (Result != ERR_NONE)
							return Result;

						//DEBUG_MSG4(DEBUG_FORMAT,"AVI IndexBlock no:%d start:%d end:%d (target:%d)",No,Stream->Index[No],Stream->Index[No+1],MediaTime);

						if (Stream->Index[No]!=Stream->Index[No+1] &&
							(Stream->Index[No+1]+LookBack > MediaTime || IndexLeft<2*INDEXSTEP))
						{
							Result = LoadIndexBuffer(p,Input,No,OldFilePos);
							if (Result != ERR_NONE)
								return Result;

							for (j=0;j<p->Format.StreamCount;++j)
							{
								s = (avistream*) p->Format.Streams[j];
								if (s->Stream.Reader == Reader)
									s->NewMediaTime = s->Index[No];
							}

							// find sub page position

							for (i=0;i<IndexCount;++i)
							{
								j = STREAMNO(IndexBuffer(p,i)->Id);
								if (j>=0 && j<p->Format.StreamCount && p->Format.Streams[j]->Reader==Reader)
								{
									s = (avistream*) p->Format.Streams[j];
									Next = s->NewMediaTime + ChunkTime(s,IndexBuffer(p,i)->Length);

									//DEBUG_MSG3(DEBUG_FORMAT,"AVI Index stream:%d pos:%d flag:%d",j,INT32LE(IndexBuffer(p,i)->Pos),IndexBuffer(p,i)->Flags[0]);

									if (s==Stream)
									{
										Pos = INT32LE(IndexBuffer(p,i)->Pos) - p->IndexAdjustPos;

										if (s->NewMediaTime >= MediaTime)
										{
											// only accept keyframes with valid position
											if (Pos >= 0 && Pos < p->DataEnd - p->DataPos &&
												(!NeedKey || (IndexBuffer(p,i)->Flags[0] & 0x10)))
											{
												if (PrevKey && s->NewMediaTime != MediaTime && Stream->PrevKey>=0)
												{
													Result = RestorePrevKey(p,Stream,No,OldFilePos,&Pos);
													if (Result != ERR_NONE)
														return Result;
												}
												break;
											}
										}

										if (Pos >= 0 && Pos < p->DataEnd - p->DataPos &&
											(IndexBuffer(p,i)->Flags[0] & 0x10))
										{
											Stream->PrevKey = i;
											Stream->PrevKeyBuffer = No;
										}
									}

									s->NewMediaTime = Next;
								}
							}
								
							// found? 
							if (i<IndexCount)
								break;
						}
					}

					if (IndexLeft <= 0)
					{
						if (Stream->PrevKey>=0)
						{
							Result = RestorePrevKey(p,Stream,No,OldFilePos,&Pos);
							if (Result != ERR_NONE)
								return Result;
						}
						else
						{
							// go to end
							for (j=0;j<p->Format.StreamCount;++j)
							{
								s = (avistream*) p->Format.Streams[j];
								s->NewMediaTime = s->Index[p->IndexReaded];
							}

							Pos = p->DataEnd - p->DataPos;
						}
					}

					Pos += p->DataPos;
				}

				if (NeedKey && Stream->NewMediaTime >= 0) // use updated time with other Readers
					Time = Scale64(Stream->NewMediaTime,Stream->MediaRate.Num,Stream->MediaRate.Den);
			}
		}

		if (Pos>=0)
		{
			Reader->FilePos = -1; // force seek, because Input changed
			if (Reader->Seek(Reader,Pos,SEEK_SET) != ERR_NONE)
			{
				Reader->FilePos = OldReaderPos;
				// restore old input position
				Input->Seek(Input,OldFilePos,SEEK_SET);
				return ERR_NOT_SUPPORTED;
			}

			// set new positions
			for (j=0;j<p->Format.StreamCount;++j)
				if (p->Format.Streams[j]->Reader == Reader)
				{
					s = (avistream*) p->Format.Streams[j];
					SetMediaTime(s,s->NewMediaTime);
				}

			Reader->OutOfSync = 0;
		}
	}

	Format_AfterSeek(&p->Format);
	return ERR_NONE;
}

static NOINLINE void AddZeroChunk(avistream* Stream)
{
	ArrayAppend(&Stream->ZeroChunk,&Stream->NewMediaTime,sizeof(Stream->NewMediaTime),512);
}

static void CheckIndex(avi* p, format_reader* Reader)
{
	bool_t FlushBuffers = 0;
	avistream* Stream;
	int i,j,k;
	bool_t OpenDML = 0;
	array Array = {NULL};
	stream* Input = Reader->Input;
	filepos_t OldFilePos = Input->Seek(Input,0,SEEK_CUR);
	filepos_t OldReaderPos = Reader->FilePos;

	if (OldFilePos<0)
		return;

	for (i=0;i<p->Format.StreamCount;++i)
	{
		Stream = (avistream*)p->Format.Streams[i];
		Stream->NewMediaTime = 0;
		Stream->OffsetMin = MAX_INT;
		Stream->OffsetMax = 0;
		Stream->LastPos = 0;

		if (Stream->SuperIndex) // we need non video streams as well for offsetmin/max
		{
			OpenDML = 1;
		
			for (j=0;j<Stream->SuperIndexCount;++j)
			{
				avistdindex StdIndex;

				if (Input->Seek(Input,Stream->SuperIndex[j].Offset,SEEK_SET) < 0)
					break;

				if (Input->Read(Input,&StdIndex,sizeof(StdIndex)) != sizeof(StdIndex))
					break;

				if (StdIndex.IndexType != AVI_INDEX_OF_CHUNKS ||
					StdIndex.LongsPerEntry != 2)
					break;

				ArrayDrop(&Array);
				if (!ArrayAppend(&Array,NULL,8*StdIndex.Count,1024))
					break;

				if (Input->Read(Input,ARRAYBEGIN(Array,int32_t),8*StdIndex.Count) != 8*StdIndex.Count)
					break;

				for (k=0;k<StdIndex.Count;++k)
				{
					int Pos = StdIndex.OffsetLow + ARRAYBEGIN(Array,int32_t)[k*2] - 8;
					int Size = ARRAYBEGIN(Array,int32_t)[k*2+1] & ~AVI_INDEX_KEY;
					if (!Size)
						AddZeroChunk(Stream);
					else
					{
						if (Stream->OffsetMin>Pos)
							Stream->OffsetMin=Pos;
						if (Stream->OffsetMax<Pos)
							Stream->OffsetMax=Pos;
					}
					Stream->NewMediaTime = Stream->NewMediaTime + ChunkTime(Stream,Size);
				}
			}
		}
	}

	if (!OpenDML && (p->IndexPos>=0 || FindIndexPos(p,Input)==ERR_NONE))
	{
		int IndexLeft = p->IndexCount;
		int IndexCount;
		int LastPos = 0;

		for (i=0;IndexLeft>0;++i,IndexLeft-=INDEXSTEP)
		{
			IndexCount = IndexLeft;
			if (IndexCount > INDEXSTEP)
				IndexCount = INDEXSTEP;

			if (CheckIndexBuffer(p,Input,i,-1,IndexCount)==ERR_NONE && 
				LoadIndexBuffer(p,Input,i,-1)==ERR_NONE)
			{
				for (k=0;k<IndexCount;++k)
				{
					j = STREAMNO(IndexBuffer(p,k)->Id);
					if (j>=0 && j<p->Format.StreamCount)
					{
						int Pos = IndexBuffer(p,k)->Pos - p->IndexAdjustPos + p->DataPos;
						int Size = IndexBuffer(p,k)->Length;
						Stream = (avistream*) p->Format.Streams[j];
						if ((Size==0 && Pos==LastPos) || (Pos==Stream->LastPos))
							AddZeroChunk(Stream);
						if (Size)
						{
							if (Stream->OffsetMin>Pos)
								Stream->OffsetMin=Pos;
							if (Stream->OffsetMax<Pos)
								Stream->OffsetMax=Pos;
						}
						LastPos = Pos;
						Stream->LastPos=Pos;
						Stream->NewMediaTime = Stream->NewMediaTime + ChunkTime(Stream,Size);
					}
				}
			}
		}
	}

	for (i=0;i<p->Format.StreamCount;++i)
	{
		Stream = (avistream*)p->Format.Streams[i];
		if (!ARRAYEMPTY(Stream->ZeroChunk))
		{
			Stream->NewMediaTime = MAX_INT;
			AddZeroChunk(Stream);
		}
		if (Stream->OffsetMax)
		{
			format_reader* Reader = Format_FindReader(&p->Format,Stream->OffsetMin,Stream->OffsetMax);
			if (Reader)
				Stream->Stream.Reader = Reader;
		}
	}

	ArrayClear(&Array);

	for (i=0;i<MAXREADER;++i)
	{
		format_reader* v = &p->Format.Reader[i];
		if (v->Input && v->OffsetMax>0)
		{
			v->Seek(v,v->OffsetMin,SEEK_SET);
			FlushBuffers = 1;
			if (Reader == v)
				OldReaderPos = v->OffsetMin;
		}
	}

	if (!FlushBuffers)
		Input->Seek(Input,OldFilePos,SEEK_SET);
	else
	{
		// probably first reader already allocated all the buffers
		Reader->FilePos = -1;
		Reader->Seek(Reader,OldReaderPos,SEEK_SET);
	}
}

static int Set(avi* p,int No,const void* Data,int Size)
{
	if (No == NODE_SETTINGSCHANGED)
		UpdateAviFrameRate(p);

	return FormatBaseSet(&p->Format,No,Data,Size);
}

static void FreeStream(avi* p, avistream* Stream)
{
	free(Stream->Index);
	free(Stream->SuperIndex);
	ArrayClear(&Stream->ZeroChunk);
}

static int Create(avi* p)
{
	p->Format.Format.Set = (nodeset)Set;
	p->Format.Init = (fmtfunc)Init;
	p->Format.Done = (fmtvoid)Done;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.FreeStream = (fmtstream)FreeStream;
	p->Format.MinHeaderLoad = MAX_INT; //manualy setting HeaderLoaded
	return ERR_NONE;
}

static const nodedef AVI = 
{
	sizeof(avi),
	AVI_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void AVI_Init()
{
	NodeRegisterClass(&AVI);
}

void AVI_Done()
{
	NodeUnRegisterClass(AVI_ID);
}

