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
 * $Id: win_win32.c 623 2006-02-01 13:19:15Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"
#include "widcommaudio.h"
#include "querykey_win32.h"
#include "openfile_win32.h"
#include "playlst.h"
#include "interface.h"

#if defined(TARGET_WINCE) || defined(TARGET_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#if _MSC_VER > 1000
#pragma warning(disable : 4201)
#endif
#include <commctrl.h>
#include <shellapi.h>

#include "resource.h"

#define MAXFONTSIZE	64

static HFONT FontCache[MAXFONTSIZE][2];
static WNDCLASS WinClass;
static WNDCLASS DialogClass;
static tchar_t WinClassName[64];
static win* Main = NULL;

#ifndef GCL_HMODULE         
#define GCL_HMODULE         (-16)
#endif

#ifndef UDS_EXPANDABLE
#define UDS_EXPANDABLE  0x0200
#endif

#if defined(TARGET_WINCE)
#define DIALOG_COLOR COLOR_STATIC
#else
#define DIALOG_COLOR COLOR_BTNFACE
#define MOD_KEYUP 0
#endif

#define VALIDCONTROL(i) ((int)(i) & ~0xFFFF)

#define TRACKMAX		30000
#define TRACKTHUMB		9

static BOOL (WINAPI* FuncUnregisterFunc1)( int Flags,int HoyKey ) = NULL;
static BOOL (WINAPI* FuncAllKeys)( int State ) = NULL;

#define TOOLBAR_ID			1000

#if !defined(TARGET_WINCE)
#define TBSTYLE_EX_MIXEDBUTTONS         0x00000008
#ifndef BTNS_SHOWTEXT
#define BTNS_SHOWTEXT					0x40
#endif
#else
#define BTNS_SHOWTEXT					0
#endif

static const menudef* MenuDef(win* Win,const menudef* p,bool_t Popup,void** Menu)
{
	int Level = p->Level;
	if (!*Menu)
		*Menu = Popup ? CreatePopupMenu() : CreateMenu();

	while (p->Level==Level && Level>=0)
	{
		bool_t Skip = 0;
		if (p->Id==-1)
		{
			if (!Win->MenuSmall && !Skip)
				AppendMenu(*Menu,MF_SEPARATOR,0,NULL);
			++p;
		}
		else
		{
			tchar_t s[256];
			int Flags = MF_STRING;
			int Id = p->Id;

			tcscpy_s(s,TSIZEOF(s),LangStr(p->Class,p->Id));

			if (p->Class == PLATFORM_ID)
			{
				if (p->Level==0 && p->Id == PLATFORM_CANCEL && Win->TitleCancel)
				{
					++p;
					continue;
				}
				if (p->Id == PLATFORM_OK)
				{
					Win->NeedOK = 1;
					if (Win->TitleOK)
					{
						++p;
						continue;
					}
				}
				if (p->Id == PLATFORM_DONE)
				{
					Win->NeedOK = 1;
					if (Win->TitleDone)
					{
						++p;
						continue;
					}
				}
			}

			++p;
			if (p->Level > Level)
			{
				if (((p[-1].Flags & MENU_SMALL) && !Win->MenuSmall) ||
					((p[-1].Flags & MENU_NOTSMALL) && Win->MenuSmall))
				{
					if (p[-1].Flags & MENU_GRAYED)
					{
						AppendMenu(*Menu,Flags,Id,s);
						EnableMenuItem(*Menu,Id,MF_GRAYED|MF_BYCOMMAND);
					}
			
					// skip
					p = MenuDef(Win,p,1,Menu);
					continue;
				}
				else
				if (p[-1].Flags & MENU_GRAYED)
				{ 
					int i = tcslen(s);
					if (i && s[i-1]==':')
						s[i-1] = 0;
				}

				Id = 0;
				p = MenuDef(Win,p,1,(HMENU*)&Id);
				Flags |= MF_POPUP;
			}

			if (!Skip)
			{
				AppendMenu(*Menu,Flags,Id,s);

				if (!(Flags & MF_POPUP) && (p[-1].Flags & MENU_GRAYED))
					EnableMenuItem(*Menu,Id,MF_GRAYED|MF_BYCOMMAND);
			}
			else
			{
				if (Flags & MF_POPUP)
					DestroyMenu((HMENU)Id);
			}
		}
	}
	return p;
}

#if defined(TARGET_WINCE)

#define SIPF_OFF	0x00000000
#define SIPF_ON 	0x00000001

typedef struct SHACTIVATEINFO
{
	DWORD cbSize;
	HWND hwndLastFocus;
	UINT fSipUp:1;
	UINT fSipOnDeactivation:1;
	UINT fActive:1;
	UINT fReserved:29;
} SHACTIVATEINFO;

#define SHMBOF_NODEFAULT    0x00000001
#define SHMBOF_NOTIFY       0x00000002

#define SHCMBM_GETSUBMENU	(WM_USER + 401)
#define SHCMBM_GETMENU      (WM_USER + 402)
#define SHCMBM_OVERRIDEKEY  (WM_USER + 403)

#define SHCMBF_EMPTYBAR      0x0001
#define SHCMBF_HIDDEN        0x0002
#define SHCMBF_HIDESIPBUTTON 0x0004
#define SHCMBF_COLORBK       0x0008
#define SHCMBF_HMENU         0x0010

typedef struct SHMENUBARINFO
{
    DWORD cbSize;
	HWND hwndParent;
    DWORD dwFlags;
    UINT nToolBarId;
    HINSTANCE hInstRes;
    int nBmpId;
    int cBmpImages;
	HWND hwndMB;
    // COLORREF clrBk; //be as compatible as possible
} SHMENUBARINFO;

#define SHIDIM_FLAGS 0x0001 

#define SHIDIF_DONEBUTTON           0x0001
#define SHIDIF_SIZEDLG              0x0002
#define SHIDIF_SIZEDLGFULLSCREEN    0x0004
#define SHIDIF_SIPDOWN              0x0008
#define SHIDIF_FULLSCREENNOMENUBAR  0x0010
#define SHIDIF_EMPTYMENU            0x0020

typedef struct SHINITDLGINFO
{
    DWORD dwMask;
    HWND  hDlg;
    DWORD dwFlags;
} SHINITDLGINFO;

#define SPI_GETSIPINFO          225
#define SPI_APPBUTTONCHANGE     228

typedef struct SIPINFO
{
    DWORD   cbSize;
    DWORD   fdwFlags;
    RECT    rcVisibleDesktop;
    RECT    rcSipRect;
    DWORD   dwImDataSize;
    void   *pvImData;
} SIPINFO;

#define SHFS_SHOWTASKBAR            0x0001
#define SHFS_HIDETASKBAR            0x0002
#define SHFS_SHOWSIPBUTTON          0x0004
#define SHFS_HIDESIPBUTTON          0x0008
#define SHFS_SHOWSTARTICON          0x0010
#define SHFS_HIDESTARTICON          0x0020

static HMODULE AygShell = NULL;
static HMODULE CoreDLL = NULL;
static BOOL (WINAPI* FuncSipGetInfo)(SIPINFO *pSipInfo) = NULL;
static BOOL (WINAPI* FuncSipShowIM)(DWORD) = NULL;
static BOOL (WINAPI* FuncSHCreateMenuBar)( SHMENUBARINFO *pmb ) = NULL;
static BOOL (WINAPI* FuncSHInitDialog)( SHINITDLGINFO *shdi ) = NULL;
static BOOL (WINAPI* FuncSHFullScreen)( HWND, DWORD ) = NULL;
static BOOL (WINAPI* FuncSHHandleWMActivate)( HWND, WPARAM, LPARAM, void*, DWORD ) = NULL;
static BOOL (WINAPI* FuncSHHandleWMSettingChange)(HWND, WPARAM, LPARAM, void*) = NULL;
static BOOL (WINAPI* FuncSHSendBackToFocusWindow)(UINT, WPARAM, LPARAM ) = NULL;
static bool_t HHTaskbarTop = 0;
static bool_t TaskBarHidden = 0;

void MergeRect(RECT* r,HWND Wnd)
{
	if (Wnd)
	{
		RECT a;
		GetWindowRect(Wnd,&a);

		if (a.bottom == r->top)
			r->top = a.top;

		if (a.top == r->bottom)
			r->bottom = a.bottom;
	}
}

void GetWorkArea(win* p,RECT* r)
{
	SystemParametersInfo(SPI_GETWORKAREA,0,r,0);

	if (p->AygShellTB && p->WndTB)
	{
		RECT RectTB;
		GetWindowRect(p->WndTB, &RectTB);
		if (r->bottom > RectTB.top)
			r->bottom = RectTB.top;
	}

	if (TaskBarHidden)
	{
		MergeRect(r,FindWindow(T("HHTaskbar"),NULL));
		MergeRect(r,FindWindow(T("Tray"),NULL));
	}
}

void DIASet(int State,int Mask)
{
	if ((Mask & DIA_SIP) && FuncSipShowIM)
		FuncSipShowIM((State & DIA_SIP)?SIPF_ON:SIPF_OFF);

	if (Mask & DIA_TASKBAR)
	{
		bool_t Hidden = !(State & DIA_TASKBAR);
		if (Hidden != TaskBarHidden)
		{
			win* p;
			HWND WndTaskbar = FindWindow(T("HHTaskbar"),NULL);
			HWND WndTray = FindWindow(T("Tray"),NULL); //for smartphone

			TaskBarHidden = Hidden;

			if (WndTaskbar)
				ShowWindow(WndTaskbar,Hidden ? SW_HIDE:SW_SHOWNA);

			if (WndTray)
				ShowWindow(WndTray,Hidden ? SW_HIDE:SW_SHOWNA);

			for (p=Main;p;p=p->Child)
			{
				if (FuncSHFullScreen)
					FuncSHFullScreen(p->Wnd,Hidden ? SHFS_HIDETASKBAR : SHFS_SHOWTASKBAR);

				if (!p->FullScreen)
				{
					RECT r;
					GetWorkArea(p,&r);
					SetWindowPos(p->Wnd,NULL, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOZORDER);
				}
				else
				{
					p->SaveScreen.x = GetSystemMetrics(SM_CXSCREEN);
					p->SaveScreen.y = GetSystemMetrics(SM_CYSCREEN);
					GetWorkArea(p,(RECT*)&p->SaveRect);
				}
			}
		}
	}
}

int DIAGet(int Mask)
{
	int Value = 0;
	if ((Mask & DIA_TASKBAR) && !TaskBarHidden)
		Value |= DIA_TASKBAR;
	if ((Mask & DIA_SIP) && FuncSipGetInfo)
	{
		SIPINFO Info;
		Info.cbSize = sizeof(Info);
		if (FuncSipGetInfo(&Info) && (Info.fdwFlags & SIPF_ON))
			Value |= DIA_SIP;
	}
	return Value;
}

