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
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "asf.h"

//#define DUMPASF

typedef struct asfstreamstate
{
	int Offset;
	int Size;
	tick_t RefTime;
	format_ref* Ref;
	format_ref** Tail;

} asfstreamstate;

typedef struct asfstream
{
	format_stream Stream;

	int TimeOffset;		// milliseconds
    int DS_Span;		// descrambling
    int DS_PacketSize;
    int DS_ChunkSize;
	asfstreamstate State;
	asfstreamstate Backup;

} asfstream;

typedef struct asfstate
{
	bool_t Key;
	bool_t Multiple;
	bool_t Compressed;

	filepos_t End;
	int ValueLength;
	int Padding;
	asfstream* Stream;

	int PayloadCount;
	filepos_t PayloadEnd;
	int PayloadOffset;
	int PayloadTime;

	int CompressedTime;
	int CompressedDelta;

} asfstate;

typedef struct asfguid
{
	guid ASFHeader;
	guid FileHeader;
	guid StreamHeader;
	guid AudioStream;
	guid VideoStream;
	guid CommentHeader;
	guid CodecCommentHeader;
	guid DataHeader;
	guid Index;
	guid HeadExt;
	guid Head2;
	guid NoErrorCorrection;
	guid AudioSpread;
	guid ExtendedComment;
	guid MarkerObject;
	guid StreamBitrate;
	guid StreamHeaderExt;

} asfguid;

typedef struct asf
{
	format_base Format;

	filepos_t DataPos;
	int PacketMinSize;
	int PacketMaxSize;
	int64_t PacketCount;
	int64_t PlayTime;
	int PreRoll;
	int Flags;
	int ClientId;
	bool_t AdjustTime;
	bool_t AdjustTime0;
	array Bitrate;
	tick_t StreamingTime;

	format_ref* HTTPRef;
	format_reader HTTPReader;
	int HTTPChunkLeft;
	int HTTPChunkFilter;

	asfstate State;
	asfstate Backup;

	asfguid GUID;

} asf;

static const asfguid ASFGUID =
{
    { 0x75B22630, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }},
    { 0x8CABDCA1, 0xA947, 0x11CF, { 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 }},
    { 0xB7DC0791, 0xA9B7, 0x11CF, { 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 }},
    { 0xF8699E40, 0x5B4D, 0x11CF, { 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B }},
    { 0xBC19EFC0, 0x5B4D, 0x11CF, { 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B }},
    { 0x75B22633, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }},
	{ 0x86D15240, 0x311D, 0x11D0, { 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 }},
	{ 0x75B22636, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }},
	{ 0x33000890, 0xE5B1, 0x11CF, { 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB }},
    { 0x5FBF03B5, 0xA92E, 0x11CF, { 0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 }},
    { 0xABD3D211, 0xA9BA, 0x11CF, { 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 }},
	{ 0x20FB5700, 0x5B55, 0x11CF, { 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B }},
	{ 0xBFC3CD50, 0x618F, 0x11CF, { 0x8B, 0xB2, 0x00, 0xAA, 0x00, 0xB4, 0xE2, 0x20 }},
	{ 0xD2D0A440, 0xE307, 0x11D2, { 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 }},
	{ 0xF487CD01, 0xA951, 0x11CF, { 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 }},
	{ 0x7BF875CE, 0x468D, 0x11D1, { 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2 }},
	{ 0x14E6A5CB, 0xC672, 0x4332, { 0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A }}
};

static void ReadGUID(format_reader* p,guid* g)
{
    g->v1 = p->ReadLE32(p);
    g->v2 = (uint16_t) p->ReadLE16(p);
    g->v3 = (uint16_t) p->ReadLE16(p);
    p->Read(p,g->v4,8);
}

static void ReadStr(format_reader* p, int Len, tchar_t *Buf, size_t BufLen)
{
	uint16_t ch[2];
	ch[1] = 0;

    for (;Len>0;Len-=2) 
	{
        ch[0] = (uint16_t) p->ReadLE16(p);
#ifndef UNICODE
        WcsToTcs(Buf,BufLen,ch);
		BufLen -= tcslen(Buf);
		Buf += tcslen(Buf);
#else
        if (BufLen > 1)
		{
			*Buf++ = ch[0];
			--BufLen;
		}
#endif
    }
    if (BufLen>0)
		*Buf = 0;
}

static void ReleaseStream(asf* p,asfstream* s)
{
	Format_ReleaseRef(&p->Format,s->State.Ref);
	s->State.Ref = NULL;
	s->State.Tail = &s->State.Ref;
	s->State.Offset = 0;
	s->State.Size = 0;
}

static void AfterSeek(asf* p)
{
	p->State.PayloadCount = 0;
	p->State.PayloadEnd = -1;
	p->State.End = -1;
	p->State.Stream = NULL;
}

