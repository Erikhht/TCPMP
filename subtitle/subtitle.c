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
 * $Id: subtitle.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "stdafx.h"

#define SUBTITLE_INPUT		0x100
#define SUBTITLE_OUTPUT		0x101

typedef struct subtitle
{
	node VMT;
	int Format;
	node* Output;

} subtitle;

static int SetFormat( subtitle* p, const int* Format )
{
	if (Format)
	{
		p->Format = *Format;
	}
	return ERR_NONE;
}

static int SetOutput( subtitle* p, node* Node )
{
	p->Output = Node;
	return ERR_NONE;
}

static void Discontinuity( subtitle* p )
{
	//...
}

static int Decode( subtitle* p, const packet* Packet )
{
	char a[256];
	tchar_t s[256];
	int Len;
	
	if (Packet->CurrTime >= 0 && Packet->RefTime >= (Packet->CurrTime + SHOWAHEAD))
		return ERR_BUFFER_FULL;

	//...
	Len = Packet->Length;
	if (Len>255)
		Len=255;
	memcpy(a,Packet->Data[0],Len);
	a[Len] = 0;

	if (p->Format == STRING_UTF8)
		UTF8ToTcs(s,a,256);
	else
		StrToTcs(s,a,256);

	DEBUG_MSG3(-1,T("Subtitle %d %d: %s"),Packet->RefTime,Packet->CurrTime,s);
	return ERR_NONE;
}

const datadef SubtitleParams[] = 
{
	{ SUBTITLE_ID,			TYPE_CLASS },
	{ SUBTITLE_INPUT,		TYPE_PACKET, DF_INPUT, PACKET_SUBTITLE },
	{ SUBTITLE_OUTPUT,		TYPE_NODE, DF_OUTPUT, VOUT_CLASS },

	DATADEF_END
};

static int Enum( subtitle* p, int No, datadef* Param )
{
	return NodeEnumType(&No,Param,NodeParams,FlowParams,SubtitleParams,NULL);
}

static int Get( subtitle* p, int No, void* Data, int Size )
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case NODE_ID: GETVALUE(SUBTITLE_ID,int); break;
	case SUBTITLE_INPUT|PACKET_FORMAT: GETVALUE(p->Format,int); break;
	case SUBTITLE_OUTPUT: GETVALUE(p->Output,node*); break;
	}
	return Result;
}

static int Set( subtitle* p, int No, const void* Data, int Size )
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case SUBTITLE_INPUT | PACKET_FORMAT: 
		if (!Size || Size == sizeof(int))
		{
			if (!Size) Data = NULL;
			Result = SetFormat(p,(const int*)Data);
		}
		break;

	case SUBTITLE_INPUT:
		if (Size == sizeof(packet))
			Result = Decode(p,(const packet*)Data);
		break;

	case SUBTITLE_OUTPUT:
		if (Size == sizeof(node*) && NodeIsClass(*(node**)Data,VOUT_CLASS))
			Result = SetOutput(p,*(node**)Data);
		break;

	case FLOW_EMPTY:
		Discontinuity(p);
		break;
	}

	return Result;
}

static subtitle SubTitle;

void SubTitle_Init()
{
	NodeFill(&SubTitle.VMT,sizeof(subtitle),(nodeenum)Enum,(nodeget)Get,(nodeset)Set);
	NodeRegister(&SubTitle.VMT,PRI_DEFAULT); 
}

void SubTitle_Done()
{
	NodeUnRegister(&SubTitle.VMT);
}