void WinSetFullScreen(win* p,bool_t State)
{
	if (State != p->FullScreen)
	{
		// no need to hide HHTaskbar because we use TOPMOST and with aygshell we use SHFS_HIDETASKBAR
		HWND WndTaskbar = FindWindow(T("HHTaskbar"),NULL);
		HWND WndTray = FindWindow(T("Tray"),NULL); //for smartphone

		DEBUG_MSG1(DEBUG_WIN,T("FULLSCREEN %d"),State);

		p->FullScreen = State; // set flag before SetWindowPos so WM_SIZE,WM_MOVE has valid info

		if (State)
		{
			HWND Mode;
			if (FuncSHFullScreen)
			{
				Mode = HWND_TOP; //incoming calls on smartphones doesn't like topmost
				FuncSHFullScreen(p->Wnd,SHFS_HIDETASKBAR);
				FuncSHFullScreen(p->Wnd,SHFS_HIDESIPBUTTON);
			}
			else
			{
				Mode = HWND_TOPMOST;
				if (WndTaskbar)
				{
					RECT r;
					GetWindowRect(WndTaskbar,&r);
					HHTaskbarTop = r.top == 0 && r.left == 0;
					if (HHTaskbarTop)
						ShowWindow(WndTaskbar,SW_HIDE); // old-style with pocketpc
				}		
			}

			if (p->WndTB)
				ShowWindow(p->WndTB,SW_HIDE);
			if (WndTray)
				ShowWindow(WndTray,SW_HIDE);

			p->SaveScreen.x = GetSystemMetrics(SM_CXSCREEN);
			p->SaveScreen.y = GetSystemMetrics(SM_CYSCREEN);
			GetWindowRect(p->Wnd,(RECT*)&p->SaveRect);
			SetWindowPos(p->Wnd,Mode, 0, 0, p->SaveScreen.x,p->SaveScreen.y,0);
			
			ShowCursor(FALSE);
			SetKeyboardBacklight(0);
		}
		else
		{
			ShowCursor(TRUE);
			SetKeyboardBacklight(1);
			
			if (p->WndTB)
				ShowWindow(p->WndTB,SW_SHOWNA);

			p->SaveRect[2] += GetSystemMetrics(SM_CXSCREEN) - p->SaveScreen.x;
			p->SaveRect[3] += GetSystemMetrics(SM_CYSCREEN) - p->SaveScreen.y;
			SetWindowPos(p->Wnd,HWND_NOTOPMOST,p->SaveRect[0],p->SaveRect[1],
				p->SaveRect[2]-p->SaveRect[0],p->SaveRect[3]-p->SaveRect[1],SWP_NOACTIVATE);

			if (!TaskBarHidden)
			{
				if (FuncSHFullScreen)
					FuncSHFullScreen(p->Wnd,SHFS_SHOWTASKBAR);
				else
				if (WndTaskbar && HHTaskbarTop)
					ShowWindow(WndTaskbar,SW_SHOWNA); // old-style with pocketpc
			}

			if (WndTray && !TaskBarHidden)
				ShowWindow(WndTray,SW_SHOWNA);
		}
	}
}

static void DestroyToolBar(win* p)
{
	if (p->WndTB)
	{
		CommandBar_Destroy(p->WndTB);
		p->WndTB = NULL;
	}
}

void WinBitmap(win* p,int BitmapId16, int BitmapId32, int BitmapNum)
{
	if (p->WndTB)
	{
		if (p->LogPixelSX >= 96*2)
			p->BitmapNo = CommandBar_AddBitmap(p->WndTB,p->Module,BitmapId32,BitmapNum,32,32);
		else
			p->BitmapNo = CommandBar_AddBitmap(p->WndTB,p->Module,BitmapId16,BitmapNum,16,16);
	}
}

static const menudef* SkipButtons(win* p,const menudef* i)
{
	while (i->Level==0)
	{
		bool_t Skip = 0;

		if (p->TitleOK && i->Class == PLATFORM_ID && i->Id == PLATFORM_OK)
		{
			p->NeedOK = 1;
			Skip = 1;
		}

		if (p->TitleDone && i->Class == PLATFORM_ID && i->Id == PLATFORM_DONE)
		{
			p->NeedOK = 1;
			Skip = 1;
		}

		if (!Skip)
			break;

		++i;
	}
	return i;
}

static void AddButton(win* p,int Pos,int Id,int Bitmap,tchar_t* Name,bool_t Check);

static void CreateToolBar(win* p)
{
	int Ver = QueryPlatform(PLATFORM_VER);
	RECT Rect,RectTB;
	p->WndTB = NULL;
	p->Menu3 = NULL;
	memset(&p->Menu2,0,sizeof(p->Menu2));
	p->AygShellTB = 0;
	p->TitleCancel = p->Smartphone && (p->Flags & WIN_DIALOG);
	p->TitleOK = p->TitleDone = !p->Smartphone && !TaskBarHidden && Ver<500;
	p->NeedOK = 0;

	if (FuncSHCreateMenuBar)
	{
		SHMENUBARINFO Info;
		memset(&Info,0,sizeof(Info));
		Info.cbSize = sizeof(Info);
		if (p->Smartphone)
			Info.cbSize += sizeof(COLORREF); //clrBk
		Info.hwndParent = p->Wnd;
		Info.nToolBarId = 0;
		Info.hInstRes = p->Module;
		Info.dwFlags = 0;
		if (!p->MenuDef)
			Info.dwFlags |= SHCMBF_EMPTYBAR;
		else
		{
			const menudef* i = p->MenuDef;
			int Mode = 0;

			i = SkipButtons(p,i);
			if (i->Level==0)
			{
				++i;
				for (;i->Level>0;++i)
					Mode |= 1;  // submenu under first menu
			}
			i = SkipButtons(p,i);
			if (i->Level==0)
			{
				++i;
				for (;i->Level>0;++i)
					Mode |= 2;  // submenu under second menu
			}

			switch (Mode)
			{
			case 0: Info.nToolBarId = IDR_MENU00; break;
			case 1: Info.nToolBarId = IDR_MENU10; break;
			case 2: Info.nToolBarId = IDR_MENU01; break;
			case 3: Info.nToolBarId = IDR_MENU11; break;
			}
		}
					
		if (!(p->Flags & WIN_DIALOG))
			Info.dwFlags |= SHCMBF_HIDESIPBUTTON;

		FuncSHCreateMenuBar(&Info);

		p->WndTB = Info.hwndMB;
		p->ToolBarHeight = 0;

		GetWindowRect(p->Wnd, &Rect);
		if (p->WndTB)
		{
			if (p->MenuDef)
			{
				TBBUTTONINFO Button;
				const menudef* i = p->MenuDef;
				int No = 0;

				while (i->Level>=0)
				{
					i = SkipButtons(p,i);
					if (i->Level<0)
						break;
					if (No<2)
					{
						int Id = No ? WIN_B : WIN_A;
						p->WinCmd[No] = i->Id;
						Button.cbSize = sizeof(Button);
						Button.dwMask = TBIF_TEXT;
						Button.pszText = (tchar_t*)LangStr(i->Class,i->Id);
						SendMessage(p->WndTB,TB_SETBUTTONINFO,Id,(LPARAM)&Button);
						++i;

						if (i->Level>0)
						{
							p->Menu2[No] = (HMENU)SendMessage(p->WndTB,SHCMBM_GETSUBMENU,0,No ? WIN_B:WIN_A);
							if (p->Menu2[No])
							{
								DeleteMenu(p->Menu2[No],No ? WIN_B_B:WIN_A_A,MF_BYCOMMAND);
								i = MenuDef(p,i,0,&p->Menu2[No]);
							}
						}

						if (No==1 && !p->Smartphone && Ver<500)
						{
							TBBUTTON Sep;
							memset(&Sep,0,sizeof(Sep));
							Sep.fsStyle = TBSTYLE_SEP;
							SendMessage(p->WndTB,TB_INSERTBUTTON,No,(LPARAM)&Sep);
							++No;
						}
					}
					else
					{
						if (p->Smartphone) 
							break;
						AddButton(p,No,i->Id,-2,(tchar_t*)LangStr(i->Class,i->Id),0);
						++i;
						assert(i->Level<=0); // no submenu allowed (only for WIN_A,WIN_B)
					}
					++No;
				}

				while (!p->Smartphone && SendMessage(p->WndTB,TB_BUTTONCOUNT,0,0)>No)
					SendMessage(p->WndTB,TB_DELETEBUTTON,No,0);
			}

			GetWindowRect(p->WndTB, &RectTB);
			if (Rect.bottom > RectTB.top)
				Rect.bottom = RectTB.top;
			//Rect.bottom -= (RectTB.bottom - RectTB.top);
			MoveWindow(p->Wnd,Rect.left,Rect.top,Rect.right - Rect.left,Rect.bottom - Rect.top,0);

			p->AygShellTB = 1;
			p->Flags |= WIN_BOTTOMTOOLBAR;
			if (p->Smartphone)
				p->Flags |= WIN_2BUTTON;

			if (p->Flags & WIN_DIALOG)
				SendMessage(p->WndTB,SHCMBM_OVERRIDEKEY,VK_ESCAPE,
					MAKELONG(SHMBOF_NODEFAULT|SHMBOF_NOTIFY,SHMBOF_NODEFAULT|SHMBOF_NOTIFY));
		}
	}

	if (!p->WndTB)
	{
		p->TitleOK = p->TitleDone = !p->Smartphone;
		p->TitleCancel = 1;
		p->WndTB = CommandBar_Create(p->Module,p->Wnd,TOOLBAR_ID);
		p->ToolBarHeight = CommandBar_Height(p->WndTB);
		if (p->MenuDef)
		{
			MenuDef(p,p->MenuDef,0,&p->Menu3);
			p->Menu2[0] = p->Menu3;
			p->Menu2[1] = p->Menu3;
			CommandBar_InsertMenubarEx(p->WndTB,NULL,(tchar_t*)p->Menu3,0);
		}
		CommandBar_Show(p->WndTB, TRUE);
	}
}

#else

static WNDPROC OldToolBarProc = NULL;

static LRESULT CALLBACK ToolBarProc( HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	if (Msg == WM_MOUSEACTIVATE)
		return MA_NOACTIVATE;	

	if (Msg == WM_LBUTTONUP)
	{
		int i;
		int n=SendMessage(Wnd,TB_BUTTONCOUNT,0,0);
		for (i=0;i<n;++i)
		{
			TBBUTTON Button;
			RECT r;
			POINT p;

			SendMessage(Wnd,TB_GETBUTTON,i,(LPARAM)&Button);
			if ((Button.fsState & TBSTATE_PRESSED))
			{
				if (IsMenu((HMENU)Button.dwData))
				{
					SendMessage(Wnd,TB_GETRECT,Button.idCommand,(LPARAM)&r);
					p.x = LOWORD(lParam);
					p.y = HIWORD(lParam);
					if (PtInRect(&r,p))
					{
						p.x = r.left;
						p.y = r.bottom;
						ClientToScreen(Wnd,&p);
						TrackPopupMenu((HMENU)Button.dwData,TPM_LEFTALIGN|TPM_TOPALIGN,
							p.x,p.y,0,GetParent(Wnd),NULL);
					}
				}
			}
		}
	}
	return CallWindowProc(OldToolBarProc,Wnd,Msg,wParam,lParam);
}

static void DestroyToolBar(win* p)
{
	DestroyWindow(p->WndTB);
	p->WndTB = NULL;
}

static void AddMenu(win* p)
{
	TBBUTTONINFO Info;
	TBBUTTON Button;
	tchar_t Name[256];
	int Pos;

	p->TitleCancel = 1;
	p->TitleOK = 0;
	p->TitleDone = 1;
	p->NeedOK = 0;
	p->Menu3 = NULL;
	memset(&p->Menu2,0,sizeof(p->Menu2));

	if (p->MenuDef)
	{
		MenuDef(p,p->MenuDef,0,&p->Menu3);
		p->Menu2[0] = p->Menu3;
		p->Menu2[1] = p->Menu3;

		for (Pos=0;Pos<GetMenuItemCount(p->Menu3);++Pos)
		{
			GetMenuString(p->Menu3,Pos,Name,255,MF_BYPOSITION);

			memset(&Button,0,sizeof(Button));
			Button.iBitmap = -2;
			Button.idCommand = GetMenuItemID(p->Menu3,Pos);
			if (Button.idCommand < 0)
				Button.idCommand = TOOLBAR_ID+Pos+1;
			Button.fsState = TBSTATE_ENABLED;
			Button.fsStyle = (BYTE)(TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT);
			if (!Button.idCommand)
				Button.fsStyle = TBSTYLE_SEP;

			Button.dwData = (DWORD) GetSubMenu(p->Menu3,Pos);
			Button.iString = 0;

			SendMessage(p->WndTB,TB_INSERTBUTTON,Pos,(LPARAM)&Button);

			Info.cbSize = sizeof(TBBUTTONINFO);
			Info.dwMask = TBIF_TEXT;
			Info.pszText = Name;

			SendMessage(p->WndTB,TB_SETBUTTONINFO,Button.idCommand,(LPARAM)&Info);
		}
	}

	SendMessage(p->WndTB,TB_AUTOSIZE,0,0);
}