static void BackupPacketState(asf* p, int Save)
{
	int No;

	// ugly...
	if (Save)
	{
		p->Backup = p->State;
		AfterSeek(p);
	}
	else
	{
		AfterSeek(p);
		p->State = p->Backup;
	}

	for (No=0;No<p->Format.StreamCount;++No)
	{
		asfstream* s = (asfstream*) p->Format.Streams[No];
		if (Save)
		{
			s->Backup = s->State;
			s->State.Ref = NULL; // don't release fragref
			ReleaseStream(p,s);
		}
		else
		{
			ReleaseStream(p,s);
			s->State = s->Backup;
		}
	}
}

static void ReadStreamHeader(asf* p, format_reader* Reader)
{
	int StreamId,i;
	int TimeOffset;
	int* BitRate;
	int FormatSize;
	int ErrorSize;
	guid Id,StreamType;
	asfstream* s;

	ReadGUID(Reader,&StreamType); // stream type

	ReadGUID(Reader,&Id); // error correction type
	TimeOffset = (int)(Reader->ReadLE64(Reader)/10000) - p->PreRoll;
	FormatSize = Reader->ReadLE32(Reader); // type-specific data length
	ErrorSize = Reader->ReadLE32(Reader); // error correction length
	StreamId = Reader->ReadLE16(Reader) & 0x7F; // flags
	Reader->Skip(Reader,4);

	for (i=0;i<p->Format.StreamCount;++i)
		if (p->Format.Streams[i]->Id == StreamId)
			return;

	s = (asfstream*) Format_AddStream(&p->Format,sizeof(asfstream));
	if (!s)
		return;

	ReleaseStream(p,s);
	s->Stream.Id = StreamId;
	s->TimeOffset = TimeOffset;
	s->DS_Span = 0;

	if (memcmp(&StreamType, &p->GUID.AudioStream, sizeof(guid))==0)
	{
		Format_WaveFormat(Reader,&s->Stream,FormatSize);
		if (memcmp(&Id,&p->GUID.AudioSpread,sizeof(guid))==0)
		{
			int SilentLength;
			s->DS_Span = Reader->Read8(Reader);
			s->DS_PacketSize = Reader->ReadLE16(Reader);
			s->DS_ChunkSize = Reader->ReadLE16(Reader);
			SilentLength = Reader->ReadLE16(Reader);
			Reader->Skip(Reader,SilentLength);
		}

		if (s->DS_Span > 1) 
			if (!s->DS_ChunkSize || (s->DS_PacketSize/s->DS_ChunkSize <= 1))
				s->DS_Span = 0;
	}
	else
	if (memcmp(&StreamType, &p->GUID.VideoStream, sizeof(guid))==0)
	{
		int BitmapInfoSize;
		PacketFormatClear(&s->Stream.Format);
		s->Stream.Format.Type = PACKET_VIDEO;
		
		Reader->Skip(Reader,4+4+1); 
		BitmapInfoSize = Reader->ReadLE16(Reader);
		if (BitmapInfoSize > 40 || FormatSize<=40+4+4+1+2)
			FormatSize = BitmapInfoSize;
		else
			FormatSize -= 4+4+1+2;
		Reader->Skip(Reader,4); 

		s->Stream.Format.Format.Video.Width = Reader->ReadLE32(Reader);
		s->Stream.Format.Format.Video.Height = Reader->ReadLE32(Reader);
		Reader->ReadLE16(Reader); // planes
		s->Stream.Format.Format.Video.Pixel.BitCount = Reader->ReadLE16(Reader);
		s->Stream.Format.Format.Video.Pixel.FourCC = Reader->ReadLE32(Reader);
		s->Stream.Format.Format.Video.Aspect = ASPECT_ONE; //todo
		Reader->Skip(Reader,20); //SizeImage,XPelsPerMeter,YPelsPerMeter,ClrUsed,ClrImportant

		FormatSize -= 40;
		if (FormatSize > 0 && PacketFormatExtra(&s->Stream.Format,FormatSize))
			Reader->Read(Reader,s->Stream.Format.Extra,s->Stream.Format.ExtraLength);

		PacketFormatDefault(&s->Stream.Format);
	}

	if (!s->Stream.Format.ByteRate)
		for (BitRate=ARRAYBEGIN(p->Bitrate,int);BitRate!=ARRAYEND(p->Bitrate,int);BitRate+=2)
			if (BitRate[0] == s->Stream.Id)
			{
				s->Stream.Format.ByteRate = BitRate[1]/8;
				break;
			}

	Format_PrepairStream(&p->Format,&s->Stream);
}

