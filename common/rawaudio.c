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
 * $Id: rawaudio.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

int RawAudioInit(rawaudio* p)
{
	format_stream* s = Format_AddStream(&p->Format,sizeof(format_stream));
	p->Head = 0;
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = p->Type;
		s->PacketBurst = 1;
		s->Fragmented = 1;
		s->DisableDrop = 1;
		Format_PrepairStream(&p->Format,s);

		if (p->Format.Comment.Node)
		{
			format_reader* Reader = p->Format.Reader;
			char Head[ID3V2_QUERYSIZE];

			// id3v1
			char Buffer[ID3V1_SIZE];
			filepos_t Save = Reader->Input->Seek(Reader->Input,0,SEEK_CUR);
			if (Save>=0 && Reader->Input->Seek(Reader->Input,-(int)sizeof(Buffer),SEEK_END)>=0)
			{
				if (Reader->Input->Read(Reader->Input,Buffer,sizeof(Buffer)) == sizeof(Buffer))
					ID3v1_Parse(Buffer,&p->Format.Comment);

				Reader->Input->Seek(Reader->Input,Save,SEEK_SET);
			}

			// id3v2
			if (Reader->Read(Reader,Head,sizeof(Head))==sizeof(Head))
			{
				int TagSize = ID3v2_Query(Head,sizeof(Head));
				if (TagSize > 0)
				{
					char* Buffer = (char*)malloc(TagSize);
					if (Buffer)
					{
						int Len;
						memcpy(Buffer,Head,sizeof(Head));
						Len = Reader->Read(Reader,Buffer+sizeof(Head),TagSize-sizeof(Head));
						if (Len>=0)
							ID3v2_Parse(Buffer,Len+sizeof(Head),&p->Format.Comment,0);
						free(Buffer);
					}
					p->Head = TagSize;
				}
			}

			Reader->Seek(Reader,p->Head,SEEK_SET);
		}
	}

	p->SeekPos = p->Head;
	return ERR_NONE;
}

static int ReadPacket(rawaudio* p, format_reader* Reader, format_packet* Packet)
{
	format_stream* Stream = p->Format.Streams[0];

	if (Reader->FilePos<=p->Head)
		Packet->RefTime = 0;
	else
	if (Stream->Format.ByteRate && !p->VBR)
		Packet->RefTime = Scale(Reader->FilePos-p->Head,TICKSPERSEC,Stream->Format.ByteRate);
	else
	if (Reader->FilePos<=p->SeekPos)
		Packet->RefTime = p->SeekTime;
	else
		Packet->RefTime = TIME_UNKNOWN;

	Packet->Data = Reader->ReadAsRef(Reader,-4096); //processing by 4K blocks are enough
	Packet->Stream = Stream;

	if (Stream->LastTime < TIME_UNKNOWN)
		Stream->LastTime = TIME_UNKNOWN;

	return ERR_NONE;
}

int RawAudioSeek(rawaudio* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	format_stream* Stream = p->Format.Streams[0];

	if (FilePos < 0)
	{
		if (Time > 0)
		{
			if (Stream->Format.ByteRate)
				FilePos = p->Head + Scale(Time,Stream->Format.ByteRate,TICKSPERSEC);
			else
				return ERR_NOT_SUPPORTED;
		}
		else
			FilePos = p->Head;
	}

	p->SeekTime = Time>0?Time:TIME_UNKNOWN;
	p->SeekPos = FilePos;
	return Format_Seek(&p->Format,FilePos,SEEK_SET);
}

static int Create(rawaudio* p)
{
	if (stscanf(LangStr(p->Format.Format.Class,RAWAUDIO_FORMAT),T("acodec/0x%x"),&p->Type)!=1)
		return ERR_INVALID_PARAM;
	p->Format.Init = (fmtfunc)RawAudioInit;
	p->Format.Seek = (fmtseek)RawAudioSeek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->SeekTime = 1; // but don't want to return 0
	p->VBR = 1;
	return ERR_NONE;
}

static const nodedef RawAudio = 
{
	sizeof(rawaudio)|CF_ABSTRACT,
	RAWAUDIO_CLASS,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void RawAudio_Init()
{
	 NodeRegisterClass(&RawAudio);
}

void RawAudio_Done()
{
	NodeUnRegisterClass(RAWAUDIO_CLASS);
}

