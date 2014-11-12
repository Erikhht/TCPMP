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
 * $Id: mov.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../common/zlib/zlib.h"
#include "mov.h"
#include "movpal.h"

#define MP4_INVALID_AUDIO_TYPE          0x00
#define MP4_MPEG1_AUDIO_TYPE            0x6B
#define MP4_MPEG2_AUDIO_TYPE            0x69
#define MP4_MPEG2_AAC_MAIN_AUDIO_TYPE   0x66
#define MP4_MPEG2_AAC_LC_AUDIO_TYPE     0x67
#define MP4_MPEG2_AAC_SSR_AUDIO_TYPE    0x68
#define MP4_MPEG4_AUDIO_TYPE            0x40
#define MP4_PRIVATE_AUDIO_TYPE          0xE0

#define MP4_INVALID_VIDEO_TYPE          0x00
#define MP4_MPEG1_VIDEO_TYPE            0x6A
#define MP4_MPEG2_SIMPLE_VIDEO_TYPE     0x60
#define MP4_MPEG2_MAIN_VIDEO_TYPE       0x61
#define MP4_MPEG2_SNR_VIDEO_TYPE        0x62
#define MP4_MPEG2_SPATIAL_VIDEO_TYPE    0x63
#define MP4_MPEG2_HIGH_VIDEO_TYPE       0x64
#define MP4_MPEG2_442_VIDEO_TYPE        0x65
#define MP4_MPEG4_VIDEO_TYPE            0x20
#define MP4_JPEG_VIDEO_TYPE             0x6C

typedef struct mp4stsc
{
	int FirstChunk;
	int Samples;

} mp4stsc;

typedef struct mp4stts
{
	int Count;
	int Delta;

} mp4stts;

typedef struct mp4streampos
{
	int CurrentTime;
	int SampleNo;
	int SamplesLeft;

	int STSSDelta;
	int STSSValue;
	int STSZDelta;
	int STSZValue;
	int STTSLeft;

	const uint8_t* STSSPos;
	const uint8_t* STSSEnd;
	const mp4stts* STTSPos;
	const mp4stsc* STSCPos;
	const uint8_t* STSZPos;
	const uint8_t* STSZEnd;
	const int* STCOPos;

} mp4streampos;

typedef struct mp4stream
{
	format_stream Stream;

	tick_t Duration;
	int TimeScale;
	int SamplesPerPacket;
	int BytesPerPacket;
	bool_t AllKey;

	filepos_t OffsetMin;
	filepos_t OffsetMax;
	int StartTime;

	mp4streampos Pos;

	int STSZDef; //default sample size
	array STSS; //key samples		(compressed)
	array STSZ; //sample size		(compressed)
	array STTS; //sample duration	(mp4stts) + delimiter
	array STSC; //sample per chunk	(mp4stsc) + delimiter
	array STCO; //chunk offset		(int) + delimiter
	
} mp4stream;

typedef struct mp4
{
	format_base Format;

	uint32_t Type;
	uint32_t CompressType;
	boolmem_t ErrorShowed;
	boolmem_t MOOVFound;
	boolmem_t RMRAFound;
	int TimeScale;

	mp4stream* Track;
	tchar_t Meta[32];

} mp4;

typedef	void (*mp4atomfunc)(mp4* p, filepos_t EndOfAtom);

typedef	struct mp4atomtable
{
	uint32_t Atom;
	mp4atomfunc Func;

} mp4atomtable;

typedef struct mp4atom
{
	uint32_t Atom;
	filepos_t EndOfAtom;

} mp4atom;


static int ReadVersion(format_reader* Reader)
{
	int Ver = Reader->Read8(Reader);
	Reader->Skip(Reader,3); //flags;
	return Ver;
}

static bool_t ReadAtom(format_reader* Reader, mp4atom* Atom)
{
	filepos_t Size;
	Atom->Atom = 0;
	Atom->EndOfAtom = Reader->FilePos;
	Size = Reader->ReadBE32(Reader);
	Reader->Read(Reader,&Atom->Atom,sizeof(Atom->Atom));
	if (Size == 1)
		Size = (filepos_t)Reader->ReadBE64(Reader);
	Atom->EndOfAtom += Size;
	return Size >= 8;
}

static void ReadSubAtoms(mp4* p,filepos_t EndOfAtom);
static INLINE format_reader* Reader(mp4* p) { return p->Format.Reader; }

static void ReadFTYP(mp4* p, filepos_t EndOfAtom)
{
	p->Type = 0;
	Reader(p)->Read(Reader(p),&p->Type,sizeof(p->Type));
}

static void ReadRMRA(mp4* p, filepos_t EndOfAtom)
{
	p->RMRAFound = 1;
	ReadSubAtoms(p,EndOfAtom);
	p->RMRAFound = 0;
}

static void ReadMOOV(mp4* p, filepos_t EndOfAtom)
{
	p->MOOVFound = 1;
	ReadSubAtoms(p,EndOfAtom);
}

static INLINE const uint8_t* ReadValue(const uint8_t* p,const uint8_t* pe,int* Delta,int* Value,bool_t Alternate)
{
	if (p>=pe)
		*Value = 0;
	else
	{
		int i = *(int8_t*)(p++);
		while (!(i & 1))
			i = (i<<7) | *(p++);
		i >>= 1;
		if (Alternate) *Delta = -*Delta;
		*Delta += i;
		*Value += *Delta;
	}
	return p;
}

static void WriteValue(array* Array,int* Delta,int* Value,int i,bool_t Alternate)
{
	uint8_t Data[8];
	int n,Flag = 1;
	
	i -= *Value;
	*Value += i;
	if (Alternate) *Delta = -*Delta;
	i -= *Delta;
	*Delta += i;

	for (n=7;i>=64||i<-64;i>>=7,--n)
	{
		Data[n] = (uint8_t)(((i & 127) << 1)|Flag);
		Flag = 0;
	}
	Data[n] = (uint8_t)((i << 1)|Flag);
	ArrayAppend(Array,Data+n,8-n,1024);
}

static NOINLINE void NextKey(mp4streampos* Pos)
{
	if (!Pos->STSSPos)
		++Pos->STSSValue;
	else
		Pos->STSSPos = ReadValue(Pos->STSSPos,Pos->STSSEnd,&Pos->STSSDelta,&Pos->STSSValue,0);
}

