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
 * $Id: amrwb.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "amrwb.h"

#include "26204/dec_if.h"

extern const UWord8 block_size[];

typedef struct amrwb
{
	codec Codec;
	buffer Buffer;
	void* Decoder;
	int16_t Synth[L_FRAME16k];

} amrwb;

static int UpdateInput(amrwb* p)
{
	if (p->Decoder)
	{
		D_IF_exit(p->Decoder);
		p->Decoder = NULL;
	}
	BufferClear(&p->Buffer);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
	    p->Decoder = D_IF_init();
		if (!p->Decoder)
			return ERR_OUT_OF_MEMORY;

		p->Codec.In.Format.Format.Audio.SampleRate = 16000;
		p->Codec.In.Format.Format.Audio.Channels = 1;

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,16);
	}

	return ERR_NONE;
}

static int Process(amrwb* p, const packet* Packet, const flowstate* State)
{
	int Size;
	if (Packet)
	{
		if (Packet->RefTime >= 0)
			p->Codec.Packet.RefTime = Packet->RefTime;

		// add new packet to buffer
		BufferPack(&p->Buffer,0);
		BufferWrite(&p->Buffer,Packet->Data[0],Packet->Length,256);
	}
	else
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	if (p->Buffer.WritePos - p->Buffer.ReadPos < 1)
		return ERR_NEED_MORE_DATA;

	if (p->Buffer.Data[p->Buffer.ReadPos] == '#' &&
		p->Buffer.WritePos - p->Buffer.ReadPos > 9 &&
		memcmp(p->Buffer.Data+p->Buffer.ReadPos,"#!AMR-WB\n",9)==0)
		p->Buffer.ReadPos += 9;

	Size = block_size[(p->Buffer.Data[p->Buffer.ReadPos] >> 3) & 0xF];

	if (p->Buffer.WritePos - p->Buffer.ReadPos < Size)
		return ERR_NEED_MORE_DATA;

    D_IF_decode(p->Decoder, p->Buffer.Data+p->Buffer.ReadPos, p->Synth, _good_frame);
	p->Buffer.ReadPos += Size;
	p->Codec.Packet.Length = sizeof(p->Synth);
	p->Codec.Packet.Data[0] = p->Synth;
	return ERR_NONE;
}

extern void D_IF_reset(void*);
	
static int Flush(amrwb* p)
{
	if (p->Decoder)
		D_IF_reset(p->Decoder);
	return ERR_NONE;
}

static int Create(amrwb* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef AMRWB =
{
	sizeof(amrwb),
	AMRWB_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

static const nodedef AMRWBFile = 
{
	0, //parent size
	AMRWB_FILE_ID,
	RAWAUDIO_CLASS,
	PRI_DEFAULT-5,
};

void AMRWB_Init()
{
	NodeRegisterClass(&AMRWB);
	NodeRegisterClass(&AMRWBFile);
}

void AMRWB_Done()
{
	NodeUnRegisterClass(AMRWB_ID);
	NodeUnRegisterClass(AMRWB_FILE_ID);
}
