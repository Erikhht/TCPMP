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
 * $Id: asap.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "asapfmt.h"
#include "asap/asap.h"

#define SAMPLE_RATE		44100
#define BUFFER_SIZE		4096

typedef struct asap 
{
	format_base Format;
	int Samples;
	int Total;
	int SamplePerBuffer;
	void* Buffer;

} asap;

static bool_t Load(asap* p)
{
	format_reader* Reader = p->Format.Reader;
	char Path8[MAXPATH];
	tchar_t Path[MAXPATH];
	void* buf;
	int n = p->Format.FileSize;
	if (n<=0) n=65536;

	buf = malloc(n);
	if (!buf) return 0;

	Reader->Input->Get(Reader->Input,STREAM_URL,Path,sizeof(Path));
	TcsToStr(Path8,sizeof(Path8),Path);

	n = Reader->Read(Reader,buf,n);

	n = ASAP_Load(Path8, buf, n);
	free(buf);
	if (!n)
		return 0;

	ASAP_PlaySong(ASAP_GetDefSong());

	p->Samples = 0;
	return 1;
}

static int Init(asap* p)
{
	format_stream* s;

	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	p->Format.Duration = 60*5*TICKSPERSEC;
	p->Total = Scale(p->Format.Duration,SAMPLE_RATE,TICKSPERSEC);

	ASAP_Initialize(SAMPLE_RATE, AUDIO_FORMAT_S16_NE, 3);

	if (!Load(p))
		return ERR_NOT_SUPPORTED;

	p->Buffer = malloc(BUFFER_SIZE);
	if (!p->Buffer)
		return ERR_OUT_OF_MEMORY;

	s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (!s)
		return ERR_OUT_OF_MEMORY;

	PacketFormatClear(&s->Format);
	s->Format.Type = PACKET_AUDIO;
	s->Format.Format.Audio.Format = AUDIOFMT_PCM;
	s->Format.Format.Audio.Bits = 16;
	s->Format.Format.Audio.SampleRate = SAMPLE_RATE;
	s->Format.Format.Audio.Channels = ASAP_GetChannels();
	PacketFormatDefault(&s->Format);

	s->Fragmented = 1;
	s->DisableDrop = 1;
	p->SamplePerBuffer = BUFFER_SIZE/2/s->Format.Format.Audio.Channels;

	Format_PrepairStream(&p->Format,s);
	return ERR_NONE;
}

static void Done(asap* p)
{
	free(p->Buffer);
	p->Buffer = NULL;
}

static int Seek(asap* p, tick_t Time, int FilePos, bool_t PrevKey)
{
	if (Time>0 || FilePos>0)
		return ERR_NOT_SUPPORTED;

	if (Format_Seek(&p->Format,0,SEEK_SET) != ERR_NONE)
		return ERR_NOT_SUPPORTED;

	Load(p);
	return ERR_NONE;
}

static int Process(asap* p,format_stream* Stream)
{
	int Result = ERR_NONE;

	if (Stream->Pending)
	{
		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			return Result;
	}

	if (p->Samples >= p->Total)
		return Format_CheckEof(&p->Format,Stream);

	ASAP_Generate(p->Buffer, BUFFER_SIZE);

	Stream->Packet.RefTime = Scale(p->Samples,TICKSPERSEC,SAMPLE_RATE);
	Stream->Packet.Data[0] = p->Buffer;
	Stream->Packet.Length = BUFFER_SIZE;
	Stream->Pending = 1;
	p->Samples += p->SamplePerBuffer;

	Result = Format_Send(&p->Format,Stream);

	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}

static int Create(asap* p)
{
	p->Format.Init = (fmtfunc) Init;
	p->Format.Done = (fmtvoid) Done;
	p->Format.Seek = (fmtseek) Seek;
	p->Format.Process = (fmtstreamprocess) Process;
	p->Format.FillQueue = NULL;
	p->Format.ReadPacket = NULL;
	p->Format.Sended = NULL;

	return ERR_NONE;
}

static const nodedef ASAPDef =
{
	sizeof(asap),
	ASAP_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void ASAP_Init()
{
	NodeRegisterClass(&ASAPDef);
}

void ASAP_Done()
{
	NodeUnRegisterClass(ASAP_ID);
}