static int NextSample(mp4stream* Stream,mp4streampos* Pos)
{
	if (Stream->Stream.Fragmented || Stream->Stream.ForceMerge)
	{
		int n,i;
		for (n=Pos->SamplesLeft;n>0;n-=i)
		{
			if (!Pos->STTSLeft)
				Pos->STTSLeft = (++Pos->STTSPos)->Count;
			i = min(Pos->STTSLeft,n);
			Pos->STTSLeft -= i;
			Pos->CurrentTime += i*Pos->STTSPos->Delta;
		}

		n = Pos->SamplesLeft;
		Pos->SampleNo += n;
		Pos->SamplesLeft = 0;

		if (Stream->STSZDef==1 && Stream->SamplesPerPacket && Stream->BytesPerPacket)
			return Scale(n,Stream->BytesPerPacket,Stream->SamplesPerPacket);

		return Stream->STSZDef*n;
	}
	else
	{
		if (!Pos->STTSLeft)
			Pos->STTSLeft = (++Pos->STTSPos)->Count;
		--Pos->STTSLeft;
		++Pos->SampleNo;
		--Pos->SamplesLeft;
		Pos->CurrentTime += Pos->STTSPos->Delta;
		if (!Stream->STSZDef)
			Pos->STSZPos = ReadValue(Pos->STSZPos,Pos->STSZEnd,&Pos->STSZDelta,&Pos->STSZValue,1);

		return Pos->STSZValue;
	}
}

static void NextChunk(mp4stream* Stream,mp4streampos* Pos)
{
	// return sample count in chunk and step to next chunk
	do
	{
		int Chunk = Pos->STCOPos - ARRAYBEGIN(Stream->STCO,int);
		while (Pos->STSCPos[1].FirstChunk <= Chunk)
			++Pos->STSCPos;
		++Pos->STCOPos;
		Pos->SamplesLeft = Pos->STSCPos->Samples;

	} while (Pos->SamplesLeft <= 0);
}

static void Head(mp4stream* Stream,mp4streampos* Pos)
{
	Pos->SampleNo = 0;
	Pos->CurrentTime = Stream->StartTime;
	Pos->SamplesLeft = 0;
	Pos->STTSLeft = 0;
	Pos->STSSDelta = 0;
	Pos->STSSValue = 0;
	Pos->STSZDelta = 0;
	Pos->STSZValue = 0;
	Pos->STSSPos = ARRAYBEGIN(Stream->STSS,uint8_t);
	Pos->STSSEnd = ARRAYEND(Stream->STSS,uint8_t);
	Pos->STTSPos = ARRAYBEGIN(Stream->STTS,mp4stts);
	Pos->STSCPos = ARRAYBEGIN(Stream->STSC,mp4stsc);
	Pos->STSZPos = ARRAYBEGIN(Stream->STSZ,uint8_t);
	Pos->STSZEnd = ARRAYEND(Stream->STSZ,uint8_t);
	Pos->STCOPos = ARRAYBEGIN(Stream->STCO,int);

	if (Pos->STTSPos)
		Pos->STTSLeft = Pos->STTSPos->Count;

	if (Stream->STSZDef)
		Pos->STSZValue = Stream->STSZDef;

	if (Pos->STSSPos)
		NextKey(Pos);
	else
	if (Stream->AllKey)
		Pos->STSSValue = 0;
	else
		Pos->STSSValue = -1;
}

static int GetFrames(mp4stream* Stream)
{
	const mp4stsc* i;
	int Frames = 0;
	int Chunks = ARRAYCOUNT(Stream->STCO,int)-1; //minus delimier
	for (i=ARRAYBEGIN(Stream->STSC,mp4stsc);i!=ARRAYEND(Stream->STSC,mp4stsc)-1;++i)
	{
		int Next = min(Chunks,i[1].FirstChunk);
		if (Next > i->FirstChunk)
			Frames += i->Samples * (Next - i->FirstChunk);
	}
	return Frames;
}

static void ReadTRAK(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream;
	p->Track = (mp4stream*) Format_AddStream(&p->Format,sizeof(mp4stream));
	p->Track->OffsetMin = MAX_INT;
	p->Track->OffsetMax = 0;

	ReadSubAtoms(p,EndOfAtom);

	Stream = p->Track;
	if (Stream)
	{
		if (ARRAYEMPTY(Stream->STCO) ||	ARRAYEMPTY(Stream->STTS) || ARRAYEMPTY(Stream->STSC))
			Format_RemoveStream(&p->Format);
		else
		{
			if (Stream->Stream.Format.Type == PACKET_VIDEO && Stream->Duration>0)
			{
				// calc FPS
				Stream->Stream.Format.PacketRate.Num = Scale(GetFrames(Stream),TICKSPERSEC*1000,Stream->Duration);
				Stream->Stream.Format.PacketRate.Den = 1000;

				if (Stream->Stream.Format.PacketRate.Num < 1000) // slideshow or podcast?
					Stream->Stream.Format.Format.Video.Pixel.Flags |= PF_NOPREROTATE;
			}

			//todo: calc ByteRate

			Head(Stream,&Stream->Pos);
			Format_PrepairStream(&p->Format,&Stream->Stream);
		}
		p->Track = NULL;
	}
}

static void ReadMETA(mp4* p,filepos_t EndOfAtom)
{
	ReadVersion(Reader(p));
	ReadSubAtoms(p,EndOfAtom);
}

static void ReadSTSS(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int Count;
	int Delta,Value;

	if (!Stream || Stream->Stream.Fragmented) return;
	ReadVersion(Reader(p));

	Count = Reader(p)->ReadBE32(Reader(p));

	if (Count <= 1 && Stream->Stream.Format.Type == PACKET_VIDEO &&
		Stream->Stream.Format.Format.Video.Pixel.FourCC == FOURCC('S','2','6','3'))
	{
		// assume all frames as keyframes
		Stream->AllKey = 1;
		return;
	}

	ArrayClear(&Stream->STSS);
	ArrayAlloc(&Stream->STSS,Count+128,1024);

	Delta = Value = 0;
	while (Count-->0)
		WriteValue(&Stream->STSS,&Delta,&Value,Reader(p)->ReadBE32(Reader(p))-1,0);

	ArrayLock(&Stream->STSS);
}

static void ReadSTSC(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int Count;

	if (!Stream) return;
	ReadVersion(Reader(p));

	Count = Reader(p)->ReadBE32(Reader(p));

	ArrayClear(&Stream->STSC);
	if (ArrayAppend(&Stream->STSC,NULL,sizeof(mp4stsc)*(Count+1),1))
	{
		mp4stsc* se = ARRAYEND(Stream->STSC,mp4stsc)-1;
		mp4stsc* s;
		for (s=ARRAYBEGIN(Stream->STSC,mp4stsc);s!=se;++s)
		{
			s->FirstChunk = Reader(p)->ReadBE32(Reader(p))-1; 
			s->Samples = Reader(p)->ReadBE32(Reader(p));
			Reader(p)->ReadBE32(Reader(p)); // desc
		}
		s->FirstChunk = MAX_INT; // delimiter
		s->Samples = 0;

		ArrayLock(&Stream->STSC);
	}
}

