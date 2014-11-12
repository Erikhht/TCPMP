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
 * $Id: ogg.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "vorbis.h"
#include "tremor/ogg.h"
#include "tremor/ivorbiscodec.h"

// OGG and OGM file format

#define PACKET_TYPE_HEADER		0x01
#define PACKET_TYPE_COMMENT		0x03
#define PACKET_TYPE_CODEBOOK	0x05
#define PACKET_TYPE_BITS		0x07

#define	PACKET_IS_SYNCPOINT		0x08
#define PACKET_LEN_BITS01		0xc0
#define PACKET_LEN_BITS2		0x02

static const char MT_Video[]	= "video";
static const char MT_Audio[]	= "audio";
static const char MT_Text[]		= "text";

typedef struct oggstream
{
	format_stream Stream;

	bool_t Invalid; // not vorbis and not ogm
	bool_t Vorbis;	// sending direct oggpackets (not the merged binary data)
	bool_t Native; // speex
	int PacketNo;

	int64_t MediaTime; // in units
	int64_t MediaRateNum; // in units/tick
	int64_t MediaRateDen;
	int DefaultLen;

	bool_t NeedMorePage;

	ogg_stream_state* OggStream;
	ogg_packet       OggPacket;

	vorbis_info      Info;
	vorbis_comment   Comment;

} oggstream;

typedef struct ogg
{
	format_base		Format;
	ogg_sync_state*	OggSync;
	ogg_page        OggPage;

} ogg;

typedef struct ogm_header_video
{
	int32_t	width;
	int32_t	height;

} ogm_header_video;
	
typedef struct ogm_header_audio
{
	int16_t	channels;
	int16_t	blockalign;
	int32_t	avgbytespersec;
} ogm_header_audio;

typedef struct ogm_header
{
	char	streamtype[8];
	char	subtype[4];

	int32_t	size;

	int64_t	time_unit;
	int64_t	samples_per_unit;
	int32_t default_len;

	int32_t buffersize;
	int16_t	bits_per_sample;

	union
	{
		ogm_header_video video;
		ogm_header_audio audio;

	} format;

} ogm_header;

extern int _vorbis_unpack_comment(vorbis_comment *vc,oggpack_buffer *opb,int eop);

static void FreeStream(ogg* p,oggstream* Stream)
{
	vorbis_comment_clear(&Stream->Comment);
	vorbis_info_clear(&Stream->Info);

	ogg_packet_release(&Stream->OggPacket);
	if (Stream->OggStream)
	{
		ogg_stream_destroy(Stream->OggStream);
		Stream->OggStream = NULL;
	}
}

static void Done(ogg* p)
{
	ogg_page_release(&p->OggPage);
	if (p->OggSync)
	{
		ogg_sync_destroy(p->OggSync);
		p->OggSync = NULL;
	}
}

static int AddBuffer(ogg* p,format_buffer* Buffer)
{
	if (!Buffer)
		return ERR_NEED_MORE_DATA;
	
	if (Buffer->Length)
	{
		ogg_buffer* Ptr = ogg_sync_bufferinext(p->OggSync);
		if (!Ptr)
		{
			Format_BufferRelease(&p->Format,Buffer);
			return ERR_OUT_OF_MEMORY;
		}

		Ptr->ext = &p->Format;
		Ptr->extdata = Buffer;
		Ptr->data = Buffer->Block.Ptr;
		Ptr->size = Buffer->Length;
		ogg_sync_wrote(p->OggSync,Buffer->Length);
	}
	else
		Format_BufferRelease(&p->Format,Buffer);

	return ERR_NONE;
}

