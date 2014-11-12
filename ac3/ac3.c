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
 * $Id: ac3.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "ac3.h"
#include "liba52/a52.h"

typedef struct a52
{
	codec Codec;
	buffer Buffer;
	a52_state_t* State;
	sample_t* Samples;
	int FrameSize;
	int SubFrame;

} a52;

static int UpdateInput(a52* p)
{
	if (p->State)
	{
	    a52_free(p->State);
		p->State = NULL;
	}
	BufferClear(&p->Buffer);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,30);
		p->Codec.Out.Format.Format.Audio.Flags = PCM_PLANES;
		if (p->Codec.Out.Format.Format.Audio.Channels>0)
			p->Codec.Out.Format.Format.Audio.Channels = 2;

		p->SubFrame = 6;
		p->FrameSize = 0;
		p->State = a52_init(0);
		p->Samples = a52_samples(p->State);
	}

	return ERR_NONE;
}

static int Process(a52* p, const packet* Packet, const flowstate* State)
{
	if (Packet)
	{
		if (Packet->RefTime >= 0)
			p->Codec.Packet.RefTime = Packet->RefTime;

		// add new packet to buffer
		BufferPack(&p->Buffer,0);
		BufferWrite(&p->Buffer,Packet->Data[0],Packet->Length,2048);
	}
	else
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	for (;;)
	{
		if (p->SubFrame >= 6)
		{
			static const int Channels[16] = { 2, 1, 2, 3, 3, 4, 4, 5, 1, 1, 2 };
			int Flags;
			int SampleRate;
			int BitRate;
			level_t Level;

			// find valid frame header
			while (!p->FrameSize)
			{
				if (p->Buffer.WritePos-p->Buffer.ReadPos < 8)
					return ERR_NEED_MORE_DATA;

                p->FrameSize = a52_syncinfo(p->Buffer.Data+p->Buffer.ReadPos,&Flags,&SampleRate,&BitRate);
				
				if (p->FrameSize)
				{
					p->Codec.In.Format.ByteRate = BitRate/8;
					p->Codec.In.Format.Format.Audio.SampleRate = SampleRate;
                    p->Codec.In.Format.Format.Audio.Channels = Channels[Flags & A52_CHANNEL_MASK];
                    if (Flags & A52_LFE)
						p->Codec.In.Format.Format.Audio.Channels++;

					if (p->Codec.Out.Format.Format.Audio.SampleRate != SampleRate ||
						p->Codec.Out.Format.Format.Audio.Channels != 2)
					{
						p->Codec.Out.Format.Format.Audio.Channels = 2;
						p->Codec.Out.Format.Format.Audio.SampleRate = SampleRate;
						ConnectionUpdate(&p->Codec.Node,CODEC_OUTPUT,p->Codec.Out.Pin.Node,p->Codec.Out.Pin.No);
					}
					break;
				}

				p->Buffer.ReadPos++;
			}

			if (p->Buffer.WritePos-p->Buffer.ReadPos < p->FrameSize)
				return ERR_NEED_MORE_DATA;

            Flags = A52_STEREO;
            Level = 1<<26;
            if (!a52_frame(p->State, p->Buffer.Data+p->Buffer.ReadPos, &Flags, &Level, 384)) 
			{
				// valid frame
				p->SubFrame = 0;
				p->Buffer.ReadPos += p->FrameSize;
			}
			p->FrameSize = 0;
		}
		else
		{
			if (!a52_block(p->State))
				break;
			p->SubFrame = 6; // skip to next frame
		}
	}

	p->Codec.Packet.Length = 256 * sizeof(sample_t);
	p->Codec.Packet.Data[0] = p->Samples;
	p->Codec.Packet.Data[1] = p->Samples+256;
	p->SubFrame++;
	return ERR_NONE;
}

static int Flush(a52* p)
{
	p->FrameSize = 0;
	p->SubFrame = 6;
	return ERR_NONE;
}

static int Create(a52* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef A52 =
{
	sizeof(a52),
	A52_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

static const nodedef AC3 = 
{
	0, //parent size
	AC3_ID,
	RAWAUDIO_CLASS,
	PRI_DEFAULT-5,
};

void AC3_Init()
{
	NodeRegisterClass(&A52);
	NodeRegisterClass(&AC3);
}

void AC3_Done()
{
	NodeUnRegisterClass(AC3_ID);
	NodeUnRegisterClass(A52_ID);
}
