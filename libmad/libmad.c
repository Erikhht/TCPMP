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
 * $Id: libmad.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "libmad.h"
#include "libmad/frame.h"
#include "libmad/synth.h"

typedef struct libmad
{
	codec Codec;

	buffer Buffer;
	struct mad_stream Stream;
	struct mad_frame Frame;
	struct mad_synth Synth;

	int BufferAlign;
	int AdjustBytes; // adjust (rewind) RefTime
	bool_t Skip; // skip next frame
	int AdjustTime;
	int FormatSet;

} libmad;

static INLINE void UpdateAdjustTime( libmad* p )
{
	if (p->Codec.In.Format.ByteRate)
		p->AdjustTime = (TICKSPERSEC * 4096) / p->Codec.In.Format.ByteRate;
	else
		p->AdjustTime = 0;
}

static int UpdateInput( libmad* p )
{
	BufferClear(&p->Buffer);
	mad_frame_finish(&p->Frame);
	mad_stream_finish(&p->Stream);
	mad_synth_finish(&p->Synth);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,MAD_F_FRACBITS+1);
		p->Codec.Out.Format.Format.Audio.Flags = PCM_PLANES;

		p->FormatSet = 0;
		p->AdjustBytes = 0;

		UpdateAdjustTime(p);

		mad_stream_init(&p->Stream);
		mad_frame_init(&p->Frame);
		mad_synth_init(&p->Synth);
	}

	return ERR_NONE;
}

static int Process( libmad* p, const packet* Packet, const flowstate* State )
{
	if (Packet)
	{
		// remove processed bytes from buffer
		BufferPack(&p->Buffer,p->Stream.next_frame - p->Stream.buffer);

		// set new reftime
		if (Packet->RefTime >= 0)
		{
			p->Codec.Packet.RefTime = Packet->RefTime;
			p->AdjustBytes = p->Buffer.WritePos - p->Buffer.ReadPos;
		}

		// add new packet to buffer
		BufferWrite(&p->Buffer,Packet->Data[0],Packet->Length,p->BufferAlign);
		mad_stream_buffer(&p->Stream, p->Buffer.Data+p->Buffer.ReadPos, p->Buffer.WritePos-p->Buffer.ReadPos);
	}
	else
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	for (;;)
	{
		while (mad_frame_decode(&p->Frame, &p->Stream) == -1)
		{
			if (p->Stream.error == MAD_ERROR_BUFLEN || p->Stream.error == MAD_ERROR_BUFPTR)
				return ERR_NEED_MORE_DATA;

			if (p->Stream.error == MAD_ERROR_NOMEM)
			{
				BufferDrop(&p->Buffer);
				return ERR_OUT_OF_MEMORY;
			}
		}

		if (p->Skip)
		{
			// the first frame, it may be corrupt
			p->Skip--;
			continue;
		}

		mad_synth_frame(&p->Synth,&p->Frame);

		// handle stereo streams with random mono frames (is this really a good mp3?)
		if (p->Codec.Out.Format.Format.Audio.Channels==2 && p->Synth.pcm.channels==1)
		{
			p->Synth.pcm.channels = 2;
			memcpy(p->Synth.pcm.samples[1],p->Synth.pcm.samples[0],
				p->Synth.pcm.length * sizeof(mad_fixed_t));
		}

		if (p->Codec.In.Format.ByteRate != (int)(p->Frame.header.bitrate >> 3))
		{
			p->Codec.In.Format.ByteRate = (p->Frame.header.bitrate >> 3);
			UpdateAdjustTime(p);
		}

		// adjust RefTime with AdjustBytes (now that we know the bitrate)
		if (p->Codec.Packet.RefTime >= 0)
			p->Codec.Packet.RefTime -= (p->AdjustBytes * p->AdjustTime) >> 12;

		// output format setup needed?
		if (p->Codec.Out.Format.Format.Audio.SampleRate != (int)p->Synth.pcm.samplerate ||
			p->Codec.Out.Format.Format.Audio.Channels != p->Synth.pcm.channels)
		{
			if (p->FormatSet)
			{
				p->FormatSet--;
				continue; // probably a bad frame, drop it
			}

			// set new output format
			p->Codec.In.Format.Format.Audio.SampleRate = p->Codec.Out.Format.Format.Audio.SampleRate = p->Synth.pcm.samplerate;
			p->Codec.In.Format.Format.Audio.Channels = p->Codec.Out.Format.Format.Audio.Channels = p->Synth.pcm.channels;
			ConnectionUpdate(&p->Codec.Node,CODEC_OUTPUT,p->Codec.Out.Pin.Node,p->Codec.Out.Pin.No);
		}
		p->FormatSet = 16; // reset countdown
		break;
	}

	// build output packet
	p->Codec.Packet.Length = p->Synth.pcm.length * sizeof(mad_fixed_t);
	p->Codec.Packet.Data[0] = p->Synth.pcm.samples[0];
	p->Codec.Packet.Data[1] = p->Synth.pcm.samples[1];
	return ERR_NONE;
}

