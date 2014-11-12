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
 * $Id: setup.c 332 2005-11-06 14:31:57Z picard $
 *
 ****************************************************************************/

#include "../common/common.h"
#include <windows.h>
#include "benchmark.h"

#ifdef UNICODE
#define tcsicmp _wcsicmp
#define tcsupr _wcsupr
#else
#define tcsicmp stricmp;
#define tcsupr _strupr
#endif

DLLEXPORT int WINAPI Install_Init(HWND Parent, BOOL FirstCall, BOOL PrevInstalled, LPCTSTR InstallDir)
{
    return 0;
}

// don't want to use common.dll, but don't want to collide with DLL import function either
#define tcscpy_s _tcscpy_s
#define tcscat_s _tcscat_s
#define IntToString _IntToString

static void IntToString(tchar_t *Out,int OutLen,int p,bool_t Hex)
{
	int n=1000000000;

	if (p<0)
	{
		p=-p;
		if (OutLen>1)
		{
			*(Out++) = '-';
			--OutLen;
		}
	}

	while (p<n && n>1)
		n/=10;

	while (n>0)
	{
		int i = p/n;
		p-=n*i;
		if (OutLen>1)
		{
			*(Out++) = (tchar_t)('0'+i);
			--OutLen;
		}
		n/=10;
	}

	if (OutLen>0)
		*Out = 0;
}

static void tcscpy_s(tchar_t* Out,int OutLen,const tchar_t* In)
{
	int n = min((int)tcslen(In),OutLen-1);
	memcpy(Out,In,n*sizeof(tchar_t));
	Out[n] = 0;
}

static void tcscat_s(tchar_t* Out,int OutLen,const tchar_t* In)
{
	int n = tcslen(Out);
	tcscpy_s(Out+n,OutLen-n,In);
}

static void Remove(const tchar_t* InstallDir,const tchar_t* Name)
{
	tchar_t Path[MAXPATH];
	tcscpy_s(Path,MAXPATH,InstallDir);
	tcscat_s(Path,MAXPATH,T("\\"));
	tcscat_s(Path,MAXPATH,Name);
	DeleteFile(Path);
}

DLLEXPORT int WINAPI Install_Exit(HWND Parent, LPCTSTR InstallDir, WORD FailedDirs,WORD FailedFiles,WORD FailedRegKeys,WORD FailedRegVals,WORD FailedShortcuts)
{
	HMODULE GAPI;
	HMODULE CoreDLL;
	tchar_t LocalPath[MAXPATH];
	tchar_t WinPath[MAXPATH];
	int32_t Slow = -1;

	DWORD Disp;
	HKEY Key;

	tchar_t* AdvBase = T("SOFTWARE\\TCPMP\\ADVP");

#ifdef ARM
	tchar_t* Base = T("System\\GDI\\Drivers\\ATI");
	tchar_t* Name = T("DisableDeviceBitmap");
	HANDLE Handle;
#endif

	//-------------------------------------
	// Remove possible old files

	Remove(InstallDir,T("lang_en.plg"));
	Remove(InstallDir,T("asf.plg"));
	Remove(InstallDir,T("avi.plg"));
	Remove(InstallDir,T("a52.plg"));
	Remove(InstallDir,T("mjpeg.plg"));
	Remove(InstallDir,T("mpeg4aac.plg"));
	Remove(InstallDir,T("mpegaudio.plg"));
	Remove(InstallDir,T("mpegvideo.plg"));
	Remove(InstallDir,T("overlay.plg"));
	Remove(InstallDir,T("vorbis.plg"));

	//-------------------------------------
	// GAPI's gx.dll keep it or not?

	WinPath[0] = 0;
	CoreDLL = LoadLibrary(T("coredll.dll"));
	if (CoreDLL)
	{
		BOOL (WINAPI* FuncSHGetSpecialFolderPath)(HWND,LPTSTR,int,BOOL);

		*(FARPROC*)&FuncSHGetSpecialFolderPath = GetProcAddress(CoreDLL,T("SHGetSpecialFolderPath"));

		if (FuncSHGetSpecialFolderPath)
			FuncSHGetSpecialFolderPath(NULL,WinPath,0x24/*CSIDL_WINDOWS*/,FALSE);

		FreeLibrary(CoreDLL);
	}
	if (!WinPath[0])
		tcscpy_s(WinPath,MAXPATH,T("\\Windows"));

	tcscat_s(WinPath,MAXPATH,T("\\gx.dll"));
	tcscpy_s(LocalPath,MAXPATH,InstallDir);
	tcscat_s(LocalPath,MAXPATH,T("\\gx.dll"));

	GAPI = LoadLibrary(WinPath);
	if (GAPI)
	{
		DeleteFile(LocalPath);
	}
	else
	{
		GAPI = LoadLibrary(LocalPath); // can we load our gx.dll? aygshell.dll available?
		if (GAPI)
		{
			// check new HPC device with aygshell support, but no GXINFO

			OSVERSIONINFO Ver;
			Ver.dwOSVersionInfoSize = sizeof(Ver);
			GetVersionEx(&Ver);

			if (Ver.dwMajorVersion >= 4)
			{
				HDC DC = GetDC(NULL);
				DWORD Code = GETGXINFO;
				GXDeviceInfo Info;
				memset(&Info,0,sizeof(Info));
				Info.Version = 100;

				if (ExtEscape(DC, ESC_QUERYESCSUPPORT, sizeof(DWORD), (char*)&Code, 0, NULL) > 0)
					ExtEscape(DC, GETGXINFO, 0, NULL, sizeof(Info), (char*)&Info);

				if (Info.cxWidth==0 || Info.cyHeight==0)
				{
					// release and remove our gx.dll
					FreeLibrary(GAPI);
					GAPI = NULL;
				}

				ReleaseDC(NULL,DC);
			}
		}

		if (!GAPI)
		{
			DeleteFile(LocalPath);
			GAPI = LoadLibrary(T("gx.dll")); // try load on path
		}
	}

	//-------------------------------------
	// Benchmark video memory

	SlowVideoRAW(&Slow);

	if (GAPI)
	{
		if (Slow == -1)
			SlowVideoGAPI(GAPI,Parent,&Slow);
		FreeLibrary(GAPI);
	}

	// EPSON display drive in NEXIO
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("Drivers\\Display\\S1D13806"), 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		Slow = 1;
		RegCloseKey(Key);
	}

	if (Slow != -1)
	{
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, AdvBase, 0, NULL, 0, 0, NULL, &Key, &Disp) == ERROR_SUCCESS)
		{
			tchar_t Name[32];
			IntToString(Name,TSIZEOF(Name),ADVANCED_SLOW_VIDEO,0);
			RegSetValueEx(Key, Name, 0, REG_DWORD, (LPBYTE)&Slow, sizeof(Slow));
			RegCloseKey(Key);
		}
	}

