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
 * $Id: rawimage.c 284 2005-10-04 08:54:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static int Init(rawimage* p)
{
	format_stream* s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_VIDEO;
		s->Format.Format.Video.Pixel.Flags = PF_FOURCC; //|PF_NOPREROTATE;
		s->Format.Format.Video.Pixel.FourCC = p->FourCC;
		s->PacketBurst = 1;
		s->DisableDrop = 1;
		Format_PrepairStream(&p->Format,s);
	}
	p->Total = 0;
	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	return ERR_NONE;
}

static int Packet(rawimage* p, format_reader* Reader, format_packet* Packet)
{
	return ERR_INVALID_PARAM;
}

static int Seek(rawimage* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	if (Time==0 || FilePos==0)
	{
		p->Total = 0;
		p->Format.Reader->Seek(p->Format.Reader,0,SEEK_SET);
		return ERR_NONE;
	}
	return ERR_NOT_SUPPORTED;
}

static void Sended(rawimage* p, format_stream* Stream)
{
	p->Total = 0;
	Format_AllocBufferBlock(&p->Format,Stream,0);
}

static int FillQueue(rawimage* p,format_reader* Reader)
{
	return ERR_NONE;
}

static int ProcessStream(rawimage* p,format_stream* Stream)
{
	int Result,Length;

	if (Stream->Pending)
	{
		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			return Result;
	}

	Length = p->Format.FileSize;
	if (Length < 0)
		Length = 1024*1024;

	if (Stream->BufferBlockLength < Length)
		Format_AllocBufferBlock(&p->Format,Stream,Length);

	do
	{
		format_ref *i;
		format_ref *Ref = Stream->Reader->ReadAsRef(Stream->Reader,-MAX_INT);

		for (i=Ref;i;i=i->Next)
		{
			int n = i->Length;
			if (n > Stream->BufferBlockLength - p->Total)
				n = Stream->BufferBlockLength - p->Total;

			if (n>0)
			{
				WriteBlock(&Stream->BufferBlock,p->Total,i->Buffer->Block.Ptr + i->Begin,n);
				p->Total += n;
			}
		}

		Format_ReleaseRef(&p->Format,Ref);
	}
	while (Stream->Reader->BufferAvailable>0);

	if (!Stream->Reader->Eof(Stream->Reader) && p->Total<Stream->BufferBlockLength)
		return ERR_NEED_MORE_DATA;

	if (!p->Total)
		return Format_CheckEof(&p->Format,Stream);

	Stream->Packet.RefTime = 0;
	Stream->Packet.Data[0] = Stream->BufferBlock.Ptr;
	Stream->Packet.Length = p->Total;
	Stream->Pending = 1;

	if (Stream->LastTime < TIME_UNKNOWN)
		Stream->LastTime = TIME_UNKNOWN;

	Result = Format_Send(&p->Format,Stream);

	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}

static int Create(rawimage* p)
{
	const tchar_t* Format = LangStr(p->Format.Format.Class,RAWIMAGE_FORMAT);
	if (tcsnicmp(Format,T("vcodec/"),7)==0)
		p->FourCC = StringToFourCC(Format+7,1);
	if (!p->FourCC)
		return ERR_INVALID_PARAM;
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)Packet;
	p->Format.FillQueue = (fmtfill)FillQueue;
	p->Format.Process = (fmtstreamprocess)ProcessStream;
	p->Format.Sended = (fmtstream)Sended;
	p->Format.Timing = 0;
	return ERR_NONE;
}

static const nodedef RawImage =
{
	sizeof(rawimage)|CF_ABSTRACT,
	RAWIMAGE_CLASS,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void RawImage_Init()
{
	 NodeRegisterClass(&RawImage);
}

void RawImage_Done()
{
	NodeUnRegisterClass(RAWIMAGE_CLASS);
}