void WinBitmap(win* p,int BitmapId16, int BitmapId32, int BitmapNum)
{
	TBADDBITMAP Add;
	Add.hInst = p->Module;
	Add.nID = BitmapId16;
	p->BitmapNo = SendMessage(p->WndTB,TB_ADDBITMAP,BitmapNum,(LPARAM)&Add);
}

static void CreateToolBar(win* p)
{
	RECT r;

	if ((p->Flags & WIN_NOMENU) == 0)
	{
		p->WndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPCTSTR) NULL, 
		WS_CHILD|TBSTYLE_FLAT|TBSTYLE_LIST|WS_VISIBLE, 
		0, 0, 0, 0, p->Wnd, (HMENU)TOOLBAR_ID, p->Module, NULL); 

		if (p->WndTB)
		{
			SendMessage(p->WndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 

			SendMessage(p->WndTB,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);

			OldToolBarProc = (WNDPROC) GetWindowLong(p->WndTB,GWL_WNDPROC);
			SetWindowLong(p->WndTB,GWL_WNDPROC,(LONG)ToolBarProc);

			GetWindowRect(p->WndTB,&r);
			p->ToolBarHeight = r.bottom - r.top;

			AddMenu(p);
		}
	}
}

void DIASet(int State,int Mask)
{
}

int DIAGet(int Mask)
{
	return DIA_TASKBAR & Mask;
}

void WinSetFullScreen(win* p,bool_t State)
{
	if (State != p->FullScreen)
	{
		p->FullScreen = State;
		if (State)
		{
			p->SaveStyle = GetWindowLong(p->Wnd,GWL_STYLE);
			GetWindowRect(p->Wnd,(RECT*)&p->SaveRect);

			SetWindowLong(p->Wnd,GWL_STYLE,WS_TABSTOP|WS_VISIBLE|WS_POPUP|WS_GROUP);
#ifdef NDEBUG
			SetWindowPos(p->Wnd,HWND_TOPMOST,0,0,
#else
			SetWindowPos(p->Wnd,HWND_NOTOPMOST,0,0,
#endif
				GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_FRAMECHANGED);
		}
		else
		{
			SetWindowLong(p->Wnd,GWL_STYLE,p->SaveStyle);
			SetWindowPos(p->Wnd,HWND_NOTOPMOST,p->SaveRect[0],p->SaveRect[1],
				p->SaveRect[2]-p->SaveRect[0],p->SaveRect[3]-p->SaveRect[1],SWP_FRAMECHANGED);
		}

		ShowWindow(p->WndTB,State ? SW_HIDE:SW_SHOWNA);
		InvalidateRect(p->Wnd,NULL,0);
	}
}

#endif

static int GetDataDefType(wincontrol* i)
{
	datadef DataDef;
	if (!NodeDataDef(i->Pin.Node,i->Pin.No,&DataDef))
		return 0;
	return DataDef.Type;
}

static void SetTrack(win* p,wincontrol* i,int Value)
{
	tick_t Tick;
	datadef DataDef;
	if (!NodeDataDef(i->Pin.Node,i->Pin.No,&DataDef))
		return;

	if (DataDef.Format2 - DataDef.Format1 <= TRACKMAX)
		Value += DataDef.Format1;
	else
		Value = DataDef.Format1 + Scale(DataDef.Format2 - DataDef.Format1,Value,TRACKMAX);

	switch (DataDef.Type)
	{
	case TYPE_INT:
		i->Pin.Node->Set(i->Pin.Node,i->Pin.No,&Value,sizeof(Value));
		break;
	case TYPE_TICK:
		Tick = Value;
		i->Pin.Node->Set(i->Pin.Node,i->Pin.No,&Tick,sizeof(Tick));
		break;
	}
}

static void WinControlDisable(win* p,wincontrol* Control,bool_t v)
{
	if (Control->Disabled != v)
	{
		wincontrol* i;
		Control->Disabled = v;
		if (Control->Control)
			EnableWindow(Control->Control,!v);
		if (Control->Edit)
			EnableWindow(Control->Edit,!v);
		for (i=p->Controls;i;i=i->Next)
			if (i->Ref == Control)
				EnableWindow(i->Control,!v);
	}
}

static void WinControlUpdate(win* p,wincontrol* i);
static void WinControlFocus(win* p,wincontrol* Control,bool_t Set);

static void WinControlSet(win* p,wincontrol* i,const datadef* DataDef,const void* Data,int Size,bool_t Update)
{
	tchar_t DataOld[MAXDATA/sizeof(tchar_t)];
	if (i->Pin.Node->Get(i->Pin.Node,i->Pin.No,DataOld,Size)==ERR_NONE && memcmp(Data,DataOld,Size)==0)
		return;

	if (i->Pin.Node->Set(i->Pin.Node,i->Pin.No,Data,Size)==ERR_NONE)
	{
		p->Flags |= WIN_PROP_CHANGED;
		if (DataDef->Flags & DF_SOFTRESET)
			p->Flags |= WIN_PROP_SOFTRESET;
		if (DataDef->Flags & DF_RESTART)
			p->Flags |= WIN_PROP_RESTART;
		if (DataDef->Flags & DF_RESYNC)
			p->Flags |= WIN_PROP_RESYNC;
		if (Update)
			WinControlUpdate(p,i);
	}
}

static void WinControlCmd(win* p,wincontrol* i,HWND Control,int Cmd)
{
	tchar_t s[256];
	datadef DataDef;
	switch (Cmd)
	{
	case CBN_CLOSEUP:
		if (i->ComboBox)
			p->ComboOpen = 0;
		break;

	case CBN_DROPDOWN:
		if (i->ComboBox)
			p->ComboOpen = 1;
		break;

	case LBN_SETFOCUS: //CBN_KILLFOCUS
		if (p->Smartphone)
			WinControlFocus(p,i,0);
		p->ComboOpen = 0;
		break;

	case EN_SETFOCUS:
		if (Control != i->Edit)
			DIASet(DIA_SIP,DIA_SIP);
		//no break
	case CBN_SETFOCUS:
	case BN_SETFOCUS:
		WinControlFocus(p,i,0);
		break;

	case EN_KILLFOCUS:
		if (Control != i->Edit)
			DIASet(0,DIA_SIP);
		break;

	case EN_CHANGE:
		if (!p->InUpdate && NodeDataDef(i->Pin.Node,i->Pin.No,&DataDef))
		{
			if (DataDef.Type == TYPE_STRING)
			{
				GetWindowText(Control,s,TSIZEOF(s));
				WinControlSet(p,i,&DataDef,s,sizeof(s),0);
			}
			else
			if (GetWindowText(Control,s,TSIZEOF(s))>0)
			{
				fraction f;
				tick_t t;
				int v = StringToInt(s,0);
				switch (DataDef.Type)
				{
				case TYPE_INT:
					if (DataDef.Flags & DF_PERCENT)
						v = ScaleRound(v,PERCENT_ONE,100);
					WinControlSet(p,i,&DataDef,&v,sizeof(v),0);
					break;

				case TYPE_TICK:
					if (DataDef.Flags & DF_MSEC)
						t = ScaleRound(v,TICKSPERSEC,1000);
					else
						t = v*TICKSPERSEC;
					WinControlSet(p,i,&DataDef,&t,sizeof(t),0);
					break;

				case TYPE_FRACTION:
					f.Num = v;
					f.Den = 100;
					WinControlSet(p,i,&DataDef,&f,sizeof(f),0);
					break;
				}
			}
		}
		break;

	case STN_CLICKED: //BN_CLICKED:
	case LBN_SELCHANGE: //STN_DBLCLK: CBN_SELCHANGE:
		if (i->Ref)
			WinControlFocus(p,i->Ref,1);
		else
		if (!p->InUpdate && NodeDataDef(i->Pin.Node,i->Pin.No,&DataDef))
		{
			if (DataDef.Type == TYPE_RESET)
			{
				int Zero = 0;
				wincontrol* j;
				for (j=p->Controls;j;j=j->Next)
					if (GetDataDefType(j)==TYPE_INT)
						WinControlSet(p,j,&DataDef,&Zero,sizeof(Zero),1);
			}
			else
			if (DataDef.Type == TYPE_BOOL)
			{
				bool_t Data;
				if (i->Pin.Node->Get(i->Pin.Node,i->Pin.No,&Data,sizeof(Data))==ERR_NONE)
				{
					Data = !Data;
					WinControlSet(p,i,&DataDef,&Data,sizeof(Data),1);
				}
			}
			else
			if (DataDef.Type == TYPE_INT)
			{
				int Data;
				if (p->Smartphone)
				{
					Data = SendMessage(Control,LB_GETCURSEL,0,0);
					if (Data>=0)
						Data = SendMessage(Control,LB_GETITEMDATA,Data,0);
					else
						Data=0;
				}
				else
				{
					Data = SendMessage(Control,CB_GETCURSEL,0,0);
					if (Data>=0)
						Data = SendMessage(Control,CB_GETITEMDATA,Data,0);
					else
						Data=0;
				}
				WinControlSet(p,i,&DataDef,&Data,sizeof(Data),0);
			}
			else
			if (DataDef.Type == TYPE_HOTKEY)
			{
				int Data;
				if (i->Pin.Node->Get(i->Pin.Node,i->Pin.No,&Data,sizeof(Data))==ERR_NONE)
				{
					if (!Data)
					{
						node* QueryKey = NodeCreate(QUERYKEY_ID);
						if (QueryKey)
						{
							int Key;
							int Key2;
							((win*)QueryKey)->Popup((win*)QueryKey,p);
							QueryKey->Get(QueryKey,QUERYKEY_KEY,&Key,sizeof(Key));
							QueryKey->Get(QueryKey,QUERYKEY_KEY2,&Key2,sizeof(Key2));
							if (Key)
							{
								wincontrol* j; 
								int Data2;
								int KeyCode = Key & ~HOTKEY_KEEP;
								int Key2Code = Key2 & ~HOTKEY_KEEP;
								if (!Key2Code) Key2Code = KeyCode;

								for (j=p->Controls;j;j=j->Next)
									if (j!=i && NodeDataDef(j->Pin.Node,j->Pin.No,&DataDef) && DataDef.Type == TYPE_HOTKEY &&
										j->Pin.Node->Get(j->Pin.Node,j->Pin.No,&Data2,sizeof(Data2))==ERR_NONE &&
										((Data2 & ~HOTKEY_KEEP) == KeyCode || (Data2 & ~HOTKEY_KEEP) == Key2Code))
									{
										Data2 = 0;
										WinControlSet(p,j,&DataDef,&Data2,sizeof(Data2),1);
									}

								if (WinEssentialKey(KeyCode) && p->Smartphone)
									Key &= ~HOTKEY_KEEP;

								Data = Key;
								WinControlSet(p,i,&DataDef,&Data,sizeof(Data),1);
							}
							NodeDelete(QueryKey);
						}
					}
					else
					{
						Data = 0;
						WinControlSet(p,i,&DataDef,&Data,sizeof(Data),1);
					}
				}
			}
		}
		break;
	}
}

static void WinControlUpdate(win* p,wincontrol* i)
{
	datadef DataDef;
	tchar_t Data[MAXDATA/sizeof(tchar_t)];
	tchar_t s[256];
	int Value;
	int Result;

	if (!NodeDataDef(i->Pin.Node,i->Pin.No,&DataDef))
		return;

	p->InUpdate = 1;
	Result = i->Pin.Node->Get(i->Pin.Node,i->Pin.No,Data,DataDef.Size);
	WinControlDisable(p,i,Result!=ERR_NONE);

	if (Result != ERR_NONE && Result != ERR_NOT_SUPPORTED)
		memset(Data,0,sizeof(Data));

	switch (DataDef.Type)
	{
	case TYPE_HOTKEY:
		HotKeyToString(s,TSIZEOF(s),*(int*)Data);
		SetWindowText(i->Edit,s);
		SetWindowText(i->Control,LangStr(PLATFORM_ID,*(int*)Data ? PLATFORM_CLEAR:PLATFORM_ASSIGN));
		break;

	case TYPE_BOOL:
		SendMessage(i->Control,BM_SETCHECK,*(bool_t*)Data?BST_CHECKED:BST_UNCHECKED,0);
		break;

	case TYPE_INT:
		if (DataDef.Flags & (DF_ENUMCLASS|DF_ENUMSTRING))
		{
			int GetCount;
			int GetItemData;
			int SetCurSel;
			int No,Count;

#if defined(TARGET_WINCE)
			if (p->Smartphone)
			{
				GetCount = LB_GETCOUNT;
				GetItemData = LB_GETITEMDATA;
				SetCurSel = LB_SETCURSEL;
			}
			else
#endif
			{
				GetCount = CB_GETCOUNT;
				GetItemData = CB_GETITEMDATA;
				SetCurSel = CB_SETCURSEL;
			}

			Count = SendMessage(i->Control,GetCount,0,0);
			for (No=0;No<Count;++No)
				if (SendMessage(i->Control,GetItemData,No,0) == *(int*)Data)
					break;
			if (No==Count)
				No=-1;
			SendMessage(i->Control,SetCurSel,No,0);
			break;
		}
		else
		if (DataDef.Flags & DF_MINMAX)
		{
			if (DataDef.Type == TYPE_TICK)
				Value = *(tick_t*)Data;
			else
				Value = *(int*)Data;

			if (DataDef.Format2 - DataDef.Format1 <= TRACKMAX)
				Value -= DataDef.Format1;
			else
				Value = Scale(Value - DataDef.Format1,TRACKMAX,DataDef.Format2 - DataDef.Format1);
			SendMessage(i->Control,TBM_SETPOS,1,Value);
			break;
		}
		else
		{
			if (DataDef.Flags & DF_PERCENT)
			{
				if (*(int*)Data > 0)
					IntToString(s,TSIZEOF(s),ScaleRound(*(int*)Data,100,PERCENT_ONE),0);
				else
					s[0]=0;
			}
			else
				IntToString(s,TSIZEOF(s),*(int*)Data,0);
			SetWindowText(i->Control,s);
			SendMessage(i->Control,EM_SETSEL,tcslen(s),tcslen(s));
		}
		break;

	case TYPE_STRING:
		SetWindowText(i->Control,Data);
		SendMessage(i->Control,EM_SETSEL,tcslen(s),tcslen(s));
		break;

	case TYPE_TICK:
		if (DataDef.Flags & DF_MSEC)
			stprintf_s(s,TSIZEOF(s),T("%d"),ScaleRound(*(tick_t*)Data,1000,TICKSPERSEC));
		else
			stprintf_s(s,TSIZEOF(s),T("%d"),ScaleRound(*(tick_t*)Data,1,TICKSPERSEC));
		SetWindowText(i->Control,s);
		SendMessage(i->Control,EM_SETSEL,tcslen(s),tcslen(s));
		break;

	case TYPE_FRACTION:
		if (((fraction*)Data)->Num > 0)
			stprintf_s(s,TSIZEOF(s),T("%d"),ScaleRound(((fraction*)Data)->Num,100,((fraction*)Data)->Den));
		else
			s[0] = 0;
		SetWindowText(i->Control,s);
		SendMessage(i->Control,EM_SETSEL,tcslen(s),tcslen(s));
		break;
	}
	p->InUpdate = 0;
}

winunit WinPixelToUnitX(win* p,int Pixel) {	return ScaleRound(Pixel,72,p->LogPixelSX); }
winunit WinPixelToUnitY(win* p,int Pixel) { return ScaleRound(Pixel,72,p->LogPixelSY); }
int WinUnitToPixelX(win* p,winunit Pos) { return ScaleRound(Pos,p->LogPixelSX,72); }
int WinUnitToPixelY(win* p,winunit Pos) { return ScaleRound(Pos,p->LogPixelSY,72); }

void WinProp( win* p, int* x0,int* x1)
{
	*x0 = 2;
	*x1 = p->LabelWidth + *x0;

	if (*x1 > (p->Width*3)/4)
		*x1 = (p->Width*3)/4;

	*x1++;
}

static wincontrol* CreateControl(win* p,int Size)
{
	wincontrol* Control = malloc(Size);
	if (Control)
	{
		wincontrol** i;
		memset(Control,0,Size);
		for (i=&p->Controls;*i;i=&(*i)->Next);
		*i = Control;
	}
	return Control;
}

static int CreateEnum(win* p, wincontrol* Control, int Left, int Top, int Width, int Height, const datadef* DataDef)
{
	int No;
	int Pos;
	int AddString;
	int ItemData;
	int MaxWidth;
	int FontSize;
	int Plus = 0;

	MaxWidth = WinUnitToPixelX(p,(DataDef->Flags & DF_ENUMSTRING) ? 80:180);
	if (Width > MaxWidth)
		Width = MaxWidth;

	if (p->Smartphone)
	{
		Control->Control = CreateWindow(T("listbox"), NULL, WS_TABSTOP|WS_VISIBLE|WS_CHILD|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|((DataDef->Flags & DF_ENUMUNSORT)?0:LBS_SORT), 
			Left, Top, Width, Height, p->WndDialog, NULL, p->Module, 0L);

		Control->Edit = CreateWindow(UPDOWN_CLASS, NULL, WS_CHILD|WS_VISIBLE | UDS_HORZ |UDS_ALIGNRIGHT | UDS_ARROWKEYS |  UDS_SETBUDDYINT | UDS_WRAP | UDS_EXPANDABLE, 
			0, 0, 0, 0, p->WndDialog, NULL, p->Module, 0L);

		SendMessage(Control->Edit, UDM_SETBUDDY, (WPARAM)Control->Control, 0);	

		AddString = LB_ADDSTRING;
		ItemData = LB_SETITEMDATA;

		FontSize = PROPSIZE;
		SendMessage(Control->Edit,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,1),0);
	}
	else
	{
		Control->Control = CreateWindow(T("COMBOBOX"), NULL, WS_VSCROLL|WS_TABSTOP|WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|((DataDef->Flags & DF_ENUMUNSORT)?0:CBS_SORT), 
			Left, Top, Width, Height*(p->MenuSmall?10:15), p->WndDialog, NULL, p->Module, 0L);

		Control->ComboBox = 1;
		AddString = CB_ADDSTRING;
		ItemData = CB_SETITEMDATA;
		Plus = 6;

		FontSize = PROPSIZE;
		SendMessage(Control->Control,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,1),0);
	}

	if (DataDef->Flags & DF_ENUMCLASS)
	{
		array List;
		int *i;
		NodeEnumClass(&List,DataDef->Format1);
		for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
		{
			Pos = SendMessage(Control->Control,AddString,0,(LPARAM)LangStr(*i,NODE_NAME));
			SendMessage(Control->Control,ItemData,Pos,*i);
		}
		ArrayClear(&List);
	}
	else
	if (DataDef->Flags & DF_ENUMSTRING)
	{
		int Id;
		for (No=0;(Id=LangEnum(DataDef->Format1,No))!=0;++No)
		{
			Pos = SendMessage(Control->Control,AddString,0,(LPARAM)LangStr(DataDef->Format1,Id));
			SendMessage(Control->Control,ItemData,Pos,Id);
		}
	}

	return Plus;
}

