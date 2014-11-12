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
 * $Id: faad.c 598 2006-01-18 21:27:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "faad.h"
#include "faad2/include/neaacdec.h"

#if SAFETAIL < 12
#error Minimum 12 bytes tail needed in buffer
#endif

#define STACK_SIZE			0x20000
#define MAX_CHANNELS		6 
#define BUFFER_NEEDED		(FAAD_MIN_STREAMSIZE * MAX_CHANNELS)
#define HEADER_NEEDED		16384

typedef struct faad
{
	codec Codec;
	buffer Buffer;
	NeAACDecHandle Decoder;
	bool_t ErrorMessage;
	bool_t Inited;
	bool_t PacketBased;
	tick_t NextRefTime;
	int FormatSet;
	char* Stack;

} faad;

static int UpdateInput( faad* p )
{
	if (p->Decoder)
	{
	    NeAACDecClose(p->Decoder);
		p->Decoder = NULL;
	}

#ifdef SWAPSP2
	if (p->Stack)
	{
		int i;
		for (i=0;i<STACK_SIZE;++i)
			if (p->Stack[i] != 0x11)
			{
				DebugMessage("Stack usage: %d",STACK_SIZE-i);
				ThreadSleep(300);
				break;
			}
	}

	free(p->Stack);
	p->Stack = NULL;
#endif

	p->Inited = 0;
	p->ErrorMessage = 0;
	BufferClear(&p->Buffer);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
	    NeAACDecConfigurationPtr Config;

		p->Decoder = NeAACDecOpen();
		Config = NeAACDecGetCurrentConfiguration(p->Decoder);
		Config->defObjectType = LC;
		Config->outputFormat = FAAD_FMT_INTERNAL;
		Config->downMatrix = 1; // force downmix to stereo
		NeAACDecSetConfiguration(p->Decoder, Config);

		if (p->Codec.In.Format.ExtraLength)
		{
			unsigned long SampleRate;
			uint8_t Channels;
	
		    if (NeAACDecInit2(p->Decoder, p->Codec.In.Format.Extra, p->Codec.In.Format.ExtraLength, 
				&SampleRate, &Channels) < 0)
			{
				ShowError(p->Codec.Node.Class,AACFULL_ID,FAAD_NOT_SUPPORTED);
				NeAACDecClose(p->Decoder);
				p->Decoder = NULL;
				return ERR_NOT_SUPPORTED;
			}

			p->Codec.In.Format.Format.Audio.SampleRate = SampleRate;
			p->Codec.In.Format.Format.Audio.Channels = Channels;
			p->Inited = 1;
		}

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,30);
		p->Codec.Out.Format.Format.Audio.Flags = PCM_PLANES;
		p->PacketBased = (p->Codec.In.Format.Format.Audio.Flags & PCM_PACKET_BASED) != 0;
		p->FormatSet = 0;

#ifdef SWAPSP2
		p->Stack = malloc(STACK_SIZE);
		if (!p->Stack)
			return ERR_OUT_OF_MEMORY;
		memset(p->Stack,0x11,STACK_SIZE);
#endif
	}
 
	return ERR_NONE;
}

