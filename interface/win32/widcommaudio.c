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
 * $Id: widcommaudio.c 332 2005-11-06 14:31:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "widcommaudio.h"

#if defined(TARGET_WINCE) && defined(ARM)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <shellapi.h>

#define MAXPOS		20

static const tchar_t RegWMP10[] = T("Software\\Microsoft\\MediaPlayer\\Skins");
static const tchar_t RegWMP9[] = T("Software\\Microsoft\\Windows Media Player\\Parameters");
static const tchar_t RegRotation[] = T("System\\GDI\\Rotation");
static const tchar_t Skin1[] = T("DefaultPortraitSkin");
static const tchar_t Skin2[] = T("DefaultLandscapeSkin");
static const tchar_t Skin3[] = T("DefaultSquareSkin");
static const tchar_t Skin4a[] = T("SkinDir");
static const tchar_t Skin4b[] = T("SkinFile");

static const tchar_t ClassName[] = T("WMP for Mobile Devices");
static const tchar_t TitleName[] = T("Windows Media");

static HWND Wnd = NULL;
static HWND Notify = NULL;
static LPARAM Pos[MAXPOS];
static DWORD Key[MAXPOS];
static int Count;
static bool_t TmpSkin = 0;
static bool_t WMP10Support = 0;
static DWORD Activate = 0;

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	if (hWnd != Wnd && !(GetWindowLong(hWnd,GWL_STYLE) & WS_POPUP))
	{
		tchar_t Name[64];
		if (GetWindowText(hWnd,Name,64)>0 && tcscmp(Name,TitleName)==0)
			*(HWND*)lParam = hWnd;
	}
	return TRUE;
}

static HWND FindWMP()
{
	HWND Result = NULL;
	EnumWindows(EnumWindowsProc,(LPARAM)&Result);
	return Result;
}

static LRESULT Proc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND WMP;
	int n;

	switch (Msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam)!=WA_INACTIVE) 
		{
			WMP = FindWMP();
			if (WMP)
			{
				SetForegroundWindow(WMP);
				return 0;
			}
		}
		break;

	case WM_COPYDATA:

		DestroyWindow(Wnd);
		Wnd = NULL;

		WMP = FindWMP();
		if (!WMP)
		{
			COPYDATASTRUCT *Data = (COPYDATASTRUCT*)lParam;
			tchar_t Path[MAXPATH];
			SHELLEXECUTEINFO Info;

			GetSystemPath(Path,TSIZEOF(Path),T("wmplayer.exe"));

			memset(&Info,0,sizeof(Info));
			Info.cbSize = sizeof(Info);
			Info.fMask = SEE_MASK_NOCLOSEPROCESS;
			Info.lpFile = Path;
			Info.nShow = SW_SHOW;
			Info.lpParameters = (tchar_t*)Data->lpData;
			ShellExecuteEx(&Info);
		}
		else
		{
			SendMessage(WMP,Msg,wParam,lParam);
			SetForegroundWindow(WMP);
		}

		Activate = GetTickCount();
		return 1;

	case WM_LBUTTONDOWN:
		if (Notify)
			for (n=0;n<Count;++n)
				if (Pos[n] == lParam)
				{
					SendMessage(Notify,WM_KEYDOWN,Key[n],0);
					break;
				}
		return 0;

	case WM_LBUTTONUP:
		if (Notify)
			for (n=0;n<Count;++n)
				if (Pos[n] == lParam)
				{
					SendMessage(Notify,WM_KEYUP,Key[n],0x80000000);
					break;
				}
		return 0;
	};
	return DefWindowProc(Wnd,Msg,wParam,lParam);
};

static void Override()
{
	HWND Found = FindWindow(ClassName,TitleName);
	if (!Wnd || Found != Wnd)
	{
		if (Wnd) DestroyWindow(Wnd);
		Wnd = CreateWindowEx(0,ClassName,TitleName,WS_POPUP,0,0,0,0,NULL,NULL,GetModuleHandle(NULL),NULL);
		ShowWindow(Wnd,SW_HIDE);
	}
}