static void CreateEditor(win* p, wincontrol* Control, int Left, int Top, int Width, int Height, const datadef* DataDef)
{
	tchar_t Post[32];
	int FontSize;
	int Style = ES_NUMBER;

	if (DataDef->Flags & DF_NEG)
		Style = 0;

	if (DataDef->Type==TYPE_STRING)
		Style = ES_AUTOHSCROLL;

#ifdef TARGET_WIN32
	if ((Width > 32) && DataDef->Type!=TYPE_STRING)
		Width = 32;
#else
	if ((Width > 30) && DataDef->Type!=TYPE_STRING)
		Width = 30;
#endif

	Control->Control = CreateWindow(T("EDIT"), NULL, WS_TABSTOP|WS_VISIBLE|WS_CHILD|Style, 
		WinUnitToPixelX(p,Left), WinUnitToPixelY(p,Top), 
		WinUnitToPixelX(p,Width), WinUnitToPixelY(p,Height), 
		p->WndDialog, NULL, p->Module, 0L);

	FontSize = PROPSIZE;
	SendMessage(Control->Control,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,1),0);

	Control->Editor = 1;
	Post[0] = 0;

	if (DataDef->Flags & DF_KBYTE)
		tcscpy_s(Post,TSIZEOF(Post),T("KB"));
	else	
	if (DataDef->Flags & DF_MHZ)
		tcscpy_s(Post,TSIZEOF(Post),T("Mhz"));
	else	
	if (DataDef->Flags & DF_PERCENT)
		tcscpy_s(Post,TSIZEOF(Post),T("%"));
	else
	if (DataDef->Type == TYPE_TICK)
		tcscpy_s(Post,TSIZEOF(Post),(DataDef->Flags & DF_MSEC) ? T("msec"):T("sec"));

	if (Post[0])
		WinLabel(p,&Top,Left+Width+1,-1,Post,11,0,NULL);
}

wincontrol* WinPropNative(win* p, node* Node, int No)
{
	wincontrol* Control = CreateControl(p,sizeof(wincontrol));
	if (Control)
	{
		Control->Pin.Node = Node;
		Control->Pin.No = No;
	}
	return Control;
}