static int Flush( libmad* p )
{
	BufferDrop(&p->Buffer);
	p->Skip = 1;

	mad_frame_finish(&p->Frame);
	mad_synth_finish(&p->Synth);
	mad_stream_finish(&p->Stream);

	mad_frame_init(&p->Frame);
	mad_synth_init(&p->Synth);
	mad_stream_init(&p->Stream);
	return ERR_NONE;
}

#ifdef BUILDFIXED

#include <math.h>

extern struct fixedfloat {
  unsigned long mantissa : 27;
  unsigned long exponent :  5;
} rq_table[8207];

#endif

static int Create( libmad* p )
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	p->BufferAlign = Context()->LowMemory?4096:16384;
	return ERR_NONE;
}

#define XING_FRAMES     0x01
#define XING_BYTES		0x02
#define XING_TOC		0x04
#define XING_SCALE		0x08

typedef struct mp3
{
	rawaudio RawAudio;

	int Frames;
	int Bytes;

	bool_t TOC;
	uint8_t TOCTable[100];

	int VBRITableSize;
	int VBRIEntryFrames;
	int* VBRITable;

} mp3;

static int Read(format_reader* Reader,int n)
{
	int v=0;
	while (n--)
	{
		v = v << 8;
		v += Reader->Read8(Reader);
	}
	return v;
}

static int InitMP3(mp3* p)
{
	static const int RateTable[4] = { 44100, 48000, 32000, 99999 };
	format_reader* Reader;
	int i,SampleRate,Id,Mode,Layer,SamplePerFrame;
	int Result = RawAudioInit(&p->RawAudio);

	p->TOC = 0;
	p->Bytes = p->RawAudio.Format.FileSize - p->RawAudio.Head;
	p->Frames = 0;
	p->RawAudio.VBR = 0;

	Reader = p->RawAudio.Format.Reader;

	for (i=0;i<2048;++i)
		if (Reader->Read8(Reader) == 0xFF)
		{
			filepos_t Frame;

			i = Reader->Read8(Reader);
			if ((i & 0xE0) != 0xE0)
				continue;

			Id = (i >> 3) & 3;
			Layer = (i >> 1) & 3;
			SampleRate = RateTable[(Reader->Read8(Reader) >> 2) & 3];
			if (Id==2)
				SampleRate >>= 1; // MPEG2
			if (Id==0)
				SampleRate >>= 2; // MPEG2.5
			Mode = (Reader->Read8(Reader) >> 6) & 3;

			SamplePerFrame = (Layer == 3)?384:1152;

			Frame = Reader->FilePos;

			//Xing offset
			if (Id==3)
			{
				// MPEG1
				Reader->Skip(Reader,Mode==3 ? 17:32);
			}
			else
			{
				// MPEG2/2.5
				Reader->Skip(Reader,Mode==3 ? 9:17);
				if (Layer == 1) // layer-3
					SamplePerFrame = 576;
			}

			if (Reader->ReadLE32(Reader) == FOURCCLE('X','i','n','g'))
			{
				int Flags = Reader->ReadBE32(Reader);
				if (Flags & XING_FRAMES) 
					p->Frames = Reader->ReadBE32(Reader);

				if (Flags & XING_BYTES)
					p->Bytes = Reader->ReadBE32(Reader);

				if (Flags & XING_TOC)
				{
					p->TOC = 1;
					Reader->Read(Reader,p->TOCTable,100);
				}
			}
			else
			{
				Reader->Seek(Reader,Frame+32,SEEK_SET);

				if (Reader->ReadLE32(Reader) == FOURCCLE('V','B','R','I'))
				{
					int Scale,EntryBytes;

					Reader->Skip(Reader,2+2+2); //Version,Delay,Quality
					p->Bytes= Reader->ReadBE32(Reader);
					p->Frames = Reader->ReadBE32(Reader);

					p->VBRITableSize = Reader->ReadBE16(Reader)+1;
					Scale = Reader->ReadBE16(Reader);
					EntryBytes = Reader->ReadBE16(Reader);
					p->VBRIEntryFrames = Reader->ReadBE16(Reader);

					p->VBRITable = malloc(sizeof(int)*p->VBRITableSize);
					if (p->VBRITable)
						for (i=0;i<p->VBRITableSize;++i)
							p->VBRITable[i] = Read(Reader,EntryBytes)*Scale;
				}
			}

			if (p->Frames>0)
			{
				p->RawAudio.VBR = 1;
				p->RawAudio.Format.Duration = Scale(p->Frames,TICKSPERSEC*SamplePerFrame,SampleRate);

				if (p->Bytes>0)
					p->RawAudio.Format.Streams[0]->Format.ByteRate = Scale(p->Bytes,TICKSPERSEC,p->RawAudio.Format.Duration);
			}

			break;
		}

	Reader->Seek(Reader,p->RawAudio.Head,SEEK_SET);
	return Result;
}

