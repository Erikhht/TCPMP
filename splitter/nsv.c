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
 * $Id: nsv.c 551 2006-01-09 11:55:09Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "nsv.h"

typedef struct nsvstream
{
	format_stream Stream;
	uint32_t FourCC;
	filepos_t FirstPos;

} nsvstream;

typedef struct nsv_t
{
	format_base Format;
	filepos_t Head;
	tick_t FirstTime;
	tick_t AVSync;
	int AVSyncFrame;
	bool_t NeedSync;
	uint32_t State;

	uint32_t AudioFourCC;
	uint32_t VideoFourCC;
	int VideoWidth;
	int VideoHeight;
	fraction VideoRate;

	int AuxCount;
	int AuxSize;
	int AudioSize;

	int64_t FrameState;
	int Frame;
	bool_t FrameValid;

} nsv;

static void Invalid(nsv* p)
{
	p->AVSyncFrame = -1;
	p->State = 0;
	p->AuxCount = 0;
	p->AuxSize = 0;
	p->AudioSize = 0;
}

static int Init(nsv* p)
{
	int i;
	format_reader* Reader = p->Format.Reader;
	p->Head = 0;
	Invalid(p);

	for (i=0;i<4;++i)
		p->State = (p->State << 8) | Reader->Read8(Reader);

	if (p->State == FOURCCBE('N','S','V','f'))
	{
		uint32_t MetaSize;
		uint32_t Duration;
		p->Head = Reader->FilePos - 4;
		p->Head += Reader->ReadLE32(Reader);

		Reader->Skip(Reader,4);
		Duration = Reader->ReadLE32(Reader);
		if (Duration != (uint32_t)-1)
			p->Format.Duration = Scale(Duration,TICKSPERSEC,1000);

		MetaSize = Reader->ReadLE32(Reader);
		Reader->Skip(Reader,8);

		//parse metadata...
		
		Reader->Seek(Reader,p->Head,SEEK_SET);
	}

	p->Frame = 0;
	p->FrameValid = 0;
	p->FrameState = 0;
	p->AVSyncFrame = -1;
	p->NeedSync = 1;
	p->FirstTime = 1; // but don't want to return 0
	return ERR_NONE;
}

static INLINE bool_t AVCStartSlice(int code,int data)
{
	return 0;
}

#define AVC_NAL_SLICE		1
#define AVC_NAL_IDR_SLICE	5

static void CountFrame(nsv* p, const uint8_t* Data, int Len)
{
	int i;
	int32_t code;
	int64_t v = p->FrameState;
	switch (p->VideoFourCC)
	{
	case FOURCC('H','2','6','4'):
		for (i=0;i<Len;++i)
		{
			v = (v<<8) | Data[i];
			code = (int32_t)(v>>8) & ~0xE0;
			if ((code == 0x100+AVC_NAL_SLICE || code == 0x100+AVC_NAL_IDR_SLICE) &&	(v & 0x80)==0x80) // bitgolomb()=0 -> slice_pos=0
				++p->Frame;
		}
		p->FrameValid = 1;
		break;
	}
	p->FrameState = v;
}

static void ReadData(nsv* p, format_reader* Reader, format_packet* Packet, uint32_t FourCC, int Size, int Mode)
{
	nsvstream* s = NULL;
	int n;

	if (FourCC == FOURCC('N','O','N','E') || !FourCC || p->NeedSync)
	{
		Reader->Skip(Reader,Size);
		return;
	}

	for (n=0;n<p->Format.StreamCount;++n)
	{
		s = (nsvstream*)p->Format.Streams[n];
		if (s->FourCC == FourCC)
			break;
	}
	if (n==p->Format.StreamCount)
	{
		packetformat Format;
		memset(&Format,0,sizeof(Format));
		Format.Type = PACKET_NONE;

		switch (Mode)
		{
		case 0:
			Format.Type = PACKET_VIDEO;
			Format.Format.Video.Width = p->VideoWidth;
			Format.Format.Video.Height = p->VideoHeight;
			Format.Format.Video.Pixel.Flags = PF_FOURCC | PF_FRAGMENTED;
			Format.Format.Video.Pixel.FourCC = UpperFourCC(FourCC);
			Format.PacketRate = p->VideoRate;
			break;

		case 1:
			switch (FourCC)
			{
			case FOURCC('M','P','3',' '):
				Format.Type = PACKET_AUDIO;
				Format.Format.Audio.Format = AUDIOFMT_MP3;
				break;

			case FOURCC('A','A','C','P'):
			case FOURCC('A','A','C',' '):
				Format.Type = PACKET_AUDIO;
				Format.Format.Audio.Format = AUDIOFMT_AAC;
				break;

			case FOURCC('S','P','X',' '):
				Format.Type = PACKET_AUDIO;
				Format.Format.Audio.Format = AUDIOFMT_SPEEX;
				break;
			}
			break;

		case 2:
			break;

		}

		if (Format.Type == PACKET_NONE || (s = (nsvstream*)Format_AddStream(&p->Format,sizeof(nsvstream)))==NULL)
		{
			Reader->Skip(Reader,Size);
			return;
		}

		s->FourCC = FourCC;
		PacketFormatCopy(&s->Stream.Format,&Format);
		s->Stream.Fragmented = 1;
		s->Stream.DisableDrop = Format.Type = PACKET_AUDIO;
		s->FirstPos = -1;

		Format_PrepairStream(&p->Format,&s->Stream);
	}

	if (s->FirstPos<0)
		s->FirstPos = Reader->FilePos;

	if (p->FrameValid && Mode==0)
		Packet->RefTime = p->FirstTime + Scale(p->Frame,TICKSPERSEC*p->VideoRate.Den,p->VideoRate.Num);
	else
	if (p->FrameValid && Mode==1 && p->AVSyncFrame>=0)
	{
		Packet->RefTime = p->FirstTime + p->AVSync + Scale(p->AVSyncFrame,TICKSPERSEC*p->VideoRate.Den,p->VideoRate.Num);
		p->AVSyncFrame = -1;
	}
	else
	if (Reader->FilePos <= s->FirstPos)
		Packet->RefTime = p->FirstTime;
	else
		Packet->RefTime = TIME_UNKNOWN;

	Packet->Data = Reader->ReadAsRef(Reader,Size);
	Packet->Stream = &s->Stream;
	if (s->Stream.LastTime < TIME_UNKNOWN)
		s->Stream.LastTime = TIME_UNKNOWN;

	if (Mode==0)
	{
		format_ref* Ref;
		for (Ref=Packet->Data;Ref;Ref=Ref->Next)
			CountFrame(p,Ref->Buffer->Block.Ptr + Ref->Begin, Ref->Length);
	}
}