static int Init(ogg* p)
{
	format_reader* Reader = p->Format.Reader;
	format_buffer* Buffer;
	int i;

	memset(&p->OggPage,0,sizeof(ogg_page));
	p->OggSync = ogg_sync_create();
	p->Format.TimeStamps = 1;
	if (!p->OggSync)
		return ERR_OUT_OF_MEMORY;

	Format_ReadBuffer(Reader,0);
	Buffer = Format_BufferRemove(Reader);
	if (!Buffer)
		return ERR_INVALID_DATA;

	// search for 'OggS' pattern
	for (i=3;i<Buffer->Length;++i)
		if (Buffer->Block.Ptr[i-3]=='O' &&
			Buffer->Block.Ptr[i-2]=='g' &&
			Buffer->Block.Ptr[i-1]=='g' &&
			Buffer->Block.Ptr[i-0]=='S')
			break;

	if (i==Buffer->Length)
	{
		Format_BufferRelease(&p->Format,Buffer);
		return ERR_INVALID_DATA;
	}
	
	return AddBuffer(p,Buffer);
}

static void AfterSeek(ogg* p)
{
	ogg_sync_reset(p->OggSync);
}

static void ReleaseStream(ogg* p,oggstream* Stream)
{
	ogg_stream_reset_serialno(Stream->OggStream,Stream->Stream.Id);
	Stream->NeedMorePage = 1;
	Stream->MediaTime = Stream->Stream.Reader->FilePos>0 && Stream->PacketNo>3 ? -1:0;
}

extern void	oggext_release(void* ext,void* ext2)
{
	if (ext && ext2)
		Format_BufferRelease((format_base*)ext,(format_buffer*)ext2);
}

static int ReadPacketTime( ogg* p, format_reader* Reader, format_packet* Packet )
{
	// only called by CalcDuration

	format_buffer* Buffer;
	int Bytes;
	ogg_buffer* Ptr;
	ogg_sync_state* Sync = ogg_sync_create();
	ogg_page Page;

	if (!Sync)
		return ERR_OUT_OF_MEMORY;

	while ((Buffer = Format_BufferRemove(Reader))!=NULL)
	{
		Ptr = ogg_sync_bufferinext(Sync);
		if (Ptr)
		{
			Ptr->ext = &p->Format;
			Ptr->extdata = Buffer;
			Ptr->data = Buffer->Block.Ptr;
			Ptr->size = Buffer->Length;
			ogg_sync_wrote(Sync,Buffer->Length);
		}
		else
			Format_BufferRelease(&p->Format,Buffer);
	}

	memset(&Page,0,sizeof(Page));

	while ((Bytes = ogg_sync_pageseek(Sync,&Page)) != 0)
	{
		if (Bytes > 0)
		{
			int64_t MediaTime = ogg_page_granulepos(&Page);
			int	Id = ogg_page_serialno(&Page);
			int i;

			if (MediaTime != -1)
				for (i=0;i<p->Format.StreamCount;++i)
					if (p->Format.Streams[i]->Id == Id)
					{
						oggstream* s = (oggstream*) p->Format.Streams[i];

						if (s->MediaRateNum)
						{
							tick_t RefTime = (tick_t)((MediaTime * s->MediaRateDen) / s->MediaRateNum);
							
							if (RefTime > Packet->RefTime)
							{
								Packet->Stream = &s->Stream;
								Packet->RefTime = RefTime;
							}
						}
						break;
					}
		}
	}

	ogg_page_release(&Page);
	ogg_sync_destroy(Sync);

	return ERR_NONE;
}

static void FrameRate( fraction* f, int64_t Num, int64_t Den )
{
	while (Num > MAX_INT || Den > MAX_INT)
	{
		Num >>= 1;
		Den >>= 1;
	}
	f->Num = (int)Num;
	f->Den = (int)Den;
}

