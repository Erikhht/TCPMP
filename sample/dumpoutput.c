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
 * $Id: dumpoutput.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "dumpoutput.h"

typedef struct dumpoutput
{
	node Node;
	pin Pin;
	packetformat Format;
	int Total;
	int Dropped;
	stream* File;

} dumpoutput;

static int Process(dumpoutput* p, const packet* Packet, const flowstate* State)
{
	if (Packet)
	{
		if (State->CurrTime >= 0 && Packet->RefTime > State->CurrTime + SHOWAHEAD)
			return ERR_BUFFER_FULL;

		if (State->CurrTime != TIME_RESEND)
		{
			if (p->Format.Type == PACKET_AUDIO)
				p->Total += Packet->Length;
			else
				++p->Total;

			if (p->File)
			{
				int y,h,w,pitch;
				const uint8_t* ptr = Packet->Data[0];
				h = p->Format.Format.Video.Height;
				w = p->Format.Format.Video.Width;
				pitch = p->Format.Format.Video.Pitch;

				DebugMessage(T("y %08x %d %d"),StreamSeek(p->File,0,SEEK_CUR),w,h);
				for (y=0;y<h;++y,ptr+=pitch)
					StreamWrite(p->File,ptr,w);

				ptr = Packet->Data[1];
				h >>= 1;
				w >>= 1;
				pitch >>= 1;

				DebugMessage(T("u %08x %d %d"),StreamSeek(p->File,0,SEEK_CUR),w,h);
				for (y=0;y<h;++y,ptr+=pitch)
					StreamWrite(p->File,ptr,w);

				DebugMessage(T("v %08x %d %d"),StreamSeek(p->File,0,SEEK_CUR),w,h);
				ptr = Packet->Data[2];
				for (y=0;y<h;++y,ptr+=pitch)
					StreamWrite(p->File,ptr,w);

			}
		}
	}
	else
	if (State->DropLevel)
		++p->Dropped;

	return ERR_NONE;
}

static int UpdateInput(dumpoutput* p)
{
	p->Total = 0;
	p->Dropped = 0;
	if ((p->Format.Type == PACKET_SUBTITLE) ||
		(p->Format.Type == PACKET_VIDEO && (p->Node.Class != DUMPVIDEO_ID || Compressed(&p->Format.Format.Video.Pixel))) ||
	    (p->Format.Type == PACKET_AUDIO && (p->Node.Class != DUMPAUDIO_ID || p->Format.Format.Audio.Format != AUDIOFMT_PCM)))
	{
		PacketFormatClear(&p->Format);
		return ERR_INVALID_DATA;
	}

	if (!p->File)
	{
		tchar_t URL[MAXPATH];
		node* Input = NULL;
		node* Player = Context()->Player;
		Player->Get(Player,PLAYER_INPUT,&Input,sizeof(Input));
		if (Input && Input->Get(Input,STREAM_URL,URL,sizeof(URL))==ERR_NONE)
		{
			SetFileExt(URL,TSIZEOF(URL),T("yuv"));
			p->File = StreamOpen(URL,1);
		}
	}

	return ERR_NONE;
}

static int Get(dumpoutput* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT: GETVALUE(p->Pin,pin); break;
	case OUT_OUTPUT|PIN_FORMAT:
	case OUT_INPUT|PIN_FORMAT: GETVALUE(p->Format,packetformat); break;
	case OUT_INPUT|PIN_PROCESS: GETVALUE((packetprocess)Process,packetprocess); break;
	case OUT_TOTAL: GETVALUE(p->Total,int); break;
	case OUT_DROPPED: GETVALUE(p->Dropped,int); break;
	case VOUT_CAPS: GETVALUE(0,int); break;
	}
	return Result;
}

static int Set(dumpoutput* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT: SETVALUE(p->Pin,pin,ERR_NONE); break;
	case OUT_INPUT|PIN_FORMAT: SETPACKETFORMAT(p->Format,packetformat,UpdateInput(p)); break;
	case OUT_TOTAL: SETVALUE(p->Total,int,ERR_NONE); break;
	case OUT_DROPPED: SETVALUE(p->Dropped,int,ERR_NONE); break;
	}
	return Result;
}

static void Delete(dumpoutput* p)
{
	if (p->File)
	{
		StreamClose(p->File);
		p->File = NULL;
	}
}

static int Create(dumpoutput* p)
{
	p->Node.Get = (nodeget)Get;
	p->Node.Set = (nodeset)Set;
	return ERR_NONE;
}

static const nodedef DumpAudio =
{
	sizeof(dumpoutput),
	DUMPAUDIO_ID,
	AOUT_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	(nodedelete)Delete,
};

static const nodedef DumpVideo =
{
	sizeof(dumpoutput),
	DUMPVIDEO_ID,
	VOUT_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void DumpOutput_Init()
{
	NodeRegisterClass(&DumpVideo);
	NodeRegisterClass(&DumpAudio);
}

void DumpOutput_Done()
{
	NodeUnRegisterClass(DUMPAUDIO_ID);
	NodeUnRegisterClass(DUMPVIDEO_ID);
}
