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
 * $Id: oggembedded.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "vorbis.h"
#include "tremor/ogg.h"
#include "tremor/ivorbiscodec.h"

// Ogg file format embedded inside an audio stream

typedef struct oggembedded
{
	codec Codec;

	int OggStreamNo;
	ogg_sync_state*	 OggSync;
	ogg_stream_state* OggStream;
	ogg_page         OggPage;
	ogg_packet       OggPacket;

	vorbis_info      Info;
	vorbis_comment   Comment;

	// output data
	tick_t RefTime;
	bool_t Reset;
	bool_t NeedMorePage;
	int Bytes;   // bytes in Sync buffer
	int PageNo;

} oggembedded;

static int UpdateInput( oggembedded* p )
{
	vorbis_comment_clear(&p->Comment);
	vorbis_info_clear(&p->Info);

	ogg_packet_release(&p->OggPacket);
	ogg_page_release(&p->OggPage);

	if (p->OggStream)
		ogg_stream_destroy(p->OggStream);

	if (p->OggSync)
		ogg_sync_destroy(p->OggSync);

	p->OggSync = NULL;
	p->OggStream = NULL;

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		PacketFormatClear(&p->Codec.Out.Format);
		p->Codec.Out.Format.Type = PACKET_AUDIO;
		p->Codec.Out.Format.Format.Audio.Format = AUDIOFMT_VORBIS_INTERNAL_VIDEO;

		p->Codec.Packet.Data[0] = &p->OggPacket;
		p->Codec.Packet.Length = sizeof(p->OggPacket);

		p->RefTime = TIME_UNKNOWN;
		p->Bytes = 0;
		p->OggStreamNo = 0;
		p->NeedMorePage = 1;
		p->PageNo = 0;
		p->OggSync = ogg_sync_create();

		vorbis_info_init(&p->Info);
		vorbis_comment_init(&p->Comment);
	}

	return ERR_NONE;
}

static int Process( oggembedded* p, const packet* Packet, const flowstate* State )
{
	if (Packet)
	{
		void* Ptr;

		// set new reftime (rewind with bytes already buffered)
		if (Packet->RefTime >= 0)
			p->RefTime = Packet->RefTime - Scale(p->Bytes,TICKSPERSEC,p->Codec.Out.Format.ByteRate);

		// add new packet to oggsync
		Ptr = ogg_sync_bufferin(p->OggSync,Packet->Length);
		if (Ptr)
		{
			p->Bytes += Packet->Length;
			memcpy(Ptr,Packet->Data[0],Packet->Length);
			ogg_sync_wrote(p->OggSync,Packet->Length);
		}

		p->NeedMorePage = 1;
	}

	for (;;)
	{
		while (p->NeedMorePage)
		{
			int StreamNo;
			do
			{
				int Bytes = ogg_sync_pageseek(p->OggSync,&p->OggPage);
				if (Bytes == 0)
					return ERR_NEED_MORE_DATA; // missing data

				if (Bytes < 0) 
				{
					// corrupt data
					p->Bytes += Bytes; //sub skipped bytes 
					StreamNo = -1;
				}
				else
				{
					p->Bytes -= Bytes;
					StreamNo = ogg_page_serialno(&p->OggPage);
					if (!p->OggStream)
					{
						//add stream
						p->OggStream = ogg_stream_create(StreamNo);
						p->OggStreamNo = StreamNo;
					}
				}
			}
			while (StreamNo != p->OggStreamNo);

			if (ogg_stream_pagein(p->OggStream,&p->OggPage) >= 0)
				p->NeedMorePage = 0;
		}

		while (!p->NeedMorePage)
		{
			int Result = ogg_stream_packetout(p->OggStream,&p->OggPacket);

			if (Result == 0)
				p->NeedMorePage = 1; // set need more page
			else
			if (Result > 0) 
			{
				// valid packet
				if (p->PageNo < 3) // need 3 header packets
				{
					// first page is vital
					if (vorbis_synthesis_headerin(&p->Info,&p->Comment,&p->OggPacket)>=0 || p->PageNo>0)
					{
						if (++p->PageNo == 3)
						{
							tchar_t Comment[256];
							int No;

							if (p->Codec.Comment.Node)
								for (No=0;No<p->Comment.comments;++No)
								{
									StrToTcs(Comment,TSIZEOF(Comment),p->Comment.user_comments[No]);
									p->Codec.Comment.Node->Set(p->Codec.Comment.Node,p->Codec.Comment.No,Comment,sizeof(Comment));
								}

							p->Codec.Out.Format.Extra = &p->Info;
							p->Codec.Out.Format.ExtraLength = -1;

							p->Codec.In.Format.ByteRate = p->Codec.Out.Format.ByteRate = p->Info.bitrate_nominal >> 3;
							p->Codec.In.Format.Format.Audio.SampleRate = p->Codec.Out.Format.Format.Audio.SampleRate = p->Info.rate;
							p->Codec.In.Format.Format.Audio.Channels = p->Codec.Out.Format.Format.Audio.Channels = p->Info.channels;

							ConnectionUpdate(&p->Codec.Node,CODEC_OUTPUT,p->Codec.Out.Pin.Node,p->Codec.Out.Pin.No);
						}
					}
				}
				else
				{
					p->Codec.Packet.RefTime = p->RefTime;
					if (p->RefTime >= 0)
					{
						if (p->Codec.Out.Format.ByteRate)
							p->RefTime += Scale(p->OggPacket.bytes,TICKSPERSEC,p->Codec.Out.Format.ByteRate);
						else
							p->RefTime = TIME_UNKNOWN;
					}
					return ERR_NONE; 
				}
			}
		}
	}

	return ERR_NEED_MORE_DATA;
}

static int Flush(oggembedded* p)
{
	if (p->OggSync)
		ogg_sync_reset(p->OggSync);

	if (p->OggStream)
		ogg_stream_reset_serialno(p->OggStream,p->OggStreamNo);

	p->Bytes = 0;
	return ERR_NONE;
}

static int Create(oggembedded* p)
{
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UseComment = 1;
	return ERR_NONE;
}

static const nodedef OGGEmbedded =
{
	sizeof(oggembedded),
	OGGEMBEDDED_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void OGGEmbedded_Init()
{
	NodeRegisterClass(&OGGEmbedded);
}

void OGGEmbedded_Done()
{
	NodeUnRegisterClass(OGGEMBEDDED_ID);
}
