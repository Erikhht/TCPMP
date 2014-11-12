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
 * $Id: win.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __WIN_H
#define __WIN_H

#define WIN_CLASS			FOURCC('W','I','N','_')
#define WIN_ICON			1000

#define INTERFACE_ID		FOURCC('I','N','T','F')

#define PROPSIZE		11

#define LABEL_BOLD		1
#define LABEL_CENTER	2

#define MENU_SMALL		0x0100
#define MENU_GRAYED		0x0200
#define MENU_NOTSMALL	0x0400
#define MENU_ADV		0x0800

#define WIN_DIALOG			0x0001
#define WIN_2BUTTON			0x0002
#define WIN_NOMENU			0x0004
#define WIN_BOTTOMTOOLBAR	0x0008
#define WIN_NOTABSTOP		0x0010

#define WIN_PROP_SOFTRESET	0x10000
#define WIN_PROP_RESTART	0x20000
#define WIN_PROP_RESYNC		0x40000
#define WIN_PROP_CHANGED	0x80000

typedef struct menudef
{
	int Level;
	int Class;
	int Id;
	int Flags;

} menudef;

#define MENUDEF_END { -1 }

typedef int winunit;
typedef struct win win;
typedef struct wincontrol wincontrol;
typedef int (*winpopup)(win*,win* Parent);
typedef int (*wincommand)(win*,int Command);

#if defined(TARGET_PALMOS)
#include "palmos/win_palmos.h"
#elif defined(TARGET_WINCE) || defined(TARGET_WIN32)
#include "win32/win_win32.h"
#else
#error Not supported platform
#endif

struct win
{
	node Node;
	nodefunc Init;
	nodefunc Done;
	wincommand Command;
	winhandler Handler;
	winpopup Popup;

	int Flags;
	winunit WinWidth;
	winunit WinHeight;
	winunit LabelWidth; // for prop dialog
	const menudef* MenuDef;
	int Result;
	win* Parent;
	win* Child;
	wincontrol* Focus;
	wincontrol* Controls;
	winunit ScreenWidth;

	WINPRIVATE
};

void Win_Init();
void Win_Done();

int WinPopupClass(int Class,win* Parent);

wincontrol* WinPropLabel(win*, winunit* Top, const tchar_t* LockedMsg, const tchar_t* Value);
wincontrol* WinPropValue(win*, winunit* Top, node* Node, int No);
wincontrol* WinPropNative(win*, node* Node, int No);

void WinPropFocus(win*, node* Node, int No, bool_t SetFocus);
void WinPropUpdate(win*, node* Node, int No);

wincontrol* WinLabel(win*,winunit* Top,winunit Left,winunit Width,const tchar_t* LockedMsg,int FontSize,int Flags,wincontrol* Ref);

void WinMenuInsert(win*,int No,int PrevId,int Id,const tchar_t* LockedMsg);
int WinMenuFind(win*,int Id);
bool_t WinMenuDelete(win*,int No,int Id);
bool_t WinMenuCheck(win*,int No,int Id,bool_t);
bool_t WinMenuEnable(win*,int No,int Id,bool_t);

void WinBeginUpdate(win*);
void WinEndUpdate(win*);

void WinTitle(win*,const tchar_t* LockedMsg);
int WinClose(win*);

#endif
