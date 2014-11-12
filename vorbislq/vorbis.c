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
 * $Id: vorbis.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "vorbis.h"
#include "tremor/ogg.h"
#include "tremor/ivorbiscodec.h"

// Vorbis audio codec

typedef struct vorbis
{
	codec Codec;
	vorbis_dsp_state DSP;
	vorbis_block Block;
	bool_t Inited;
	ogg_int32_t **PCM;
	int Samples;

} vorbis;

#define OGG_F_BITS	24
#define OGG_F_ONE	(1<<24)
#define ogg_f_mul(a,b) (((a)>>12)*((b)>>12))

#undef malloc
void* _calloc(int n,int m)
{
	void* p = malloc(n*m);
	if (p) memset(p,0,n*m);
	return p;
}

static int Flush(vorbis* p)
{
	vorbis_synthesis_restart(&p->DSP);
	p->Samples = 0;
	return ERR_NONE;
}

static int UpdateInput(vorbis* p)
{
	vorbis_block_clear(&p->Block);
	vorbis_dsp_clear(&p->DSP);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		vorbis_info* Info = (vorbis_info*)p->Codec.In.Format.Extra;
		if (Info)
		{
			vorbis_synthesis_init(&p->DSP,Info);
			vorbis_block_init(&p->DSP,&p->Block);

			p->Codec.In.Format.ByteRate = Info->bitrate_nominal >> 3;
			p->Codec.In.Format.Format.Audio.SampleRate = Info->rate;
			p->Codec.In.Format.Format.Audio.Channels = Info->channels;
		}

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,OGG_F_BITS+1);
		p->Codec.Out.Format.Format.Audio.Flags = PCM_PLANES;
	}

	return ERR_NONE;
}

static int Process(vorbis* p, const packet* Packet, const flowstate* State)
{
	if (Packet)
	{
		if (Packet->Length != sizeof(ogg_packet))
			return ERR_INVALID_DATA;

		while (vorbis_synthesis(&p->Block,(ogg_packet*)Packet->Data[0],1)==0)
		{
			vorbis_synthesis_blockin(&p->DSP,&p->Block);
			p->Samples = vorbis_synthesis_pcmout(&p->DSP,&p->PCM);

			if (p->Samples)
			{
				p->Codec.Packet.RefTime = Packet->RefTime;
				p->Codec.Packet.Length = p->Samples * sizeof(ogg_int32_t);
				p->Codec.Packet.Data[0] = p->PCM[0];
				p->Codec.Packet.Data[1] = p->PCM[1];
				return ERR_NONE;
			}
		}
	}
	else
	if (p->Samples)
	{
		vorbis_synthesis_read(&p->DSP,p->Samples);
		p->Samples = 0;
	}

	return ERR_NEED_MORE_DATA;
}

static int Create(vorbis* p)
{
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Process = (packetprocess)Process;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef VorbisV = 
{
	sizeof(vorbis),
	VORBIS_V_ID,
	CODEC_CLASS,
	VORBIS_V_PRI,
	(nodecreate)Create,
};

static const nodedef VorbisA = 
{
	sizeof(vorbis),
	VORBIS_A_ID,
	CODEC_CLASS,
	VORBIS_A_PRI,
	(nodecreate)Create,
};

void Vorbis_Init()
{
	NodeRegisterClass(&VorbisV);
	NodeRegisterClass(&VorbisA);
}

void Vorbis_Done()
{
	NodeUnRegisterClass(VORBIS_A_ID);
	NodeUnRegisterClass(VORBIS_V_ID);
}
