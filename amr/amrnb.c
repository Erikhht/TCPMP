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
 * $Id: amrnb.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "amrnb.h"

#include "26104/interf_dec.h"
#include "26104/sp_dec.h"
#include "26104/typedef.h"

typedef struct amrnb
{
	codec Codec;
	buffer Buffer;
	void* Decoder;
	int16_t Synth[160];

} amrnb;

static int UpdateInput(amrnb* p)
{
	if (p->Decoder)
	{
		Decoder_Interface_exit(p->Decoder);
		p->Decoder = NULL;
	}
	BufferClear(&p->Buffer);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
	    p->Decoder = Decoder_Interface_init();
		if (!p->Decoder)
			return ERR_OUT_OF_MEMORY;

		p->Codec.In.Format.Format.Audio.SampleRate = 8000;
		p->Codec.In.Format.Format.Audio.Channels = 1;

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,16);
	}

	return ERR_NONE;
}

static int Process(amrnb* p, const packet* Packet, const flowstate* State)
{
	static const uint8_t PackedSize[16] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};
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
		p->Buffer.WritePos - p->Buffer.ReadPos > 6 &&
		memcmp(p->Buffer.Data+p->Buffer.ReadPos,"#!AMR\n",6)==0)
		p->Buffer.ReadPos += 6;
	
	Size = PackedSize[(p->Buffer.Data[p->Buffer.ReadPos] >> 3) & 0xF]+1;

	if (p->Buffer.WritePos - p->Buffer.ReadPos < Size)
		return ERR_NEED_MORE_DATA;

    Decoder_Interface_Decode(p->Decoder, p->Buffer.Data+p->Buffer.ReadPos, p->Synth, 0);
	p->Buffer.ReadPos += Size;
	p->Codec.Packet.Length = sizeof(p->Synth);
	p->Codec.Packet.Data[0] = p->Synth;
	return ERR_NONE;
}

extern void Decoder_Interface_reset(void*);
	
static int Flush(amrnb* p)
{
	if (p->Decoder)
		Decoder_Interface_reset(p->Decoder);
	return ERR_NONE;
}

static int Create(amrnb* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef AMRNB =
{
	sizeof(amrnb),
	AMRNB_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

static const nodedef AMRNBFile = 
{
	0, //parent size
	AMRNB_FILE_ID,
	RAWAUDIO_CLASS,
	PRI_DEFAULT-5,
};

void AMRNB_Init()
{
	NodeRegisterClass(&AMRNB);
	NodeRegisterClass(&AMRNBFile);
}

void AMRNB_Done()
{
	NodeUnRegisterClass(AMRNB_ID);
	NodeUnRegisterClass(AMRNB_FILE_ID);
}