static bool_t SpeexHeader(ogg* p, oggstream* s, const char* Data, int Length )
{
	if (Length<80 && strncmp(Data,"Speex   ",8)!=0)
		return 0;

	PacketFormatClear(&s->Stream.Format);
	s->Stream.Format.Type = PACKET_AUDIO;
	s->Stream.Format.Format.Audio.Format = AUDIOFMT_SPEEX;
	s->Stream.Format.Format.Audio.Channels = INT32LE(*(int32_t*)(Data+48));
	s->Stream.Format.ByteRate = INT32LE(*(int32_t*)(Data+52))/8;
	s->Stream.Format.Format.Audio.SampleRate = INT32LE(*(int32_t*)(Data+36));

	s->MediaRateDen = TICKSPERSEC;
	s->MediaRateNum = s->Stream.Format.Format.Audio.SampleRate;
	s->Native = 1;

	if (PacketFormatExtra(&s->Stream.Format,Length))
		memcpy(s->Stream.Format.Extra,Data,s->Stream.Format.ExtraLength);

	return 1;
}

static bool_t OGMHeader(ogg* p, oggstream* s, const char* Data, int Length )
{
	int i;
	if (Length<40 || (*Data & PACKET_TYPE_BITS) != PACKET_TYPE_HEADER)
		return 0;

	if (strncmp(Data+1, "Direct Show Samples embedded in Ogg", 35) == 0)
	{
		// old header

		if (INT32LE(*(int32_t*)(Data+96)) == 0x05589F80)
		{
			PacketFormatClear(&s->Stream.Format);
			s->Stream.Format.Type = PACKET_VIDEO;
			s->Stream.Format.Format.Video.Width = INT32LE(*(int32_t*)(Data+176));
			s->Stream.Format.Format.Video.Height = INT32LE(*(int32_t*)(Data+180));
			s->Stream.Format.Format.Video.Pixel.FourCC = INT32LE(*(int32_t*)(Data+68));
			s->Stream.Format.Format.Video.Pixel.BitCount = INT16LE(*(int16_t*)(Data+182));

			i = INT16LE(*(int16_t*)(Data+136));	// extrasize
			if (i && PacketFormatExtra(&s->Stream.Format,i))
				memcpy(s->Stream.Format.Extra,Data+142,s->Stream.Format.ExtraLength);

			s->MediaRateDen = INT64LE(*(int32_t*)(Data+164))*TICKSPERSEC;
			s->MediaRateNum = 10000000;
			s->DefaultLen = 1;

			FrameRate(&s->Stream.Format.PacketRate,s->MediaRateNum,s->MediaRateDen/TICKSPERSEC);
			return 1;
		}

		if (INT32LE(*(int32_t*)(Data+96)) == 0x05589F81)
		{
			PacketFormatClear(&s->Stream.Format);
			s->Stream.Format.Type = PACKET_AUDIO;
			s->Stream.Format.Format.Audio.Format = INT16LE(*(int16_t*)(Data+124));
			s->Stream.Format.Format.Audio.Channels = INT16LE(*(int16_t*)(Data+126));
			s->Stream.Format.Format.Audio.BlockAlign = INT16LE(*(int16_t*)(Data+136));
			s->Stream.Format.Format.Audio.Bits = INT16LE(*(int16_t*)(Data+138));
			s->Stream.Format.Format.Audio.SampleRate = INT32LE(*(int32_t*)(Data+128));
			s->Stream.Format.ByteRate = INT32LE(*(int32_t*)(p+132));

			i = INT16LE(*(int16_t*)(Data+136));	// extrasize
			if (i && PacketFormatExtra(&s->Stream.Format,i))
				memcpy(s->Stream.Format.Extra,Data+142,s->Stream.Format.ExtraLength);

			s->MediaRateDen = TICKSPERSEC;
			s->MediaRateNum = INT32LE(*(int32_t*)(Data+128));
			s->DefaultLen = 1;
			return 1;
		}
	}
	else
	if (Length >= sizeof(ogm_header)+1)
	{
		ogm_header Head;
		memcpy(&Head,Data+1,sizeof(Head));

		// new header
		if (strncmp(Head.streamtype, MT_Video, strlen(MT_Video)) == 0)
		{
			PacketFormatClear(&s->Stream.Format);
			s->Stream.Format.Type = PACKET_VIDEO;
			s->Stream.Format.Format.Video.Width = INT32LE(Head.format.video.width);
			s->Stream.Format.Format.Video.Height = INT32LE(Head.format.video.height);
			s->Stream.Format.Format.Video.Pixel.FourCC = INT32LE(*(int32_t*)Head.subtype);
			s->Stream.Format.Format.Video.Pixel.BitCount = INT16LE(Head.bits_per_sample);

			s->MediaRateDen = INT64LE(Head.time_unit)*TICKSPERSEC;
			s->MediaRateNum = INT64LE(Head.samples_per_unit) * 10000000;
			s->DefaultLen = INT32LE(Head.default_len);

			FrameRate(&s->Stream.Format.PacketRate,s->MediaRateNum,s->MediaRateDen/TICKSPERSEC);
			i = Length - (sizeof(ogm_header)+1);
			if (i && PacketFormatExtra(&s->Stream.Format,i))
				memcpy(s->Stream.Format.Extra,Data+1+sizeof(ogm_header),s->Stream.Format.ExtraLength);
			return 1;
		}
		
		if (strncmp(Head.streamtype, MT_Audio, strlen(MT_Audio)) == 0)
		{
			PacketFormatClear(&s->Stream.Format);
			s->Stream.Format.Type = PACKET_AUDIO;
			s->Stream.Format.Format.Audio.Format = 0;
			for (i=0;i<4;++i)
				if (Head.subtype[i])
					s->Stream.Format.Format.Audio.Format = s->Stream.Format.Format.Audio.Format*16+Hex(Head.subtype[i]);
			s->Stream.Format.Format.Audio.Channels = INT16LE(Head.format.audio.channels);
			s->Stream.Format.Format.Audio.Bits = INT16LE(Head.bits_per_sample);
			s->Stream.Format.Format.Audio.BlockAlign = INT16LE(Head.format.audio.blockalign);
			s->Stream.Format.ByteRate = INT32LE(Head.format.audio.avgbytespersec);
			s->Stream.Format.Format.Audio.SampleRate = (int)INT64LE(Head.samples_per_unit);

			s->MediaRateDen = INT64LE(Head.time_unit)*TICKSPERSEC;
			s->MediaRateNum = INT64LE(Head.samples_per_unit) * 10000000;
			s->DefaultLen = INT32LE(Head.default_len);

			i = Length - (sizeof(ogm_header)+1);
			if (i && PacketFormatExtra(&s->Stream.Format,i))
				memcpy(s->Stream.Format.Extra,Data+1+sizeof(ogm_header),s->Stream.Format.ExtraLength);
			return 1;
		}

		if (strncmp(Data+1, MT_Text,  strlen(MT_Text)) == 0)
		{
			PacketFormatClear(&s->Stream.Format);
			s->Stream.Format.Type = PACKET_SUBTITLE;
			s->Stream.Format.Format.Subtitle.FourCC = SUBTITLE_OEM; //???

			s->MediaRateDen = INT64LE(Head.time_unit)*TICKSPERSEC;
			s->MediaRateNum = INT64LE(Head.samples_per_unit) * 10000000;
			s->DefaultLen = INT32LE(Head.default_len);

			i = Length - (sizeof(ogm_header)+1);
			if (i && PacketFormatExtra(&s->Stream.Format,i))
				memcpy(s->Stream.Format.Extra,Data+1+sizeof(ogm_header),s->Stream.Format.ExtraLength);
			return 1;
		}
	}

	return 0;
}