static void ReadSTCO(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int Count;

	if (!Stream) return;
	ReadVersion(Reader(p));

	Count = Reader(p)->ReadBE32(Reader(p));

	ArrayClear(&Stream->STCO);
	if (ArrayAppend(&Stream->STCO,NULL,sizeof(int)*(Count+1),1))
	{
		int* ce = ARRAYEND(Stream->STCO,int)-1;
		int* c;
		for (c=ARRAYBEGIN(Stream->STCO,int);c!=ce;++c)
		{
			*c = Reader(p)->ReadBE32(Reader(p));
			// skip first and last chunks (some crappy encoders make bad interleaving, but leaving the last chunk at file end)
			if (Count<3 || (c!=ARRAYBEGIN(Stream->STCO,int) && c!=ce-1))
			{
				if (Stream->OffsetMin > *c)
					Stream->OffsetMin = *c;
				if (Stream->OffsetMax < *c)
					Stream->OffsetMax = *c;
			}
		}
		*c = MAX_INT;

		ArrayLock(&Stream->STCO);
	}
}

static void ReadCTTS(mp4* p,filepos_t EndOfAtom)
{
	// presentation time offset (TCPMP only handles decoding time at the moment)

	mp4stream* Stream = p->Track;
	int Count;

	if (!Stream) return;
	ReadVersion(Reader(p));

	Count = Reader(p)->ReadBE32(Reader(p));

	if (Count>0)
	{
		int Offset;
		int Min = MAX_INT;

		while (Count-->0)
		{
			Reader(p)->ReadBE32(Reader(p)); // samples
			Offset = Reader(p)->ReadBE32(Reader(p));
			if (Min > Offset)
				Min = Offset;
		}

		if (Min>0 && Min<Stream->TimeScale)
			Stream->StartTime = Min;
	}
}

static void ReadSTSZ(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int Count;

	if (!Stream) return;
	ReadVersion(Reader(p));

	Stream->STSZDef = Reader(p)->ReadBE32(Reader(p));
	Count = Reader(p)->ReadBE32(Reader(p));

	if (!Stream->STSZDef)
	{
		int Delta,Value;

		Stream->Stream.Fragmented = 0;

		ArrayClear(&Stream->STSZ);
		ArrayAlloc(&Stream->STSZ,(Stream->Stream.Format.Type==PACKET_VIDEO?2*Count:Count)+1024,1024);

		Delta = Value = 0;
		while (Count-->0)
			WriteValue(&Stream->STSZ,&Delta,&Value,Reader(p)->ReadBE32(Reader(p)),1);

		ArrayLock(&Stream->STSZ);
	}
	else
	if (Stream->STSZDef==1)
		Stream->Stream.Fragmented = 1;
}

static void ReadSTTS(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int Count;

	if (!Stream) return;
	ReadVersion(Reader(p));

	Count = Reader(p)->ReadBE32(Reader(p));

	ArrayClear(&Stream->STTS);
	if (ArrayAppend(&Stream->STTS,NULL,sizeof(mp4stts)*(Count+1),1))
	{
		mp4stts* se = ARRAYEND(Stream->STTS,mp4stts)-1;
		mp4stts* s;
		for (s=ARRAYBEGIN(Stream->STTS,mp4stts);s!=se;++s)
		{
			s->Count = Reader(p)->ReadBE32(Reader(p)); 
			s->Delta = Reader(p)->ReadBE32(Reader(p));
		}
		s->Count = MAX_INT;
		s->Delta = 0;

		ArrayLock(&Stream->STTS);
	}
}

static void ReadExtra(mp4* p,filepos_t EndOfAtom)
{
	filepos_t Len = EndOfAtom - Reader(p)->FilePos;
	packetformat* Format;

	if (!p->Track || Len<=0) return;

	Format = &p->Track->Stream.Format;
	if (PacketFormatExtra(Format,Len))
		Reader(p)->Read(Reader(p),Format->Extra,Format->ExtraLength);
}

static int DescLen(mp4* p)
{
	int Len = 0;
	int i;
	for (i=0;i<4;++i)
	{
		int v = Reader(p)->Read8(Reader(p));
		Len = (Len << 7) | (v & 0x7F);
		if (!(v & 0x80))
			break;
	}
	return Len;
}

static void ReadESDS(mp4* p,filepos_t EndOfAtom)
{
	int Tag,Len;
	mp4stream* Stream = p->Track;

	if (!Stream) return;

	ReadVersion(Reader(p));

	Tag = Reader(p)->Read8(Reader(p));
	Len = DescLen(p);

	Reader(p)->Skip(Reader(p),2); //ID
	if (Tag == 3)
		Reader(p)->Skip(Reader(p),1); //priority

	Tag = Reader(p)->Read8(Reader(p));
	Len = DescLen(p);

	if (Tag == 4) //config descr
	{
		int ObjTypeId = Reader(p)->Read8(Reader(p));

		Reader(p)->Skip(Reader(p),12); //stream_type,buffer_size,max_bitrate,min_bitrate
		
		Tag = Reader(p)->Read8(Reader(p));
		Len = DescLen(p);

		if (Tag == 5 && PacketFormatExtra(&Stream->Stream.Format,Len))
			Reader(p)->Read(Reader(p),Stream->Stream.Format.Extra,Stream->Stream.Format.ExtraLength);

		//override fourcc or format if neccessary

		if (Stream->Stream.Format.Type == PACKET_VIDEO)
		{
			switch (ObjTypeId)
			{
			case MP4_MPEG1_VIDEO_TYPE:
			case MP4_MPEG2_SIMPLE_VIDEO_TYPE:
			case MP4_MPEG2_MAIN_VIDEO_TYPE:
			case MP4_MPEG2_SNR_VIDEO_TYPE:
			case MP4_MPEG2_SPATIAL_VIDEO_TYPE:
			case MP4_MPEG2_HIGH_VIDEO_TYPE:
			case MP4_MPEG2_442_VIDEO_TYPE:
				Stream->Stream.Format.Format.Video.Pixel.FourCC = FOURCC('m','p','e','g');
				break;

			case MP4_MPEG4_VIDEO_TYPE:
				Stream->Stream.Format.Format.Video.Pixel.FourCC = FOURCC('m','p','4','v');
				break;

			case MP4_JPEG_VIDEO_TYPE:
				Stream->Stream.Format.Format.Video.Pixel.FourCC = FOURCC('j','p','e','g');
				Stream->AllKey = 1;
				break;
			}
		}

		if (Stream->Stream.Format.Type == PACKET_AUDIO)
		{
			switch (ObjTypeId)
			{
			case MP4_MPEG1_AUDIO_TYPE:
			case MP4_MPEG2_AUDIO_TYPE:
				Stream->Stream.Format.Format.Audio.Format = AUDIOFMT_MP3;
				break;

			case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
			case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
			case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
			case MP4_MPEG4_AUDIO_TYPE:
				Stream->Stream.Format.Format.Audio.Format = AUDIOFMT_AAC;
				break;

			case MP4_PRIVATE_AUDIO_TYPE+1:
				if (Stream->Stream.Format.ExtraLength>4 && 
					*(uint32_t*)Stream->Stream.Format.Extra == FOURCC('Q','L','C','M'))
					Stream->Stream.Format.Format.Audio.Format = AUDIOFMT_QCELP;
				break;
			}
		}
	}
}