wincontrol* WinPropValue(win* p, winunit* DlgTop, node* Node, int No)
{
	int FontSize;
	int Height,Width;
	int Gap = WinUnitToPixelX(p,2);
	int Top = WinUnitToPixelY(p,*DlgTop);
	int Left;
	winunit x,x0,x1,y1,y2;
	wincontrol* Control;
	datadef DataDef;

	if (!NodeDataDef(Node,No,&DataDef))
		return NULL;

	Control = WinPropNative(p,Node,No);
	if (!Control)
		return NULL;

	if (DataDef.Flags & DF_SHORTLABEL)
	{
		if (p->Smartphone)
			x1 = 36;
		else
			x1 = 48;
		x0 = 2;
		Width = x1-x0-2; 
	}
	else
	if ((DataDef.Flags & DF_CHECKLIST) && DataDef.Type==TYPE_BOOL)
	{
		x0 = 4+PROPSIZE;
		x1 = 2;
		Width = -1;
	}
	else
	{
		WinProp(p,&x0,&x1);
		Width = x1-x0-2;
	}

	if (DataDef.Flags & DF_NOWRAP)
		Width = -1;

	y1 = *DlgTop;
	if (DataDef.Type != TYPE_RESET)
		WinLabel(p,&y1,x0,Width,DataDef.Name,PROPSIZE,0,Control);
	else
		x1 = x0;

	y2 = *DlgTop + PROPSIZE+1;
	Width = WinUnitToPixelX(p,PROPSIZE+1);
	Height = WinUnitToPixelY(p,PROPSIZE+1);
	Left = WinUnitToPixelX(p,x1);

	switch (DataDef.Type)
	{
	case TYPE_RESET:
		Control->Control = CreateWindow(T("BUTTON"),NULL,WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_NOTIFY,
			Left+Gap,Top,Width*5,WinUnitToPixelY(p,PROPSIZE+2), p->WndDialog, NULL, p->Module, NULL);

		y2 += 2;
		FontSize = PROPSIZE;
		SendMessage(Control->Control,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,0),0);
		SetWindowText(Control->Control,DataDef.Name);
		break;

	case TYPE_HOTKEY:
		x = (Width*7)/2;
		Control->Edit = CreateWindow(T("EDIT"),NULL,ES_READONLY|ES_AUTOHSCROLL|WS_VISIBLE|WS_CHILD,
			Left,Top,x,Height, p->WndDialog, NULL, p->Module, NULL );

		Control->Control = CreateWindow(T("BUTTON"),NULL,WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_NOTIFY,
			Left+x+Gap,Top,(Width*5)/2,WinUnitToPixelY(p,PROPSIZE+2), p->WndDialog, NULL, p->Module, NULL);

		FontSize = PROPSIZE;
		SendMessage(Control->Edit,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,1),0);

		FontSize = PROPSIZE;
		SendMessage(Control->Control,WM_SETFONT,(WPARAM)WinFont(p,&FontSize,0),0);

		y2 += 2;
		break;

	case TYPE_BOOL:
		Control->Control = CreateWindow(T("BUTTON"),NULL,WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_CHECKBOX|BS_NOTIFY,
			Left,Top,Width,Height,p->WndDialog,NULL,p->Module,NULL);
		break;

	case TYPE_INT:
		if (DataDef.Flags & (DF_ENUMCLASS|DF_ENUMSTRING))
		{
			y2 += CreateEnum(p,Control,Left,Top,WinUnitToPixelX(p,p->Width-x1),Height,&DataDef);
			break;
		}
		else
		if (DataDef.Flags & DF_MINMAX)
		{
			int Interval;

			Width = p->Width-x1;
			if (Width > 100)
				Width = 100;
			if (Width < 65 && !(DataDef.Flags & DF_NOWRAP))
			{
				y2 = y1 + PROPSIZE+1;
				x1 = x0;
				Width = p->Width-x1;
				if (Width > 200)
					Width = 200;
				Left = WinUnitToPixelX(p,x1);
				Top = WinUnitToPixelY(p,y1);
			}

			Control->Control = CreateWindowEx( 
				0,                    
				TRACKBAR_CLASS,       
				NULL,   
				WS_TABSTOP|WS_CHILD | WS_VISIBLE | TBS_HORZ|TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH, 
				Left,Top,    
				WinUnitToPixelX(p,Width), Height,   
				p->WndDialog,	
				NULL,						
				p->Module,                     
				NULL); 

			Interval = DataDef.Format2 - DataDef.Format1;
			if (Interval > TRACKMAX)
				Interval = TRACKMAX;

			SendMessage(Control->Control, TBM_SETRANGE, FALSE,MAKELONG(0,Interval));
			SendMessage(Control->Control, TBM_SETPAGESIZE,0,Interval/32);
			SendMessage(Control->Control, TBM_SETLINESIZE,0,Interval/32);
			SendMessage(Control->Control, TBM_SETTHUMBLENGTH, WinUnitToPixelY(p,TRACKTHUMB),0);
			break;
		}
		// no break

	case TYPE_STRING:
	case TYPE_TICK:
	case TYPE_FRACTION:
		CreateEditor(p,Control,x1,*DlgTop,p->Width-x1,PROPSIZE+1,&DataDef);
		y2 += 1;
		break;
	}
	SetWindowLong(Control->Control,GWL_USERDATA,(LONG)Control);
	WinControlUpdate(p,Control);
	*DlgTop = y1>y2?y1:y2;
	return Control;
}

bool_t WinNoHotKey(int HotKey)
{
	int Model = QueryPlatform(PLATFORM_MODEL);
	if (Model == MODEL_TOSHIBA_E800) // record button mapping works on E800...
		return 0;

	HotKey &= ~HOTKEY_KEEP;
	if (((HotKey & HOTKEY_MASK) == 0xC5 || HotKey == VK_ESCAPE || HotKey == VK_F1 || 
		 HotKey == VK_F2 || HotKey == VK_F3 || HotKey == VK_F4 ||
		 HotKey == VK_F6 || HotKey == VK_F7) && 
		(QueryPlatform(PLATFORM_TYPENO) == TYPE_POCKETPC ||
		 QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE))
		return 1; // record, softkey1,2, talk, end, volume up/odwn

	return 0;
}

bool_t WinEssentialKey(int HotKey)
{
	HotKey &= ~HOTKEY_KEEP;
	return 
		HotKey == 0x86 || //action
		HotKey == VK_RETURN ||
		HotKey == VK_LEFT || // (need to be essential for D-pad rotation support too!)
		HotKey == VK_RIGHT ||
		HotKey == VK_UP ||
		HotKey == VK_DOWN; // should use normal WM_KEYDOWN event for them
}

static void AddButton(win* p,int Pos,int Id,int Bitmap,tchar_t* Name,bool_t Check)
{
	TBBUTTON Button;
	memset(&Button,0,sizeof(Button));
	Button.iBitmap = p->BitmapNo + Bitmap;
	Button.idCommand = Id;
	Button.fsState = TBSTATE_ENABLED;
	Button.fsStyle = (BYTE)((Check ? TBSTYLE_CHECK : TBSTYLE_BUTTON) | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT);
	Button.dwData = 0;
	Button.iString = 0;

	if (Id<0)
	{
		Button.idCommand = 0;
		Button.fsStyle = TBSTYLE_SEP;

		if (!p->AygShellTB && !(p->Flags & WIN_DIALOG) && GetSystemMetrics(SM_CXSCREEN)<=240)
			return;
	}

	if (p->WndTB)
	{
		SendMessage(p->WndTB,TB_INSERTBUTTON,Pos,(LPARAM)&Button);

		if (Name && Id>0)
		{
			TBBUTTONINFO Button;
			Button.cbSize = sizeof(Button);
			Button.dwMask = TBIF_TEXT;
			Button.pszText = Name;
			SendMessage(p->WndTB,TB_SETBUTTONINFO,Id,(LPARAM)&Button);
		}
	}
}

static int FindButton(win* p,int Id)
{
	if (p->WndTB)
	{
		int i;
		int n=SendMessage(p->WndTB,TB_BUTTONCOUNT,0,0);
		for (i=0;i<n;++i)
		{
			TBBUTTON Button;
			SendMessage(p->WndTB,TB_GETBUTTON,i,(LPARAM)&Button);
			if (Button.idCommand == Id)
				return i;
		}
	}
	return -1;
}

void WinDeleteButton(win* p,int Id)
{
	int No = FindButton(p,Id);
	if (No>=0)
		SendMessage(p->WndTB,TB_DELETEBUTTON,No,0);
}

void WinAddButton(win* p,int Id,int Bitmap,tchar_t* Name,bool_t Check)
{
	if (p->WndTB)
		AddButton(p,SendMessage(p->WndTB,TB_BUTTONCOUNT,0,0),Id,Bitmap,Name,Check);
}

win* WinGetObject(HWND Wnd)
{
	return (win*) GetWindowLong(Wnd,GWL_USERDATA);
}

void WinSetObject(HWND Wnd,win* p)
{
	SetWindowLong(Wnd,GWL_USERDATA,(LONG)p);
}

void WinRegisterHotKey(win* p,int Id,int HotKey)
{
#if defined(NDEBUG) || defined(TARGET_WINCE)
	int Mod = MOD_KEYUP;
	HotKey &= ~HOTKEY_KEEP;
	//if (WinNoHotKey(HotKey)) return; //don't force, some devices support those hotkeys
	if (HotKey & HOTKEY_ALT) Mod |= MOD_ALT;
	if (HotKey & HOTKEY_SHIFT) Mod |= MOD_SHIFT;
	if (HotKey & HOTKEY_CTRL) Mod |= MOD_CONTROL;
	if (HotKey & HOTKEY_WIN) Mod |= MOD_WIN;
	HotKey &= HOTKEY_MASK;
	if (FuncUnregisterFunc1)
		FuncUnregisterFunc1(Mod,HotKey);
	RegisterHotKey(p->Wnd,Id,Mod,HotKey);
#endif
}

void WinUnRegisterHotKey(win* p,int Id)
{
#if defined(NDEBUG) || defined(TARGET_WINCE)
	UnregisterHotKey(p->Wnd,Id);
#endif
}

wincontrol* WinPropLabel(win* p, winunit* Top, const tchar_t* LockedMsg, const tchar_t* Value)
{
	wincontrol* Control;
	int x0,x1;
	int y2=*Top,y1=*Top;
	WinProp(p,&x0,&x1);
	Control = WinLabel(p,&y2,x1,-1,Value,PROPSIZE,LABEL_BOLD,NULL);
	if (Control && !(p->Flags & WIN_NOTABSTOP))
		Control->Disabled = 0; // tab stop
	WinLabel(p,&y1,x0,x1-x0-2,LockedMsg,PROPSIZE,0,Control);
	*Top = y1>y2 ? y1:y2;
	return Control;
}

static void WinUpdateScroll(win* p);
static void WinNext(win* p,bool_t Prev);

int WinKeyState(int Key)
{
	if (GetKeyState(VK_CONTROL)<0) Key |= HOTKEY_CTRL;
	if (GetKeyState(VK_SHIFT)<0) Key |= HOTKEY_SHIFT;
	if (GetKeyState(VK_MENU)<0) Key |= HOTKEY_ALT;
	if (GetKeyState(VK_LWIN)<0 || GetKeyState(VK_RWIN)<0) Key |= HOTKEY_WIN;
	//if (Key >= 0xC1 && Key <= 0xCF)	Key |= HOTKEY_WIN; // h3800 ???
	return Key;
}

void WinAllKeys(bool_t State)
{
	if (FuncAllKeys)
		FuncAllKeys(State);
}

void* WinCursorArrow()
{
#if defined(TARGET_WINCE)
	return NULL;
#else
	return LoadCursor(NULL,IDC_ARROW);
#endif
}

static void WinControlDelete(win* p,wincontrol* Control)
{
	wincontrol** i;
	for (i=&p->Controls;*i && *i!=Control;i=&(*i)->Next);
	if (*i)
		*i = Control->Next;
	if (!Control->External)
	{
		if (Control->Control)
			DestroyWindow(Control->Control);
		if (Control->Edit)
			DestroyWindow(Control->Edit);
	}
	free(Control);
}

static void WinClear( win* p)
{
	p->Focus = NULL;
	while (p->Controls)
		WinControlDelete(p,p->Controls);
}

void WinBeginUpdate(win* p)
{
	ShowWindow(p->WndDialog,SW_HIDE);
	WinClear(p);
}

void WinEndUpdate(win* p)
{
	if (!p->Focus)
		WinNext(p,0);
	ShowWindow(p->WndDialog,SW_SHOWNA);
	WinUpdateScroll(p);
}

void WinMenuInsert(win* p,int No,int PrevId,int Id,const tchar_t* LockedMsg)
{
	if (p->Menu2[No])
		InsertMenu(p->Menu2[No],PrevId,MF_BYCOMMAND|MF_STRING,Id,LockedMsg);
}

bool_t WinMenuDelete(win* p,int No,int Id)
{
	if (p->Menu2[No])
		return DeleteMenu(p->Menu2[No],Id,MF_BYCOMMAND)!=0;
	return 0;
}

