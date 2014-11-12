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
 * $Id: main.c 341 2005-11-16 00:15:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"

static const tchar_t ProgramName[] = T("TCPMP");
static const tchar_t ProgramVersion[] = 
#include "../version"
;

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#if defined(TARGET_WINCE)
#define TWIN(a) L ## a
extern int ProgramId;
#else
#define TWIN(a) a
static int ProgramId = 0;
#endif

// don't want to use common.dll, but don't want to collide with DLL import function either
#define tcscpy_s _tcscpy_s

#if !defined(NO_PLUGINS) || defined(NDEBUG)
static void tcscpy_s(tchar_t* Out,int OutLen,const tchar_t* In)
{
	if (OutLen>0)
	{
		int n = min((int)tcslen(In),OutLen-1);
		memcpy(Out,In,n*sizeof(tchar_t));
		Out[n] = 0;
	}
}
#endif

#ifdef NDEBUG
static BOOL CALLBACK EnumWindowsProc(HWND Wnd,LPARAM Param)
{
	HWND* p = (HWND*)Param;
	if (GetWindow(Wnd,GW_OWNER) == *p)
	{
		*p = Wnd;
		return 0;
	}
	return 1;
}

static bool_t FindRunning(const tchar_t* CmdLine)
{
	HWND Wnd;
	tchar_t ClassName[32];
	int n = tcslen(ProgramName);
	tcscpy_s(ClassName,TSIZEOF(ClassName),ProgramName);
	tcscpy_s(ClassName+n,TSIZEOF(ClassName)-n,T("_Win"));
	Wnd = FindWindow(ClassName, NULL);
	if (Wnd)
	{
		HWND WndMain = Wnd;

		while (!IsWindowEnabled(Wnd))
		{
			HWND Last = Wnd;
			EnumWindows(EnumWindowsProc,(LPARAM)&Wnd);
			if (Wnd == Last)
				break;
		}

		SetForegroundWindow(Wnd);

		if (CmdLine && CmdLine[0])
		{
			COPYDATASTRUCT Data;
			Data.dwData = 0;
			Data.cbData = (tcslen(CmdLine)+1)*sizeof(tchar_t);
			Data.lpData = (PVOID)CmdLine;
			SendMessage(WndMain,WM_COPYDATA,(WPARAM)WndMain,(LPARAM)&Data);
		}

		return 1;
	}
	return 0;
}
#endif

#ifdef NO_PLUGINS
extern void Main(const tchar_t* Name,const tchar_t* Version,int Id,const tchar_t* CmdLine);
#else
static HANDLE Load(const tchar_t* Name)
{
	HANDLE Module;
	tchar_t Path[MAXPATH];
	tchar_t *s;
	GetModuleFileName(NULL,Path,MAXPATH);
	s = tcsrchr(Path,'\\');
	if (s) s[1]=0;
	tcscpy_s(Path+tcslen(Path),TSIZEOF(Path)-tcslen(Path),Name);
	Module = LoadLibrary(Path);
	if (!Module)
		Module = LoadLibrary(Name);
	return Module;
}
#endif

#if !defined(TARGET_WINCE) && defined(UNICODE)
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hParent,LPSTR CmdA,int CmdShow)
{
	WCHAR Cmd[2048];
	mbstowcs(Cmd,CmdA,sizeof(Cmd)/sizeof(WCHAR)); //!!!
#else
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hParent,TCHAR* Cmd,int CmdShow)
{
#endif

#ifndef NDEBUG
//	DLLTest(); // just to help debugging plugins. comment out if not needed
	Context();
#endif

#if defined(TARGET_WINCE) && defined(ARM)
	if (ProgramId == 2)
	{
		OSVERSIONINFO Ver;
		Ver.dwOSVersionInfoSize = sizeof(Ver);
		GetVersionEx(&Ver);
		if (Ver.dwMajorVersion*100 + Ver.dwMinorVersion >= 421)
		{
			// old shell menu not supported after WM2003SE
			MessageBox(NULL,T("This ARM_CE2 version of the player is not compatible with this device. Please install ARM_CE3 version."),NULL,MB_OK|MB_ICONERROR); 
			return 1; 
		}
	}
#endif

#ifdef NDEBUG
	if (!FindRunning(Cmd))
	{
		HANDLE Handle = CreateMutex(NULL,FALSE,ProgramName);
		if (GetLastError() != ERROR_ALREADY_EXISTS)
#endif
		{
#ifndef NO_PLUGINS
			HMODULE Module;
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			Module = Load(T("interface.plg"));
			if (Module)
			{
				void (*Main)(const tchar_t* Name,const tchar_t* Version,int Id,const tchar_t* CmdLine);
				*(FARPROC*)&Main = GetProcAddress(Module,TWIN("Main"));
				if (!Main)
					*(FARPROC*)&Main = GetProcAddress(Module,TWIN("_Main@16"));
				if (Main)
					Main(ProgramName,ProgramVersion,ProgramId,Cmd);
				FreeLibrary(Module);
			}
#else
			Main(ProgramName,ProgramVersion,ProgramId,Cmd);
#endif
#ifdef NDEBUG
			CloseHandle(Handle);
#endif
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
#ifdef NDEBUG
	}
#endif
	return 0;
}