static void SetText(mp4* p,uint32_t FourCC)
{
	packetformat* Format = &p->Track->Stream.Format;
	PacketFormatClear(Format);
	Format->Type = PACKET_SUBTITLE;
	Format->Format.Subtitle.FourCC = FourCC;
	PacketFormatDefault(Format);
}

static void SetAudio(mp4* p,int AudioFormat,int SampleRate,int Bits,int Channels)
{
	packetformat* Format = &p->Track->Stream.Format;
	PacketFormatClear(Format);
	Format->Type = PACKET_AUDIO;
	Format->Format.Audio.Format = AudioFormat;
	Format->Format.Audio.Flags = PCM_PACKET_BASED;
	Format->Format.Audio.SampleRate = SampleRate;
	Format->Format.Audio.Bits = Bits;
	Format->Format.Audio.Channels = Channels;
	PacketFormatDefault(Format);
}

static void ReadAudio(mp4* p,int AudioFormat,int Atom,filepos_t EndOfAtom,int FixedBits)
{
	packetformat* Format = &p->Track->Stream.Format;
	PacketFormatClear(Format);
	Format->Type = PACKET_AUDIO;
	Format->Format.Audio.Format = AudioFormat;
	Format->Format.Audio.Channels = 1;
	Format->Format.Audio.Flags = PCM_PACKET_BASED;

#ifdef IS_BIG_ENDIAN
	if (Atom == FOURCC('s','o','w','t'))
		Format->Format.Audio.Flags |= PCM_SWAPEDBYTES;
#else
	if (Atom == FOURCC('t','w','o','s'))
		Format->Format.Audio.Flags |= PCM_SWAPEDBYTES;
#endif

	if (Atom == FOURCC('r','a','w',' ') || Atom == FOURCC('N','O','N','E'))
	{
#ifndef IS_BIG_ENDIAN
		Format->Format.Audio.Flags |= PCM_SWAPEDBYTES; // some digital cameras use 'raw ' with 16bit big endian
#endif
		Format->Format.Audio.Bits = 8;
	}
	else
		Format->Format.Audio.Bits = 16;

	if (EndOfAtom - Reader(p)->FilePos >= 20)
	{
		int Ver = Reader(p)->ReadBE16(Reader(p));
		Reader(p)->Skip(Reader(p),2+4); // revision, vendor

		Format->Format.Audio.Channels = Reader(p)->ReadBE16(Reader(p)); // channel count
		Format->Format.Audio.Bits = Reader(p)->ReadBE16(Reader(p)); // sample size

		Reader(p)->Skip(Reader(p),2+2); // compression_id,packet_size

		Format->Format.Audio.SampleRate = Reader(p)->ReadBE16(Reader(p)); // sample rate
		Reader(p)->Skip(Reader(p),2); // reserved (part of 32bit sample rate?)

		if (FixedBits < 0)
			FixedBits = Format->Format.Audio.Bits; 

		if (Ver == 1)
		{
			p->Track->SamplesPerPacket = Reader(p)->ReadBE32(Reader(p));
			Reader(p)->ReadBE32(Reader(p));
			p->Track->BytesPerPacket = Reader(p)->ReadBE32(Reader(p));
			Reader(p)->ReadBE32(Reader(p));

			if (AudioFormat == AUDIOFMT_ADPCM_MS || AudioFormat == AUDIOFMT_ADPCM_IMA)
				Format->Format.Audio.BlockAlign = p->Track->BytesPerPacket;
		}
		else
		if (FixedBits)
		{
			p->Track->SamplesPerPacket = 1;
			p->Track->BytesPerPacket = (FixedBits * Format->Format.Audio.Channels) >> 3;
		}

		ReadSubAtoms(p,EndOfAtom);
	}

	PacketFormatDefault(Format);
}

static void ReadVideo(mp4* p,int FourCC,filepos_t EndOfAtom)
{
	int ColTable,BitCount,ColCount;
	bool_t ColGray;
	packetformat* Format = &p->Track->Stream.Format;
	PacketFormatClear(Format);
	Format->Type = PACKET_VIDEO;
	
	Reader(p)->Skip(Reader(p),16); // reserved

	Format->Format.Video.Pixel.FourCC = FourCC;
	Format->Format.Video.Width = Reader(p)->ReadBE16(Reader(p));
	Format->Format.Video.Height = Reader(p)->ReadBE16(Reader(p)); 
	Format->Format.Video.Aspect = ASPECT_ONE; //todo: fix

	if (FourCC == FOURCC('m','p','4','v'))
	{
		// use mpeg4 vol information instead 
		Format->Format.Video.Width = 0;
		Format->Format.Video.Height = 0;
	}

	Reader(p)->Skip(Reader(p),14+32); // reserved,compressor

	BitCount = Reader(p)->ReadBE16(Reader(p));
	ColGray = (BitCount & 32)!=0;
	BitCount &= 31;
	ColCount = 1<<BitCount;
	Format->Format.Video.Pixel.BitCount = BitCount;

	ColTable = Reader(p)->ReadBE16(Reader(p));

    if ((BitCount==2 || BitCount==4 || BitCount==8) &&
		PacketFormatExtra(Format,sizeof(rgb)*ColCount)) 
	{
		rgb* Pal = Format->Format.Video.Pixel.Palette = (rgb*)Format->Extra;
		if (ColGray)
		{
			int i;
			for (i=0;i<ColCount;++i)
			{
				int v = 255 - (i*255)/(ColCount-1);
				Pal[i].c.r = (uint8_t)v;
				Pal[i].c.g = (uint8_t)v;
				Pal[i].c.b = (uint8_t)v;
				Pal[i].c.a = 0;
			}
		}
		else
		if (ColTable & 8)
		{
			// default palette
			int i;
			const rgbval_t* v = MovPal;
			if (BitCount>2) v += 1<<2;
			if (BitCount>4) v += 1<<4;
			for (i=0;i<ColCount;++i)
				Pal[i].v = v[i];
		}
		else
		{
			int Start,End;
			Start = Reader(p)->ReadBE32(Reader(p));
			Reader(p)->Skip(Reader(p),2); // count
			End = Reader(p)->ReadBE16(Reader(p))+1;
			if (End > ColCount)
				End = ColCount;

			for (;Start<End;++Start)
			{
				uint8_t v[4][2];
				Reader(p)->Read(Reader(p),v,sizeof(v)); // 16bit a,r,g,b 
				Pal[Start].c.r = v[1][0];
				Pal[Start].c.g = v[2][0];
				Pal[Start].c.b = v[3][0];
				Pal[Start].c.a = 0;
			}
		}
	}

	ReadSubAtoms(p,EndOfAtom);
	PacketFormatDefault(Format);
}