#ifdef ARM

	//-------------------------------------
	// ATI Imageon 3200 registry settings

	Handle = LoadLibrary(T("ACE_DDI.DLL"));
	if (Handle)
	{
		FreeLibrary(Handle);

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
		{
			DWORD Value;
			DWORD RegType;
			DWORD RegSize;

			RegSize=sizeof(Value);
			if (RegQueryValueEx(Key, Name, 0, &RegType, (LPBYTE)&Value, &RegSize) != ERROR_SUCCESS || !Value)
			{
				Value = 1;
				RegSetValueEx(Key, Name, 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));

				MessageBox(Parent,
					T("TCPMP installer disabled ATI IMAGEON device bitmap caching for better video playback. ") 
					T("After install please warm boot the device for changes to take effect! ")
					T("You can change this setting later in the player."),T("Setup"),MB_OK|MB_SETFOREGROUND);
			}
			RegCloseKey(Key);
		}
	}
	else
	{
		tcscpy_s(LocalPath,MAXPATH,InstallDir);
		tcscat_s(LocalPath,MAXPATH,T("\\ati3200.plg"));
		DeleteFile(LocalPath);
	}

	//-------------------------------------
	// Intel 2700G 

	Handle = LoadLibrary(T("PVRVADD.DLL"));
	if (Handle)
	{
		FreeLibrary(Handle);
	}
	else
	{
		tcscpy_s(LocalPath,MAXPATH,InstallDir);
		tcscat_s(LocalPath,MAXPATH,T("\\intel2700g.plg"));
		DeleteFile(LocalPath);
	}

#endif

	return 0;
}

DLLEXPORT int WINAPI Uninstall_Init(HWND Parent, LPCTSTR InstallDir)
{
    return 0;
}

static void RegistryRestore(const tchar_t* Base, const tchar_t* SubString)
{
	tchar_t* BackupName = T("TCPMP.bak");
	tchar_t Backup[MAXPATH];
	HKEY Key;
	DWORD RegSize;
	DWORD RegType;

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, Base, 0, KEY_READ|KEY_WRITE, &Key) == ERROR_SUCCESS)
	{
		RegSize=sizeof(Backup);
		if (RegQueryValueEx(Key, NULL, 0, &RegType, (LPBYTE)Backup, &RegSize)==ERROR_SUCCESS)
		{
			tcsupr(Backup);
			if (tcsstr(Backup,SubString))
			{
				RegSize=sizeof(Backup);
				if (RegQueryValueEx(Key, BackupName, 0, &RegType, (LPBYTE)Backup, &RegSize)==ERROR_SUCCESS)
				{
					RegSetValueEx(Key, NULL, 0, REG_SZ, (LPBYTE)Backup, RegSize);
					RegDeleteValue(Key, BackupName);
				}
			}
		}
		RegCloseKey(Key);
	}
}

DLLEXPORT int WINAPI Uninstall_Exit(HWND hwndParent)
{
	tchar_t Base[MAXPATH];
	tchar_t Base2[MAXPATH];
	HKEY Key;
	DWORD RegSize;
	int n;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("SOFTWARE"), 0, KEY_READ|KEY_WRITE, &Key) == ERROR_SUCCESS)
	{
		RegDeleteKey(Key,T("TCPMP"));
		RegCloseKey(Key);
	}

	RegSize = sizeof(Base);
	for (n=0;RegEnumKeyEx(HKEY_CLASSES_ROOT,n,Base,&RegSize,NULL,NULL,NULL,NULL)==ERROR_SUCCESS;++n)
	{
		if (Base[0] == '.')
		{
			tcscpy_s(Base2,TSIZEOF(Base2),Base+1);
			tcscat_s(Base2,TSIZEOF(Base2),T("%sFile"));
			tcsupr(Base2);
			RegistryRestore(Base,Base2);
		}
		else
		{
			tcscpy_s(Base2,TSIZEOF(Base2),Base);
			tcscat_s(Base2,TSIZEOF(Base2),T("\\DefaultIcon"));
			RegistryRestore(Base2,T("\\PLAYER.EXE"));
			tcscpy_s(Base2,TSIZEOF(Base2),Base);
			tcscat_s(Base2,TSIZEOF(Base2),T("\\shell\\open\\command"));
			RegistryRestore(Base2,T("\\PLAYER.EXE"));
		}
		RegSize = sizeof(Base);
	}

	return 0;
}
