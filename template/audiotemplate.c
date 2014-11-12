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
 * $Id: audiotemplate.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "audiotemplate.h"
#include <math.h>

#define BUFFERSIZE		4096

typedef struct audio_template
{
	codec Codec;

	int16_t* Buffer;
	int16_t Sin[1024];

	tick_t LastTime;
	int PhaseLeft;
	int PhaseRight;

} audio_template;

static int Process( audio_template* p, const packet* Packet, const flowstate* State )
{
	int i;
	int Size;

	// you can return ERR_NEED_MORE_DATA if data is not enough for decode output packet
	// but this means the decoder has to buffer the input
	
	// if (Packet)
	//   AddBuffer(Packet)
	// if (Buffer.Length < 1024)
	//   return ERR_NEED_MORE_DATA;

	if (!Packet)
		return ERR_NEED_MORE_DATA;

	if (p->LastTime<0 && Packet->RefTime>=0)
		p->LastTime = Packet->RefTime;

	// this is fake audio codec, so we try to generate 
	// as much data to be in sync with RefTime

	Size = Scale(Packet->RefTime - p->LastTime,44100,TICKSPERSEC);
	if (Size <= 0)
		return ERR_NONE;

	if (Size > BUFFERSIZE)
		Size = BUFFERSIZE;

	p->Codec.Packet.RefTime = Packet->RefTime; 
	p->Codec.Packet.Data[0] = p->Buffer;
	p->Codec.Packet.Length = Size*2*sizeof(int16_t);
	if (p->LastTime >= 0)
		p->LastTime += Scale(Size,TICKSPERSEC,44100);

	for (i=0;i<Size;++i)
	{
		p->Buffer[2*i] = p->Sin[p->PhaseLeft & 1023];
		p->Buffer[2*i+1] = p->Sin[p->PhaseRight & 1023];

		p->PhaseLeft += 4;
		p->PhaseRight += 7;
	}

	return ERR_NONE;
}

// empty buffers and do what ever needed after seek
static int Flush( audio_template* p )
{
	p->LastTime = -1;
	p->PhaseLeft = 0;
	p->PhaseRight = 0;
	return ERR_NONE;
}

static int UpdateInput( audio_template* p )
{
	free(p->Buffer);
	p->Buffer = NULL;

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		PacketFormatClear(&p->Codec.Out.Format);
		p->Codec.Out.Format.Type = PACKET_AUDIO;
		p->Codec.Out.Format.Format.Audio.Format = AUDIOFMT_PCM;
		p->Codec.Out.Format.Format.Audio.Bits = 16;
		p->Codec.Out.Format.Format.Audio.FracBits = 15;
		p->Codec.Out.Format.Format.Audio.Channels = 2;
		p->Codec.Out.Format.Format.Audio.SampleRate = 44100;

		p->Buffer = (int16_t*) malloc(BUFFERSIZE*2*sizeof(int16_t));
		if (!p->Buffer)
			return ERR_OUT_OF_MEMORY;
	}

	return ERR_NONE;
}

static int Create( audio_template* p )
{
	int i;
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;

	for (i=0;i<1024;++i)
		p->Sin[i] = (int16_t)(4096*sin(2*M_PI*i/1024));
	return ERR_NONE;
}

static const nodedef Template =
{
	sizeof(audio_template),
	AUDIO_TEMPLATE_ID,
	CODEC_CLASS,
	PRI_DEFAULT+10, // use higher priorty as defualt MP3 codec so this example can override it
	(nodecreate)Create,
	NULL,
};

void Audio_Init()
{
	StringAdd(1,AUDIO_TEMPLATE_ID,NODE_NAME,T("Audio template codec"));
	StringAdd(1,AUDIO_TEMPLATE_ID,NODE_CONTENTTYPE,T("acodec/0x0055"));
	NodeRegisterClass(&Template);
}

void Audio_Done()
{
	NodeUnRegisterClass(AUDIO_TEMPLATE_ID);
}