static int ReadHeaderExt(asf* p,format_reader* Reader,int Size)
{
	guid Id;
	filepos_t EndPos;
	filepos_t End = Reader->FilePos+Size;

    while (Reader->FilePos<End)
	{
		if (Reader->Eof(Reader))
			return ERR_INVALID_DATA;

        ReadGUID(Reader,&Id);

        EndPos = (filepos_t)Reader->ReadLE64(Reader) - 24;
        if (EndPos < 0)
			return ERR_INVALID_DATA;
		EndPos += Reader->FilePos;

		DEBUG_MSG6(DEBUG_FORMAT,T("ASFExt %08x %08x-%04x-%04x-%08x%08x"),Reader->FilePos,Id.v1,Id.v2,Id.v3,*(int32_t*)&Id.v4[0],*(int32_t*)&Id.v4[4]);

        if (memcmp(&Id, &p->GUID.StreamHeaderExt, sizeof(guid))==0) 
		{
			int i,n,PayloadCount,NameCount;

			// skip extended information
			// we are only interested in StreamHeader at the end...

			Reader->Skip(Reader,8); // start time
			Reader->Skip(Reader,8); // end time
			Reader->Skip(Reader,4+4+4); // data bitrate, buffer size, initial buffer fullness
			Reader->Skip(Reader,4+4+4); // alternate data bitrate, buffer size, initial buffer fullness
			Reader->Skip(Reader,4+4); // maximum object size, flags
			Reader->Skip(Reader,2); // stream id
			Reader->Skip(Reader,2+8); // language id, avg time per frame
			NameCount = Reader->ReadLE16(Reader);
			PayloadCount = Reader->ReadLE16(Reader);

			for (i=0;i<NameCount;++i)
			{
				Reader->Skip(Reader,2); // lang id
				n = Reader->ReadLE16(Reader);
				Reader->Skip(Reader,n*2);
			}

			for (i=0;i<PayloadCount;++i)
			{
				Reader->Skip(Reader,16+2); // guid, data size
				n = Reader->ReadLE32(Reader);
				Reader->Skip(Reader,n);
			}

			if (EndPos>Reader->FilePos+16+8)
			{
		        ReadGUID(Reader,&Id);
				Reader->ReadLE64(Reader);
				if (memcmp(&Id, &p->GUID.StreamHeader, sizeof(guid))==0)
					ReadStreamHeader(p,Reader);
			}
		}
		
        Reader->Seek(Reader,EndPos,SEEK_SET);
	}
	return ERR_NONE;
}

