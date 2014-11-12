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
 * $Id: nulloutput.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

typedef struct nulloutput
{
	node Node;
	pin Pin;
	packetformat Format;
	int Total;
	int Dropped;

} nulloutput;

static int Process(nulloutput* p, const packet* Packet, const flowstate* State)
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
		}
	}
	else
	if (State->DropLevel)
		++p->Dropped;

	return ERR_NONE;
}

static int UpdateInput(nulloutput* p)
{
	p->Total = 0;
	p->Dropped = 0;
	if ((p->Format.Type == PACKET_SUBTITLE) ||
		(p->Format.Type == PACKET_VIDEO && (p->Node.Class != NULLVIDEO_ID || Compressed(&p->Format.Format.Video.Pixel))) ||
	    (p->Format.Type == PACKET_AUDIO && (p->Node.Class != NULLAUDIO_ID || p->Format.Format.Audio.Format != AUDIOFMT_PCM)))
	{
		PacketFormatClear(&p->Format);
		return ERR_INVALID_DATA;
	}
	return ERR_NONE;
}

static int Get(nulloutput* p, int No, void* Data, int Size)
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

static int Set(nulloutput* p, int No, const void* Data, int Size)
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

static int Create(nulloutput* p)
{
	p->Node.Get = (nodeget)Get;
	p->Node.Set = (nodeset)Set;
	return ERR_NONE;
}

static const nodedef NullAudio =
{
	sizeof(nulloutput),
	NULLAUDIO_ID,
	AOUT_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	NULL,
};

static const nodedef NullVideo =
{
	sizeof(nulloutput),
	NULLVIDEO_ID,
	VOUT_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	NULL,
};

void NullOutput_Init()
{
	NodeRegisterClass(&NullVideo);
	NodeRegisterClass(&NullAudio);
}

void NullOutput_Done()
{
	NodeUnRegisterClass(NULLAUDIO_ID);
	NodeUnRegisterClass(NULLVIDEO_ID);
}