static void SendComments(oggstream* Stream)
{
	tchar_t	s[256];
	if (Stream->Stream.Comment.Node)
	{
		int No;
		for (No=0;No<Stream->Comment.comments;++No)
		{
			UTF8ToTcs(s,TSIZEOF(s),Stream->Comment.user_comments[No]);
			Stream->Stream.Comment.Node->Set(Stream->Stream.Comment.Node,Stream->Stream.Comment.No,s,sizeof(s));
		}
	}
}

static bool_t VorbisHeader(ogg* p,oggstream* s)
{
	tchar_t URL[MAXPATH];

	if (vorbis_synthesis_headerin(&s->Info,&s->Comment,&s->OggPacket)<0)
		return 0;

	PacketFormatClear(&s->Stream.Format);
	s->Stream.Format.Type = PACKET_AUDIO;
	s->Stream.Format.Format.Audio.Channels = s->Info.channels;
	s->Stream.Format.Format.Audio.SampleRate = s->Info.rate;
	s->Stream.Format.ByteRate = s->Info.bitrate_nominal >> 3;

	if (p->Format.Reader->Input->Get(p->Format.Reader->Input,STREAM_URL,URL,sizeof(URL))==ERR_NONE &&
		CheckExts(URL,T("ogg:A")) != 0)
		s->Stream.Format.Format.Audio.Format = AUDIOFMT_VORBIS_INTERNAL_AUDIO;
	else
		s->Stream.Format.Format.Audio.Format = AUDIOFMT_VORBIS_INTERNAL_VIDEO;

	s->Vorbis = 1;
	s->MediaRateDen = TICKSPERSEC;
	s->MediaRateNum = s->Info.rate;
	return 1;
}