static int ReadHeader(asf* p,format_reader* Reader)
{
	int i,Count;
	tchar_t Buf[256];
	guid Id;
	filepos_t EndPos;

	ReadGUID(Reader,&Id);
	if (memcmp(&Id, &p->GUID.ASFHeader, sizeof(guid))!=0)
		return ERR_INVALID_DATA;

	ArrayDrop(&p->Bitrate);

	Reader->Skip(Reader,8+4+1+1);
    for (;;)
	{
		if (Reader->Eof(Reader))
			return ERR_INVALID_DATA;

        ReadGUID(Reader,&Id);

        EndPos = (filepos_t)Reader->ReadLE64(Reader) - 24;

		if (!memcmp(&Id, &p->GUID.DataHeader, sizeof(guid))) // allow invalid EndPos for this chunk
		{
			Reader->Skip(Reader,sizeof(guid)+8+2); // file id, total data packets, reserved
            break;
        } 

        if (EndPos < 0)
			return ERR_INVALID_DATA;
		EndPos += Reader->FilePos;

		DEBUG_MSG6(DEBUG_FORMAT,T("ASF %08x %08x-%04x-%04x-%08x%08x"),Reader->FilePos,Id.v1,Id.v2,Id.v3,*(int32_t*)&Id.v4[0],*(int32_t*)&Id.v4[4]);

        if (memcmp(&Id, &p->GUID.FileHeader, sizeof(guid))==0) 
		{
			int BitRate;
			Reader->Skip(Reader,sizeof(guid)+8+8);
			p->PacketCount = Reader->ReadLE64(Reader);
			p->PlayTime = Reader->ReadLE64(Reader);
			Reader->Skip(Reader,8); // send_time (sometimes not even valid)
			p->AdjustTime = 0;
			p->PreRoll = Reader->ReadLE32(Reader);
			Reader->Skip(Reader,4); // ignore
			p->Flags = Reader->ReadLE32(Reader);
			p->PacketMinSize = Reader->ReadLE32(Reader); // min packet size
			p->PacketMaxSize = Reader->ReadLE32(Reader); // max packet size
			BitRate = Reader->ReadLE32(Reader); // max bitrate
			if (BitRate > 0)
				p->Format.SumByteRate = BitRate/8;
			if (!(p->Flags & 1)) // not broadcast
				p->Format.Duration = (tick_t)(((p->PlayTime - (int64_t)p->PreRoll * 10000) * TICKSPERSEC) / 10000000);
			else
			{
				p->AdjustTime = 1;
				p->Format.Duration = -1;
				if (p->PacketMinSize <= 0)
					p->PacketMinSize = 0;
				if (p->PacketMaxSize <= 0)
					p->PacketMaxSize = 4*1024*1024;
			}
			p->AdjustTime0 = p->AdjustTime;
        }
		else if (memcmp(&Id, &p->GUID.StreamHeader, sizeof(guid))==0)
		{
			ReadStreamHeader(p,Reader);
		}
		else if (!memcmp(&Id, &p->GUID.MarkerObject, sizeof(guid)) && p->Format.Comment.Node)
		{
			int Len;
			Reader->Skip(Reader,sizeof(guid)); // reserved
			Count = Reader->ReadLE32(Reader);
			Reader->Skip(Reader,2);			  // reserved
			Len = Reader->ReadLE16(Reader);
			ReadStr(Reader,Len,Buf,TSIZEOF(Buf)); // marker name

			for (i=0;i<Count;++i)
			{
				int64_t Time;
				filepos_t EndPos;

				Reader->Skip(Reader,8); // position
				Time = Reader->ReadLE64(Reader);
				EndPos = Reader->ReadLE16(Reader);
				EndPos += Reader->FilePos;
				Reader->Skip(Reader,8); // send time, flags

				stprintf_s(Buf,TSIZEOF(Buf),T("CHAPTER%02dNAME="),i+1);
				Len = Reader->ReadLE32(Reader);
				ReadStr(Reader,Len*sizeof(uint16_t),Buf+tcslen(Buf),TSIZEOF(Buf)-tcslen(Buf));
				p->Format.Comment.Node->Set(p->Format.Comment.Node,p->Format.Comment.No,Buf,sizeof(Buf));

				BuildChapter(Buf,TSIZEOF(Buf),i+1,Time,10000);
				p->Format.Comment.Node->Set(p->Format.Comment.Node,p->Format.Comment.No,Buf,sizeof(Buf));

				Reader->Seek(Reader,EndPos,SEEK_SET);
			}
		}			
		else if (!memcmp(&Id, &p->GUID.ExtendedComment, sizeof(guid)) && p->Format.Comment.Node)
		{
			int Type,Len;

			Count = Reader->ReadLE16(Reader);
			for (i=0;i<Count;++i)
			{
				Len = Reader->ReadLE16(Reader);
				ReadStr(Reader,Len,Buf,TSIZEOF(Buf));
				Type = Reader->ReadLE16(Reader);
				Len = Reader->ReadLE16(Reader);
				if (Type == 0)
				{
					tcsupr(Buf);
					tcscat_s(Buf,TSIZEOF(Buf),T("="));
					ReadStr(Reader,Len,Buf+tcslen(Buf),TSIZEOF(Buf)-tcslen(Buf));
					p->Format.Comment.Node->Set(p->Format.Comment.Node,p->Format.Comment.No,Buf,sizeof(Buf));
				}
				else
					Reader->Skip(Reader,Len);
			}
		}
		else if (!memcmp(&Id, &p->GUID.HeadExt, sizeof(guid)))
		{
	        ReadGUID(Reader,&Id);
			if (!memcmp(&Id, &p->GUID.Head2, sizeof(guid)))
			{
				int Result;
				Reader->Skip(Reader,2);
				Result = ReadHeaderExt(p,Reader,Reader->ReadLE32(Reader));
				if (Result != ERR_NONE)
					return Result;
			}
		}
		else if (!memcmp(&Id, &p->GUID.StreamBitrate, sizeof(guid)))
		{
			int Len = Reader->ReadLE16(Reader);
			ArrayAppend(&p->Bitrate,NULL,Len*2*sizeof(int),32);
			for (i=0;i<Len;++i)
			{
				int No,Id,BitRate;
				ARRAYBEGIN(p->Bitrate,int)[i*2] = Id = Reader->ReadLE16(Reader);
				ARRAYBEGIN(p->Bitrate,int)[i*2+1] = BitRate = Reader->ReadLE32(Reader);

				for (No=0;No<p->Format.StreamCount;++No)
				{
					format_stream* Stream = p->Format.Streams[No];
					if (Stream->Id == Id && !Stream->Format.ByteRate)
						Stream->Format.ByteRate = BitRate/8;
				}
			}
		}
		else if (!memcmp(&Id, &p->GUID.CommentHeader, sizeof(guid)) && p->Format.Comment.Node) 
		{
			static const tchar_t* const Comment[5] = { T("TITLE="), T("AUTHOR="), T("COPYRIGHT="), T("COMMENT="), T("RATING=") };
            int Len[5];

			for (i=0;i<5;++i) 
				Len[i] = Reader->ReadLE16(Reader);

			for (i=0;i<5;++i)
				if (Len[i])
				{
					tcscpy_s(Buf,TSIZEOF(Buf),Comment[i]);
					ReadStr(Reader,Len[i],Buf+tcslen(Buf),TSIZEOF(Buf)-tcslen(Buf));
					if (tcslen(Buf) != tcslen(Comment[i]))
						p->Format.Comment.Node->Set(p->Format.Comment.Node,p->Format.Comment.No,Buf,sizeof(Buf));
				}
		}

        Reader->Seek(Reader,EndPos,SEEK_SET);
    }

	return ERR_NONE;
}

