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
 * $Id: codec.c 438 2005-12-29 17:23:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable CodecParams[] = 
{
	{ CODEC_INPUT,		TYPE_PACKET, DF_INPUT },
	{ CODEC_OUTPUT,		TYPE_PACKET, DF_OUTPUT },
	{ CODEC_COMMENT,	TYPE_COMMENT, DF_OUTPUT },

	DATATABLE_END(CODEC_CLASS)
};

static void ClearInput(codec* p)
{
	PacketFormatClear(&p->In.Format);
	PacketFormatClear(&p->Out.Format);
	p->Out.Process = DummyProcess;
	p->In.Process = DummyProcess;
}

int CodecEnum(codec* p, int* No, datadef* Param)
{
	if (FlowEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	if (NodeEnumTable(No,Param,CodecParams)==ERR_NONE)
	{
		if (!p->UseComment && Param->No==CODEC_COMMENT)
			Param->Flags |= DF_RDONLY;
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}

int CodecGet(codec* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case CODEC_INPUT: GETVALUE(p->In.Pin,pin); break;
	case CODEC_INPUT|PIN_FORMAT: GETVALUE(p->In.Format,packetformat); break;
	case CODEC_INPUT|PIN_PROCESS: GETVALUE(p->In.Process,packetprocess); break;
	case CODEC_OUTPUT: GETVALUE(p->Out.Pin,pin); break;
	case CODEC_OUTPUT|PIN_FORMAT: GETVALUE(p->Out.Format,packetformat); break;
	case CODEC_OUTPUT|PIN_PROCESS: GETVALUE(p->Out.Process,packetprocess); break;
	case CODEC_COMMENT: GETVALUE(p->Comment,pin); break;
	case FLOW_BUFFERED: GETVALUE(p->ReSend!=NULL,bool_t); break;
	}
	return Result;
}

static int Flush(codec* p)
{
	int Result = ERR_NONE;

	p->Pending = 0;
	p->Packet.RefTime = TIME_UNKNOWN;

	if (p->Flush)
		p->Flush(p);

	return Result;
}

static int Process(codec* p, const packet* Packet, const flowstate* State)
{
	flowstate State2;
	int Result;
	if (p->Pending)
	{
		do
		{
			Result = p->Out.Process(p->Out.Pin.Node,&p->Packet,State);
			if (Result == ERR_BUFFER_FULL)
				return Result;
		}
		while (p->Process(p,NULL,NULL) == ERR_NONE);

		p->Pending = 0;
	}

	if (State->DropLevel > 1 && !p->NoHardDropping)
	{
		Flush(p);
		p->Out.Process(p->Out.Pin.Node,NULL,State); // signal dropped packet
		return ERR_NONE;
	}

	Result = p->Process(p,Packet,State);

	if (Result == CODEC_NO_DROPPING)
	{
		State2.CurrTime = State->CurrTime;
		State2.DropLevel = 0;
		State = &State2;
		Result = ERR_NONE;
	}

	if (Result != ERR_NONE)
	{
		if (!Packet)
			return p->Out.Process(p->Out.Pin.Node,NULL,State);
		return Result;
	}

	do
	{
		int n = p->Out.Process(p->Out.Pin.Node,&p->Packet,State);
		if (n == ERR_BUFFER_FULL)
		{
			p->Pending = 1; 
			if (!Packet) //return previous result if Packet already processed
				Result = n;
			break;
		}
		Result = n;
		if (Result == ERR_NONE && State->CurrTime == TIME_SYNC)
		{
			p->Packet.RefTime = TIME_UNKNOWN;
			break;
		}
	}
	while (p->Process(p,NULL,NULL) == ERR_NONE);

	return Result;
}

static int UpdateInput(codec* p)
{
	int Result2;
	int Result = ERR_NONE;
	
	Flush(p);

	if (p->In.Format.Type != PACKET_NONE)
	{
		if (!PacketFormatMatch(p->Node.Class, &p->In.Format))
		{
			ClearInput(p);
			Result = ERR_NOT_SUPPORTED;
		}

		if (p->Process)
			p->In.Process = (packetprocess)Process; // using default codec process
	}

	if (p->UpdateInput)
	{
		Result2 = p->UpdateInput(p);
		if (Result == ERR_NONE)
			Result = Result2;
	}

	if (Result != ERR_NONE)
		ClearInput(p);

	Result2 = ConnectionUpdate(&p->Node,CODEC_OUTPUT,p->Out.Pin.Node,p->Out.Pin.No);
	if (Result == ERR_NONE)
		Result = Result2;

	return Result;
}

int CodecSet(codec* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case CODEC_INPUT: SETVALUE(p->In.Pin,pin,ERR_NONE); break;
	case CODEC_INPUT|PIN_FORMAT: SETPACKETFORMAT(p->In.Format,packetformat,UpdateInput(p)); break;
	case CODEC_OUTPUT: SETVALUE(p->Out.Pin,pin,ERR_NONE); break;
	case CODEC_OUTPUT|PIN_FORMAT: if (p->UpdateOutput) SETPACKETFORMAT(p->In.Format,packetformat,p->UpdateOutput(p)); break;
	case CODEC_OUTPUT|PIN_PROCESS: SETVALUE(p->Out.Process,packetprocess,ERR_NONE); break;
	case CODEC_COMMENT: SETVALUE(p->Comment,pin,ERR_NONE); break;
	case FLOW_FLUSH: Result = Flush(p); break;
	case FLOW_RESEND: if (p->ReSend) Result = p->ReSend(p); break;
	case FLOW_NOT_SUPPORTED: SETVALUE(p->NotSupported,pin,ERR_NONE); break;
	}
	return Result;
}

static int Create(codec* p)
{
	p->Node.Enum = (nodeenum)CodecEnum;
	p->Node.Get = (nodeget)CodecGet;
	p->Node.Set = (nodeset)CodecSet;
	ClearInput(p);
	return ERR_NONE;
}

static void Delete(codec* p)
{
	ClearInput(p);
	if (p->UpdateInput)
		p->UpdateInput(p);
}

static const nodedef Codec =
{
	sizeof(codec)|CF_ABSTRACT,
	CODEC_CLASS,
	FLOW_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void Codec_Init()
{
	NodeRegisterClass(&Codec);
}

void Codec_Done()
{
	NodeUnRegisterClass(CODEC_CLASS);
}