static int FillQueue(ogg* p,format_reader* Reader)
{
	for (;;)
	{
		int Bytes = ogg_sync_pageseek(p->OggSync,&p->OggPage);

		if (Bytes == 0) // need more data
		{
			int Result;
			format_buffer* Buffer;

			if (!Reader->BufferAvailable && (!p->Format.SyncMode || p->Format.SyncRead<=0))
				return ERR_NEED_MORE_DATA;

			Buffer = Format_BufferRemove(Reader);
			if (!Buffer && p->Format.SyncMode && p->Format.SyncRead>0 && Format_ReadBuffer(Reader,0))
				Buffer = Format_BufferRemove(Reader);

			Result = AddBuffer(p,Buffer);
			if (Result != ERR_NONE)
				return Result;
		}
		else		
		if (Bytes < 0)
			Reader->FilePos -= Bytes;
		else
		if (Bytes > 0)
		{
			int StreamNo;
			oggstream* s;
			int Id;
			
			Reader->FilePos += Bytes;

			Id = ogg_page_serialno(&p->OggPage);
		
			DEBUG_MSG4(DEBUG_FORMAT,T("OGG Page id:%d size:%d gran:%d filepos:%d"),Id,p->OggPage.body_len,(int)ogg_page_granulepos(&p->OggPage),Reader->FilePos - Bytes);

			for (StreamNo=0;StreamNo<p->Format.StreamCount;++StreamNo)
				if (p->Format.Streams[StreamNo]->Id == Id)
					break;

			if (StreamNo==p->Format.StreamCount)
			{
				// check for restarted audio http streaming (comments changed)
				if (p->Format.StreamCount==1 && 
					p->Format.Streams[0]->Format.Type == PACKET_AUDIO &&
					p->Format.Streams[0]->LastTime>0)
				{
					StreamNo = 0;
					s = (oggstream*) p->Format.Streams[0];
					if (s->Vorbis)
					{
						// vorbis decoder have to release s->Info
						s->Stream.Format.Extra = NULL;
						s->Stream.Format.ExtraLength = 0;
						ConnectionUpdate((node*)&p->Format,FORMAT_STREAM+0,s->Stream.Pin.Node,s->Stream.Pin.No);
					}
					FreeStream(p,s);
				}
				else
				{
					s = (oggstream*) Format_AddStream(&p->Format,sizeof(oggstream));
					if (!s)	continue;
				}

				// init stream
				s->Stream.Id = Id;
				s->OggStream = ogg_stream_create(Id);
				s->NeedMorePage = 1;
				s->MediaTime = 0;
				s->Invalid = 0;
				s->Vorbis = 0;
				s->Native = 0;
				s->PacketNo = 0;

				vorbis_info_init(&s->Info);
				vorbis_comment_init(&s->Comment);
			}

			s = (oggstream*) p->Format.Streams[StreamNo];

			if (s->Invalid) // drop invalid streams
				continue;

			if (s->PacketNo>=3)
			{
				if (!s->Stream.Pin.Node) // drop unused streams
					continue;

				if (p->Format.InSeek)
				{
					// reftime needed for SeekByPacket
					if ((s->MediaTime = ogg_page_granulepos(&p->OggPage)) != -1)
					{
						// no need for GlobalOffset here
						s->Stream.LastTime = (tick_t)(s->MediaTime * s->MediaRateDen / s->MediaRateNum);
						if (s->Stream.Format.Type == PACKET_AUDIO)
						{
							s->Stream.LastTime += p->Format.AVOffset;
							if (s->Stream.LastTime < 0)
								s->Stream.LastTime = 0;
						}
					}
				}
			}

			// add page to stream
			if (ogg_stream_pagein(s->OggStream,&p->OggPage) >= 0)
			{
				if (s->PacketNo<3) // header packet needed?
				{
					int i = ogg_stream_packetout(s->OggStream,&s->OggPacket);
					if (i == 0) // stream needs more pages
						continue;
					
					if (++s->PacketNo==1) // first packet?
					{
						ogg_reference* Ref;
						const void* Data;
						int Length;

						if (i < 0)
						{
							// first header packet is a must have
							s->Invalid = 1;
							continue;
						}

						if (p->Format.UseBufferBlock)
						{
							for (Length=0,Ref=s->OggPacket.packet;Ref;Ref=Ref->next)
								Length += Ref->length;

							if (s->Stream.BufferBlockLength<Length && !Format_AllocBufferBlock(&p->Format,&s->Stream,Length))
							{
								Length = 0;
								Data = NULL;
							}
							else
							{
								for (Length=0,Ref=s->OggPacket.packet;Ref;Ref=Ref->next)
								{
									WriteBlock(&s->Stream.BufferBlock,Length,Ref->buffer->data + Ref->begin,Ref->length);
									Length += Ref->length;
								}
								Data = s->Stream.BufferBlock.Ptr;
							}
						}
						else
						{
							BufferDrop(&s->Stream.BufferMem);
							for (Ref=s->OggPacket.packet;Ref;Ref=Ref->next)
								BufferWrite(&s->Stream.BufferMem,Ref->buffer->data + Ref->begin, Ref->length, 16384);
							Data = s->Stream.BufferMem.Data;
							Length = s->Stream.BufferMem.WritePos;
						}

						if (OGMHeader(p,s,(char*)Data,Length) || SpeexHeader(p,s,(char*)Data,Length))
						{
							PacketFormatDefault(&s->Stream.Format);
							s->PacketNo = 3; // no more headers
						}
						else
						if (!VorbisHeader(p,s))
						{
							s->Invalid = 1;
							continue;
						}

						while (s->MediaRateNum > (1<<30))
						{
							s->MediaRateDen >>= 1;
							s->MediaRateNum >>= 1;
						}

						Format_PrepairStream(&p->Format,&s->Stream);
						continue;
					}
					else
					{
						assert(s->Vorbis);

						// error in second or third header packet will not cause fatal error
						vorbis_synthesis_headerin(&s->Info,&s->Comment,&s->OggPacket); 

						if (s->PacketNo == 3)
						{
							// got the three header packets: reinit codec with vorbis_info
							s->Stream.Format.Extra = &s->Info;
							s->Stream.Format.ExtraLength = -1;
							ConnectionUpdate((node*)&p->Format,FORMAT_STREAM+StreamNo,s->Stream.Pin.Node,s->Stream.Pin.No);
							SendComments(s);
						}
						continue;
					}
				}

				s->NeedMorePage = 0;
				break;
			}
		}
	}
	return ERR_NONE;
}