static int Init(asf* p)
{
	format_reader* Reader = p->Format.Reader;
	int Result;

	Result = ReadHeader(p,Reader);
	if (Result != ERR_NONE)
		return Result;
 
	AfterSeek(p);

	p->StreamingTime = -1;
	p->Format.TimeStamps = 1;
	p->Format.HeaderLoaded = 1;
	p->Format.SeekByPacket_DataStart = Reader->FilePos;
	if (p->PacketMaxSize == p->PacketMinSize)
		p->Format.SeekByPacket_BlockAlign = p->PacketMaxSize;

	return Result;
}

static void Done(asf* p)
{
	ArrayClear(&p->Bitrate);
}

static int NOINLINE ReadValue(asf* p, format_reader* Reader,int Bits,int Default)
{
	switch (Bits & 3)
	{
	default:
	case 0: return Default;
	case 1: return Reader->Read8(Reader);
	case 2: return Reader->ReadLE16(Reader);
	case 3: return Reader->ReadLE32(Reader);
	}
}

static bool_t ReadPacketHead(asf* p, format_reader* Reader,filepos_t End)
{
	filepos_t pos;
	int len,time,padding;
	int i;

	pos = Reader->FilePos;
	if (p->PacketMinSize>0 && p->PacketMaxSize==p->PacketMinSize)
	{
		p->State.End = pos + p->PacketMinSize;
		p->State.Padding = 0;

	}

	i = Reader->Read8(Reader);
	if (i<0)
		return 0;

	if (i & 0x80)
	{
		// no real error correction supported
		if (i != 0x82 && i != 0x90)
			return 1;

		Reader->Skip(Reader,i & 15);

		i = Reader->Read8(Reader);
	}
	
	p->State.Multiple = i & 1;
    p->State.ValueLength = Reader->Read8(Reader);

	len = ReadValue(p,Reader,i>>5,p->PacketMaxSize);
	if (len<8 || len>p->PacketMaxSize)
		return 1;

    ReadValue(p,Reader,i>>1,0); // sequence
	padding = ReadValue(p,Reader,i>>3,0);
	if (padding<0 || padding>=p->PacketMaxSize) 
		return 1;

    time = Reader->ReadLE32(Reader); // send time (milliseconds)
	if (p->StreamingTime >= 0)
	{
		if (!p->AdjustTime)
		{
			// orb server uses global time as timestamp, have to adjust it (but there is duration)
			p->Format.GlobalOffset = p->StreamingTime - Scale(time,TICKSPERSEC,1000);
			p->Format.TimeStamps = 0;
		}
		p->StreamingTime = -1;
	}

    Reader->Skip(Reader,2); // duration (milliseconds)

    if (p->State.Multiple)
	{
		i = Reader->Read8(Reader);
		p->State.PayloadCount = i&63;
		p->State.ValueLength |= i<<8;
    } 
	else 
		p->State.PayloadCount = 1;

	pos += len - padding;
	if (pos <= Reader->FilePos)
	{
		p->State.PayloadCount = 0;
		return 1;
	}

	assert(pos-Reader->FilePos<0x10000000);

    if (len < p->PacketMinSize)
        padding += p->PacketMinSize - len;

	p->State.End = pos;
    p->State.Padding = padding;

	if (p->HTTPRef) // adjust padding to remaining http packet length
	{
		p->State.Padding = Reader->ReadLen + p->HTTPChunkLeft - Reader->ReadPos - (pos - Reader->FilePos);
		if (p->State.Padding < 0)
		{
			p->State.End += p->State.Padding;
			p->State.Padding = 0;
		}
	}

//	DEBUG_MSG3(-1,T("asf %08x %d count:%d"),p->State.End,p->State.Padding,p->State.PayloadCount);
	return 1;
}