static bool_t ReadSkin(const tchar_t* FileName)
{
	tchar_t s[MAXLINE];
	parser Parser = {NULL};
	stream* Skin = StreamOpen(FileName,0);
	if (!Skin)
		return 0;

	ParserStream(&Parser,Skin);
	while (ParserLine(&Parser,s,sizeof(s)))
	{
		int k = 0;
		tchar_t* p = s;

		tcsupr(s);
		while (*p && IsSpace(*p)) ++p;
	
		if (WMP10Support && tcsncmp(p,T("PLAYPAUSESTOP"),13)==0)
		{
			p += 13;
			k = 0xB3; //VK_MEDIA_PLAY_PAUSE;
		}
		else
		if (tcsncmp(p,T("PLAYPAUSE"),9)==0)
		{
			p += 9;
			k = 0xB3; //VK_MEDIA_PLAY_PAUSE;
		}
		else
		if (tcsncmp(p,T("PLAY"),4)==0)
		{
			p += 4;
			k = 0xB3; //VK_MEDIA_PLAY_PAUSE;
		}
		else
		if (tcsncmp(p,T("PAUSE"),5)==0)
		{
			p += 5;
			k = 0xB3; //VK_MEDIA_PLAY_PAUSE;
		}
		else
		if (tcsncmp(p,T("NEXT"),4)==0)
		{
			p += 4;
			k = 0xB0; //VK_MEDIA_NEXT_TRACK;
		}
		else
		if (tcsncmp(p,T("PREV"),4)==0)
		{
			p += 4;
			k = 0xB1; //VK_MEDIA_PREV_TRACK;
		}
		else
		if (tcsncmp(p,T("STOP"),4)==0)
		{
			p += 4;
			k = 0xB2; //VK_MEDIA_STOP;
		}

		if (k)
		{
			int x,y,w,h;

			while (*p && IsSpace(*p)) ++p;
			while (*p && !IsSpace(*p)) ++p;

			if (stscanf(p,T("%d,%d,%d,%d"),&x,&y,&w,&h)==4 && Count<MAXPOS)
			{
				Key[Count] = k;
				Pos[Count] = (x+(w>>1))|((y+(h>>1))<<16);
				++Count;
			}
		}
	}

	ParserStream(&Parser,NULL);
	StreamClose(Skin);
	return 1;
}

static void ProcessSkin(HKEY Key,const tchar_t* Name1, const tchar_t* Name2)
{
	tchar_t FileName[MAXPATH];
	DWORD Size = sizeof(FileName);

	if (RegQueryValueEx(Key,Name1,NULL,NULL,(LPBYTE)FileName, &Size) == ERROR_SUCCESS)
	{
		if (Name2)
		{
			tchar_t* Tail = FileName + tcslen(FileName);
			Size = sizeof(FileName) - (Tail - FileName)*sizeof(tchar_t);
			RegQueryValueEx(Key,Name2,NULL,NULL,(LPBYTE)Tail, &Size);
		}

		if (ReadSkin(FileName))
			return;
	}

	GetSystemPath(FileName,TSIZEOF(FileName),T("widcommaudio.skn"));

	Size = sizeof(FileName);
	RegSetValueEx(Key,Name1, 0, REG_SZ, (PBYTE)FileName, Size );
	if (Name2)
		RegSetValueEx(Key,Name2, 0, REG_SZ, (PBYTE)T(""), 0);

	if (!TmpSkin)
	{
		stream* Skin = StreamOpen(FileName,1);
		if (Skin)
		{
			StreamPrintf(Skin,
				T(" PlayPause      NA 200,100,0,0\n")
				T(" PlayPauseStop  NA 200,100,0,0\n")
				T(" Next           NA 216,100,0,0\n")
				T(" Prev           NA 232,100,0,0\n")
				T(" Play           NA 248,100,0,0\n")
				T(" Pause          NA 264,100,0,0\n")
				T(" Stop           NA 280,100,0,0\n"));

			StreamClose(Skin);
			TmpSkin = 1;
		}
	}

	ReadSkin(FileName);
}

