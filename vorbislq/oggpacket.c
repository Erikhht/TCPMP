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
 * $Id: oggpacket.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "vorbis.h"
#include "tremor/ogg.h"
#include "tremor/ivorbiscodec.h"

typedef struct oggpacket
{
	codec Codec;
	vorbis_info     Info;
	vorbis_comment  Comment;
	ogg_reference	OggRef;
	ogg_buffer		OggBuffer;
	ogg_packet      OggPacket;

} oggpacket;

static bool_t SynthesisHeader(oggpacket* p)
{
	++p->OggPacket.packetno;
	p->OggPacket.b_o_s = p->OggPacket.packetno == 0;
	p->OggPacket.bytes = p->OggPacket.packet->length;
	return vorbis_synthesis_headerin(&p->Info,&p->Comment,&p->OggPacket) >= 0;
}

static int Process( oggpacket* p, const packet* Packet, const flowstate* State )
{
	if (Packet)
	{
		p->OggBuffer.data = (uint8_t*)Packet->Data[0];
		p->OggPacket.bytes = p->OggRef.length = p->OggBuffer.size = Packet->Length;
		++p->OggPacket.packetno;

		p->Codec.Packet.RefTime = Packet->RefTime;
		p->Codec.Packet.Key = Packet->Key;
		Packet = &p->Codec.Packet;
	}
	return p->Codec.Out.Process(p->Codec.Out.Pin.Node,Packet,State);
}

static int UpdateInput( oggpacket* p )
{
	vorbis_comment_clear(&p->Comment);
	vorbis_info_clear(&p->Info);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		int	i,j,n;

		if (p->Codec.In.Format.ExtraLength < 1 || !p->Codec.In.Format.Extra)
			return ERR_INVALID_DATA;

		p->Codec.Packet.Data[0] = &p->OggPacket;
		p->Codec.Packet.Length = sizeof(p->OggPacket);

		vorbis_info_init(&p->Info);
		vorbis_comment_init(&p->Comment);

		memset(&p->OggPacket,0,sizeof(ogg_packet));

		p->OggBuffer.data = (uint8_t*) p->Codec.In.Format.Extra;
		p->OggBuffer.size = p->Codec.In.Format.ExtraLength;
		p->OggBuffer.refcount = 1;

		p->OggRef.buffer = &p->OggBuffer;
		p->OggRef.next = NULL;

		p->OggPacket.packet = &p->OggRef;
		p->OggPacket.packetno = -1; 

		n = p->OggBuffer.data[0];
		i = 1+n;
		j = 1;

		while (p->OggPacket.packetno < 3 && n>=j)
		{
			p->OggRef.begin = i;
			p->OggRef.length = 0;
			do
			{
				p->OggRef.length += p->OggBuffer.data[j];
			}
			while (p->OggBuffer.data[j++] == 255 && n>=j);
			i += p->OggRef.length;

			if (i > p->OggBuffer.size)
				return ERR_INVALID_DATA;

			if (!SynthesisHeader(p) && p->OggPacket.packetno==0)
				return ERR_INVALID_DATA;
		}
		
		if (p->OggPacket.packetno < 3)
		{
			p->OggRef.begin = i;
			p->OggRef.length = p->OggBuffer.size - i; 

			SynthesisHeader(p);
		}

		if (p->Codec.Comment.Node)
			for (i=0;i<p->Comment.comments;++i)
			{
				tchar_t Comment[256];
				StrToTcs(Comment,TSIZEOF(Comment),p->Comment.user_comments[i]);
				p->Codec.Comment.Node->Set(p->Codec.Comment.Node,p->Codec.Comment.No,Comment,sizeof(Comment));
			}

		PacketFormatClear(&p->Codec.Out.Format);
		p->Codec.Out.Format.Type = PACKET_AUDIO;
		p->Codec.Out.Format.Format.Audio.Format = AUDIOFMT_VORBIS_INTERNAL_VIDEO;
		p->Codec.Out.Format.Extra = &p->Info;
		p->Codec.Out.Format.ExtraLength = -1;

		p->Codec.In.Format.ByteRate = p->Codec.Out.Format.ByteRate = p->Info.bitrate_nominal >> 3;
		p->Codec.In.Format.Format.Audio.SampleRate = p->Codec.Out.Format.Format.Audio.SampleRate = p->Info.rate;
		p->Codec.In.Format.Format.Audio.Channels = p->Codec.Out.Format.Format.Audio.Channels = p->Info.channels;

		p->OggPacket.b_o_s = 0;
		p->OggPacket.packetno = 0;
		p->OggRef.begin = 0;

		p->Codec.In.Process = (packetprocess)Process;
	}

	return ERR_NONE;
}

static int Create(oggpacket* p)
{
	p->Codec.UseComment = 1;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	return ERR_NONE;
}

static const nodedef OGGPacket =
{
	sizeof(oggpacket),
	OGGPACKET_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void OGGPacket_Init()
{
	NodeRegisterClass(&OGGPacket);
}

void OGGPacket_Done()
{
	NodeUnRegisterClass(OGGPACKET_ID);
}