static bool_t ReadPayload(asf* p,format_reader* Reader)
{
	int n,v = Reader->Read8(Reader); // stream number + keyframe bit
	if (v<0)
	{
		p->State.PayloadCount = 0;
		return 0;
	}

	p->State.Stream = NULL;
	p->State.Key = v>>7;
	v &= 0x7F;

	for (n=0;n<p->Format.StreamCount;++n)
		if (p->Format.Streams[n]->Id == v)
		{
			p->State.Stream = (asfstream*) p->Format.Streams[n];
			break;
		}

	ReadValue(p,Reader,p->State.ValueLength>>4,0); // sequence
	v = ReadValue(p,Reader,p->State.ValueLength>>2,0);
	n = ReadValue(p,Reader,p->State.ValueLength,0); // replicated data length

	p->State.Compressed = n==1;
	if (p->State.Compressed)
	{
		p->State.CompressedTime = p->State.PayloadTime = v;
		p->State.CompressedDelta = Reader->Read8(Reader);
		p->State.PayloadOffset = 0;
	}
	else 
	{
		p->State.PayloadOffset = v;
		if (n>1)
		{
			if (n<8)
			{
				p->State.PayloadCount = 0;
				return 1;
			}
			if (p->State.Stream)
			{
				p->State.Stream->State.Size = Reader->ReadLE32(Reader);
				p->State.PayloadTime = Reader->ReadLE32(Reader);
				n -= 8;
			}
			Reader->Skip(Reader,n);
		}
	}

	if (p->AdjustTime)
	{
		p->AdjustTime = 0;
		for (n=0;n<p->Format.StreamCount;++n)
			((asfstream*)p->Format.Streams[n])->TimeOffset = -p->State.PayloadTime;
	}

	if (p->State.Multiple)
	{
		p->State.PayloadEnd = ReadValue(p,Reader,p->State.ValueLength>>14,0);
		p->State.PayloadEnd += Reader->FilePos;
		if (p->State.PayloadEnd > p->State.End)
		{
			p->State.PayloadCount = 0;
			p->State.PayloadEnd = -1;
			return 1;
		}
	}
	else
		p->State.PayloadEnd = p->State.End;

	if (!p->State.Stream)
	{
		Reader->Skip(Reader,p->State.PayloadEnd - Reader->FilePos);
		p->State.PayloadEnd = -1;
		--p->State.PayloadCount;
	}
	return 1;
}

static int ReadPacket(asf* p, format_reader* Reader, format_packet* Packet)
{
	filepos_t End = Reader->FilePos+BLOCKSIZE;
	for (;;)
	{
		if (p->State.PayloadCount<=0)
		{
			if (Reader->FilePos >= End)
				break;

			if (p->State.End >= 0)
			{
				Reader->Skip(Reader,p->State.End+p->State.Padding-Reader->FilePos);
				p->State.End = -1;
			}

			if (!ReadPacketHead(p,Reader,End))
				break;
		}
		else 
		if (p->State.PayloadEnd<0)
		{
			if (!ReadPayload(p,Reader))
				break;
		}
		else
		{
			int Size = p->State.PayloadEnd - Reader->FilePos;
			if (Size<=0)
			{
				p->State.PayloadEnd = -1;
				--p->State.PayloadCount;
			}
			else
			{
				asfstream* s = p->State.Stream;
				if (p->State.Compressed)
				{
					s->State.Size = Reader->Read8(Reader);
					if (s->State.Size > Size)
					{
						Reader->Skip(Reader,Size);
						continue;
					}
					Size = s->State.Size;
					p->State.PayloadTime = p->State.CompressedTime;
					p->State.CompressedTime += p->State.CompressedDelta;
				}

				if (s->State.Offset != p->State.PayloadOffset) // gap in offset
				{
					ReleaseStream(p,s);
					if (p->State.PayloadOffset)
					{
						Reader->Skip(Reader,Size);
						continue;
					}
				}

				if (s->State.Offset==0) // start of a new packet
				{
					s->State.RefTime = Scale(p->State.PayloadTime + s->TimeOffset,TICKSPERSEC,1000);
					if (s->State.RefTime < 0)
						s->State.RefTime = 0;
				}

				*s->State.Tail = Reader->ReadAsRef(Reader,Size);
				while (*s->State.Tail)
					s->State.Tail = &(*s->State.Tail)->Next;

				s->State.Offset += Size;
				if (s->State.Offset >= s->State.Size) 
				{
					Packet->Key = p->State.Key;
					Packet->Data = s->State.Ref;
					Packet->RefTime = s->State.RefTime;
					Packet->Stream = &s->Stream;

					if (s->DS_Span > 1) 
					{
						format_ref* New = NULL;
						format_ref** Ptr = &New;

						int Left = s->State.Size;
						int Row = 0;
						int Col = 0;
						while (Left>0) 
						{
							int Idx = Row+Col*s->DS_Span;

							*Ptr = Format_DupRef(&p->Format,Packet->Data,Idx * s->DS_ChunkSize, s->DS_ChunkSize );
							while (*Ptr)
								Ptr = &(*Ptr)->Next;

							Left -= s->DS_ChunkSize;
							Col++;
							if (Col == s->DS_Span)
							{
								Col = 0;
								Row++;
							}
						}

						Format_ReleaseRef(&p->Format,Packet->Data);
						Packet->Data = New;
					}

					s->State.Size = 0;
					s->State.Offset = 0;
					s->State.Ref = NULL;
					s->State.Tail = &s->State.Ref;
					return ERR_NONE;
				}
			}
		}
	}

	return ERR_DATA_NOT_FOUND;
}

