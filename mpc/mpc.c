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
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "mpc.h"
#include "libmusepack/include/musepack/musepack.h"

typedef struct mpc 
{
	format_base Format;

	mpc_reader Reader;
	mpc_decoder Decoder;
	mpc_streaminfo Info;
	MPC_SAMPLE_FORMAT* Buffer;

	int SampleRate;
	int SampleSize;
	int64_t Samples;
	mpc_uint16_t* SeekTable;

} mpc;

static void Done(mpc* p)
{
	free(p->SeekTable);
	p->SeekTable = NULL;
	free(p->Buffer);
	p->Buffer = NULL;
}

static int Init(mpc* p)
{
	format_stream* s;

	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	
	mpc_streaminfo_init(&p->Info);

	p->Samples = 0;
	p->Buffer = (MPC_SAMPLE_FORMAT*)malloc(sizeof(MPC_SAMPLE_FORMAT)*MPC_DECODER_BUFFER_LENGTH);
	if (!p->Buffer)
		return ERR_OUT_OF_MEMORY;

	if (mpc_streaminfo_read(&p->Info,&p->Reader) != ERROR_CODE_OK)
		return ERR_INVALID_DATA;

	s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = AUDIOFMT_PCM;
		s->Format.Format.Audio.Bits = MPC_FIXED_POINT_SCALE_SHIFT;
		s->Format.Format.Audio.SampleRate = p->Info.sample_freq;
		s->Format.Format.Audio.Channels = p->Info.channels;
		PacketFormatDefault(&s->Format);
		s->Format.ByteRate = (int)p->Info.average_bitrate >> 3;

		s->PacketBurst = 1;
		s->Fragmented = 1;
		s->DisableDrop = 1;
		p->Format.Duration = (tick_t)((p->Info.pcm_samples * TICKSPERSEC) / p->Info.sample_freq);

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

			if (p->Info.header_position>0)
			{
				// id3v2
				filepos_t Save = Reader->FilePos;
				if (Reader->Seek(Reader,0,SEEK_SET)==ERR_NONE)
				{
					void* Buffer = malloc(p->Info.header_position);
					if (Buffer)
					{
						int Len = Reader->Read(Reader,Buffer,p->Info.header_position);
						ID3v2_Parse(Buffer,Len,&p->Format.Comment,0);
						free(Buffer);
					}
					Reader->Seek(Reader,Save,SEEK_SET);
				}
			}
		}
	}

	mpc_decoder_setup(&p->Decoder,&p->Reader);

	p->SeekTable = (mpc_uint16_t*)malloc(sizeof(mpc_uint16_t)*(p->Info.frames+64));
	if (p->SeekTable)
	{
		memset(p->SeekTable,0,sizeof(mpc_uint16_t)*(p->Info.frames+64));
		p->Decoder.SeekTable = p->SeekTable;
	}

    if (!mpc_decoder_initialize(&p->Decoder, &p->Info)) 
		return ERR_INVALID_DATA;

	p->SampleRate = p->Info.sample_freq;
	p->SampleSize = p->Info.channels * sizeof(MPC_SAMPLE_FORMAT);

	return ERR_NONE;
}

static int Seek(mpc* p, tick_t Time, int FilePos, bool_t PrevKey)
{
	int64_t Samples;

	if (Time < 0)
	{
		if (FilePos<0 || p->Format.FileSize<0)
			return ERR_NOT_SUPPORTED;

		Time = Scale(FilePos,p->Format.Duration,p->Format.FileSize);
	}

	Samples = ((int64_t)Time * p->SampleRate+(TICKSPERSEC/2)) / TICKSPERSEC;

	if (!mpc_decoder_seek_sample(&p->Decoder,Samples))
		return ERR_NOT_SUPPORTED;

	p->Samples = Samples;
	return ERR_NONE;
}

static int Process(mpc* p,format_stream* Stream)
{
	int Result = ERR_NONE;
	int Samples,No,Burst;

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

		Samples = mpc_decoder_decode(&p->Decoder,p->Buffer,NULL,NULL);

		if (Samples == -1)
			return ERR_INVALID_DATA;

		if (Samples == 0)
			return Format_CheckEof(&p->Format,Stream);

		Stream->Packet.RefTime = (tick_t)((p->Samples * TICKSPERSEC) / p->SampleRate);
		Stream->Packet.Data[0] = p->Buffer;
		Stream->Packet.Length = Samples * p->SampleSize;
		Stream->Pending = 1;
		p->Samples += Samples;

		Result = Format_Send(&p->Format,Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			break;
	}

	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}

static mpc_int32_t IORead(void *data, void *ptr, mpc_int32_t size)
{
	format_reader* Reader = (format_reader*)data;
	return Reader->Read(Reader,ptr,size);
}

static mpc_bool_t IOSeek(void *data, mpc_int32_t offset)
{
	format_reader* Reader = (format_reader*)data;
	return (mpc_bool_t)(Format_Seek(Reader->Format,(filepos_t)offset,SEEK_SET) == ERR_NONE);
}

static mpc_int32_t IOTell(void *data)
{
	format_reader* Reader = (format_reader*)data;
    return Reader->FilePos;
}

static mpc_int32_t IOGetSize(void *data)
{
	format_reader* Reader = (format_reader*)data;
	if (Reader->Format->FileSize < 0)
		return 0;
	return Reader->Format->FileSize;
}

static mpc_bool_t IOCanSeek(void *data)
{
    return 1;
}

static int Create(mpc* p)
{
	p->Format.Init = (fmtfunc) Init;
	p->Format.Done = (fmtvoid) Done;
	p->Format.Seek = (fmtseek) Seek;
	p->Format.Process = (fmtstreamprocess) Process;
	p->Format.FillQueue = NULL;
	p->Format.ReadPacket = NULL;
	p->Format.Sended = NULL;

	p->Reader.data = p->Format.Reader;
    p->Reader.read = IORead;
	p->Reader.seek = IOSeek;
	p->Reader.tell = IOTell;
	p->Reader.get_size = IOGetSize;
	p->Reader.canseek = IOCanSeek;

	return ERR_NONE;
}

static const nodedef MPC =
{
	sizeof(mpc),
	MPC_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void MPC_Init()
{
	NodeRegisterClass(&MPC);
}

void MPC_Done()
{
	NodeUnRegisterClass(MPC_ID);
}