static NOINLINE int Process2( faad* p, const packet* Packet, const flowstate* State )
{
	bool_t Eof = !Packet && State;

	if (Packet)
	{
		// set new reftime
		if (Packet->RefTime >= 0)
		{
			if (p->PacketBased)
			{
				p->Codec.Packet.RefTime = p->NextRefTime;
				p->NextRefTime = Packet->RefTime;
			}
			else
			{
				p->Codec.Packet.RefTime = Packet->RefTime;
				if (p->Codec.In.Format.ByteRate>0)
				{
					p->Codec.Packet.RefTime -= Scale(p->Buffer.WritePos - p->Buffer.ReadPos,TICKSPERSEC,p->Codec.In.Format.ByteRate);
					if (p->Codec.Packet.RefTime<0)
						p->Codec.Packet.RefTime=0;
				}
			}
		}

		// add new packet to buffer
		BufferPack(&p->Buffer,0);
		BufferWrite(&p->Buffer,Packet->Data[0],Packet->Length,2048);

		if (!p->Inited)
		{
			unsigned long SampleRate;
			uint8_t Channels;
			int Readed;
			uint8_t* Buffer = p->Buffer.Data + p->Buffer.ReadPos;
			int Length = p->Buffer.WritePos - p->Buffer.ReadPos;

			if (!Eof && !p->PacketBased && Length < HEADER_NEEDED)
				return ERR_NEED_MORE_DATA;

			if (memcmp(Buffer, "ADIF", 4) == 0) 
			{
				int BitRate;

				if (Buffer[4] & 0x80) // copyright_id present?
					Buffer += 9;

				BitRate = (((int)Buffer[4] & 0x0F) << 19) |
						  ((int)Buffer[5] << 11) |
						  ((int)Buffer[6] << 3) |
						  ((int)Buffer[7] & 0xE0);

				p->Codec.In.Format.ByteRate = BitRate/8;
			}
			else
			{
				int Trash = 0;
				for (Trash=0;Trash<Length-16;++Trash)
					if (Buffer[Trash] == 0xFF && (Buffer[Trash+1] & 0xF6)==0xF0)
						break;

				if (Trash<Length-16)
				{
					static const int SampleRates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
					int Samples = 0;
					int Bytes = 0;

					Buffer += Trash;
					Length -= Trash;

					SampleRate = SampleRates[(Buffer[2] & 0x3C) >> 2];
					// check for SBR profile, double samples ???

					while (Length > 7)
					{
						int n;

						if (Buffer[0] != 0xFF || (Buffer[1] & 0xF6)!=0xF0)
							break;

						n = (((int)Buffer[3] & 3) << 11) | (((int)Buffer[4]) << 3) | (Buffer[5] >> 5);

						Bytes += n;
						Samples += 1024; 

						Length -= n;
						Buffer += n;
					}

					p->Codec.In.Format.ByteRate = Scale(Bytes,SampleRate,Samples);

					if (Length < 256 || Samples >= 32*1024) // successfull
						p->Buffer.ReadPos += Trash;
 				}
			}

			Readed = NeAACDecInit(p->Decoder, p->Buffer.Data + p->Buffer.ReadPos,
				p->Buffer.WritePos - p->Buffer.ReadPos, &SampleRate, &Channels);
			
			if (Readed < 0)
			{
				if (!p->ErrorMessage)
				{
					ShowError(p->Codec.Node.Class,AACFULL_ID,FAAD_NOT_SUPPORTED);
					p->ErrorMessage = 1;
				}
				BufferDrop(&p->Buffer);
				return ERR_NOT_SUPPORTED;
			}

			p->Inited = 1;
			p->Buffer.ReadPos += Readed;
		}

	}
	else
	if (p->Inited)
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	while ((p->PacketBased && Packet) || Eof || p->Buffer.WritePos >= p->Buffer.ReadPos+BUFFER_NEEDED)
	{
	    NeAACDecFrameInfo FrameInfo;
	    void* Buffer[16];
		int Size = p->Buffer.WritePos-p->Buffer.ReadPos;
		if (Size <= 0)
			break;

        NeAACDecDecode2(p->Decoder, &FrameInfo,p->Buffer.Data+p->Buffer.ReadPos,Size,Buffer,MAX_INT);

		if (p->PacketBased)
			p->Buffer.ReadPos = p->Buffer.WritePos;
		else
			p->Buffer.ReadPos += FrameInfo.bytesconsumed;

        if (!FrameInfo.error && FrameInfo.samples>0)
		{
			if (p->Codec.Out.Format.Format.Audio.SampleRate != (int)FrameInfo.samplerate ||
				p->Codec.Out.Format.Format.Audio.Channels != (int)FrameInfo.channels)
			{
				if (p->FormatSet && (p->Codec.Out.Format.Format.Audio.Channels!=1 || FrameInfo.channels!=2)) // allow PS mono->stereo
				{
					p->FormatSet--;
					break; // probably a bad frame, drop it
				}
				p->Codec.In.Format.Format.Audio.SampleRate = 
				p->Codec.Out.Format.Format.Audio.SampleRate = FrameInfo.samplerate;
				p->Codec.In.Format.Format.Audio.Channels = 
				p->Codec.Out.Format.Format.Audio.Channels = FrameInfo.channels;
				ConnectionUpdate(&p->Codec.Node,CODEC_OUTPUT,p->Codec.Out.Pin.Node,p->Codec.Out.Pin.No);
			}
			p->FormatSet = 16; // reset countdown

			p->Codec.Packet.Length = FrameInfo.samples * sizeof(int32_t);
			p->Codec.Packet.Data[0] = Buffer[0];
			p->Codec.Packet.Data[1] = Buffer[1];
			return ERR_NONE;
		}

		if (FrameInfo.error==28)
		{
			BufferDrop(&p->Buffer);
			return ERR_OUT_OF_MEMORY;
		}

		if (p->PacketBased)
			break;

		if (!FrameInfo.bytesconsumed)
			p->Buffer.ReadPos++;
	}

	return ERR_NEED_MORE_DATA;
}

static int Process( faad* p, const packet* Packet, const flowstate* State )
{
	int Result;
#ifdef SWAPSP2
	void* Old = SwapSP(p->Stack+STACK_SIZE);
#endif
	Result = Process2(p,Packet,State);
#ifdef SWAPSP2
	SwapSP(Old);
#endif
	return Result;
}

extern void *faad_malloc(size_t size);

uint8_t safestack(uint8_t (*decode)(NeAACDecHandle hDecoder, void *ics),NeAACDecHandle hDecoder, void *ics, void** stack)
{
	uint8_t ret;
#ifdef SWAPSP
	void* old;
	if (!*stack)
	{
		*stack = faad_malloc(STACK_SIZE);
		if (!*stack)
			return 1;
	}
	old = SwapSP(*stack+STACK_SIZE);
#endif
	ret = decode(hDecoder,ics);
#ifdef SWAPSP
	SwapSP(old);
#endif
	return ret;
}

static int Flush(faad* p)
{
	p->NextRefTime = TIME_UNKNOWN;
	BufferDrop(&p->Buffer);
	if (p->Decoder)
		NeAACDecPostSeekReset(p->Decoder,0);
	return ERR_NONE;
}

static int Create(faad* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef FAAD = 
{
	sizeof(faad),
#ifdef AACFULL_EXPORTS
	AACFULL_ID,
	CODEC_CLASS,
	PRI_DEFAULT+256,
#else
	FAAD_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
#endif
	(nodecreate)Create,
};

static const nodedef AAC = 
{
	0, //parent size
	AAC_ID,
	RAWAUDIO_CLASS,
#ifdef AACFULL_EXPORTS
	PRI_DEFAULT-4,
#else
	PRI_DEFAULT-5,
#endif
};

void FAAD_Init()
{
	NodeRegisterClass(&FAAD);
	NodeRegisterClass(&AAC);
}

void FAAD_Done()
{
	NodeUnRegisterClass(FAAD.Class);
	NodeUnRegisterClass(AAC_ID);
}

#ifdef __SYMBIAN32__
uint32_t random_int2(void)
{
    return rand()|(rand()<<16);
}
#endif
