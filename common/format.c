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
 * $Id: format.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable Params[] = 
{
	{ FORMAT_INPUT,			TYPE_NODE, DF_INPUT|DF_HIDDEN, STREAM_CLASS },
	{ FORMAT_OUTPUT,		TYPE_NODE, DF_HIDDEN, STREAM_CLASS },
	{ FORMAT_DURATION,		TYPE_TICK },
	{ FORMAT_FILEPOS,		TYPE_INT, DF_HIDDEN },
	{ FORMAT_FILESIZE,		TYPE_INT, DF_KBYTE },
	{ FORMAT_AUTO_READSIZE, TYPE_BOOL, DF_HIDDEN },
	{ FORMAT_GLOBAL_COMMENT,TYPE_COMMENT, DF_OUTPUT },
	{ FORMAT_FIND_SUBTITLES,TYPE_BOOL, DF_HIDDEN },
	{ FORMAT_STREAM_COUNT,	TYPE_INT, DF_HIDDEN },

	DATATABLE_END(FORMAT_CLASS)
};

int FormatEnum(void* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static const nodedef Format =
{
	CF_ABSTRACT,
	FORMAT_CLASS,
	MEDIA_CLASS,
	PRI_DEFAULT,
};

static const nodedef Media =
{
	CF_ABSTRACT,
	MEDIA_CLASS,
	NODE_CLASS,
	PRI_DEFAULT,
};

void Format_Init()
{
	NodeRegisterClass(&Media);
	NodeRegisterClass(&Format);
}

void Format_Done()
{
	NodeUnRegisterClass(FORMAT_CLASS);
	NodeUnRegisterClass(MEDIA_CLASS);
}