static int Seek(asf* p,tick_t Time,filepos_t FilePos,bool_t PrevKey)
{
	return Format_SeekByPacket(&p->Format,Time,FilePos,PrevKey);
}

#ifdef DUMPASF
static FILE* Dump = NULL;
#endif

static bool_t GetHTTPBuffer(format_reader* HTTPReader)
{
	int n,Ofs = 0;
	asf* p = ((asf*)((char*)(HTTPReader)-(int)&(((asf*)0)->HTTPReader)));
	format_reader* Reader = p->Format.Reader;

	for (n=0;n<16;++n) // maximum chunk tries
	{
		if (p->HTTPRef)
		{
			Format_ReleaseRef(&p->Format,p->HTTPRef);
			p->HTTPRef = NULL;
			HTTPReader->ReadBuffer = NULL;
			Ofs = HTTPReader->ReadPos - HTTPReader->ReadLen;
			HTTPReader->ReadLen = 0;
		}

		if (p->HTTPChunkLeft > 0)
		{
			p->HTTPRef = Reader->ReadAsRef(Reader,-p->HTTPChunkLeft);
			if (!p->HTTPRef)
			{
				HTTPReader->NoMoreInput = Reader->Eof(Reader);
				break;
			}
			else
			{
				HTTPReader->ReadBuffer = p->HTTPRef->Buffer;
				HTTPReader->ReadPos = p->HTTPRef->Begin + Ofs;
				HTTPReader->ReadLen = p->HTTPRef->Begin + p->HTTPRef->Length;
				p->HTTPChunkLeft -= p->HTTPRef->Length;

#ifdef DUMPASF
				if (Dump)
					fwrite(HTTPReader->ReadBuffer->Block.Ptr+HTTPReader->ReadPos,1,HTTPReader->ReadLen-HTTPReader->ReadPos,Dump);
#endif
				break;
			}
		}
		else
		{
			int Type,Len;

			Type = Reader->ReadLE16(Reader);
			Len = Reader->ReadLE16(Reader)-8;
			Reader->Skip(Reader,8);

			if ((Type & ASF_CHUNK_MASK) != p->HTTPChunkFilter)
			{
				if (Type == ASF_CHUNK_END && Len<0)
					Len += 8;

				HTTPReader->NoMoreInput = Reader->Eof(Reader);
				if (HTTPReader->NoMoreInput)
					break;

				if (Len >= 0)
				{
#if 0 //def DUMPASF
					if (Dump)
					{
						void* i = malloc(Len);
						Reader->Read(Reader,i,Len);
						fwrite(i,1,Len,Dump);
						free(i);
					}
					else
#endif

					Reader->Skip(Reader,Len);
				}
			}
			else
				p->HTTPChunkLeft = Len;
		}
	}

	return HTTPReader->ReadBuffer != NULL;
}

static void AfterSeekHTTP(asf* p)
{
	AfterSeek(p);

	Format_ReleaseRef(&p->Format,p->HTTPRef);
	p->HTTPRef = NULL;
	p->HTTPChunkLeft = 0;
	p->HTTPReader.NoMoreInput = 0;
	p->HTTPReader.FilePos = 0;
	p->HTTPReader.ReadPos = 0;
	p->HTTPReader.ReadLen = 0;
	p->HTTPReader.ReadBuffer = NULL;
}

static int SetPragma(asf* p,tick_t Time)
{
	int i;
	tchar_t Pragma[512];

	if (p->Format.StreamCount &&
		p->Format.Reader[0].Input->Get(p->Format.Reader[0].Input,STREAM_PRAGMA_GET,Pragma,sizeof(Pragma)) == ERR_NONE &&
		tcslen(Pragma)>0)
	{
		bool_t First=1;
		int Count=0;
		p->StreamingTime = Time;

		for (i=0;i<p->Format.StreamCount;++i)
			if (p->Format.Streams[i]->Pin.Node)
				++Count;

		stprintf_s(Pragma,TSIZEOF(Pragma),
			T("Pragma: no-cache,rate=1.000000,stream-time=%d,stream-offset=4294967295:4294967295,")
			T("request-context=2,max-duration=0,")
			T("xPlayStrm=1,")
			T("xClientGUID={3300AD50-2C39-46c0-AE0A-132C%08X},")
			T("stream-switch-count=%d,")
			T("stream-switch-entry=")
			,Scale(Time,1000,TICKSPERSEC),p->ClientId,Count);
		
		for (i=0;i<p->Format.StreamCount;++i)
			if (p->Format.Streams[i]->Pin.Node)
			{
				if (!First) tcscat_s(Pragma,TSIZEOF(Pragma),T(" ")); else First=0;
				stcatprintf_s(Pragma,TSIZEOF(Pragma),T("ffff:%d:0"),p->Format.Streams[i]->Id);
			}

		tcscat_s(Pragma,TSIZEOF(Pragma),T("\n"));

		if (p->Format.Reader[0].Input->Set(p->Format.Reader[0].Input,STREAM_PRAGMA_SEND,Pragma,sizeof(Pragma)) == ERR_NONE)
		{
			int Result = Format_SeekForce(&p->Format,0,SEEK_SET); //reopen (because of seeking)
			if (Result == ERR_NONE)
				p->AdjustTime = p->AdjustTime0;
			return Result;
		}
	}

	return ERR_NOT_SUPPORTED;
}

