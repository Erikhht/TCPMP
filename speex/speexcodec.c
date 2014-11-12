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
 * $Id: speexcodec.c 620 2006-01-31 14:46:34Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "speex.h"
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#define MAX_FRAME_SIZE 2000

typedef struct speex
{
	codec Codec;
	int ExtraHeaders;
	int AfterFlush;
	int FrameSize;
	int Frames;
	int FramesPerPacket;
	void *Decoder;
	SpeexBits Bits;
	SpeexStereoState Stereo;
	spx_int16_t Output[MAX_FRAME_SIZE];

} speex;

static int UpdateInput(speex* p)
{
	if (p->Decoder)
	{
		speex_decoder_destroy(p->Decoder);
		speex_bits_destroy(&p->Bits);
		p->Decoder = NULL;
	}

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		int Enabled = 1;
		const SpeexMode *Mode;
		SpeexHeader *Header;
		SpeexCallback Callback;

		if (p->Codec.In.Format.ExtraLength < 8)
		{
			if (p->Codec.In.Format.Format.Audio.SampleRate > 25000)
				Mode = &speex_uwb_mode; // ideal 32000hz
			else if (p->Codec.In.Format.Format.Audio.SampleRate > 12500)
				Mode = &speex_wb_mode; // ideal 16000hz
			else 
				Mode = &speex_nb_mode; // ideal 8000hz

			Header = malloc(sizeof(SpeexHeader));
			if (Header)
				speex_init_header(Header, p->Codec.In.Format.Format.Audio.SampleRate, p->Codec.In.Format.Format.Audio.Channels, Mode);
		}
		else
			Header = speex_packet_to_header(p->Codec.In.Format.Extra,p->Codec.In.Format.ExtraLength);

		if (!Header || Header->mode >= SPEEX_NB_MODES || Header->speex_version_id > 1)
		{
			free(Header);
			return ERR_NOT_SUPPORTED;
		}

		Mode = speex_lib_get_mode(Header->mode);
		if (Mode->bitstream_version != Header->mode_bitstream_version)
		{
			free(Header);
			return ERR_NOT_SUPPORTED;
		}

		p->Decoder = speex_decoder_init(Mode);
		if (!p->Decoder)
		{
			free(Header);
			return ERR_OUT_OF_MEMORY;
		}

		p->FramesPerPacket = max(1,Header->frames_per_packet);
		p->ExtraHeaders = Header->extra_headers;

		speex_decoder_ctl(p->Decoder, SPEEX_SET_ENH, &Enabled);
		speex_decoder_ctl(p->Decoder, SPEEX_GET_FRAME_SIZE, &p->FrameSize);

#ifdef NO_FLOATINGPOINT
		Header->nb_channels = 1; //force mono. stereo would need floating point calculations
#endif

		p->Codec.In.Format.Format.Audio.SampleRate = Header->rate;
		p->Codec.In.Format.Format.Audio.Channels = Header->nb_channels;

		if (Header->nb_channels != 1)
		{
			Callback.callback_id = SPEEX_INBAND_STEREO;
			Callback.func = speex_std_stereo_request_handler;
			Callback.data = &p->Stereo;
			speex_decoder_ctl(p->Decoder, SPEEX_SET_HANDLER, &Callback);
		}

		speex_decoder_ctl(p->Decoder, SPEEX_SET_SAMPLING_RATE, &Header->rate);

		speex_bits_init(&p->Bits);

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,15);

		free(Header);
	}

	return ERR_NONE;
}

static int Process(speex* p, const packet* Packet, const flowstate* State)
{
	int Result;
	if (Packet)
	{
		if (Packet->RefTime >= 0)
		{
			if (Packet->RefTime == 0)
				p->AfterFlush = 0;
			p->Codec.Packet.RefTime = Packet->RefTime;
		}

		if (p->ExtraHeaders>0)
		{
			--p->ExtraHeaders;
			return ERR_NEED_MORE_DATA;
		}

		speex_bits_read_from(&p->Bits, (char*)Packet->Data[0], Packet->Length);
		p->Frames = p->FramesPerPacket;
	}
	else
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	if (p->Frames<=0)
		return ERR_NEED_MORE_DATA;

    Result = speex_decode_int(p->Decoder, &p->Bits, p->Output);
	if (Result==-1)
	{
		p->Frames = 0;
		return ERR_NEED_MORE_DATA;
	}

	if (Result==-2 || speex_bits_remaining(&p->Bits)<0)
	{
		p->Frames = 0;
		return ERR_INVALID_DATA;
	}

	if (p->Codec.In.Format.Format.Audio.Channels==2)
		speex_decode_stereo_int(p->Output, p->FrameSize, &p->Stereo);

	p->Codec.Packet.Length = p->FrameSize * p->Codec.In.Format.Format.Audio.Channels * sizeof(int16_t);
	p->Codec.Packet.Data[0] = p->Output;
	--p->Frames;

	if (p->AfterFlush)
	{
		// don't output this frame
		--p->AfterFlush;
		return Process(p,NULL,State);
	}

	return ERR_NONE;
}

static int Flush(speex* p)
{
	p->AfterFlush = 4;
	p->Frames = 0;
	return ERR_NONE;
}

static int Create(speex* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef Speex =
{
	sizeof(speex),
	SPEEX_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

static const nodedef SPX_HQ = 
{
	0, // parent size
	SPX_HQ_ID,
	FOURCC('O','G','H','V'),
	PRI_DEFAULT,
	NULL,
};

static const nodedef SPX_LQ = 
{
	0, // parent size
	SPX_LQ_ID,
	FOURCC('O','G','G','V'),
	PRI_DEFAULT,
	NULL,
};

void Speex_Init()
{
	NodeRegisterClass(&Speex);
	NodeRegisterClass(&SPX_LQ);
	NodeRegisterClass(&SPX_HQ);
}

void Speex_Done()
{
	NodeUnRegisterClass(SPEEX_ID);
	NodeUnRegisterClass(SPX_LQ_ID);
	NodeUnRegisterClass(SPX_HQ_ID);
}