static void ReadSTSD(mp4* p,filepos_t EndOfAtom)
{
	mp4stream* Stream = p->Track;
	int DescCount;
	mp4atom Atom;

	if (!Stream) return;

	ReadVersion(Reader(p));

	DescCount = Reader(p)->ReadBE32(Reader(p));
	if (DescCount && ReadAtom(Reader(p),&Atom))
	{
		uint32_t FirstAtom = Atom.Atom;
		Reader(p)->Skip(Reader(p),6+2); // reserved,reference index

		Stream->Stream.Fragmented = 0;

		// fourcc codes from libavformat

		if (Atom.Atom == FOURCC('A','V','D','J'))
			Atom.Atom = FOURCC('j','p','e','g');

		switch (Atom.Atom)
		{
		case FOURCC('p','n','g',' '):
			Atom.Atom = FOURCC('P','N','G','_');
		case FOURCC('t','i','f','f'):
		case FOURCC('j','p','e','g'):
		case FOURCC('m','j','p','a'):
		case FOURCC('m','j','p','b'):
			Stream->AllKey = 1;
			// no break;
		case FOURCC('3','I','V','2'):
		case FOURCC('3','I','V','1'):
		case FOURCC('a','v','c','1'):
		case FOURCC('S','V','Q','1'):
		case FOURCC('S','V','Q','3'):
		case FOURCC('m','p','4','v'):
		case FOURCC('D','I','V','X'):
		case FOURCC('V','P','3','1'):
		case FOURCC('h','2','6','3'):
		case FOURCC('s','2','6','3'):
		case FOURCC('c','v','i','d'):
			ReadVideo(p,Atom.Atom,Atom.EndOfAtom);
			break;

//		case FOURCC('O','g','g','V'):
//			Stream->Stream.Fragmented = 1;
//			ReadAudio(p,AUDIOFMT_VORBIS_MODE1,Atom.Atom,Atom.EndOfAtom,-1);
//			break;

		case FOURCC('s','o','w','t'):
		case FOURCC('t','w','o','s'):
		case FOURCC('r','a','w',' '):
		case FOURCC('N','O','N','E'):
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_PCM,Atom.Atom,Atom.EndOfAtom,-1);
			break;

		case FOURCC('u','l','a','w'):
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_MULAW,Atom.Atom,Atom.EndOfAtom,8);
			break;

		case FOURCC('a','l','a','w'):
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_ALAW,Atom.Atom,Atom.EndOfAtom,8);
			break;

		case FOURCC('Q','D','M','2'):
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_QDESIGN2,Atom.Atom,Atom.EndOfAtom,0);
			break;

		case FOURCC('.','m','p','3'):
		case 0x6D730055:
		case 0x5500736D:
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_MP3,Atom.Atom,Atom.EndOfAtom,0);
			break;

		case 0x6D730002:
		case 0x0200736D:
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_ADPCM_MS,Atom.Atom,Atom.EndOfAtom,0);
			break;

		case 0x6D730011:
		case 0x1100736D:
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_ADPCM_IMA,Atom.Atom,Atom.EndOfAtom,0);
			break;

		case FOURCC('i','m','a','4'):
			Stream->Stream.Fragmented = 1;
			ReadAudio(p,AUDIOFMT_ADPCM_IMA_QT,Atom.Atom,Atom.EndOfAtom,0);

			Stream->SamplesPerPacket = 64;
			Stream->BytesPerPacket = 34*Stream->Stream.Format.Format.Audio.Channels;
			break;

		case FOURCC('m','p','4','a'):
			ReadAudio(p,AUDIOFMT_AAC,Atom.Atom,Atom.EndOfAtom,0);
			break;

		case FOURCC('s','a','m','r'):
			SetAudio(p,AUDIOFMT_AMR_NB,8000,16,1);
			break;

		case FOURCC('s','a','w','b'):
			SetAudio(p,AUDIOFMT_AMR_WB,16000,16,1);
			break;

		case FOURCC('t','e','x','t'):
			SetText(p,UpperFourCC(Atom.Atom));
			break;

		case FOURCC('t','x','3','g'):
			SetText(p,UpperFourCC(Atom.Atom));
			break;
		}

		while (--DescCount>0)
		{
			Reader(p)->Seek(Reader(p),Atom.EndOfAtom,SEEK_SET);
			ReadAtom(Reader(p),&Atom);
			if (Atom.Atom != FirstAtom)
			{
				if (!p->ErrorShowed)
				{
					p->ErrorShowed = 1;
					ShowError(p->Format.Format.Class,MP4_ID,MP4_MULTIPLE_FORMAT);
				}
				Stream->Stream.Format.Type = PACKET_NONE;
				break;
			}
		}
	}

	if (Stream->Stream.Format.Type == PACKET_NONE)
	{
		Format_RemoveStream(&p->Format);
		p->Track = NULL;
	}
}

static void ReadMVHD(mp4* p,filepos_t EndOfAtom)
{
	int Duration;

	ReadVersion(Reader(p));
	Reader(p)->ReadBE32(Reader(p)); // creation time
	Reader(p)->ReadBE32(Reader(p)); // modification time
	
	p->TimeScale = Reader(p)->ReadBE32(Reader(p)); // timescale
	Duration = Reader(p)->ReadBE32(Reader(p)); // duration

	if (Duration>0)
		p->Format.Duration = Scale(Duration,TICKSPERSEC,p->TimeScale);
}

static void ReadMDHD(mp4* p,filepos_t EndOfAtom)
{
	int Duration = -1;

	if (!p->Track) return;

	switch (ReadVersion(Reader(p)))
	{
	case 0:
		Reader(p)->ReadBE32(Reader(p)); // creation time
		Reader(p)->ReadBE32(Reader(p)); // modification time
		p->Track->TimeScale = Reader(p)->ReadBE32(Reader(p)); // timescale
		Duration = Reader(p)->ReadBE32(Reader(p)); // duration
		break;

	case 1:
		Reader(p)->ReadBE64(Reader(p)); // creation time
		Reader(p)->ReadBE64(Reader(p)); // modification time
		p->Track->TimeScale = Reader(p)->ReadBE32(Reader(p)); // timescale
		Duration = (int)Reader(p)->ReadBE64(Reader(p)); // duration
		break;
	}

	if (Duration>0)
	{
		Duration = Scale(Duration,TICKSPERSEC,p->Track->TimeScale);
		if (p->Format.Duration < Duration)
			p->Format.Duration = Duration;
	}

	p->Track->Duration = Duration;
}

static void ReadCMOV(mp4* p,filepos_t EndOfAtom)
{
	ReadSubAtoms(p,EndOfAtom);
	p->CompressType = 0;
}

