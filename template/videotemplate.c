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
 * $Id: videotemplate.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "videotemplate.h"

typedef struct video_template
{
	codec Codec;
	nodeget GetInherited;
	nodeset SetInherited;
	
	int BufferSize;
	int YSize;
	uint8_t* Buffer;
	int Pos;

} video_template;

static int UpdateInput( video_template* p )
{
	free(p->Buffer);
	p->Buffer = NULL;

	if (p->Codec.In.Format.Type == PACKET_VIDEO)
	{
		PacketFormatClear(&p->Codec.Out.Format);
		p->Codec.Out.Format.Type = PACKET_VIDEO;
		p->Codec.Out.Format.Format.Video.Pixel.Flags = PF_FOURCC;
		p->Codec.Out.Format.Format.Video.Pixel.FourCC = FOURCC_I420;
		p->Codec.Out.Format.Format.Video.Width = p->Codec.In.Format.Format.Video.Width;
		p->Codec.Out.Format.Format.Video.Height = p->Codec.In.Format.Format.Video.Height;
		p->Codec.Out.Format.Format.Video.Pitch = p->Codec.In.Format.Format.Video.Width;
		
		p->YSize = p->Codec.Out.Format.Format.Video.Width*p->Codec.Out.Format.Format.Video.Height;
		p->BufferSize = p->YSize+(p->YSize/2);
		p->Buffer = (uint8_t*) malloc(p->BufferSize);
		if (!p->Buffer)
			return ERR_OUT_OF_MEMORY;

		memset(p->Buffer,128,p->BufferSize);
		memset(p->Buffer,0,p->YSize);
		p->Pos = 0;
	}

	return ERR_NONE;
}

static int Process( video_template* p, const packet* Packet, const flowstate* State )
{
	if (!Packet)
		return ERR_NEED_MORE_DATA;

	if (State->DropLevel)
		return p->Codec.Out.Process(p->Codec.Out.Pin.Node,NULL,State);

	p->Codec.Packet.RefTime = Packet->RefTime; 
	p->Codec.Packet.Data[0] = p->Buffer;
	p->Codec.Packet.Length = p->BufferSize;

	// draw one pixel in Y plane
	p->Buffer[p->Pos++ % p->YSize] = 255;

	return ERR_NONE;
}

static int Flush( video_template* p )
{
	memset(p->Buffer,0,p->YSize);
	p->Pos = 0;
	return ERR_NONE;
}

static int Create( video_template* p )
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef Template =
{
	sizeof(video_template),
	VIDEO_TEMPLATE_ID,
	CODEC_CLASS,
	PRI_DEFAULT+10, // use higher priorty as defualt MPEG4 codec so this example can override it
	(nodecreate)Create,
	NULL,
};

void Video_Init()
{
	StringAdd(1,VIDEO_TEMPLATE_ID,NODE_NAME,T("Video template codec"));
	StringAdd(1,VIDEO_TEMPLATE_ID,NODE_CONTENTTYPE,T("vcodec/dx50,vcodec/xvid"));
	NodeRegisterClass(&Template); 
}

void Video_Done()
{
	NodeUnRegisterClass(VIDEO_TEMPLATE_ID);
}