void WidcommAudio_Init()
{
	Count = 0;
	if (QueryAdvanced(ADVANCED_WIDCOMMAUDIO))
	{
		bool_t WMP10;
		HDC DC;
		int Width,Height;
		DWORD Angle = 0;
		DWORD Disp;
		HKEY Key = NULL;
		WNDCLASS Class;
		tchar_t FileName[MAXPATH];

		memset(&Class,0,sizeof(Class));
		Class.style			= CS_HREDRAW | CS_VREDRAW;
		Class.lpfnWndProc	= (WNDPROC) Proc;
		Class.hInstance		= GetModuleHandle(NULL);
		Class.lpszClassName	= ClassName;
		RegisterClass(&Class);

		if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, RegRotation, 0, KEY_READ, &Key ) == ERROR_SUCCESS)
		{
			DWORD Size = sizeof(Angle);
			RegQueryValueEx( Key, T("Angle"), NULL, NULL, (LPBYTE)&Angle, &Size);
			RegCloseKey(Key);
		}

		GetSystemPath(FileName,TSIZEOF(FileName),T("BtCeAvIf.dll"));
		WMP10Support = FileDate(FileName) > 20050101000000000;

		WMP10 = WMP10Support && RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegWMP10, 0, KEY_READ|KEY_WRITE, &Key) == ERROR_SUCCESS;
		if (!WMP10 && RegCreateKeyEx( HKEY_LOCAL_MACHINE, RegWMP9, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp ) != ERROR_SUCCESS)
			Key = NULL;

		if (Key)
		{
			DC = GetDC(0);
			Width = GetDeviceCaps(DC,HORZRES);
			Height = GetDeviceCaps(DC,VERTRES);
			ReleaseDC(0,DC);

			if (WMP10 && Width>0 && Width==Height)
			{
				ProcessSkin(Key,Skin3,NULL);
				ProcessSkin(Key,Skin1,NULL);
				ProcessSkin(Key,Skin2,NULL);
			}
			else
			if (Angle==0 || Angle==180)
			{
				ProcessSkin(Key,Skin1,NULL);
				ProcessSkin(Key,Skin2,NULL);
				ProcessSkin(Key,Skin3,NULL);
				if (!WMP10)
					ProcessSkin(Key,Skin4a,Skin4b);
			}
			else
			if (Angle==90 || Angle==270)
			{
				ProcessSkin(Key,Skin2,NULL);
				ProcessSkin(Key,Skin1,NULL);
				ProcessSkin(Key,Skin3,NULL);
				if (!WMP10)
					ProcessSkin(Key,Skin4a,Skin4b);
			}
			else
			{
				if (!WMP10)
					ProcessSkin(Key,Skin4a,Skin4b);
				ProcessSkin(Key,Skin1,NULL);
				ProcessSkin(Key,Skin2,NULL);
				ProcessSkin(Key,Skin3,NULL);
			}

			RegCloseKey(Key);
		}

		Override();
	}
}

void WidcommAudio_Wnd(void* p)
{
	Notify = p;
	if (Wnd)
		Override();
}

void WidcommAudio_Done()
{
	if (Wnd)
		DestroyWindow(Wnd);
}

bool_t WidcommAudio_SkipActivate()
{
	DWORD Time = GetTickCount();
	return Time > Activate && (int64_t)Time - (int64_t)Activate < 100;
}

#else

void WidcommAudio_Init() {}
void WidcommAudio_Done() {}
void WidcommAudio_Wnd(void* p) {}
bool_t WidcommAudio_SkipActivate() { return 0; }

#endif