static void ReadDCOM(mp4* p,filepos_t EndOfAtom)
{
	p->CompressType = 0;
	Reader(p)->Read(Reader(p),&p->CompressType,sizeof(p->CompressType));
}

static void ReadCMVD(mp4* p,filepos_t EndOfAtom)
{
	if (p->CompressType == FOURCC('z','l','i','b'))
	{
		uint32_t DstLen = Reader(p)->ReadBE32(Reader(p));
		int SrcLen = EndOfAtom - Reader(p)->FilePos;

		uint8_t* Src = (uint8_t*)malloc(SrcLen);
		uint8_t* Dst = (uint8_t*)malloc(DstLen);

		if (Src && Dst && Reader(p)->Read(Reader(p),Src,SrcLen)==SrcLen && 
			uncompress(Dst,&DstLen,Src,SrcLen)==Z_OK)
		{
			format_buffer Buffer;
			format_reader* Reader = p->Format.Reader;
			format_reader Backup = *Reader;

			Buffer.Block.Id = 0;
			Buffer.Block.Ptr = Dst;
			Buffer.Length = DstLen;

			Reader->BufferFirst = NULL;
			Reader->BufferLast = NULL;
			Reader->NoMoreInput = 1;
			Reader->ReadPos = 0;
			Reader->ReadLen = DstLen;
			Reader->FilePos = 0;
			Reader->Input = NULL;
			Reader->InputBuffer = NULL;
			Reader->ReadBuffer = &Buffer;
			Reader->Format = &p->Format;
			Reader->BufferAvailable = 0;

			ReadSubAtoms(p,DstLen);

			*Reader = Backup;
		}
		free(Src);
		free(Dst);
	}
}

static void AddMeta(mp4* p,const tchar_t* Meta,int Size)
{
	pin* Comment = &p->Format.Comment;
	format_stream* Stream = (format_stream*)p->Track;
	if (Stream)
		Comment = &Stream->Comment;
	if (Comment->Node)
		Comment->Node->Set(Comment->Node,Comment->No,Meta,Size);
}

static void ReadMetaString(mp4* p,filepos_t EndOfAtom)
{
	tchar_t Meta[512];
	char UTF8[512];
	int Len = EndOfAtom - Reader(p)->FilePos;
	if (Len>=512) Len=511;
	Len = Reader(p)->Read(Reader(p),UTF8,Len);
	if (Len>0)
	{
		UTF8[Len] = 0;
		tcscpy_s(Meta,TSIZEOF(Meta),p->Meta);
		UTF8ToTcs(Meta+tcslen(p->Meta),TSIZEOF(Meta)-tcslen(p->Meta),UTF8);
		AddMeta(p,Meta,sizeof(Meta));
	}
}

static void ReadRDRF(mp4* p,filepos_t EndOfAtom)
{
	if (p->RMRAFound)
	{
		//redirect
		if (Reader(p)->ReadBE32(Reader(p))==0)
		{
			int Size;
			uint32_t Type = 0;
			Reader(p)->Read(Reader(p),&Type,sizeof(Type));
			Size = Reader(p)->ReadBE32(Reader(p));
			if (Type == FOURCC('u','r','l',' '))
			{
				tcscpy_s(p->Meta,TSIZEOF(p->Meta),T("REDIRECT="));
				ReadMetaString(p,min(EndOfAtom,Reader(p)->FilePos + Size));
				p->Meta[0] = 0;
			}
		}
	}
}

static void ReadDATA(mp4* p,filepos_t EndOfAtom)
{
	ReadVersion(Reader(p));
	Reader(p)->Skip(Reader(p),4); //reserved;
	if (tcscmp(p->Meta,T("COVER="))==0)
	{
		tchar_t Meta[256];
		stprintf_s(Meta,TSIZEOF(Meta),T("%s:%d:%d:"),p->Meta,Reader(p)->FilePos,EndOfAtom-Reader(p)->FilePos);
		AddMeta(p,Meta,sizeof(Meta));
	}
	else
	if (tcscmp(p->Meta,T("GENRE="))==0)
	{
		tchar_t Meta[256];
		tcscpy_s(Meta,TSIZEOF(Meta),p->Meta);
		if (ID3v1_Genre(Reader(p)->ReadBE16(Reader(p))-1,Meta+tcslen(Meta),256-tcslen(Meta)))
			AddMeta(p,Meta,sizeof(Meta));
	}
	else
	if (p->Meta[0])
		ReadMetaString(p,EndOfAtom);
}

static void ReadNeroChapter(mp4* p,filepos_t EndOfAtom)
{
	int Count,i,Len;

	Reader(p)->Read8(Reader(p)); // version?
	Reader(p)->Skip(Reader(p),4); // ???
	Count = Reader(p)->ReadBE32(Reader(p));
	for (i=0;i<Count;++i)
	{
		int64_t Time = Reader(p)->ReadBE64(Reader(p));

		stprintf_s(p->Meta,TSIZEOF(p->Meta),T("CHAPTER%02dNAME="),i+1);
		Len = Reader(p)->Read8(Reader(p));
		ReadMetaString(p,Reader(p)->FilePos + Len);

		BuildChapter(p->Meta,TSIZEOF(p->Meta),i+1,Time,10000);
		AddMeta(p,p->Meta,sizeof(p->Meta));
	}
}

static void ReadNAME(mp4* p,filepos_t EndOfAtom)
{
	ReadVersion(Reader(p));
	if (p->Meta[0])
		ReadMetaString(p,EndOfAtom);
	else
	{
		char UTF8[512];
		int Len = EndOfAtom - Reader(p)->FilePos;
		if (Len>510) Len=510;
		Len = Reader(p)->Read(Reader(p),UTF8,Len);
		if (Len>0)
		{
			UTF8[Len] = '=';
			UTF8[Len+1] = 0;
			UTF8ToTcs(p->Meta,TSIZEOF(p->Meta),UTF8);
		}
	}
}

static void ReadMeta(mp4* p,filepos_t EndOfAtom, const tchar_t* Meta)
{
	mp4atom Atom;
	int Pos = Reader(p)->FilePos;
	tcscpy_s(p->Meta,TSIZEOF(p->Meta),Meta);
	ReadAtom(Reader(p),&Atom);
	Reader(p)->Seek(Reader(p),Pos,SEEK_SET);
	if (Atom.EndOfAtom > EndOfAtom)
		ReadNAME(p,EndOfAtom);
	else
		ReadSubAtoms(p,EndOfAtom);
	p->Meta[0] = 0;
}

