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
 * $Id: stdafx.c 334 2005-11-07 15:53:50Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "adpcm.h"
#include "law.h"
#include "mjpeg.h"
#include "tiff.h"
#include "png.h"

LIBGCC

int DLLRegister(int Version)
{
	if (Version != CONTEXT_VERSION) 
		return ERR_NOT_COMPATIBLE;
	MJPEG_Init();
	TIFF_Init();
	PNG_Init();
#if !defined(TARGET_PALMOS)
	Law_Init();
	ADPCM_Init();
#endif
	return ERR_NONE;
}

void DLLUnRegister()
{
	MJPEG_Done();
	TIFF_Done();
	PNG_Done();
#if !defined(TARGET_PALMOS)
	Law_Done();
	ADPCM_Done();
#endif
}

//void DLLTest() {}
