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
 * $Id: mves.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "mpeg1.h"
#include "mpeg_decode.h"
#include "mpeg_stream.h"

typedef struct mves
{
	format_base Format;
	int LastSeekPos;
	tick_t LastSeekTime;

} mves;

static int Init(mves* p)
{
	format_stream* s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		format_reader* Reader = p->Format.Reader;
		uint8_t Head[20];
		tchar_t URL[MAXPATH];

		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_VIDEO;
		s->Format.Format.Video.Pixel.Flags = PF_FOURCC | PF_FRAGMENTED;

		s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','E','G');
		if (p->Format.Reader->Input->Get(p->Format.Reader->Input,STREAM_URL,URL,sizeof(URL))==ERR_NONE &&
			CheckExts(URL,T("m4v:V")) != 0)
			s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','4','V');

		if (Reader->Read(Reader,Head,sizeof(Head))==sizeof(Head))
		{
			int i;
			if (Head[0]==0 && Head[1]==0 && (Head[2]>>2)==0x20)
				s->Format.Format.Video.Pixel.FourCC = FOURCC('H','2','6','3');
			else
			for (i=0;i<20-4;++i)
				if (Head[i]==0 && Head[i+1]==0 && Head[i+2]==1)
				{
					if (Head[i+3]==0xB0)
						s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','4','V');
					else
					if (Head[i+3]==0xB3)
						s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','E','G');
					break;
				}
		}

		Reader->Seek(Reader,0,SEEK_SET);

		s->Fragmented = 1;

		Format_PrepairStream(&p->Format,s);
	}
	p->LastSeekPos = 0;
	p->LastSeekTime = 0;

	return ERR_NONE;
}

static int Seek(mves* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	packetformat Format;
	format_stream* Stream = p->Format.Streams[0];

	Format.ByteRate = 0;
	if (Stream->Pin.Node)
		Stream->Pin.Node->Get(Stream->Pin.Node,Stream->Pin.No|PIN_FORMAT,&Format,sizeof(Format));

	if (FilePos < 0)
	{
		if (Time > 0)
		{
			if (Format.ByteRate)
				FilePos = Scale(Time,Format.ByteRate,TICKSPERSEC);
			else
				return ERR_NOT_SUPPORTED;
		}
		else
			FilePos = 0;
	}
	else
	if (Format.ByteRate)
		Time = Scale(FilePos,TICKSPERSEC,Format.ByteRate);
	else
		Time = 1; //don't care (just not zero)

	p->LastSeekPos = FilePos;
	p->LastSeekTime = Time;

	return Format_Seek(&p->Format,FilePos,SEEK_SET);
}

static int ReadPacket(mves* p, format_reader* Reader, format_packet* Packet)
{
	format_stream* Stream = p->Format.Streams[0];

	if (Reader->FilePos==0)
		Packet->RefTime = 0;
	else
	if (Reader->FilePos==p->LastSeekPos)
		Packet->RefTime = p->LastSeekTime;
	else
		Packet->RefTime = TIME_UNKNOWN;

	Packet->Data = Reader->ReadAsRef(Reader,-4096);
	Packet->Stream = Stream;
	return ERR_NONE;
}

static int Create(mves* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.MinHeaderLoad = MINBUFFER/2;
	return ERR_NONE;
}

static const nodedef MVES =
{
	sizeof(mves),
	MVES_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT-5,
	(nodecreate)Create,
	NULL,
};

void MVES_Init()
{
	NodeRegisterClass(&MVES);
}

void MVES_Done()
{
	NodeUnRegisterClass(MVES_ID);
}