static void ReadMetaCover(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("COVER=")); }
static void ReadMetaName(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("TITLE=")); }
static void ReadMetaArtist(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("ARTIST=")); }
static void ReadMetaWriter(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("WRITER=")); }
static void ReadMetaAlbum(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("ALBUM=")); }
static void ReadMetaDate(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("DATE=")); }
static void ReadMetaTool(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("TOOL=")); }
static void ReadMetaComment(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("COMMENT=")); }
static void ReadMetaGenre(mp4* p,filepos_t EndOfAtom) { ReadMeta(p,EndOfAtom,T("GENRE=")); }

static const mp4atomtable AtomTable[] =
{
	{ FOURCC('f','t','y','p'), ReadFTYP },
	{ FOURCC('m','o','o','v'), ReadMOOV },
	{ FOURCC('t','r','a','k'), ReadTRAK },
	{ FOURCC('m','v','h','d'), ReadMVHD },
	{ FOURCC('m','d','h','d'), ReadMDHD },
	{ FOURCC('s','t','b','l'), ReadSubAtoms },
	{ FOURCC('m','i','n','f'), ReadSubAtoms },
	{ FOURCC('e','d','t','s'), ReadSubAtoms },
	{ FOURCC('m','d','i','a'), ReadSubAtoms },
	{ FOURCC('u','d','t','a'), ReadSubAtoms },
	{ FOURCC('i','l','s','t'), ReadSubAtoms },
	{ FOURCC('w','a','v','e'), ReadSubAtoms },
	{ FOURCC('-','-','-','-'), ReadSubAtoms },
	{ FOURCC('r','m','d','a'), ReadSubAtoms },
	{ FOURCC('r','m','r','a'), ReadRMRA },
	{ FOURCC('r','d','r','f'), ReadRDRF },
	{ FOURCC('m','e','t','a'), ReadMETA },
	{ FOURCC('s','t','s','d'), ReadSTSD },
	{ FOURCC('s','t','s','s'), ReadSTSS },
	{ FOURCC('s','t','s','c'), ReadSTSC },
	{ FOURCC('s','t','c','o'), ReadSTCO },
	{ FOURCC('c','t','t','s'), ReadCTTS },
	{ FOURCC('s','t','s','z'), ReadSTSZ },
	{ FOURCC('s','t','t','s'), ReadSTTS },
	{ FOURCC('c','m','o','v'), ReadCMOV },
	{ FOURCC('d','c','o','m'), ReadDCOM },
	{ FOURCC('c','m','v','d'), ReadCMVD },
	{ FOURCC('e','s','d','s'), ReadESDS },
	{ FOURCC('S','M','I',' '), ReadExtra },
	{ FOURCC('a','v','c','C'), ReadExtra },
	{ FOURCC('d','a','t','a'), ReadDATA },
	{ FOURCC('n','a','m','e'), ReadNAME },
	{ FOURCC('c','h','p','l'), ReadNeroChapter },
	{ FOURCC('\251','n','a','m'), ReadMetaName },
	{ FOURCC('\251','A','R','T'), ReadMetaArtist },
	{ FOURCC('\251','w','r','t'), ReadMetaWriter },
	{ FOURCC('\251','a','l','b'), ReadMetaAlbum },
	{ FOURCC('\251','d','a','y'), ReadMetaDate },
	{ FOURCC('\251','t','o','o'), ReadMetaTool },
	{ FOURCC('\251','c','m','t'), ReadMetaComment },
	{ FOURCC('\251','g','e','n'), ReadMetaGenre },
	{ FOURCC('c','o','v','r'), ReadMetaCover },
	{ FOURCC('g','n','r','e'), ReadMetaGenre },

	{ 0, NULL }
};

static void ReadSubAtoms(mp4* p,filepos_t EndOfAtom)
{
	mp4atom Atom;
	const mp4atomtable* i;

	while (Reader(p)->FilePos < EndOfAtom - 8)
	{
		if (!ReadAtom(Reader(p),&Atom))
			break;

		if (p->MOOVFound && Atom.Atom == FOURCC('m','d','a','t'))
			break;

		for (i=AtomTable;i->Atom;++i)
			if (i->Atom == Atom.Atom)
			{
				i->Func(p,Atom.EndOfAtom);
				break;
			}

		Reader(p)->Seek(Reader(p),Atom.EndOfAtom,SEEK_SET);
	}

	p->Meta[0] = 0;
}

static mp4stream* FindNextChunk(mp4* p, format_reader* Reader)
{
	mp4stream* Stream = NULL;
	int Min = MAX_INT;
	int No;
	for (No=0;No<p->Format.StreamCount;++No)
	{
		mp4stream* s = (mp4stream*)p->Format.Streams[No];
		if (s->Stream.Pin.Node && s->Stream.Reader == Reader && Min > *s->Pos.STCOPos)
		{
			Min = *s->Pos.STCOPos;
			Stream = s;
		}
	}

	if (Stream)
	{
		if (Reader->Seek(Reader,Min,SEEK_SET) == ERR_NONE)
		{
			Reader->Current = &Stream->Stream;
			NextChunk(Stream,&Stream->Pos);
		}
		else
			Stream = NULL;
	}
	return Stream;
}

static void AddTextChapters(mp4* p,mp4stream* s)
{
	format_reader* Reader = p->Format.Reader;
	mp4streampos Pos;
	int No = 1;

	Head(s,&Pos);
	for (;;)
	{
		filepos_t FilePos = *Pos.STCOPos;
		if (FilePos == MAX_INT)
			break;

		NextChunk(s,&Pos);
		while (Pos.SamplesLeft)
		{
			if (Reader->Seek(Reader,FilePos,SEEK_SET) == ERR_NONE)
			{
				int Len = Reader->ReadBE16(Reader);
				if (Len)
				{
					stprintf_s(p->Meta,TSIZEOF(p->Meta),T("CHAPTER%02dNAME="),No);
					ReadMetaString(p,Reader->FilePos+Len);
					BuildChapter(p->Meta,TSIZEOF(p->Meta),No,(int64_t)Pos.CurrentTime*1000,s->TimeScale);
					AddMeta(p,p->Meta,sizeof(p->Meta));
					++No;
				}
			}
			FilePos += NextSample(s,&Pos);
		}
	}
}

static int Init(mp4* p)
{
	int No;

	p->MOOVFound = 0;
	p->Type = FOURCC('m','o','o','v'); //quicktime .mov
	p->TimeScale = 0;
	p->ErrorShowed = 0;
	p->Format.HeaderLoaded = 1;
	p->Meta[0] = 0;

	ReadSubAtoms(p,MAX_INT);

	if (!p->MOOVFound)
		return ERR_INVALID_DATA;

	if (p->Format.Comment.Node)
		for (No=0;No<p->Format.StreamCount;++No)
		{
			mp4stream* s = (mp4stream*)p->Format.Streams[No];
			if (s->Stream.Format.Type == PACKET_SUBTITLE && 
				s->Stream.Format.Format.Subtitle.FourCC == FOURCC('T','E','X','T') && GetFrames(s)<100)
				AddTextChapters(p,s);
		}

	for (No=0;No<p->Format.StreamCount;++No)
	{
		mp4stream* s = (mp4stream*)p->Format.Streams[No];
		format_reader* Reader = Format_FindReader(&p->Format,s->OffsetMin,s->OffsetMax);
		if (!Reader)
			break;
		s->Stream.Reader = Reader;
	}

	for (No=0;No<MAXREADER;++No)
		FindNextChunk(p,p->Format.Reader+No);

	return ERR_NONE;
}