static int InitHTTP(asf* p)
{
	int Result;

	p->StreamingTime = 0;
	p->HTTPRef = NULL;

	memcpy(&p->HTTPReader,p->Format.Reader,sizeof(p->HTTPReader));
	p->HTTPReader.BufferAvailable = 0;
	p->HTTPReader.BufferFirst = NULL;
	p->HTTPReader.BufferLast = NULL;
	p->HTTPReader.Input = NULL;
	p->HTTPReader.InputBuffer = NULL;
	p->HTTPReader.GetReadBuffer = GetHTTPBuffer;

	AfterSeekHTTP(p);
	p->HTTPChunkFilter = ASF_CHUNK_HEADER;

	Result = ReadHeader(p,&p->HTTPReader);
	if (Result != ERR_NONE)
		return Result;
 
	AfterSeekHTTP(p);
	p->HTTPChunkFilter = ASF_CHUNK_DATA;
	p->PacketMinSize = 0; // it's possible packets are not padded to MinSize

	p->ClientId = (rand() << 16) | rand();
	p->Format.HeaderLoaded = 1;
	p->Format.ReadSize = 4096;

	if (!GetHTTPBuffer(&p->HTTPReader) && p->HTTPReader.Eof(&p->HTTPReader))
		Result = SetPragma(p,0); // try reopen http stream with proper pragma

#ifdef DUMPASF
	Dump = fopen("\\dumpasf.asf","wb+");
#endif

	return Result;
}

static void DoneHTTP(asf* p)
{
#ifdef DUMPASF
	if (Dump)
	{
		fclose(Dump);
		Dump = NULL;
	}
#endif
	Format_ReleaseRef(&p->Format,p->HTTPRef);
	Done(p);
}

static int SetHTTP(asf* p,int No,const void* Data,int Size)
{
	int Result = FormatBaseSet(&p->Format,No,Data,Size);

	if (No==FORMAT_UPDATEMASK && Size==sizeof(tick_t))
		SetPragma(p,*(tick_t*)Data);

	return Result;
}

static int SeekHTTP(asf* p,tick_t Time,filepos_t FilePos,bool_t PrevKey)
{
	if (Time < 0)
		return ERR_NOT_SUPPORTED;
	return SetPragma(p,Time);
}

static int ReadPacketHTTP(asf* p, format_reader* Reader, format_packet* Packet)
{
	return ReadPacket(p,&p->HTTPReader,Packet);
}

static int Create(asf* p)
{
	p->GUID = ASFGUID;
	p->Format.Init = (fmtfunc)Init;
	p->Format.Done = (fmtvoid)Done;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.ReleaseStream = (fmtstream)ReleaseStream;
	p->Format.AfterSeek = (fmtvoid)AfterSeek;
	p->Format.BackupPacketState = (fmtbackup)BackupPacketState;
	return ERR_NONE;
}

static int CreateHTTP(asf* p)
{
	p->GUID = ASFGUID;
	p->Format.Init = (fmtfunc)InitHTTP;
	p->Format.Done = (fmtvoid)DoneHTTP;
	p->Format.Seek = (fmtseek)SeekHTTP;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacketHTTP;
	p->Format.AfterSeek = (fmtvoid)AfterSeekHTTP;
	p->Format.Format.Set = (nodeset)SetHTTP;
	return ERR_NONE;
}

static const nodedef ASF = 
{
	sizeof(asf),
	ASF_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT-1, // prefer WMVF and WMAF (just for the reader name)
	(nodecreate)Create,
};

static const nodedef ASFHTTP = 
{
	0, // parent size
	ASF_HTTP_ID,
	ASF_ID,
	PRI_DEFAULT,
	(nodecreate)CreateHTTP,
	NULL,
};

void ASF_Init()
{
	NodeRegisterClass(&ASF);
	NodeRegisterClass(&ASFHTTP);
}

void ASF_Done()
{
	NodeUnRegisterClass(ASF_ID);
	NodeUnRegisterClass(ASF_HTTP_ID);
}
