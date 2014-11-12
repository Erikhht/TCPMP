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
 * $Id: mpg.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "mpg.h"

typedef struct mpeg
{
	format_base Format;
	bool_t ErrorShowed;

} mpeg;

static int Init(mpeg* p)
{
	p->Format.TimeStamps = 1;
	p->Format.StartTime = 0;
	p->ErrorShowed = 0;
	return ERR_NONE;
}

static int findcode(format_reader* p)
{
	int n;
	int32_t code = -1;
	for (n=0;n<0x4000;++n)
	{
        int v = p->Read8(p);
		if (v<0)
			break;
		code = (code<<8)|v;
		if ((code >> 8)==1)
			return code;
    }
    return -1;
}

static int64_t readtime(format_reader* p,int i)
{
    int64_t t;
    t = (int64_t)((i >> 1) & 0x07) << 30;
    t |= (p->ReadBE16(p) >> 1) << 15;
    t |= (p->ReadBE16(p) >> 1);
    return t;
}

static int ReadPacket(mpeg* p, format_reader* Reader, format_packet* Packet)
{
	format_stream* s;
	int PCM = 0;
	int code = findcode(Reader);
	int i;
	int64_t time;
	filepos_t end;

    if (code == 0x1BE || code == 0x1BF) 
	{
		i = Reader->ReadBE16(Reader);
		Reader->Skip(Reader,i);
		return ERR_NONE;
    }

    if (code != 0x1BD && (code<0x1C0 || code>=0x1F0))
        return ERR_NONE;

    end = Reader->ReadBE16(Reader);
	end += Reader->FilePos;
    time = -1;
 
    while ((i = Reader->Read8(Reader))==255)
		if (Reader->FilePos >= end)
			return ERR_NONE;

    if ((i>>6)==1) 
	{
		Reader->Read8(Reader);
        i = Reader->Read8(Reader);
    }

	if ((i>>5)==1)
	{
		time = readtime(Reader,i);
		if (i&16)
			time = readtime(Reader,Reader->Read8(Reader));
	}
	else 
	if ((i>>6)==2) 
	{
		filepos_t end2;

		if ((i>>4)!=8) 
		{
			if (!p->ErrorShowed)
			{
				p->ErrorShowed = 1;
				ShowError(p->Format.Format.Class,MPG_ID,MPG_ENCRYPTED);
			}
	        return ERR_END_OF_FILE;
		}

        i = Reader->Read8(Reader);
        end2 = Reader->Read8(Reader);
		end2 += Reader->FilePos;

		if (end2 > end)
			return ERR_NONE;

        if (i&128) 
		{
            time = readtime(Reader,Reader->Read8(Reader));
			if (i&64)
				time = readtime(Reader,Reader->Read8(Reader));
		}

		Reader->Skip(Reader,end2-Reader->FilePos);
    }

    if (code == 0x1BD)
	{
        code = Reader->Read8(Reader);
        if (code >= 0x80 && code <= 0xBF) 
			Reader->Skip(Reader,3);
		if (code >= 0xA0 && code <= 0xBF) 
		{
			Reader->Skip(Reader,1);
			PCM = Reader->Read8(Reader);
			Reader->Skip(Reader,1);
		}
    }

	if (time>=0)
	{
		Packet->RefTime = (tick_t)((time * TICKSPERSEC) / 90000) - p->Format.StartTime;
		if (Packet->RefTime < 0)
			Packet->RefTime = 0;
	}
	else
		Packet->RefTime = TIME_UNKNOWN;

	s = NULL;
	for (i=0;i<p->Format.StreamCount;++i)
		if (p->Format.Streams[i]->Id == code)
		{
			s = p->Format.Streams[i];
			break;
		}

	if (!s)
	{
		if (Packet->RefTime < 0) //skip
			return ERR_NONE;

		if (!p->Format.StartTime)
		{
			p->Format.StartTime = Packet->RefTime;
			Packet->RefTime = 0;
		}

		switch (code>>4)
		{
		case 8:
		case 9:
			s = Format_AddStream(&p->Format,sizeof(format_stream));
			if (s)
			{
				s->Id = code;
				PacketFormatClear(&s->Format);
				s->Format.Type = PACKET_AUDIO;
				s->Format.Format.Audio.Format = AUDIOFMT_A52;
				Format_PrepairStream(&p->Format,s);
			}
			break;
		case 10:
		case 11:
			s = Format_AddStream(&p->Format,sizeof(format_stream));
			if (s)
			{
				static const int Freq[4] = { 48000, 96000, 44100, 32000 };
				s->Id = code;
				PacketFormatClear(&s->Format);
				s->Format.Type = PACKET_AUDIO;
				s->Format.Format.Audio.Format = AUDIOFMT_PCM;
				s->Format.Format.Audio.SampleRate = Freq[(PCM >> 4) & 3];
				s->Format.Format.Audio.Channels = 1 + (PCM & 7);
				s->Format.Format.Audio.Bits = 16;
				s->Format.Format.Audio.FracBits = 15;
				s->Format.ByteRate = s->Format.Format.Audio.SampleRate * s->Format.Format.Audio.Channels * 2;
				Format_PrepairStream(&p->Format,s);
			}
			break;
		case 28:
		case 29:
			s = Format_AddStream(&p->Format,sizeof(format_stream));
			if (s)
			{
				s->Id = code;
				PacketFormatClear(&s->Format);
				s->Format.Type = PACKET_AUDIO;
				s->Format.Format.Audio.Format = AUDIOFMT_MP2;
				Format_PrepairStream(&p->Format,s);
			}
			break;
		case 30:
			s = Format_AddStream(&p->Format,sizeof(format_stream));
			if (s)
			{
				// minimal mpeg4 detection
				int32_t code1 = 0;
				int32_t code2 = 0;
				filepos_t pos = Reader->FilePos;
				if (end >= pos+8)
				{
					code1 = Reader->ReadBE32(Reader);
					if (code1 == 0x100)
						code2 = Reader->ReadBE32(Reader);
					Reader->Seek(Reader,pos,SEEK_SET);
				}

				s->Id = code;
				PacketFormatClear(&s->Format);
				s->Format.Type = PACKET_VIDEO;
				s->Format.Format.Video.Pixel.Flags = PF_FOURCC | PF_FRAGMENTED;
				if ((code1 == 0x100 && code2 == 0x120) || code1 == 0x120)
					s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','4','V');
				else
					s->Format.Format.Video.Pixel.FourCC = FOURCC('M','P','E','G');
				Format_PrepairStream(&p->Format,s);
			}
			break;
		} 
		
		if (!s)
		{
			Reader->Skip(Reader,end - Reader->FilePos);
			return ERR_NONE;
		}
	}

	Packet->Stream = s;
	Packet->Data = Reader->ReadAsRef(Reader,end - Reader->FilePos);
	return ERR_NONE;
}

static int Create(mpeg* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Format_SeekByPacket;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.MinHeaderLoad = MINBUFFER/2;
	return ERR_NONE;
}

static const nodedef MPG =
{
	sizeof(mpeg),
	MPG_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT-10,		// lower priority so others can override
	(nodecreate)Create,
	NULL,
};

void MPG_Init()
{
	NodeRegisterClass(&MPG);
}

void MPG_Done()
{
	NodeUnRegisterClass(MPG_ID);
}

