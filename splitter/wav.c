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
 * $Id: wav.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "wav.h"

typedef struct wav_t
{
	format_base Format;

	filepos_t FileSize;
	filepos_t DataPos;
	filepos_t DataEnd;
	int Samples;

} wav;

#define ALIGN2(x) (((x)+1)&~1)

static int Init(wav* p)
{
	p->DataPos = -1;
	p->DataEnd = -1;
	p->FileSize = 0;
	p->Samples = -1;

	return ERR_NONE;
}

static NOINLINE void BeginData(wav* p,format_reader* Reader,int Length)
{
	p->DataPos = Reader->FilePos;
	p->DataEnd = Length + Reader->FilePos;
	p->Format.HeaderLoaded = 1;

	if (p->Format.StreamCount)
	{
		tick_t Duration = 0;
		format_stream* Stream = p->Format.Streams[0];

		if (p->Samples >= 0)
			Duration = Scale(p->Samples,TICKSPERSEC,Stream->Format.Format.Audio.SampleRate);
		else
		if (Stream->Format.ByteRate)
			Duration = Scale(Length,TICKSPERSEC,Stream->Format.ByteRate);

		if (Duration>0 && p->Format.Duration<Duration)
			p->Format.Duration = Duration;
	}
}

static NOINLINE int Comment(wav* p,format_reader* Reader,int Id,int Len)
{
	char s8[256];
	tchar_t s[256];

	if (p->Format.Comment.Node)
	{
		Len = Reader->Read(Reader,s8,min(Len,sizeof(s8)-1));
		if (Len>0)
		{
			s8[Len] = 0;
			tcscpy_s(s,TSIZEOF(s),PlayerComment(Id));
			tcscat_s(s,TSIZEOF(s),T("="));
			StrToTcs(s+tcslen(s),TSIZEOF(s)-tcslen(s),s8);
			p->Format.Comment.Node->Set(p->Format.Comment.Node,p->Format.Comment.No,s,(tcslen(s)+1)*sizeof(tchar_t));
		}
		else
			Len=0;
	}
	else
		Len = 0;
	return Len;
}

static int ReadExtendedBE(format_reader* Reader)
{
	int exp = 0x403E - Reader->ReadBE16(Reader);
	uint64_t v = Reader->ReadBE64(Reader);
	return (int)(v>>exp);
}