static void ReadPayload(nsv* p, format_reader* Reader)
{
	int AuxCount = Reader->Read8(Reader);
	p->AuxSize = (Reader->ReadLE16(Reader) << 4) + (AuxCount >> 4);
	p->AuxCount = AuxCount & 15;
	p->AudioSize = Reader->ReadLE16(Reader);
	p->State = 0;
	if (p->AuxSize > 32768 || p->AuxSize > 0x80000 + p->AuxCount * (0x8000+6))
		Invalid(p);
}	

static int ReadPacket(nsv* p, format_reader* Reader, format_packet* Packet)
{
	filepos_t End = Reader->FilePos + BLOCKSIZE;
	while (Reader->FilePos<End)
	{
		if (p->AuxCount>0)
		{
			int Size = Reader->ReadLE16(Reader);
			if (Size > 32768)
				Invalid(p);
			else
			{
				uint32_t FourCC = Reader->ReadLE32(Reader);
				ReadData(p,Reader,Packet,FourCC,Size,2);
				p->AuxSize -= 6-Size;
				--p->AuxCount;
				return ERR_NONE;
			}
		}
		else
		if (p->AuxSize>0)
		{
			ReadData(p,Reader,Packet,p->VideoFourCC,p->AuxSize,0);
			p->AuxSize = 0;
			return ERR_NONE;
		}
		else
		if (p->AudioSize>0)
		{
			ReadData(p,Reader,Packet,p->AudioFourCC,p->AudioSize,1);
			p->AudioSize = 0;
			return ERR_NONE;
		}
		else
		if (p->State == FOURCCBE('N','S','V','s'))
		{
			int FrameRate;
			p->VideoFourCC = Reader->ReadLE32(Reader);
			p->AudioFourCC = Reader->ReadLE32(Reader);
			p->VideoWidth = Reader->ReadLE16(Reader);
			p->VideoHeight = Reader->ReadLE16(Reader);

			FrameRate = Reader->Read8(Reader);
			p->VideoRate.Num = FrameRate;
			p->VideoRate.Den = 1;
			if (FrameRate & 128)
			{
				int t = (FrameRate & 127) >> 2;
				if (t<16)
				{
					p->VideoRate.Num = 1;
					p->VideoRate.Den = t+1;
				}
				else
				{
					p->VideoRate.Num = t-15;
					p->VideoRate.Den = 1;
				}

				switch (FrameRate & 3)
				{
				case 0:
					p->VideoRate.Num *= 30;
					break;
				case 1:
					p->VideoRate.Num *= 30*1000;
					p->VideoRate.Den *= 1001;
					break;
				case 2:
					p->VideoRate.Num *= 25;
					break;
				case 3:
					p->VideoRate.Num *= 24*1000;
					p->VideoRate.Den *= 1001;
					break;
				}
			}

			p->AVSync = Scale((int16_t)Reader->ReadLE16(Reader),TICKSPERSEC,1000);
			p->AVSyncFrame = p->Frame;

			ReadPayload(p,Reader);
			if (p->NeedSync && (p->AuxSize || p->AudioSize))
				p->NeedSync = 0;
		}
		else
		if ((p->State & 0xFFFF) == 0xEFBE)
			ReadPayload(p,Reader);
		else
		{
			int ch = Reader->Read8(Reader);
			if (ch<0)
			{
				if (Reader->Eof(Reader))
					return ERR_END_OF_FILE;
				break;
			}
			p->State = (p->State << 8) + ch;
		}
	}
	return ERR_DATA_NOT_FOUND;
}

static int Seek(nsv* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	int i;
	if (FilePos < 0)
	{
		if (Time>0)
		{
			if (p->Format.Duration>=0 && p->Format.FileSize>=p->Head)
				FilePos = p->Head + Scale(Time,p->Format.FileSize-p->Head,p->Format.Duration);
			else
				return ERR_NOT_SUPPORTED;
		}
		else
			FilePos = p->Head;
	}

	Invalid(p);
	p->NeedSync = 1;
	p->Frame = 0;
	p->FrameValid = 0;
	p->FrameState = 0;
	p->FirstTime = Time>0?Time:TIME_UNKNOWN;
	for (i=0;i<p->Format.StreamCount;++i)
		((nsvstream*)p->Format.Streams[i])->FirstPos = -1;

	return Format_Seek(&p->Format,FilePos,SEEK_SET);
}

static int Create(nsv* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.MinHeaderLoad = MINBUFFER/2;
	return ERR_NONE;
}

static const nodedef NSV = 
{
	sizeof(nsv),
	NSV_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void NSV_Init()
{
	 NodeRegisterClass(&NSV);
}

void NSV_Done()
{
	NodeUnRegisterClass(NSV_ID);
}