static int ReadPacket(mp4* p, format_reader* Reader, format_packet* Packet)
{
	mp4stream* Stream = (mp4stream*) Reader->Current;
	if (!Stream)
	{
		Stream = FindNextChunk(p,Reader);
		if (!Stream)
			return ERR_END_OF_FILE;
	}

	Packet->Stream = &Stream->Stream;
	Packet->RefTime = Scale(Stream->Pos.CurrentTime,TICKSPERSEC,Stream->TimeScale);
	if (Stream->Pos.STSSValue >= 0)
	{
		Packet->Key = Stream->Pos.SampleNo == Stream->Pos.STSSValue;
		if (Packet->Key)
			NextKey(&Stream->Pos);
	}
	Packet->Data = Reader->ReadAsRef(Reader,NextSample(Stream,&Stream->Pos));
	if (Stream->Pos.SamplesLeft==0)
		Reader->Current = NULL;
	return ERR_NONE;
}

static bool_t GetNeedKey(mp4stream* Stream)
{
	return (!ARRAYEMPTY(Stream->STSS) || Stream->AllKey) && Stream->Stream.Format.Type == PACKET_VIDEO;
}

static int SeekReader(mp4* p, format_reader* Reader, tick_t* DstTime, filepos_t DstPos, bool_t PrevKey)
{
	mp4stream* Stream = (mp4stream*) Format_DefSyncStream(&p->Format,Reader);
	if (Stream && Reader->Input)
	{
		mp4streampos Last;
		mp4streampos Pos;
		bool_t NeedKey;
		tick_t SyncTime;
		int DstMediaTime,No;
		filepos_t FilePos;
		filepos_t LastFilePos = -1;

		DstMediaTime = -1;
		if (*DstTime >= 0)
			DstMediaTime = Scale(*DstTime,Stream->TimeScale,TICKSPERSEC);

		NeedKey = GetNeedKey(Stream);
		if (!NeedKey)
			PrevKey = 0;
		else
		if (Stream->AllKey) // SyncTime will be independent anyway. we always need current chapter image
			PrevKey = 1;

		if (PrevKey && DstMediaTime >= 0)
			DstMediaTime += 1+Stream->TimeScale/256; // seek to same position even if there is rounding error

		Head(Stream,&Last);
		Head(Stream,&Pos);

		for (;;)
		{
			FilePos = *Pos.STCOPos;
			if (FilePos == MAX_INT)
				break;

			NextChunk(Stream,&Pos);

			while (Pos.SamplesLeft)
			{
				bool_t Key = Pos.SampleNo == Pos.STSSValue;

				if (DstMediaTime >= 0 && Pos.CurrentTime >= DstMediaTime && (!NeedKey || Key))
				{
					if (PrevKey && Pos.CurrentTime != DstMediaTime && LastFilePos>=0)
					{
						FilePos = LastFilePos;
						Pos = Last;
					}
					goto found;
				}

				if (DstPos >= 0 && FilePos >= DstPos)
					goto found;

				if (Key || Pos.STSSValue<0)
				{
					LastFilePos = FilePos;
					Last = Pos;
				}

				if (Key) NextKey(&Pos);
				FilePos += NextSample(Stream,&Pos);
			}
		}

		if (LastFilePos>=0)
		{
			FilePos = LastFilePos;
			Pos = Last;
		}
found:

		if (FilePos==MAX_INT || Reader->Seek(Reader,FilePos,SEEK_SET) != ERR_NONE)
			return ERR_NOT_SUPPORTED;

		Stream->Stream.Reader->Current = Pos.SamplesLeft ? &Stream->Stream:NULL;
		Stream->Pos = Pos;

		SyncTime = Scale(Pos.CurrentTime,TICKSPERSEC,Stream->TimeScale);
		if (*DstTime >= 0 && (!Stream->AllKey || (abs(*DstTime-SyncTime)<TICKSPERSEC/16))) // use updated time with other Readers
			*DstTime = SyncTime;

		// set current positions for other streams
		for (No=0;No<p->Format.StreamCount;++No)
		{
			mp4stream* s = (mp4stream*) p->Format.Streams[No];
			if (s->Stream.Reader==Reader && s != Stream)
			{
				Head(s,&s->Pos);
				for (;;)
				{
					if (*s->Pos.STCOPos >= FilePos)
						break;

					NextChunk(s,&s->Pos);
					while (s->Pos.SamplesLeft)
					{
						if (s->Pos.SampleNo == s->Pos.STSSValue) 
							NextKey(&s->Pos);
						NextSample(s,&s->Pos);
					}
				}
			}
		}
	}

	return ERR_NONE;
}

static int Seek(mp4* p, tick_t DstTime, filepos_t DstPos, bool_t PrevKey)
{
	int	Result;
	int Skip = -1;
	int No;

	for (No=0;No<MAXREADER;++No)
	{
		mp4stream* Stream = (mp4stream*) Format_DefSyncStream(&p->Format,&p->Format.Reader[No]);
		if (Stream && GetNeedKey(Stream))
		{
			Skip = No;
			Result = SeekReader(p,&p->Format.Reader[No],&DstTime,DstPos,PrevKey);
			if (Result != ERR_NONE)
				return Result;
			if (Stream->AllKey && DstTime >= 0)
				p->Format.SyncTime = DstTime;
			break;
		}
	}

	if (Skip<0)
		PrevKey = 0;

	for (No=0;No<MAXREADER;++No)
		if (No != Skip)
		{
			Result = SeekReader(p,&p->Format.Reader[No],&DstTime,DstPos,PrevKey);
			if (Result != ERR_NONE)
				return Result;
		}

	Format_AfterSeek(&p->Format);
	return ERR_NONE;
}

static void FreeStream(mp4* p, mp4stream* Stream)
{
	ArrayClear(&Stream->STSS);
	ArrayClear(&Stream->STSZ);
	ArrayClear(&Stream->STTS);
	ArrayClear(&Stream->STSC);
	ArrayClear(&Stream->STCO);
}

static int Create(mp4* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Seek = (fmtseek)Seek;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacket;
	p->Format.FreeStream = (fmtstream)FreeStream;
	return ERR_NONE;
}

static const nodedef MP4 =
{
	sizeof(mp4),
	MP4_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void MP4_Init()
{
	NodeRegisterClass(&MP4);
}

void MP4_Done()
{
	NodeUnRegisterClass(MP4_ID);
}

