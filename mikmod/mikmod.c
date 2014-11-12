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
 * $Id: mikmod.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "libmikmod/include/mikmod_build.h"
#include "mikmod.h"

typedef struct mikmod 
{
	format_base Format;

} mikmod;

static int Create(mikmod* p)
{
	//todo...
	return ERR_NONE;
}

static const nodedef MikMod =
{
	sizeof(mikmod),
	MIKMOD_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void MikModClass_Init()
{
	MikMod_Init(NULL);
	MikMod_RegisterAllLoaders();
	NodeRegisterClass(&MikMod);
}

void MikModClass_Done()
{
	NodeUnRegisterClass(MIKMOD_ID);
	MikMod_Exit(NULL);
}

void* _mm_malloc(size_t size)
{
	void* p = malloc(size);
	if (p)
		memset(p,0,size);
	return p;
}

void* _mm_calloc(size_t nitems,size_t size)
{
	void* p = malloc(nitems*size);
	if (p)
		memset(p,0,nitems*size);
	return p;
}

int random() { return rand(); }
