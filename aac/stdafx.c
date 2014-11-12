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
 * $Id: stdafx.c 286 2005-10-05 14:37:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "faad.h"

LIBGCC
LIBGCCFLOAT

int DLLRegister(int Version)
{
	if (Version != CONTEXT_VERSION) 
		return ERR_NOT_COMPATIBLE;

	FAAD_Init();
	return ERR_NONE;
}

void DLLUnRegister()
{
	FAAD_Done();
}

//void DLLTest() {}
