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
 * $Id: stdafx.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "win.h"
#include "about.h"
#include "benchresult.h"
#include "mediainfo.h"
#include "settings.h"
#include "win32/interface.h"

DLLEXPORT void Main(const tchar_t* Name,const tchar_t* Version,int Id,const tchar_t* CmdLine)
{
	SAFE_BEGIN
	if (Context_Init(Name,Version,Id,CmdLine,NULL))
	{
		WaitEnd();
		WinPopupClass(INTERFACE_ID,NULL);
		Context_Done();
	}
	SAFE_END
}

int DLLRegister(int Version)
{
	if (Version != CONTEXT_VERSION) 
		return ERR_NOT_COMPATIBLE;
	Win_Init();
	About_Init();
	BenchResult_Init();
	MediaInfo_Init();
	Settings_Init();
	return ERR_NONE;
}

void DLLUnRegister()
{
	About_Done();
	BenchResult_Done();
	MediaInfo_Done();
	Settings_Done();
	Win_Done();
}

#ifndef NO_PLUGINS
//void DLLTest() {}
#endif

