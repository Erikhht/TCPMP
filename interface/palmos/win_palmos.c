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
 * $Id: win_palmos.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"

#if defined(TARGET_PALMOS)

void WinTitle(win* p,const tchar_t* LockedMsg)
{
}

int WinClose(win* p)
{
	return ERR_NONE;
}

int WinHandler(win* p,void* Event)
{
	return 0;
}

void WinBeginUpdate(win* p)
{
}

void WinEndUpdate(win* p)
{
}

int WinMenuFind(win* p,int Id)
{
	return 0;
}

void WinMenuInsert(win* p,int No,int PrevId,int Id,const tchar_t* LockedMsg)
{
}

bool_t WinMenuDelete(win* p,int No,int Id)
{
	return 0;
}

bool_t WinMenuCheck(win* p,int No,int Id,bool_t State)
{
	return 0;
}

wincontrol* WinPropValue(win* p, winunit* DlgTop, node* Node, int No)
{
	return NULL;
}

wincontrol* WinPropLabel(win* p, winunit* Top, const tchar_t* LockedMsg, const tchar_t* Value)
{
	return NULL;
}

wincontrol* WinLabel(win* p,winunit* DlgTop,winunit DlgLeft,winunit DlgWidth,const tchar_t* Msg,int FontSize,int Flags,wincontrol* Ref)
{
	return NULL;
}

void Win_Init()
{
}

void Win_Done()
{
}

#endif
