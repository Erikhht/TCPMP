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
 * $Id: querykey_win32.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"
#include "querykey_win32.h"
#include "widcommaudio.h"

#if defined(TARGET_WINCE) || defined(TARGET_WIN32)

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#define TIMER_CLOSE		507

typedef struct querykey
{
	win Win;
	WPARAM VCode;
	WPARAM VCode2;
	int Key;
	int Key2;
	bool_t Keep;

} querykey;

static bool_t DialogProc(querykey* p,int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	switch (Msg)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:

		if (wParam != VK_CONTROL && wParam != VK_MENU && 
			wParam != VK_LWIN && wParam != VK_RWIN && wParam != VK_SHIFT)
		{
			int Key;
			if ((wParam == VK_F1 || wParam == VK_F2) &&
				QueryPlatform(PLATFORM_TYPENO)==TYPE_SMARTPHONE)
			{
				// can't loose menu buttons
				WinAllKeys(0);
				keybd_event((BYTE)wParam,1,0,0);
				break; 
			}

			Key = WinKeyState(wParam);
			if (!p->VCode)
			{
				p->VCode = wParam;
				p->Key = Key;
			}
			else
			if (!p->VCode2)
			{
				p->VCode2 = wParam;
				p->Key2 = Key;
			}
		}
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:

		if (p->VCode == wParam)
		{
			p->VCode = 0;
			if (!p->VCode2)
				SetTimer(p->Win.Wnd,TIMER_CLOSE,100,NULL);
		}

		if (p->VCode2 == wParam)
		{
			p->VCode2 = 0;
			if (!p->VCode)
				PostMessage(p->Win.Wnd,WM_CLOSE,0,0);
		}
		break;

	case WM_KILLFOCUS:
		WinAllKeys(0);
		break;

	case WM_SETFOCUS:
		WinAllKeys(1);
		WidcommAudio_Wnd(p->Win.WndDialog);
		break;
	}
	return 0;
}

static int Command(querykey* p,int Cmd)
{
	switch (Cmd)
	{
	case PLATFORM_CANCEL:
		p->Key = p->Key2 = 0;
		WinClose(&p->Win);
		return ERR_NONE;

	case QUERYKEY_KEEP:
		p->Keep = !p->Keep;
		WinMenuCheck(&p->Win,1,QUERYKEY_KEEP,p->Keep);
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}

static bool_t Proc(querykey* p,int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	switch (Msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) 
			WinClose(&p->Win);
		break;

	case WM_CLOSE:
		KillTimer(p->Win.Wnd,TIMER_CLOSE);
		if (p->Keep)
		{
			if (p->Key) p->Key |= HOTKEY_KEEP;
			if (p->Key2) p->Key2 |= HOTKEY_KEEP;
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_CLOSE)
			WinClose(&p->Win);
		break;
	}
	return 0;
}

static int Get(querykey* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case QUERYKEY_KEY: GETVALUE(p->Key,int); break;
	case QUERYKEY_KEY2: GETVALUE(p->Key2,int); break;
	}
	return Result;
}

static menudef MenuDef[] =
{
	{ 0, PLATFORM_ID, PLATFORM_CANCEL },
	{ 0, QUERYKEY_ID, QUERYKEY_OPTIONS },
	{ 1, QUERYKEY_ID, QUERYKEY_KEEP },

	MENUDEF_END
};

static int Init(querykey* p)
{
	winunit y;

	p->Keep = 0;
	p->VCode = 0;
	p->VCode2 = 0;
	p->Key = 0;
	p->Key2 = 0;

	y = (p->Win.Height-12) >> 1;
	WinLabel(&p->Win,&y, -1, -1,LangStr(QUERYKEY_ID,QUERYKEY_MSG),12,LABEL_CENTER,NULL);
	return ERR_NONE;
}

WINCREATE(QueryKey)

static int Create(querykey* p)
{
	QueryKeyCreate(&p->Win);
	p->Win.WinWidth = 200;
	p->Win.WinHeight = 200;
	p->Win.MenuDef = MenuDef;
	p->Win.Flags |= WIN_DIALOG;
	p->Win.Proc = Proc;
	p->Win.DialogProc = DialogProc;
	p->Win.Command = (wincommand)Command;
	p->Win.Init = Init;
	p->Win.Node.Get = Get;
	return ERR_NONE;
}

static const nodedef QueryKey =
{
	sizeof(querykey),
	QUERYKEY_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void QueryKey_Init()
{
	NodeRegisterClass(&QueryKey);
}

void QueryKey_Done()
{
	NodeUnRegisterClass(QUERYKEY_ID);
}

#endif