static int Process(ogg* p,oggstream* Stream)
{
	int Result;
	int No;
	int Burst = 1;

	if (Stream->Invalid)
		return ERR_NONE;
	
	if (Stream->Stream.Pending)
	{
		Result = Format_Send(&p->Format,&Stream->Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			return Result;
	}

	// process a limited number of packets at once (other streams need cpu time too)

	Result = ERR_NONE;
	Burst = Stream->Stream.PacketBurst;

	for (No=0;No<Burst;++No)
	{
		while (Stream->NeedMorePage)
		{
			Result = FillQueue(p,Stream->Stream.Reader);

			if (Result == ERR_NEED_MORE_DATA && Stream->Stream.Reader->NoMoreInput)
				Result = Format_CheckEof(&p->Format,&Stream->Stream);

			if (Result != ERR_NONE || (p->Format.Bench && Stream->NeedMorePage))
				return Result;
		}

		// sync: only SyncStream is processed
		if (p->Format.SyncMode && Stream != (oggstream*)p->Format.SyncStream)
			return ERR_NEED_MORE_DATA;

		Result = ogg_stream_packetout(Stream->OggStream,&Stream->OggPacket);
		if (Result == 0)
		{
			// stream needs more pages
			Stream->NeedMorePage = 1;
			if (p->Format.Bench) break; // benchmark mode: no additional pages are added in this loop
			--No;
			continue;
		}
		if (Result < 0) // packet hole or span
			continue;
	
		DEBUG_MSG3(DEBUG_FORMAT,T("OGG Packet id:%d size:%d gran:%d"),Stream->Stream.Id,Stream->OggPacket.bytes,(int)Stream->OggPacket.granulepos);

		if (!Stream->Vorbis && Stream->PacketNo==3)
		{
			// non vorbis: second packet can be a comment (after first header)
			++Stream->PacketNo;

			if (Stream->Native)
			{
				oggpack_buffer opb;
			    oggpack_readinit(&opb,Stream->OggPacket.packet);
				if (_vorbis_unpack_comment(&Stream->Comment,&opb,0)>=0)
					SendComments(Stream);
				continue; // always assume comment packet
			}
			else
			{
				Stream->Info.rate=1;
				if (vorbis_synthesis_headerin(&Stream->Info,&Stream->Comment,&Stream->OggPacket) >= 0)
				{
					SendComments(Stream);
					continue;
				}
			}
		}

		// build output packet

		if (Stream->OggPacket.granulepos >= 0)
			Stream->MediaTime = Stream->OggPacket.granulepos;

		if (Stream->MediaTime >= 0)
		{
			tick_t t = (tick_t)((Stream->MediaTime * Stream->MediaRateDen) / Stream->MediaRateNum);
			t += p->Format.GlobalOffset;
			if (Stream->Stream.Format.Type == PACKET_AUDIO)
			{
				t += p->Format.AVOffset;
				if (t<0) t=0;
			}
			if (Stream->Stream.LastTime>t && t-p->Format.GlobalOffset<TICKSPERSEC)
				Format_TimeStampRestarted(&p->Format,&Stream->Stream,&t);
			Stream->Stream.LastTime = Stream->Stream.Packet.RefTime = t;
		}
		else
		{
			if (Stream->Stream.LastTime < 0)
				continue;
			Stream->Stream.Packet.RefTime = TIME_UNKNOWN;
		}

		if (!Stream->Vorbis)
		{
			ogg_reference* Ref;
			const uint8_t* Data;
			int Length;
			int Len;
			int PacketTime;

			// merge buffers

			if (p->Format.UseBufferBlock)
			{
				for (Length=0,Ref=Stream->OggPacket.packet;Ref;Ref=Ref->next)
					Length += Ref->length;

				if (Stream->Stream.BufferBlockLength<Length && !Format_AllocBufferBlock(&p->Format,&Stream->Stream,Length))
				{
					Length = 0;
					Data = NULL;
				}
				else
				{
					for (Length=0,Ref=Stream->OggPacket.packet;Ref;Ref=Ref->next)
					{
						WriteBlock(&Stream->Stream.BufferBlock,Length,Ref->buffer->data + Ref->begin,Ref->length);
						Length += Ref->length;
					}
					Data = Stream->Stream.BufferBlock.Ptr;
				}
			}	
			else
			{
				BufferDrop(&Stream->Stream.BufferMem);
				for (Ref=Stream->OggPacket.packet;Ref;Ref=Ref->next)
					BufferWrite(&Stream->Stream.BufferMem,Ref->buffer->data + Ref->begin, Ref->length, 16384);
				Data = (const uint8_t*)Stream->Stream.BufferMem.Data;
				Length = Stream->Stream.BufferMem.WritePos;
			}

			if (Stream->Native)
			{
				Stream->Stream.Packet.Data[0] = Data;
				Stream->Stream.Packet.Length = Length;
				Stream->MediaTime = -1;
			}
			else
			{
				// process OGM information
				if (*Data & PACKET_TYPE_HEADER)
					continue; // not data packet

				Len = (*Data & PACKET_LEN_BITS01) >> 6;
				Len |= (*Data & PACKET_LEN_BITS2) << 1;

				Stream->Stream.Packet.Key = (*Data & PACKET_IS_SYNCPOINT) != 0;
				Stream->Stream.Packet.Data[0] = Data + 1 + Len;
				Stream->Stream.Packet.Length = Length - 1 - Len;

				if (!Len)
					PacketTime = Stream->DefaultLen;
				else
					for (PacketTime=0;Len;--Len)
					{
						PacketTime <<= 8;
						PacketTime |= Data[Len];
					}

				Stream->MediaTime += PacketTime;
			}
		}
		else
		{
			// send oggpacket directly 
			if (!Stream->OggPacket.packet)
				continue;

			Stream->Stream.Packet.Data[0] = &Stream->OggPacket;
			Stream->Stream.Packet.Length = sizeof(Stream->OggPacket);
			Stream->MediaTime = -1;
		}
		Stream->Stream.Pending = 1;

		Result = Format_Send(&p->Format,&Stream->Stream);

		if (Result == ERR_BUFFER_FULL || Result == ERR_SYNCED)
			break;

		if (Stream->Stream.State.DropLevel==2)
			Burst = Stream->Stream.PacketBurst*2; // try to catch up
	}

	// buffer full: there was possibly some processing so don't allow sleeping
	// need data: burst count exceeded, but it doesn't mean there is no data left in buffers
	if (Result == ERR_BUFFER_FULL || Result == ERR_NEED_MORE_DATA)
		Result = ERR_NONE;

	return Result;
}

static int Create(ogg* p)
{
	p->Format.Init = (fmtfunc)Init;
	p->Format.Done = (fmtvoid)Done;
	p->Format.Seek = (fmtseek)Format_SeekByPacket;
	p->Format.FillQueue = (fmtfill)FillQueue;
	p->Format.ReadPacket = (fmtreadpacket)ReadPacketTime;
	p->Format.ReleaseStream = (fmtstream)ReleaseStream;
	p->Format.AfterSeek = (fmtvoid)AfterSeek;
	p->Format.FreeStream = (fmtstream)FreeStream;
	p->Format.Process = (fmtstreamprocess)Process;
	p->Format.DisableReader = 1;
	p->Format.Sended = NULL;
	return ERR_NONE;
}

static const nodedef OGG = 
{
	sizeof(ogg),
	OGG_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void OGG_Init()
{
	NodeRegisterClass(&OGG);
}

void OGG_Done()
{
	NodeUnRegisterClass(OGG_ID);
}