static int ReadPacket(wav* p, format_reader* Reader, format_packet* Packet)
{
	format_stream* Stream;
	int Type,Length,DataLength,Offset;

	if (p->DataPos < 0 || Reader->FilePos < p->DataPos)
	{
		if (p->Format.Format.Class == WAV_ID)
		{
			Type = Reader->ReadLE32(Reader);
			DataLength = Reader->ReadLE32(Reader);
			Length = ALIGN2(DataLength);

			switch (Type)
			{
			case FOURCCLE('R','M','P','3'):
			case FOURCCLE('R','I','F','F'):
			case FOURCCLE('S','D','S','S'):
				p->FileSize = Length + 8;
				Reader->ReadLE32(Reader); // 'WAVE'
				return ERR_NONE;

			case FOURCCLE('f','m','t',' '):

				Stream = Format_AddStream(&p->Format,sizeof(format_stream));
				if (Stream)
				{
					Format_WaveFormat(Reader,Stream,Length);
					Stream->Fragmented = 0;
					Format_PrepairStream(&p->Format,Stream);
				}
				else
					Reader->Skip(Reader,Length);
				return ERR_NONE;

			case FOURCCLE('f','a','c','t'):
				p->Samples = Reader->ReadLE32(Reader);
				Length -= 4;
				break;

			case FOURCCLE('d','a','t','a'):
				BeginData(p,Reader,DataLength);
				return ERR_NONE;
			}
		}
		else
		{
			Type = Reader->ReadLE32(Reader);
			DataLength = Reader->ReadBE32(Reader);
			Length = ALIGN2(DataLength);

			switch (Type)
			{
			case FOURCCLE('F','O','R','M'):
				p->FileSize = Length + 8;
				Reader->ReadLE32(Reader); // 'AIFF' or 'AIFC'
				return ERR_NONE;

			case FOURCCLE('C','O','M','M'):

				Stream = Format_AddStream(&p->Format,sizeof(format_stream));
				if (Stream)
				{
					PacketFormatClear(&Stream->Format);
					Stream->Format.Type = PACKET_AUDIO;
					Stream->Format.Format.Audio.Format = AUDIOFMT_PCM;
#ifndef IS_BIG_ENDIAN
					Stream->Format.Format.Audio.Flags = PCM_SWAPEDBYTES;
#endif
					Stream->Format.Format.Audio.Channels = Reader->ReadBE16(Reader);
					p->Samples = Reader->ReadBE32(Reader);
					Stream->Format.Format.Audio.Bits = (Reader->ReadBE16(Reader)+7) & ~7;
					Stream->Format.Format.Audio.SampleRate = ReadExtendedBE(Reader);
					Length -= 2+4+2+10;
					if (Length>=4)
					{
						//compression
						uint32_t Type = Reader->ReadLE32(Reader);
						Length -= 4;
						switch (Type)
						{
						case FOURCCLE('i','m','a','4'):
							Stream->Format.Format.Audio.Format = AUDIOFMT_ADPCM_IMA;
							break;
						case FOURCCLE('u','l','a','w'):
							Stream->Format.Format.Audio.Format = AUDIOFMT_MULAW;
							break;
						case FOURCCLE('a','l','a','w'):
							Stream->Format.Format.Audio.Format = AUDIOFMT_ALAW;
							break;
						}
					}

					PacketFormatDefault(&Stream->Format);
					Stream->Format.Format.Audio.Flags &= ~PCM_UNSIGNED;

					Stream->Fragmented = 0;
					Format_PrepairStream(&p->Format,Stream);
				}
				break;

			case FOURCCLE('N','A','M','E'):
				Length -= Comment(p,Reader,COMMENT_TITLE,DataLength);
				break;

			case FOURCCLE('(','c',')',' '):
				Length -= Comment(p,Reader,COMMENT_COPYRIGHT,DataLength);
				break;

			case FOURCCLE('A','U','T','H'):
				Length -= Comment(p,Reader,COMMENT_AUTHOR,DataLength);
				break;

			case FOURCCLE('S','S','N','D'):
				Offset = Reader->ReadBE32(Reader);
				Reader->ReadBE32(Reader); //blocksize
				Reader->Skip(Reader,Offset);
				BeginData(p,Reader,DataLength-8-Offset);
				return ERR_NONE;
			}
		}

		if (Reader->FilePos + Length < p->FileSize)
			Reader->Skip(Reader,Length);
	}
	else
	{
		if (Reader->FilePos >= p->DataEnd)
			return ERR_END_OF_FILE;

		if (p->Format.StreamCount)
		{
			int Length;
			Stream = p->Format.Streams[0];

			if (Reader->FilePos == p->DataPos)
				Packet->RefTime = 0;
			else
			if (Stream->Format.ByteRate)
				Packet->RefTime = Scale(Reader->FilePos - p->DataPos,TICKSPERSEC,Stream->Format.ByteRate);
			else
				Packet->RefTime = TIME_UNKNOWN;

			Length = p->DataEnd - Reader->FilePos;
			if (Length > 4096) Length = 4096;
			Packet->Data = Reader->ReadAsRef(Reader,-Length);
			Packet->Stream = Stream;
		}
	}
	return ERR_NONE;
}

static int Seek(wav* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	format_stream* Stream;

	if (p->DataPos < 0 || !p->Format.StreamCount)
	{
		// no avi header yet, only seek to the beginning supported
		if (Time > 0 || FilePos > 0)
			return ERR_NOT_SUPPORTED;

		return Format_Seek(&p->Format,0,SEEK_SET);
	}

	Stream = p->Format.Streams[0];

	if (FilePos < 0)
	{
		if (Time > 0 && !Stream->Format.ByteRate)
			return ERR_NOT_SUPPORTED;

		FilePos = Scale(Time,Stream->Format.ByteRate,TICKSPERSEC);
	}

	if (Stream->Format.Format.Audio.BlockAlign > 1)
		FilePos = (FilePos / Stream->Format.Format.Audio.BlockAlign) * Stream->Format.Format.Audio.BlockAlign;

	if (FilePos<0) FilePos=0;
	if (FilePos>p->DataEnd-p->DataPos) FilePos=p->DataEnd-p->DataPos;

	return Format_Seek(&p->Format,p->DataPos+FilePos,SEEK_SET);
}

static int Create(wav* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	return ERR_NONE;
}
		
static const nodedef WAV = 
{
	sizeof(wav),
	WAV_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT-5,
	(nodecreate)Create,
	NULL,
};

static const nodedef AIFF = 
{
	0,
	AIFF_ID,
	WAV_ID,
	PRI_DEFAULT-5,
};

void WAV_Init()
{
	 NodeRegisterClass(&WAV);
	 NodeRegisterClass(&AIFF);
}

void WAV_Done()
{
	NodeUnRegisterClass(AIFF_ID);
	NodeUnRegisterClass(WAV_ID);
}
