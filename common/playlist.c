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
 * $Id: playlist.c 346 2005-11-21 22:20:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable Params[] = 
{
	{ PLAYLIST_STREAM,		TYPE_NODE, DF_HIDDEN, STREAM_CLASS },

	DATATABLE_END(PLAYLIST_CLASS)
};

static int Enum(playlist* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static int Get(playlist* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLAYLIST_STREAM: GETVALUE(p->Stream,stream*); break;
	}
	return Result;
}

static int UpdateStream(playlist* p)
{
	p->No = -1;
	ParserStream(&p->Parser,p->Stream);
	if (p->UpdateStream)
		return p->UpdateStream(p);
	return ERR_NONE;
}

static int Set(playlist* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLAYLIST_STREAM: SETVALUENULL(p->Stream,stream*,UpdateStream(p),p->Stream=NULL); break;
	case PLAYLIST_DATAFEED: ParserDataFeed(&p->Parser,Data,Size); Result = ERR_NONE; break;
	}
	return Result;
}

static int Create(playlist* p)
{
	p->Enum = (nodeenum)Enum;
	p->Get = (nodeget)Get;
	p->Set = (nodeset)Set;
	return ERR_NONE;
}

static const nodedef Playlist =
{
	sizeof(playlist)|CF_ABSTRACT,
	PLAYLIST_CLASS,
	MEDIA_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void Playlist_Init()
{
	NodeRegisterClass(&Playlist);
}

void Playlist_Done()
{
	NodeUnRegisterClass(PLAYLIST_CLASS);
}
