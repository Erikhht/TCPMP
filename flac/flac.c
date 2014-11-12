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
#include "flac.h"
#include "FLAC/seekable_stream_decoder.h"

typedef struct flac 
{
	format_base Format;

	FLAC__SeekableStreamDecoder *Decoder;
	FLAC__StreamMetadata_StreamInfo Info;
	int16_t* Buffer;
	int BufferLen;
	int BufferFilled;

	int SampleRate;
	int SampleSize;
	int64_t Samples;

} flac;

#undef malloc
void* _calloc(int n,int m)
{
	void* p = malloc(n*m);
	if (p) memset(p,0,n*m);
	return p;
}

// libFLAC callbacks

static FLAC__SeekableStreamDecoderReadStatus ReadCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	format_reader* Reader = ((flac *)client_data)->Format.Reader;
	*bytes = Reader->Read(Reader,buffer,*bytes);
	return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
}

static FLAC__SeekableStreamDecoderSeekStatus SeekCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	format_reader* Reader = ((flac *)client_data)->Format.Reader;
	return Reader->Seek(Reader,(filepos_t)absolute_byte_offset,SEEK_SET) >= 0 ? FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK : FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
}

static FLAC__SeekableStreamDecoderTellStatus TellCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	format_reader* Reader = ((flac *)client_data)->Format.Reader;
	*absolute_byte_offset = Reader->FilePos;
	return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__SeekableStreamDecoderLengthStatus LengthCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	format_reader* Reader = ((flac *)client_data)->Format.Reader;
	if (Reader->Format->FileSize < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	*stream_length = Reader->Format->FileSize;

	return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool EofCallback(const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	format_reader* Reader = ((flac *)client_data)->Format.Reader;
	return (Reader->Format->FileSize == Reader->FilePos) ? true : false;
}

static FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	flac* p = (flac *)client_data;  
	int16_t *pBuffer = p->Buffer;
	int size = frame->header.blocksize * frame->header.channels;
	int sizeChannel = frame->header.blocksize;
	int s;

	if (p->BufferLen < size)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if (frame->header.channels == 1) 
	{
		for(s = 0; s < size; s++) 
			pBuffer[s] = (int16_t)buffer[0][s];
	} 
	else 
	{    
		// Interleave the channels    
		unsigned c;
		for(s = 0; s < sizeChannel; s++) 
			for (c = 0; c < frame->header.channels; c++) 
				*(pBuffer++) = (int16_t)buffer[c][s];
	}

	p->BufferFilled = sizeChannel;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void MetadataCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	flac* p = (flac *)client_data;
	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) 
	{
		p->Info = metadata->data.stream_info;
	} 
	else 
	if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) 
	{
		const FLAC__StreamMetadata_VorbisComment *comments = &metadata->data.vorbis_comment;
		tchar_t	s[256];
    
		if (p->Format.Comment.Node) 
		{
			uint32_t No;
			for (No = 0; No < comments->num_comments; ++No) 
			{
				UTF8ToTcs(s, TSIZEOF(s), (const char *)comments->comments[No].entry);
				p->Format.Comment.Node->Set(p->Format.Comment.Node, p->Format.Comment.No, s, sizeof(s));
			}
		}    
	}
}

static void ErrorCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
}

static void Done(flac* p)
{
	free(p->Buffer);
	p->Buffer = NULL;

	if (p->Decoder != NULL) 
	{
		FLAC__seekable_stream_decoder_delete(p->Decoder);
		p->Decoder = NULL;
	}
}