bool_t WinMenuEnable(win* p,int No,int Id,bool_t State)
{
	if (p->Menu2[No])
		return EnableMenuItem(p->Menu2[No],Id,(State ? MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND) != -1;
	return 0;
}

int WinMenuFind(win* p,int Id)
{
	int No;
	MENUITEMINFO Info;

	if (p->Menu3)
		return 0;

	Info.cbSize = sizeof(Info);
	Info.fMask = 0;

	for (No=0;No<2;++No)
	{
		if (p->Menu2[No] && GetMenuItemInfo(p->Menu2[No],Id,FALSE,&Info))
			return No;
	}

	return 0;
}	

bool_t WinMenuCheck(win* p,int No,int Id,bool_t State)
{
	if (p->Menu2[No])
		return CheckMenuItem(p->Menu2[No],Id,State?MF_CHECKED:MF_UNCHECKED) != -1;
	return 0;
}

void WinTitle(win* p,const tchar_t* LockedMsg)
{
	SetWindowText(p->Wnd,LockedMsg);
}

int WinClose(win* p)
{
	PostMessage(p->Wnd,WM_CLOSE,0,0);
	return ERR_NONE;
}

wincontrol* WinLabel(win* p,winunit* DlgTop,winunit DlgLeft,winunit DlgWidth,const tchar_t* Msg,int FontSize,int Flags,wincontrol* Ref)
{
	tchar_t s[256];
	wincontrol* Result = NULL;
	SIZE Size;
	HDC DC = GetDC(p->Wnd);
	HFONT OldFont;
	HFONT Font = (HFONT)WinFont(p,&FontSize,(Flags & LABEL_BOLD)!=0);
	int Style = WS_VISIBLE | WS_CHILD;
	int Left,Width;
	int Extra = WinUnitToPixelX(p,2);

	if (DlgLeft<0)
	{
		DlgLeft = 2;
		DlgWidth = p->Width-4;
	}

	Left = WinUnitToPixelX(p,DlgLeft);
	Width = DlgWidth>0 ? WinUnitToPixelX(p,DlgWidth) : -1;

	if (Ref)
		Style |= SS_NOTIFY;

	if (Flags & LABEL_CENTER)
		Style |= SS_CENTER;
	else
		Style |= SS_LEFTNOWORDWRAP;

	OldFont = SelectObject(DC,Font);

	if (Msg)
		while (*Msg)
		{
			while (*Msg == 10)
			{
				++Msg;
				*DlgTop += FontSize/3; // smaller gaps
			}

			if (*Msg)
			{
				tchar_t* i;
				
				wincontrol* Control = CreateControl(p,sizeof(wincontrol));
				if (!Control)
					break;

				tcscpy_s(s,TSIZEOF(s),Msg);

				i = tcschr(s,10);
				if (i) *i = 0;

				for (;;)
				{
					GetTextExtentPoint(DC,s,tcslen(s),&Size);
					Size.cx += Extra;

					if (!s[0] || !s[1] || Width<0 || Size.cx <= Width)
						break;

					i = tcsrchr(s,' ');
					if (i)
						*i = 0;
					else
						s[tcslen(s)-1] = 0;
				}

				Control->Disabled = 1; // no user input
				Control->Ref = Ref;
				Control->Control = CreateWindow(T("STATIC"),s,Style,Left,WinUnitToPixelY(p,*DlgTop),
					Width>0 ? Width : Size.cx, WinUnitToPixelY(p,FontSize), p->WndDialog, NULL, p->Module, NULL );

				SetWindowLong(Control->Control,GWL_USERDATA,(LONG)Control);
				SendMessage(Control->Control,WM_SETFONT,(WPARAM)Font,0);

				*DlgTop += FontSize;
				Msg += tcslen(s);

				while (*Msg == ' ') ++Msg;
				if (*Msg == 10) ++Msg;

				if (!Result)
					Result = Control;
			}
		}

	SelectObject(DC,OldFont);
	ReleaseDC(p->Wnd,DC);
	return Result;
}

void* WinFont(win* p,winunit* FontSize,bool_t Bold)
{
	HFONT Font;
	int Size = WinUnitToPixelY(p,*FontSize);
	if (Size && Size < MAXFONTSIZE)
	{
		if (!FontCache[Size][Bold])
		{
			LOGFONT LogFont;

			memset(&LogFont,0,sizeof(LogFont));
			tcscpy_s(LogFont.lfFaceName,TSIZEOF(LogFont.lfFaceName),T("MS Sans Serif"));

			LogFont.lfHeight = Size;
			LogFont.lfWidth = 0;

			switch (Context()->Lang)
			{
			case FOURCC('A','R','_','_'):
				LogFont.lfCharSet = ARABIC_CHARSET;
				break;
			case FOURCC('C','H','T','_'):
				LogFont.lfCharSet = CHINESEBIG5_CHARSET;
				break;
			case FOURCC('C','H','S','_'):
				LogFont.lfCharSet = GB2312_CHARSET;
				break;
			case FOURCC('R','U','_','_'):
				LogFont.lfCharSet = RUSSIAN_CHARSET;
				break;
			case FOURCC('J','A','_','_'):
				LogFont.lfCharSet = SHIFTJIS_CHARSET;
				break;
			case FOURCC('T','R','_','_'):
				LogFont.lfCharSet = TURKISH_CHARSET;
				break;
			case FOURCC('K','O','_','_'):
				LogFont.lfCharSet = HANGEUL_CHARSET;
				break;
			default:
				LogFont.lfCharSet = OEM_CHARSET;
			}

			if (Bold)
				LogFont.lfWeight = FW_BOLD;

			FontCache[Size][Bold] = CreateFontIndirect(&LogFont);
		}
		Font = FontCache[Size][Bold];
	}
	else
	{
		LOGFONT LogFont;
		Font = (HFONT)GetStockObject(SYSTEM_FONT);
		GetObject(Font,sizeof(LogFont),&LogFont);
		*FontSize = WinPixelToUnitY(p,abs(LogFont.lfHeight));
	}
	return Font;
}

static void UpdateDPI(win* p)
{
	HDC DC = GetDC(p->Wnd);
	p->LogPixelSX = GetDeviceCaps(DC, LOGPIXELSY);
	p->LogPixelSY = GetDeviceCaps(DC, LOGPIXELSX);
	p->ScreenWidth = WinPixelToUnitY(p,GetDeviceCaps(DC,HORZRES));
	p->MenuSmall = WinPixelToUnitY(p,GetDeviceCaps(DC,VERTRES)) < 210;
	ReleaseDC(p->Wnd,DC);
}

static void VScroll(win* p,int Cmd,int Delta);
static void HScroll(win* p,int Cmd,int Delta);

static void UpdateDialogSize(win* p)
{
	RECT r;
	GetClientRect(p->WndDialog,&r);

	if (!p->ScrollV)
		r.right -= GetSystemMetrics(SM_CXVSCROLL);

	if (!p->ScrollH)
		r.bottom -= GetSystemMetrics(SM_CYHSCROLL);

	p->Width = WinPixelToUnitX(p,r.right);
	p->Height = WinPixelToUnitY(p,r.bottom);
}

static void WinControlFocus(win* p,wincontrol* Control,bool_t Set)
{
	wincontrol* i;

	if (p->Focus == Control)
		return;

	if (Control && Control->Disabled)
		return;

	if (p->Focus)
		for (i=p->Controls;i;i=i->Next)
			if (i->Ref == p->Focus)
				InvalidateRect(i->Control,NULL,TRUE);

	p->Focus = Control;

	if (Control)
	{
		POINT o;
		RECT r;
		int Min=MAX_INT;
		int Max=-MAX_INT;

		if (Control->Control && GetParent(Control->Control)==p->WndDialog)
		{
			GetWindowRect(Control->Control,&r);
			if (r.bottom > Max)
				Max = r.bottom;
			if (r.top < Min)
				Min = r.top;
		}

		for (i=p->Controls;i;i=i->Next)
			if (i->Ref == Control)
			{
				GetWindowRect(i->Control,&r);
				if (r.bottom > Max)
					Max = r.bottom;
				if (r.top < Min)
					Min = r.top;
				InvalidateRect(i->Control,NULL,TRUE);
			}

		if (Max > Min)
		{
			o.x = o.y = 0;
			ClientToScreen(p->WndDialog,&o);
			GetClientRect(p->WndDialog,&r);

			r.top += o.y;
			r.bottom += o.y;

			if (r.top > Min)
				VScroll(p,0,Max-r.bottom);
			else
			if (r.bottom < Max)
				VScroll(p,0,Min-r.top);
		}

		if (Set)
			if (Control->Control)
				SetFocus(Control->Control);
			else
				SetFocus(p->WndDialog);
	}
	else
	if (Set)
		SetFocus(p->WndDialog);
}

static void WinNext(win* p,bool_t Prev)
{
	wincontrol* Start = p->Focus;
	wincontrol* Last;
	wincontrol* i;

	if (!Start)
	{
		Start = p->Controls;
		if (!Start)
			return;
		if (!Start->Disabled)
		{
			WinControlFocus(p,Start,1);
			return;
		}
	}

	Last = Start;
	do
	{
		if (Prev)
		{
			for (i=p->Controls;i->Next && i->Next!=Last;i=i->Next);
		}
		else
		{
			i = Last->Next;
			if (!i) 
				i=p->Controls;
		}
		Last = i;
	}
	while (i && i!=Start && i->Disabled);

	WinControlFocus(p,i,1);
}

void WinPropFocus(win* p, node* Node, int No, bool_t SetFocus)
{
	wincontrol* Control;
	for (Control=p->Controls;Control;Control=Control->Next)
		if (Control->Pin.Node == Node && Control->Pin.No == No)
		{
			WinControlFocus(p,Control,SetFocus);
			break;
		}
}

void WinPropUpdate(win* p, node* Node, int No)
{
	wincontrol* Control;
	for (Control=p->Controls;Control;Control=Control->Next)
		if (Control->Pin.Node == Node && Control->Pin.No == No)
			WinControlUpdate(p,Control);
}

static LRESULT CALLBACK DialogProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	wincontrol* i;
	int Result;
	win* p = (win*)GetWindowLong(Wnd,GWL_USERDATA);

	if (p && p->DialogProc && p->DialogProc(p,Msg,wParam,lParam,&Result))
		return Result;

	switch (Msg)
	{
	case WM_CREATE:
		p = (win*)((CREATESTRUCT*)lParam)->lpCreateParams;
		p->WndDialog = Wnd;
		p->Controls = NULL;
		p->Focus = NULL;
		SetWindowLong(Wnd,GWL_USERDATA,(LONG)p);
		break;

	case WM_COMMAND:
		i = (wincontrol*) GetWindowLong((HWND)lParam,GWL_USERDATA);
		if (VALIDCONTROL(i))
			WinControlCmd(p,i,(HWND)lParam,HIWORD(wParam));
		break;

	case WM_SIZE:
		if (p)
			UpdateDialogSize(p);
		break;

	case WM_SETFOCUS:
		if (p->Focus && p->Focus->Control)
			SetFocus(p->Focus->Control);
		break;

	case WM_DESTROY:
		p->WndDialog = NULL;
		break;

	case WM_HSCROLL:
		if (!lParam || !(GetWindowLong((HWND)lParam,GWL_STYLE) & (TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH)))
			HScroll(p,LOWORD(wParam),0);
		else
		{
			i = (wincontrol*) GetWindowLong((HWND)lParam,GWL_USERDATA);
			if (VALIDCONTROL(i))
			{
				WinControlFocus(p,i,1);
				SetTrack(p,i,SendMessage((HWND)lParam,TBM_GETPOS,0,0));
			}
		}
		break;
	case WM_VSCROLL:
		if (!lParam || !(GetWindowLong((HWND)lParam,GWL_STYLE) & (TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH)))
			VScroll(p,LOWORD(wParam),0);
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			WinClose(p);
		if (p->ScrollV && wParam == VK_UP)
			VScroll(p,SB_LINEUP,0);
		if (p->ScrollV && wParam == VK_DOWN)
			VScroll(p,SB_LINEDOWN,0);
		if (p->ScrollH && wParam == VK_LEFT)
			HScroll(p,SB_LINEUP,0);
		if (p->ScrollH && wParam == VK_RIGHT)
			HScroll(p,SB_LINEDOWN,0);
		break;

#if defined(TARGET_WINCE)
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam,GetSysColor(COLOR_BTNFACE));
		return (LRESULT) GetSysColorBrush(COLOR_BTNFACE);
#endif

	case WM_CTLCOLORSTATIC:
		i = (wincontrol*)GetWindowLong((HWND)lParam,GWL_USERDATA);
		if (i && p->Focus && p->Focus == i->Ref)
		{
			if (p->Smartphone)
			{
				SetTextColor((HDC)wParam,GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor((HDC)wParam,GetSysColor(COLOR_HIGHLIGHT));
				return (LRESULT) GetSysColorBrush(COLOR_HIGHLIGHT);
			}
			SetTextColor((HDC)wParam,0xC00000);
		}
		SetBkColor((HDC)wParam,GetSysColor(DIALOG_COLOR));
		return (LRESULT) GetSysColorBrush(DIALOG_COLOR);
	}

	return DefWindowProc(Wnd,Msg,wParam,lParam);
}

