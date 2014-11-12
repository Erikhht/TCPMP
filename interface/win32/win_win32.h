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
 * $Id: win_win32.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __WIN_WIN32_H
#define __WIN_WIN32_H

typedef int (*winhandler)();
#define WINCREATE(Name) static INLINE void Name##Create(win* p) {}

#define MSG_POWER_ON		(WM_APP+0x251)
#define MSG_PREPARE			(WM_APP+0x250)

#define SETTINGCHANGE_RESET  0x3002

struct wincontrol
{
	void* Control;
	void* Edit;
	int Editor:1;
	int ComboBox:1;
	int ListView:1;
	int Disabled:1;
	int External:1;
	pin Pin;
	wincontrol* Ref;
	wincontrol* Next; // controls chain
};

#define WINPRIVATE																		\
	void* Wnd;																			\
	void* WndDialog;																	\
	void* WndTB;																		\
	void* Menu2[2];																		\
	void* Menu3;																		\
	void* Module;																		\
	winunit Width;																		\
	winunit Height;																		\
	int BitmapNo;																		\
	bool_t (*Proc)(void*, int Msg, uint32_t wParam, uint32_t lParam,int* Result);		\
	bool_t (*DialogProc)(void*, int Msg, uint32_t wParam, uint32_t lParam,int* Result);	\
	bool_t Smartphone;																	\
	bool_t FullScreen;																	\
	bool_t ScrollV;																		\
	bool_t ScrollH;																		\
	bool_t InUpdateScroll;																\
	bool_t InUpdate;																	\
	bool_t OwnDialogSize;																\
	bool_t CaptureKeys;																	\
	bool_t AygShellTB;																	\
	bool_t MenuSmall;																	\
	bool_t TitleOK;																		\
	bool_t TitleDone;																	\
	bool_t TitleCancel;																	\
	bool_t NeedOK;																		\
	bool_t HotKeyEscape;																\
	bool_t ComboOpen;																	\
	bool_t Closed;																		\
	int ToolBarHeight;																	\
	int LogPixelSX;																		\
	int LogPixelSY;																		\
	void* Activate;																		\
	int SaveStyle;																		\
	point SaveScreen;																	\
	int32_t SaveRect[4];																\
	int WinCmd[2];

bool_t WinEssentialKey(int HotKey);
bool_t WinNoHotKey(int HotKey);
void WinRegisterHotKey(win* p,int Id,int HotKey);
void WinUnRegisterHotKey(win* p,int Id);
void WinAllKeys(bool_t State);
int WinKeyState(int Key);
void WinBitmap(win*,int BitmapId16,int BitmapId32,int BitmapNum);
void WinDeleteButton(win*,int Id);
void WinAddButton(win*,int Id,int Bitmap,tchar_t* Name,bool_t Check); // -1 for separator
void WinButtonEnable(win*,int Id,bool_t);
void WinButtonBitmap(win*,int Id,int Bitmap);
void WinSetFullScreen(win*,bool_t State);
void* WinFont(win*,winunit* FontSize,bool_t Bold);
void* WinCursorArrow();
win* WinGetObject(void* Wnd);
void WinSetObject(void* Wnd,win* p);

winunit WinPixelToUnitX(win* p,int Pixel);
winunit WinPixelToUnitY(win* p,int Pixel);
int WinUnitToPixelX(win* p,winunit Pos);
int WinUnitToPixelY(win* p,winunit Pos);

#define WINSHOWHTML
void WinShowHTML(const tchar_t* Name);

void ForwardMenuButtons(win* p,int Msg,uint32_t wParam,uint32_t lParam);

#endif
