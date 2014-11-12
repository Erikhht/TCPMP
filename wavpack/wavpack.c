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
 * $Id: mpc.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2006 Jory 'jcsston' Stone
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "wavpack.h"
#include "decoder/wavpack.h"

#define WAVPACK_BLOCK_LEN 512

typedef struct wavpack 
{
	format_base Format;

	WavpackContext *Decoder;
	long* Buffer;
	int BufferLen;
	int BufferFilled;

	int Bps;
	int Channels;
	int SampleRate;
	int SampleSize;
	int64_t Samples;
	int64_t FileLen;

  char Error[80];
} wavpack;

// wavpack callbacks

static long read_bytes(void *buff, long bcount, void *privdata)
{
	format_reader* Reader = ((wavpack *)privdata)->Format.Reader;
	return Reader->Read(Reader,buff,bcount);
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with (possibly) less than 4 bytes / sample.

static uchar *format_samples(int bps, uchar *dst, long *src, ulong samcnt)
{
	long temp;

	switch (bps) 
	{
	case 1:
		while (samcnt--)
			*dst++ = (uchar)(*src++ + 128);
		break;

	case 2:
		while (samcnt--) 
		{
			*dst++ = (uchar)(temp = *src++);
			*dst++ = (uchar)(temp >> 8);
		}
		break;

	case 3:
		while (samcnt--) 
		{
			*dst++ = (uchar)(temp = *src++);
			*dst++ = (uchar)(temp >> 8);
			*dst++ = (uchar)(temp >> 16);
		}
		break;

	case 4:
		while (samcnt--) 
		{
			*dst++ = (uchar)(temp = *src++);
			*dst++ = (uchar)(temp >> 8);
			*dst++ = (uchar)(temp >> 16);
			*dst++ = (uchar)(temp >> 24);
		}
		break;
	}
	return dst;
}

static void Done(wavpack* p)
{
	free(p->Buffer);
	p->Buffer = NULL;

	if (p->Decoder != NULL) 
	{
		WavpackClose(p->Decoder);
		p->Decoder = NULL;
	}
}

static int Init(wavpack* p)
{
	format_stream* s;

	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	p->Decoder = WavpackOpenFileInput(read_bytes, p->Error, p);

	p->Samples = 0;

	s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
    if (WavpackGetMode (p->Decoder) & MODE_FLOAT) {
      // Floating point?
      return ERR_NOT_SUPPORTED;
    }

		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = AUDIOFMT_PCM;
		s->Format.Format.Audio.Bits = WavpackGetBitsPerSample(p->Decoder);
		s->Format.Format.Audio.SampleRate = WavpackGetSampleRate(p->Decoder);
		s->Format.Format.Audio.Channels = WavpackGetNumChannels(p->Decoder);
		PacketFormatDefault(&s->Format);
		if (p->Format.FileSize>0)
			s->Format.ByteRate = (int)
			  (((float) p->Format.FileSize / WavpackGetNumSamples(p->Decoder) * s->Format.Format.Audio.Channels * s->Format.Format.Audio.Bits / 8) 
			  * s->Format.Format.Audio.SampleRate 
			  * s->Format.Format.Audio.Channels 
			  * s->Format.Format.Audio.Bits 
			  / 128);

		s->PacketBurst = 1;
		s->Fragmented = 1;
		s->DisableDrop = 1;
		p->Format.Duration = (tick_t)(((int64_t)WavpackGetNumSamples(p->Decoder) * TICKSPERSEC) / s->Format.Format.Audio.SampleRate);

		Format_PrepairStream(&p->Format,s);

    p->Bps = WavpackGetBytesPerSample(p->Decoder);
    p->Channels = s->Format.Format.Audio.Channels;
    p->SampleRate = s->Format.Format.Audio.SampleRate;
    p->SampleSize = s->Format.Format.Audio.Channels * sizeof(int16_t);

    if (p->Format.Comment.Node)
    {
      // id3v1
      format_reader* Reader = p->Format.Reader;
      char Buffer[ID3V1_SIZE];
      filepos_t Save = Reader->Input->Seek(Reader->Input,0,SEEK_CUR);
      if (Save>=0 && Reader->Input->Seek(Reader->Input,-(int)sizeof(Buffer),SEEK_END)>=0)
      {
        if (Reader->Input->Read(Reader->Input,Buffer,sizeof(Buffer)) == sizeof(Buffer))
          ID3v1_Parse(Buffer,&p->Format.Comment);

        Reader->Input->Seek(Reader->Input,Save,SEEK_SET);
      }
    }

    p->BufferLen = WAVPACK_BLOCK_LEN * p->Channels;
    p->Buffer = (long*)malloc(sizeof(long)*p->BufferLen);
    if (!p->Buffer)
      return ERR_OUT_OF_MEMORY;
	}

	return ERR_NONE;
}

static int Seek(wavpack* p, tick_t Time, int FilePos, bool_t PrevKey)
{
	int64_t Samples;

	if (Time < 0)
	{
		if (FilePos<0 || p->Format.FileSize<0)
			return ERR_NOT_SUPPORTED;

		Time = Scale(FilePos,p->Format.Duration,p->Format.FileSize);
	}
	if (FilePos < 0)
	{
		if (Time<0 || p->Format.FileSize<0 || p->Format.Duration<0)
			return ERR_NOT_SUPPORTED;

		FilePos = Scale(Time,p->Format.FileSize,p->Format.Duration);
	}

	Samples = ((int64_t)Time * p->SampleRate+(TICKSPERSEC/2)) / TICKSPERSEC;

	WavpackFlush (p->Decoder);

	Format_Seek(&p->Format, (filepos_t)FilePos, SEEK_SET);

	p->Samples = Samples;
	return ERR_NONE;
}

static int Process(wavpack* p,format_stream* Stream)
{
	int Result = ERR_NONE;

	if (Stream->Pending)
	{
		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			return Result;
	}

	if (p->Format.Reader[0].BufferAvailable < (MINBUFFER/2) && !p->Format.Reader[0].NoMoreInput)
		return ERR_NEED_MORE_DATA;

    p->BufferFilled = 0;
    p->BufferFilled = WavpackUnpackSamples (p->Decoder, p->Buffer, WAVPACK_BLOCK_LEN);
    if (p->BufferFilled < 0) 
		return ERR_INVALID_DATA;

	if (p->BufferFilled == 0)
		return Format_CheckEof(&p->Format,Stream);

    format_samples(p->Bps, (uchar *)p->Buffer, p->Buffer, p->BufferFilled * p->Channels);

	Stream->Packet.RefTime = (tick_t)((p->Samples * TICKSPERSEC) / p->SampleRate);
	Stream->Packet.Data[0] = p->Buffer;
	Stream->Packet.Length = p->BufferFilled * p->SampleSize;
	Stream->Pending = 1;
	p->Samples += p->BufferFilled;

	Result = Format_Send(&p->Format,Stream);

	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}


static int Create(wavpack* p)
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

static const nodedef WVP =
{
	sizeof(wavpack),
	WVP_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void WVP_Init()
{
	NodeRegisterClass(&WVP);
}

void WVP_Done()
{
	NodeUnRegisterClass(WVP_ID);
}