static void WinScroll(win* p,int dx,int dy)
{
	ScrollWindowEx(p->WndDialog, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE );
}

static void WinUpdateScroll(win* p)
{
	SCROLLINFO Scroll;

	POINT Offset;
	RECT Page;
	int MaxWidth,MinWidth;
	int MaxHeight,MinHeight;
	HWND Child;
	int XVScroll = GetSystemMetrics(SM_CXVSCROLL);
	int YHScroll = GetSystemMetrics(SM_CYHSCROLL);

	if (p->InUpdateScroll || p->OwnDialogSize)
		return;

	p->InUpdateScroll = 1;
	
	Offset.x = Offset.y = 0;
	ClientToScreen(p->WndDialog,&Offset);
	GetClientRect(p->WndDialog,&Page);

	if (p->ScrollV)
		Page.right += XVScroll;

	if (p->ScrollH)
		Page.bottom += YHScroll;

	MinWidth = 0;
	MinHeight = 0;
	MaxWidth = 0;
	MaxHeight = 0;

	Child = GetWindow(p->WndDialog,GW_CHILD);
	while (Child && IsWindow(Child))
	{
		RECT r;
		GetWindowRect(Child,&r);

		r.left -= Offset.x;
		r.right -= Offset.x; 
		r.top -= Offset.y;
		r.bottom -= Offset.y; 
		r.bottom += YHScroll;

		if (r.left < MinWidth) MinWidth = r.left;
		if (r.right > MaxWidth) MaxWidth = r.right;
		if (r.top < MinHeight) MinHeight = r.top;
		if (r.bottom > MaxHeight) MaxHeight = r.bottom;

		Child = GetWindow(Child,GW_HWNDNEXT);
	}

	Scroll.cbSize = sizeof(Scroll);
	Scroll.fMask =  SIF_PAGE | SIF_RANGE | SIF_POS;

	if (MaxHeight < Page.bottom)
		MaxHeight = Page.bottom-1;

	p->ScrollV = MaxHeight-MinHeight > Page.bottom;
	Scroll.nMin = MinHeight;
	Scroll.nMax = MaxHeight;
	Scroll.nPos = 0;
	Scroll.nPage = Page.bottom;
	SetScrollInfo(p->WndDialog,SB_VERT,&Scroll,TRUE);

	if (p->ScrollV)
		Page.right -= XVScroll;

	if (MaxWidth < Page.right)
		MaxWidth = Page.right-1;

	p->ScrollH = MaxWidth-MinWidth > Page.right;
	Scroll.nMin = MinWidth;
	Scroll.nMax = MaxWidth;
	Scroll.nPos = 0;
	Scroll.nPage = Page.right;
	SetScrollInfo(p->WndDialog,SB_HORZ,&Scroll,TRUE);

	p->InUpdateScroll = 0;
}

static void VScroll(win* p,int Cmd,int Delta)
{
	int Pos;
	SCROLLINFO Scroll;

	Scroll.cbSize = sizeof(Scroll);
	Scroll.fMask  = SIF_ALL;
	GetScrollInfo(p->WndDialog, SB_VERT, &Scroll);

	Pos = Scroll.nPos;

	if (Delta)
		Scroll.nPos += Delta;
	else
	switch (Cmd)
	{
	case SB_TOP:
		Scroll.nPos = Scroll.nMin;
		break;
	case SB_BOTTOM:
		Scroll.nPos = Scroll.nMax;
		break;
	case SB_LINEUP:
		Scroll.nPos -= WinUnitToPixelY(p,16);
		break;
	case SB_LINEDOWN:
		Scroll.nPos += WinUnitToPixelY(p,16);
		break;
	case SB_PAGEUP:
		Scroll.nPos -= Scroll.nPage;
		break;
	case SB_PAGEDOWN:
		Scroll.nPos += Scroll.nPage;
		break;
	case SB_THUMBTRACK:
		Scroll.nPos = Scroll.nTrackPos;
		break;
	default:
		break;
	}

	Scroll.fMask = SIF_POS;
	SetScrollInfo(p->WndDialog, SB_VERT, &Scroll, TRUE);
	GetScrollInfo(p->WndDialog, SB_VERT, &Scroll);

	if (Scroll.nPos != Pos)
		WinScroll(p,0,Pos - Scroll.nPos);
}

static void HScroll(win* p,int Cmd,int Delta)
{
	int Pos;
	SCROLLINFO Scroll;

	Scroll.cbSize = sizeof(Scroll);
	Scroll.fMask  = SIF_ALL;
	GetScrollInfo(p->WndDialog, SB_HORZ, &Scroll);

	Pos = Scroll.nPos;

	if (Delta)
		Scroll.nPos += Delta;
	else
	switch (Cmd)
	{
	case SB_TOP:
		Scroll.nPos = Scroll.nMin;
		break;
	case SB_BOTTOM:
		Scroll.nPos = Scroll.nMax;
		break;
	case SB_LINEUP:
		Scroll.nPos -= WinUnitToPixelX(p,16);
		break;
	case SB_LINEDOWN:
		Scroll.nPos += WinUnitToPixelX(p,16);
		break;
	case SB_PAGEUP:
		Scroll.nPos -= Scroll.nPage;
		break;
	case SB_PAGEDOWN:
		Scroll.nPos += Scroll.nPage;
		break;
	case SB_THUMBTRACK:
		Scroll.nPos = Scroll.nTrackPos;
		break;
	default:
		break;
	}

	Scroll.fMask = SIF_POS;
	SetScrollInfo(p->WndDialog, SB_HORZ, &Scroll, TRUE);
	GetScrollInfo(p->WndDialog, SB_HORZ, &Scroll);

	if (Scroll.nPos != Pos)
		WinScroll(p,Pos - Scroll.nPos,0);
}

static bool_t OwnWindow(HWND Wnd)
{
	tchar_t ClassName[64];
	for (;Wnd;Wnd=GetParent(Wnd))
	{
		GetClassName(Wnd,ClassName,64);
		if (tcscmp(ClassName,WinClass.lpszClassName)==0 || 
			tcscmp(ClassName,T("VolumeBack"))==0)
			return 1;
	}
	return 0;
}

void ForwardMenuButtons(win* p,int Msg,uint32_t wParam,uint32_t lParam)
{
#if defined(TARGET_WINCE)
	if (p->WndTB && (wParam == VK_F1 || wParam == VK_F2) &&
		(QueryPlatform(PLATFORM_TYPENO) == TYPE_POCKETPC || 
		 QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE))
	{
		if (Msg == WM_KEYDOWN && !(lParam & (1<<30)))
		{
			PostMessage(p->WndTB,WM_HOTKEY,0,MAKELPARAM(0,wParam));
			PostMessage(p->WndTB,WM_HOTKEY,0,MAKELPARAM(0x1000,wParam));
		}
	}
#endif
}

static LRESULT CALLBACK Proc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int Result = 0;
	win* p = (win*)GetWindowLong(Wnd,GWL_USERDATA);

	switch (Msg)
	{
#if defined(TARGET_WINCE)
	case WM_HOTKEY:
		if (p->Parent && HIWORD(lParam)==VK_ESCAPE)
		{
			HWND Focus = GetFocus();
			wincontrol* i = (wincontrol*)GetWindowLong(Focus,GWL_USERDATA);
			if (FuncSHSendBackToFocusWindow && ((VALIDCONTROL(i) && i->Editor) || (!VALIDCONTROL(i) && i)))
				FuncSHSendBackToFocusWindow(Msg,wParam,lParam);
			else
			{
				p->HotKeyEscape = !p->HotKeyEscape;
				if (!p->HotKeyEscape)
					PostMessage(p->Wnd,WM_CLOSE,0,0);
			}
			return 0;
		}
		break;
#endif

	case WM_CREATE:

		p = (win*)((CREATESTRUCT*)lParam)->lpCreateParams;
		p->Wnd = Wnd;
		p->Module = ((CREATESTRUCT*)lParam)->hInstance;
		p->Result = 1;
		p->Focus = NULL;
		p->Closed = 0;
		
		UpdateDPI(p);

#if defined(TARGET_WINCE)
		p->Activate = malloc(sizeof(SHACTIVATEINFO));
		if (p->Activate)
		{
			memset(p->Activate,0,sizeof(SHACTIVATEINFO));
			((SHACTIVATEINFO*)p->Activate)->cbSize = sizeof(SHACTIVATEINFO);
		}
#endif

		if (p->Proc)
			p->Proc(p,MSG_PREPARE,0,0,&Result);
		CreateToolBar(p);

		SetWindowLong(Wnd,GWL_USERDATA,(LONG)p); // only after CreateToolBar (WM_MOVE)

		if (p->Flags & WIN_DIALOG)
		{
			RECT r;
			GetClientRect(p->Wnd,&r);
			CreateWindow(DialogClass.lpszClassName,NULL,WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE,
				0,p->ToolBarHeight,r.right,r.bottom-p->ToolBarHeight,p->Wnd,NULL,DialogClass.hInstance,p);
		}

		if (p->Init)
			p->Init(p);

		if (!p->Focus)
			WinNext(p,0);
		break;

	case WM_CLOSE:

		p->Closed = 1;
		p->Focus = NULL;
		SetFocus(NULL);

		if (p->Done)
			p->Done(p);

		if (p->WndDialog)
			ShowWindow(p->WndDialog,SW_HIDE);
		WinClear(p);

		if (p->Parent)
		{
			EnableWindow(p->Parent->Wnd,TRUE);
			p->Parent->Child = NULL;
			SetForegroundWindow(p->Parent->Wnd);
			DestroyWindow(p->Wnd);
		}
		break;

	case WM_SETFOCUS:
		if (p->WndDialog)
			SetFocus(p->WndDialog);
		if (!p->Parent)
			WidcommAudio_Wnd(p->Wnd);
		break;

	case WM_SIZE:
		if (p)
		{
			RECT r;
			GetClientRect(p->Wnd,&r);

			if (p->ToolBarHeight)
				MoveWindow(p->WndTB,0,0,r.right,p->ToolBarHeight,TRUE);

			if (p->WndDialog && !p->OwnDialogSize)
				MoveWindow(p->WndDialog,0,p->ToolBarHeight,r.right,r.bottom-p->ToolBarHeight,TRUE);
		}
		break;

	case WM_DESTROY:
		WinSetFullScreen(p,0);
		DestroyToolBar(p);
		if (p->Menu3)
			DestroyMenu(p->Menu3);
		p->Wnd = NULL;
		p->WndTB = NULL;
		memset(p->Menu2,0,sizeof(p->Menu2));
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == WIN_A)
			wParam = p->WinCmd[0];
		if (LOWORD(wParam) == WIN_B)
			wParam = p->WinCmd[1];
		if (LOWORD(wParam) == IDOK)
			wParam = PLATFORM_OK;
		if (p->Command && p->Command(p,LOWORD(wParam))==ERR_NONE)
			return 0;
		if (LOWORD(wParam) == PLATFORM_OK ||
			LOWORD(wParam) == PLATFORM_DONE ||
			LOWORD(wParam) == PLATFORM_CANCEL)
		{
			WinClose(p);
			return 0;
		}
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam)!=WA_INACTIVE)
		{
			if (WidcommAudio_SkipActivate())
				return 0;
			if (!p->Parent)
				WidcommAudio_Wnd(p->Wnd);
			if (p->Child)
			{
				SetForegroundWindow(p->Child->Wnd);
				return 0;
			}
		}

#if defined(TARGET_WINCE)
		if (FuncSHHandleWMActivate && p->Activate && !TaskBarHidden)
			FuncSHHandleWMActivate(Wnd, wParam, lParam, p->Activate, FALSE);
