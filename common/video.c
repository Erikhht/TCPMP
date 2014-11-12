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
 * $Id: video.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable VOutParams[] = 
{
	{ VOUT_PRIMARY,		TYPE_BOOL, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ VOUT_OVERLAY,		TYPE_BOOL, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ VOUT_IDCT,		TYPE_NODE, DF_SETUP | DF_RDONLY | DF_HIDDEN, IDCT_CLASS },
	{ VOUT_FX,			TYPE_BLITFX, DF_HIDDEN },
	{ VOUT_WND,			TYPE_INT, DF_HIDDEN },
	{ VOUT_VIEWPORT,	TYPE_RECT, DF_HIDDEN },
	{ VOUT_FULLSCREEN,	TYPE_BOOL, DF_HIDDEN },
	{ VOUT_OUTPUTRECT,	TYPE_RECT, DF_HIDDEN | DF_RDONLY },
	{ VOUT_PLAY,		TYPE_BOOL, DF_HIDDEN },
	{ VOUT_VISIBLE,		TYPE_BOOL, DF_HIDDEN },
	{ VOUT_CLIPPING,	TYPE_BOOL, DF_HIDDEN },
	{ VOUT_COLORKEY,	TYPE_RGB, DF_HIDDEN },
	{ VOUT_ASPECT,		TYPE_FRACTION, DF_HIDDEN },
	{ VOUT_CAPS,		TYPE_INT, DF_HIDDEN },
	{ VOUT_AUTOPREROTATE,TYPE_BOOL, DF_HIDDEN },
	{ VOUT_UPDATING,	TYPE_BOOL, DF_HIDDEN },

	DATATABLE_END(VOUT_CLASS)
};

int VOutEnum(void* p, int* No, datadef* Param)
{
	if (OutEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,VOutParams);
}

static int Create(node* p)
{
	p->Enum = VOutEnum;
	return ERR_NONE;
}

static const nodedef VOut =
{
	CF_ABSTRACT,
	VOUT_CLASS,
	OUT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

//--------------------------------------------------------------

typedef struct vbuffer
{
	codec Codec;
	planes Buf[2];

} vbuffer;

static int CreateBuffer(vbuffer* p)
{
//	p->Codec.Node.Set = (nodeset)BufferSet;
//	p->Codec.ReSend = 

	return ERR_NONE;
}

static const nodedef VBuffer =
{
	sizeof(vbuffer),
	VBUFFER_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)CreateBuffer,
	NULL,
};

//--------------------------------------------------------------

void Video_Init()
{
	NodeRegisterClass(&VOut);
	//NodeRegisterClass(&VBuffer);
}

void Video_Done()
{
	NodeUnRegisterClass(VOUT_CLASS);
	//NodeUnRegisterClass(VBUFFER_CLASS);
}

bool_t VOutIDCT(int Class)
{
	return NodeIsClass(VOUT_IDCT_CLASS(Class),IDCT_CLASS);
}

