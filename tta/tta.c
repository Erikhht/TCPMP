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
#include "tta.h"
#include "ttalib/ttalib.h"

typedef struct tta 
{
	format_base Format;

	tta_info Decoder;
	uint8_t* Buffer;
  int BufferLen;
  int BufferFilled;

	int SampleRate;
	int SampleSize;
	int64_t Samples;
} tta;

static void Done(tta* p)
{
	free(p->Buffer);
	p->Buffer = NULL;

  close_tta_file(&p->Decoder);
  player_stop();
}

static int Init(tta* p)
{
  int ret;
	format_stream* s;

	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	
  p->Decoder.Reader = p->Format.Reader;
  ret = open_tta_file(&p->Decoder, 0);
  if (ret != 0)
    return ERR_INVALID_DATA;

  ret = player_init(&p->Decoder);
  if (ret != 0)
    return ERR_INVALID_DATA;

	p->Samples = 0;

	s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = AUDIOFMT_PCM;
		s->Format.Format.Audio.Bits = p->Decoder.BPS;
		s->Format.Format.Audio.SampleRate = p->Decoder.SAMPLERATE;
		s->Format.Format.Audio.Channels = p->Decoder.NCH;
		PacketFormatDefault(&s->Format);
		s->Format.ByteRate = (int)p->Decoder.BITRATE * 125;

		s->PacketBurst = 1;
		s->Fragmented = 1;
		s->DisableDrop = 1;
		p->Format.Duration = (tick_t)(((int64_t)p->Decoder.DATALENGTH * TICKSPERSEC) / s->Format.Format.Audio.SampleRate);

		Format_PrepairStream(&p->Format,s);

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

    p->SampleRate = s->Format.Format.Audio.SampleRate;
    p->SampleSize = p->Decoder.BSIZE * s->Format.Format.Audio.Channels;
	}

  p->BufferLen = PCM_BUFFER_LENGTH;
  p->Buffer = (uint8_t*)malloc(p->SampleSize*p->BufferLen);
  if (!p->Buffer)
    return ERR_OUT_OF_MEMORY;

	return ERR_NONE;
}

static int Seek(tta* p, tick_t Time, int FilePos, bool_t PrevKey)
{
	int64_t Samples;
  unsigned int Pos;


	if (Time < 0)
	{
		if (FilePos<0 || p->Format.FileSize<0)
			return ERR_NOT_SUPPORTED;

		Time = Scale(FilePos,p->Format.Duration,p->Format.FileSize);
	}

	Samples = ((int64_t)Time * p->SampleRate+(TICKSPERSEC/2)) / TICKSPERSEC;

  Pos = (unsigned int)((int64_t)Time * 1000 / TICKSPERSEC / SEEK_STEP);

	if (set_position(Pos))
		return ERR_NOT_SUPPORTED;

	Format_AfterSeek(&p->Format);
	p->Samples = Samples;
	return ERR_NONE;
}

static int Process(tta* p,format_stream* Stream)
{
	int Result = ERR_NONE;
	int No,Burst;

	if (Stream->Pending)
	{
		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			return Result;
	}

	Burst = Stream->PacketBurst;

	for (No=0;No<Burst;++No)
	{
		if (p->Format.Reader[0].BufferAvailable < (MINBUFFER/2) && 
			!p->Format.Reader[0].NoMoreInput)
			return ERR_NEED_MORE_DATA;

    p->BufferFilled = get_samples(p->Buffer);
    if (p->BufferFilled == -1)
			return ERR_INVALID_DATA;

		if (p->BufferFilled == 0)
			return Format_CheckEof(&p->Format,Stream);

		Stream->Packet.RefTime = (tick_t)((p->Samples * TICKSPERSEC) / p->SampleRate);
		Stream->Packet.Data[0] = p->Buffer;
		Stream->Packet.Length = p->BufferFilled * p->SampleSize;
		Stream->Pending = 1;
		p->Samples += p->BufferFilled;

		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			break;
	}

	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}


static int Create(tta* p)
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

static const nodedef TTA =
{
	sizeof(tta),
	TTA_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void TTA_Init()
{
	NodeRegisterClass(&TTA);
}

void TTA_Done()
{
	NodeUnRegisterClass(TTA_ID);
}