#endif
		break;

	case WM_SETTINGCHANGE:
#if defined(TARGET_WINCE) 
		if (FuncSHHandleWMSettingChange && p->Activate && !TaskBarHidden)
			FuncSHHandleWMSettingChange(Wnd, wParam, lParam, p->Activate);
#endif
		UpdateDPI(p);
		break;
	}

	if (p)
	{
		if (p->Proc && p->Proc(p,Msg,wParam,lParam,&Result))
			return Result;

		if (Msg == WM_ACTIVATE && !OwnWindow((HWND)lParam))
		{
			if (LOWORD(wParam)==WA_INACTIVE)
			{
				bool_t b = 1;
				node* Player = Context()->Player;
				if (Player && Main && !Main->Closed)
					Player->Set(Player,PLAYER_BACKGROUND,&b,sizeof(b));

#if defined(TARGET_WINCE)
				if (TaskBarHidden)
				{
					DIASet(DIA_TASKBAR,DIA_TASKBAR);
					TaskBarHidden = 1;
				}
#endif
			}
			else
			{
				bool_t b = 0;
				node* Player = Context()->Player;
				if (Player && Main && !Main->Closed)
					Player->Set(Player,PLAYER_BACKGROUND,&b,sizeof(b));

#if defined(TARGET_WINCE)
				if (TaskBarHidden)
				{
					TaskBarHidden = 0;
					DIASet(0,DIA_TASKBAR);
				}
#endif
			}
		}

#if defined(TARGET_WINCE)
		if (Msg == WM_CREATE && !p->AygShellTB && ((p->Flags & WIN_DIALOG) || GetSystemMetrics(SM_CXSCREEN)>240))
			CommandBar_AddAdornments(p->WndTB,p->NeedOK ? CMDBAR_OK:0,0);
#endif
		if (p->WndDialog && (Msg == WM_CREATE || Msg == WM_SIZE))
			WinUpdateScroll(p);
	}

	return DefWindowProc(Wnd,Msg,wParam,lParam);
}

static void HandleMessage(win* p,MSG* Msg)
{
	int Result;

#if defined(TARGET_WINCE)
	if (Msg->message == WM_HIBERNATE)
	{
		if (AvailMemory() < 256*1024) // don't mess up format_base buffering...
			NodeHibernate();
	}
#endif

	if (Msg->message >= WM_APP + 0x200 && 
		Msg->message <= WM_APP + 0x220)
	{
		// send to application window
		win* Appl=p;
		while (Appl->Parent)
			Appl=Appl->Parent;

		if (Appl->Proc(Appl,Msg->message,Msg->wParam,Msg->lParam,&Result))
			return;
	}

	if (Msg->message == WM_KEYDOWN)
	{
		if (p->CaptureKeys && p->Proc && p->Proc(p,Msg->message,Msg->wParam,Msg->lParam,&Result))
			return;

		if (p->Focus)
		{
			int Type;
			switch (Msg->wParam)
			{
			case VK_LEFT:
			case VK_RIGHT:
				if (!p->Focus->ListView)
				{
					Type = GetDataDefType(p->Focus);
					if (!Type || Type == TYPE_BOOL || Type == TYPE_RESET || Type == TYPE_HOTKEY)
					{
						SendMessage(p->WndDialog,Msg->message,Msg->wParam,Msg->lParam);
						return;
					}
				}
				break;
			case VK_RETURN:
				Type = GetDataDefType(p->Focus);
				if (Type == TYPE_BOOL || Type == TYPE_RESET || Type == TYPE_HOTKEY)
				{
					SendMessage(p->Focus->Control,BM_CLICK,0,0);
					return;
				}
				if (p->Focus->ComboBox && !p->ComboOpen)
				{
					SendMessage(p->Focus->Control,CB_SHOWDROPDOWN,1,0);
					return;
				}
				if (Type == TYPE_STRING)
				{
					SendMessage(p->WndDialog,Msg->message,Msg->wParam,Msg->lParam);
					return;
				}
				break;
			case VK_DOWN:
			case VK_UP:
				if (!p->ComboOpen && !p->Focus->ListView)
				{
					WinNext(p,Msg->wParam==VK_UP);
					return;
				}
				break;
			case VK_TAB:
				WinNext(p,GetKeyState(VK_SHIFT)<0);
				return;
			}
		}
	}

	TranslateMessage(Msg);
	DispatchMessage(Msg);
}

int WinPopupClass(int Class,win* Parent)
{
	int Result = 0;
	node* p = NodeCreate(Class);
	if (p && NodeIsClass(p->Class,WIN_CLASS))
	{
		Result = ((win*)p)->Popup((win*)p,Parent);
		NodeDelete(p);
	}
	return Result;
}	

static int Popup(win* p,win* Parent)
{
	HWND Wnd;
	MSG Msg;
	int Style = WS_VISIBLE;
	int ExStyle = 0;
	int x,y;
	int Width,Height;
	int Priority;

	// we need higher priority in main window for better user interface responses
	// but open dialog and other parts can't have that, because of some buggy keyboard drivers...
	Priority = ThreadPriority(NULL,Parent?0:-1);

	p->Result = 0;
	p->Parent = Parent;

	p->BitmapNo = 0;
	p->AygShellTB = 0;

#if defined(TARGET_WINCE)
	if (AygShell && Parent)
		ExStyle |= WS_EX_CAPTIONOKBTN;

	if (Parent)
		Style |= WS_POPUP;

	{
		RECT r;
		GetWorkArea(p,&r);
		x = r.left;
		y = r.top;
		Width = r.right - r.left;
		Height = r.bottom - r.top;
	}
#else
	Style |= WS_OVERLAPPEDWINDOW;
	y = x = CW_USEDEFAULT;
	Width = WinUnitToPixelX(p,p->WinWidth);
	Height = WinUnitToPixelY(p,p->WinHeight);
#endif

	Wnd = CreateWindowEx(ExStyle,WinClass.lpszClassName,LangStr(p->Node.Class,NODE_NAME),Style,x,y,Width,Height,
		Parent?Parent->Wnd:NULL,NULL,WinClass.hInstance,p);

	if (Wnd)
	{
		if (p->Parent)
		{
			p->Parent->Child = p;
			EnableWindow(p->Parent->Wnd,0);
		}
		else
			Main = p;

		while (p->Wnd && GetMessage(&Msg, NULL, 0, 0)) 
			HandleMessage(p,&Msg);

		if (Main == p)
			Main = NULL;
	}
	
	ThreadPriority(NULL,Priority);
	return p->Result;
}

static int Create(win* p)
{
	UpdateDPI(p); // with NULL Wnd, because we need ScreenWidth
	p->Smartphone = QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE;
	p->Popup = Popup;
	p->Closed = 1;
	return ERR_NONE;
}

static const nodedef Win = 
{
	CF_ABSTRACT,
	WIN_CLASS,
	NODE_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create
};

void Win_Init()
{
	HMODULE Module = Context()->LoadModule;
	if (!Module) Module = GetModuleHandle(NULL);

	InitCommonControls();
	WidcommAudio_Init();

	stprintf_s(WinClassName,TSIZEOF(WinClassName),T("%s_Win"),Context()->ProgramName);

	memset(&WinClass,0,sizeof(WinClass));
    WinClass.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    WinClass.lpfnWndProc	= Proc;
    WinClass.cbClsExtra		= 0;
    WinClass.cbWndExtra		= 0;
    WinClass.hInstance		= Module;
	WinClass.hIcon			= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(WIN_ICON));
    WinClass.hCursor		= WinCursorArrow();
    WinClass.hbrBackground	= NULL;
    WinClass.lpszMenuName	= 0;
    WinClass.lpszClassName	= WinClassName;
	RegisterClass(&WinClass);

	memset(&DialogClass,0,sizeof(DialogClass));
    DialogClass.style			= CS_HREDRAW | CS_VREDRAW;
    DialogClass.lpfnWndProc		= DialogProc;
    DialogClass.cbClsExtra		= 0;
    DialogClass.cbWndExtra		= 0;
    DialogClass.hInstance		= Module;
    DialogClass.hCursor			= WinCursorArrow();
#if defined(TARGET_WINCE)
	DialogClass.hbrBackground	= GetSysColorBrush(COLOR_STATIC);
#else			
	DialogClass.hbrBackground	= GetSysColorBrush(COLOR_BTNFACE);
#endif
    DialogClass.lpszMenuName	= 0;
    DialogClass.lpszClassName	= T("DialogBase");
	RegisterClass(&DialogClass);

	memset(&FontCache,0,sizeof(FontCache));

#if defined(TARGET_WINCE)
	if (Context()->ProgramId >= 3 && !QueryAdvanced(ADVANCED_OLDSHELL))
	{
		AygShell = LoadLibrary(T("aygshell.dll"));
		*(FARPROC*)&FuncSHCreateMenuBar = GetProcAddress(AygShell,T("SHCreateMenuBar"));
		*(FARPROC*)&FuncSHInitDialog = GetProcAddress(AygShell,T("SHInitDialog"));
		*(FARPROC*)&FuncSHFullScreen = GetProcAddress(AygShell,T("SHFullScreen"));
		*(FARPROC*)&FuncSHHandleWMActivate = GetProcAddress(AygShell,MAKEINTRESOURCE(84));
		*(FARPROC*)&FuncSHHandleWMSettingChange = GetProcAddress(AygShell,MAKEINTRESOURCE(83));
		*(FARPROC*)&FuncSHSendBackToFocusWindow = GetProcAddress(AygShell,MAKEINTRESOURCE(97));
	}
	CoreDLL = LoadLibrary(T("coredll.dll"));
	*(FARPROC*)&FuncUnregisterFunc1 = GetProcAddress(CoreDLL,T("UnregisterFunc1"));
	*(FARPROC*)&FuncAllKeys = GetProcAddress(CoreDLL,T("AllKeys")); //MAKEINTRESOURCE(1453));
	*(FARPROC*)&FuncSipShowIM = GetProcAddress(CoreDLL,T("SipShowIM"));
	*(FARPROC*)&FuncSipGetInfo = GetProcAddress(CoreDLL,T("SipGetInfo"));
#endif
	NodeRegisterClass(&Win);
	QueryKey_Init();
	OpenFile_Init();
	Interface_Init();
	PlaylistWin_Init();
}

void Win_Done()
{
	int No;
	Interface_Done();
	PlaylistWin_Done();
	QueryKey_Done();
	OpenFile_Done();
	NodeUnRegisterClass(WIN_CLASS);
	for (No=0;No<MAXFONTSIZE;++No)
	{
		if (FontCache[No][0])
			DeleteObject(FontCache[No][0]);
		if (FontCache[No][1])
			DeleteObject(FontCache[No][1]);
	}
	UnregisterClass(DialogClass.lpszClassName,DialogClass.hInstance);
	UnregisterClass(WinClass.lpszClassName,WinClass.hInstance);
	WidcommAudio_Done();
#if defined(TARGET_WINCE)
	if (CoreDLL)
		FreeLibrary(CoreDLL);
	if (AygShell)
		FreeLibrary(AygShell);
#endif
}

void WinShowHTML(const tchar_t* Name)
{
	SHELLEXECUTEINFO Info;	
	tchar_t Path[MAXPATH];
	tchar_t Base[MAXPATH];
	tchar_t* p;
	
	GetModuleFileName(NULL,Base,MAXPATH);
	p = tcsrchr(Base,'\\');
	if (p) *p=0;
	AbsPath(Path,TSIZEOF(Path),Name,Base);

	memset(&Info,0,sizeof(Info));
	Info.cbSize = sizeof(Info);
	Info.lpVerb = T("open");
	Info.lpFile = Path;
	Info.nShow = SW_SHOW;

	DIASet(DIA_TASKBAR,DIA_TASKBAR);
	ShellExecuteEx(&Info);
}

#endif