static void DoneMP3(mp3* p)
{
	if (p->VBRITable)
	{
		free(p->VBRITable);
		p->VBRITable = NULL;
	}
}

static int SeekMP3(mp3* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	if (FilePos < 0 && Time > 0 && p->RawAudio.Format.Duration > 0)
	{
		int i,a,b;

		if (Time > p->RawAudio.Format.Duration)
			Time = p->RawAudio.Format.Duration;

		if (p->VBRITable)
		{
			int i;
			tick_t Left;
			tick_t EntryTime = p->RawAudio.Format.Duration / p->VBRITableSize;

			FilePos = p->RawAudio.Head;

			Left = Time;
			for (i=0;Left>0 && i<p->VBRITableSize;++i)
			{
				FilePos += p->VBRITable[i];
				Left -= EntryTime;
			}

			if (i>0)
				FilePos += Scale(Left,p->VBRITable[i-1],EntryTime);
		}
		else
		if (p->TOC && p->Bytes>0)
		{
			i = Scale(Time,100,p->RawAudio.Format.Duration);

			if (i>99) i=99;
			a = p->TOCTable[i];
			if (i==99)
				b = 256;
			else
				b = p->TOCTable[i+1];

			a <<= 10;
			b <<= 10;
			a += Scale(Time - Scale(i,p->RawAudio.Format.Duration,100),b-a,p->RawAudio.Format.Duration);

			FilePos = p->RawAudio.Head + Scale(a,p->Bytes,256*1024);
		}
	}
	return RawAudioSeek(&p->RawAudio,Time,FilePos,PrevKey);
}

static int CreateMP3( mp3* p )
{
	p->RawAudio.Format.Init = (fmtfunc)InitMP3;
	p->RawAudio.Format.Done = (fmtvoid)DoneMP3;
	p->RawAudio.Format.Seek = (fmtseek)SeekMP3;
	return ERR_NONE;
}

static const nodedef MP3 = 
{
	sizeof(mp3),
	MP3_ID,
	RAWAUDIO_CLASS,
	PRI_DEFAULT-5,
	(nodecreate)CreateMP3,
};

static const nodedef LibMad = 
{
	sizeof(libmad),
	LIBMAD_ID,
	CODEC_CLASS,
	PRI_DEFAULT-5,
	(nodecreate)Create,
};

void LibMad_Init()
{
#ifdef BUILDFIXED
	int x;
	struct fixedfloat* p=rq_table;
	for (x=0;x<8207;++x,++p)
	{
		int exp;
		double v = frexp(pow(x,4./3.),&exp);
		if (exp) ++exp;
		p->exponent = (unsigned short)exp;
		p->mantissa = (int)(0x10000000 * (v*0.5));
	}
#endif

	NodeRegisterClass(&LibMad);
	NodeRegisterClass(&MP3);
}

void LibMad_Done()
{
	NodeUnRegisterClass(LIBMAD_ID);
	NodeUnRegisterClass(MP3_ID);
}