static int Init(flac* p)
{
	format_stream* s;
	FLAC__StreamDecoderState state;

	p->Format.HeaderLoaded = 1;
	p->Format.TimeStamps = 0;
	p->Decoder = FLAC__seekable_stream_decoder_new();

	FLAC__seekable_stream_decoder_set_client_data(p->Decoder, p);
	FLAC__seekable_stream_decoder_set_read_callback(p->Decoder, ReadCallback);	
	FLAC__seekable_stream_decoder_set_seek_callback(p->Decoder, SeekCallback);
	FLAC__seekable_stream_decoder_set_length_callback(p->Decoder, LengthCallback);
	FLAC__seekable_stream_decoder_set_tell_callback(p->Decoder, TellCallback);
	FLAC__seekable_stream_decoder_set_eof_callback(p->Decoder, EofCallback);
	FLAC__seekable_stream_decoder_set_write_callback(p->Decoder, WriteCallback);
	FLAC__seekable_stream_decoder_set_metadata_callback(p->Decoder, MetadataCallback);
	FLAC__seekable_stream_decoder_set_error_callback(p->Decoder, ErrorCallback);
	FLAC__seekable_stream_decoder_set_metadata_respond_all(p->Decoder);

	state = FLAC__seekable_stream_decoder_init(p->Decoder);

	p->Samples = 0;

	if (FLAC__seekable_stream_decoder_process_until_end_of_metadata(p->Decoder) != true)
		return ERR_INVALID_DATA;

	p->BufferLen = p->Info.max_blocksize * p->Info.channels;
	p->Buffer = (int16_t*)malloc(sizeof(int16_t)*p->BufferLen);
	if (!p->Buffer)
		return ERR_OUT_OF_MEMORY;

	s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_AUDIO;
		s->Format.Format.Audio.Format = AUDIOFMT_PCM;
		s->Format.Format.Audio.Bits = 16;
		s->Format.Format.Audio.SampleRate = p->Info.sample_rate;
		s->Format.Format.Audio.Channels = p->Info.channels;
		PacketFormatDefault(&s->Format);
		if (p->Format.FileSize>0)
			s->Format.ByteRate = (int)
				(((float) p->Format.FileSize / (int64_t)p->Info.total_samples * s->Format.Format.Audio.Channels * s->Format.Format.Audio.Bits / 8) 
				* s->Format.Format.Audio.SampleRate 
				* s->Format.Format.Audio.Channels 
				* s->Format.Format.Audio.Bits / 128);

		s->Fragmented = 1;
		s->DisableDrop = 1;
		p->Format.Duration = (tick_t)((p->Info.total_samples * TICKSPERSEC) / p->Info.sample_rate);

		Format_PrepairStream(&p->Format,s);
	}

	p->SampleRate = p->Info.sample_rate;
	p->SampleSize = p->Info.channels * sizeof(int16_t);

	return ERR_NONE;
}

static int Seek(flac* p, tick_t Time, int FilePos, bool_t PrevKey)
{
	int64_t Samples;

	if (Time < 0)
	{
		if (FilePos<0 || p->Format.FileSize<0)
			return ERR_NOT_SUPPORTED;

		Time = Scale(FilePos,p->Format.Duration,p->Format.FileSize);
	}

	Samples = ((int64_t)Time * p->SampleRate+(TICKSPERSEC/2)) / TICKSPERSEC;

	if (!FLAC__seekable_stream_decoder_seek_absolute(p->Decoder,Samples))
		return ERR_NOT_SUPPORTED;

	Format_AfterSeek(&p->Format);
	p->Samples = Samples;
	return ERR_NONE;
}

static int Process(flac* p,format_stream* Stream)
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

    if (FLAC__seekable_stream_decoder_process_single(p->Decoder) != true) 
	{
		FLAC__StreamDecoderState state;
		// Check state
		state = FLAC__seekable_stream_decoder_get_stream_decoder_state(p->Decoder);
		switch (state) 
		{
		case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
		case FLAC__STREAM_DECODER_READ_METADATA:
			// We should have already read in the metadata
			return ERR_INVALID_DATA;

		case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:          
		case FLAC__STREAM_DECODER_READ_FRAME:
			// Need more data
			return ERR_NEED_MORE_DATA;

		case FLAC__STREAM_DECODER_END_OF_STREAM:          
		case FLAC__STREAM_DECODER_ABORTED:
		case FLAC__STREAM_DECODER_UNPARSEABLE_STREAM:
			return ERR_INVALID_DATA;

		case FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR:
			return ERR_OUT_OF_MEMORY;

		case FLAC__STREAM_DECODER_ALREADY_INITIALIZED:
		case FLAC__STREAM_DECODER_INVALID_CALLBACK:
		case FLAC__STREAM_DECODER_UNINITIALIZED:
			// I don't see how this could happen
			return ERR_INVALID_DATA;
		}
		return ERR_INVALID_DATA;
    }

	if (p->BufferFilled == 0)
		return Format_CheckEof(&p->Format,Stream);

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


static int Create(flac* p)
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

static const nodedef FLAC =
{
	sizeof(flac),
	FLAC_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void FLAC_Init()
{
	NodeRegisterClass(&FLAC);
}

void FLAC_Done()
{
	NodeUnRegisterClass(FLAC_ID);
}
