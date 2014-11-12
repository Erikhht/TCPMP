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
 * $Id: audio.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable AOutParams[] = 
{
	{ AOUT_VOLUME,	TYPE_INT, DF_HIDDEN },
	{ AOUT_MUTE,	TYPE_BOOL, DF_HIDDEN },
	{ AOUT_STEREO,	TYPE_INT, DF_HIDDEN },
	{ AOUT_QUALITY, TYPE_INT, DF_HIDDEN },
	{ AOUT_MODE,	TYPE_BOOL, DF_HIDDEN },
	{ AOUT_TIMER,	TYPE_NODE, DF_SETUP|DF_RDONLY|DF_HIDDEN, TIMER_CLASS },

	DATATABLE_END(AOUT_CLASS)
};

int AOutEnum(void* p, int* No, datadef* Param)
{
	if (OutEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,AOutParams);
}

static int Create(node* p)
{
	p->Enum = (nodeenum)AOutEnum;
	return ERR_NONE;
}

static const nodedef AOut =
{
	CF_ABSTRACT,
	AOUT_CLASS,
	OUT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void Audio_Init()
{
	NodeRegisterClass(&AOut);
}

void Audio_Done()
{
	NodeUnRegisterClass(AOUT_CLASS);
}

