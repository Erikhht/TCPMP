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
 * $Id: node_win32.c 622 2006-01-31 19:02:53Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(TARGET_WINCE)
#define TWIN(a) L ## a
#define HKEY_ROOT HKEY_LOCAL_MACHINE
#else
#define TWIN(a) a
#define HKEY_ROOT HKEY_CURRENT_USER
#endif

void NOINLINE NodeBase(int Class, tchar_t* Base, int BaseLen)
{
	tcscpy_s(Base,BaseLen,T("SOFTWARE\\"));
	tcscat_s(Base,BaseLen,Context()->ProgramName);
	if (Class)
	{
		tchar_t s[16];
		FourCCToString(s,TSIZEOF(s),Class);
		tcscat_s(Base,BaseLen,T("\\"));
		tcscat_s(Base,BaseLen,s);
	}
}

static bool_t LoadValue(HKEY Key,int Id,void* Data,int Size,int Type)
{
	uint8_t Buffer[MAXDATA];
	tchar_t Name[16];
	DWORD RegType;
	DWORD RegSize = sizeof(Buffer);

	IntToString(Name,TSIZEOF(Name),Id,0);
	if (RegQueryValueEx(Key, Name, 0, &RegType, Buffer, &RegSize) == ERROR_SUCCESS)
	{
		switch (Type)
		{
		case TYPE_BOOL:
			if (RegType == REG_DWORD)
			{
				*(bool_t*)Data = *(DWORD*)Buffer;
				return 1;
			}
			break;

		case TYPE_RGB:
			if (RegType == REG_DWORD)
			{
				*(rgbval_t*)Data = *(DWORD*)Buffer;
				return 1;
			}
			break;

		case TYPE_TICK:
			if (RegType == REG_DWORD)
			{
				*(tick_t*)Data = *(DWORD*)Buffer;
				return 1;
			}
			break;

		case TYPE_INT:
		case TYPE_HOTKEY:
			if (RegType == REG_DWORD)
			{
				*(int*)Data = *(DWORD*)Buffer;
				return 1;
			}
			break;

		case TYPE_POINT:
		case TYPE_RECT:
		case TYPE_BINARY:
		case TYPE_FRACTION:
			if (RegType == REG_BINARY && Size<=MAXDATA)
			{
				memcpy(Data,Buffer,Size);
				return 1;
			}
			break;

		case TYPE_STRING:
			if (RegType == REG_SZ && Size<=MAXDATA)
			{
				memcpy(Data,Buffer,Size);
				return 1;
			}
			break;
		}
	}
	return 0;
}

static void SaveValue(HKEY Key,int Id,const void* Data,int Size,int Type)
{
	tchar_t Name[16];
	IntToString(Name,TSIZEOF(Name),Id,0);

	if (!Data)
		RegDeleteValue(Key,Name);
	else
	{
		DWORD RegType,RegSize,v;
		RegType = REG_DWORD;
		RegSize = sizeof(DWORD);

		switch (Type)
		{
		case TYPE_BOOL:
			v = *(bool_t*)Data;
			break;

		case TYPE_RGB:
			v = *(int32_t*)Data;
			break;

		case TYPE_TICK:
			v = *(tick_t*)Data;
			break;

		case TYPE_INT:
		case TYPE_HOTKEY:
			v = *(int*)Data;
			break;

		case TYPE_POINT:
		case TYPE_RECT:
		case TYPE_BINARY:
		case TYPE_FRACTION:
			RegType = REG_BINARY;
			RegSize = Size;
			break;

		case TYPE_STRING:
			RegType = REG_SZ;
			RegSize = (DWORD)((tcslen((const tchar_t*)Data)+1)*sizeof(tchar_t));
			break;

		default:
			return;
		}

		if (RegType == REG_DWORD)
			Data = &v;

		RegSetValueEx(Key, Name, 0, RegType, Data, RegSize);
	}
}

bool_t NodeRegLoadValue(int Class, int Id, void* Data, int Size, int Type)
{
	bool_t Result = 0;
	HKEY Key;
	tchar_t Base[256];

	NodeBase(Class,Base,TSIZEOF(Base));
	if (RegOpenKeyEx(HKEY_ROOT, Base, 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		Result = LoadValue(Key,Id,Data,Size,Type);
		RegCloseKey(Key);
	}
	return Result;
}

void NodeRegSaveValue(int Class, int Id, const void* Data, int Size, int Type)
{
	DWORD Disp;
	HKEY Key;
	tchar_t Base[256];
	NodeBase(Class,Base,TSIZEOF(Base));

	if (RegCreateKeyEx(HKEY_ROOT, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
	{
		SaveValue(Key,Id,Data,Size,Type);
		RegCloseKey(Key);
	}
}

void NodeRegLoad(node* p)
{
	if (p)
	{
		uint8_t Buffer[MAXDATA];
		tchar_t Base[256];
		datadef Type;
		HKEY Key;
		int No;

		NodeBase(p->Class,Base,TSIZEOF(Base));
		if (RegOpenKeyEx(HKEY_ROOT, Base, 0, KEY_READ, &Key) == ERROR_SUCCESS)
		{
			for (No=0;NodeEnum(p,No,&Type)==ERR_NONE;++No)
				if ((Type.Flags & DF_SETUP) && !(Type.Flags & (DF_NOSAVE|DF_RDONLY)) &&
					LoadValue(Key,Type.No,Buffer,Type.Size,Type.Type))
					p->Set(p,Type.No,Buffer,Type.Size);

			RegCloseKey(Key);
		}
	}
}

void NodeRegSave(node* p)
{
	if (p)
	{
		uint8_t Buffer[MAXDATA];
		datadef Type;
		DWORD Disp;
		HKEY Key;
		tchar_t Base[256];

		NodeBase(p->Class,Base,TSIZEOF(Base));
		if (RegCreateKeyEx(HKEY_ROOT, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
		{
			int No;
			int* i;
			DWORD RegSize;
			DWORD RegType;
			tchar_t Name[64];
			array List = {NULL};

			RegSize = TSIZEOF(Name);
			for (No=0;RegEnumValue(Key,No,Name,&RegSize,NULL,&RegType,NULL,NULL)==ERROR_SUCCESS;++No)
			{
				int Id = StringToInt(Name,0);
				if (Id >= NODE_MAX_REGISTRY)
					ArrayAppend(&List,&Id,sizeof(Id),256);
				RegSize = TSIZEOF(Name);
			}
			
			for (No=0;NodeEnum(p,No,&Type)==ERR_NONE;++No)
				if ((Type.Flags & DF_SETUP) && !(Type.Flags & (DF_NOSAVE|DF_RDONLY)))
				{
					int Result = p->Get(p,Type.No,Buffer,Type.Size);
					if (Result == ERR_NONE || Result == ERR_NOT_SUPPORTED) // not supported settings should be still saved
					{
						for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
							if (*i == Type.No)
								*i = 0;
						SaveValue(Key,Type.No,Buffer,Type.Size,Type.Type);
					}
				}

			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
				if (*i)
					SaveValue(Key,*i,NULL,0,TYPE_INT);

			ArrayClear(&List);
			RegCloseKey(Key);
		}
	}
}

void NodeRegLoadGlobal() {}
void NodeRegSaveGlobal() {}

void GetModulePath(tchar_t* Path,const tchar_t* Module);
void FindFiles(const tchar_t* Path, const tchar_t* Mask,void(*Process)(const tchar_t*,void*),void* Param);

static void PluginError(const tchar_t* FileName)
{
	tchar_t Name[MAXPATH];
	tchar_t Ext[MAXPATH];
	SplitURL(FileName,NULL,0,NULL,0,Name,TSIZEOF(Name),Ext,TSIZEOF(Ext));
	tcsupr(Name);
	ShowError(0,ERR_ID,ERR_NOT_COMPATIBLE,Name);
}

void* NodeLoadModule(const tchar_t* Path,int* Id,void** AnyFunc,void** Db)
{
	HMODULE Module;
	int Error = 0;

#if !defined(TARGET_WINCE)
	int OldMode = SetErrorMode(SEM_NOOPENFILEERRORBOX| SEM_FAILCRITICALERRORS);
#endif

	Module = LoadLibrary(Path);
	if (!Module)
	{
		Error = GetLastError();
		if (Error == ERROR_NOT_ENOUGH_MEMORY || Error == ERROR_OUTOFMEMORY)
		{
			NodeHibernate();
			Module = LoadLibrary(Path);
			if (!Module)
				Error = GetLastError();
		}
	}

	Context()->LoadModule = Module;

#if !defined(TARGET_WINCE)
	SetErrorMode(OldMode);
#endif

	if (Module)
	{
		FARPROC Func = GetProcAddress(Module,TWIN("DLLRegister"));
		if (!Func)
			Func = GetProcAddress(Module,TWIN("_DLLRegister@4"));

		if (Func)
		{
			int Result;
			if (AnyFunc)
				*(FARPROC*)AnyFunc = Func; // set before calling DLLRegister

			Result = ((int(*)(int))Func)(CONTEXT_VERSION);
			if (Result != ERR_NONE)
			{
				Func = NULL;
				if (AnyFunc)
					*(FARPROC*)AnyFunc = NULL;
				if (Result == ERR_NOT_COMPATIBLE)
					PluginError(Path);
			}
		}

		if (!Func)
		{
			FreeLibrary(Module);
			Module = NULL;
		}
	}
	else
	if (Error == ERROR_NOT_ENOUGH_MEMORY || Error == ERROR_OUTOFMEMORY)
		ShowOutOfMemory();
	else
		PluginError(Path);

	return Module;
}

void NodeFreeModule(void* Module,void* Db)
{
	if (Module)
	{
		FARPROC UnRegister = GetProcAddress(Module,TWIN("DLLUnRegister"));
		if (!UnRegister)
			UnRegister = GetProcAddress(Module,TWIN("_DLLUnRegister@4"));

		if (UnRegister)
			((void(*)())UnRegister)();

		FreeLibrary(Module);
	}
}

static void AddModule(const tchar_t* Path,HKEY* Key)
{
	int64_t Date = FileDate(Path);
	if (Date != -1)
	{
		int64_t Value;
		DWORD RegType;
		DWORD RegSize = sizeof(Value);
		bool_t Changed = *Key==NULL || RegQueryValueEx(*Key, Path, 0, &RegType, (PBYTE)&Value, &RegSize) != ERROR_SUCCESS || Value != Date;

		NodeAddModule(Path,0,Date,Changed,0);

		if (Changed && *Key) // save file stamp to registry
			RegSetValueEx(*Key, Path, 0, REG_BINARY, (PBYTE)&Date, sizeof(Date));
	}
}

static void FindPlugins(array* List)
{
	tchar_t* s;
	tchar_t Name[64];
	tchar_t Path[MAXPATH];
	tchar_t Path2[MAXPATH];
	DWORD RegSize,RegType,NameSize,Disp;
	HKEY Key,KeyStamp;
	int No,Class;

	NodeBase(0,Path,TSIZEOF(Path));
	tcscat_s(Path,TSIZEOF(Path),T("\\DLLStamp"));
	if (RegCreateKeyEx(HKEY_ROOT, Path, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &KeyStamp, &Disp) != ERROR_SUCCESS)
		KeyStamp = NULL;

	// add plugins in exe directory
	GetModulePath(Path,NULL);
	FindFiles(Path,T("*.plg"),AddModule,&KeyStamp);

	// maybe exe is in different directory as common.dll and other plugins (mostly with debugging)
	GetModulePath(Path2,T("common.dll"));
	if (tcsicmp(Path2,Path)!=0)
		FindFiles(Path2,T("*.plg"),AddModule,&KeyStamp);

	// additional plugins
	NodeBase(0,Path,TSIZEOF(Path));
	tcscat_s(Path,TSIZEOF(Path),T(".Plugins"));
	if (RegOpenKeyEx(HKEY_ROOT, Path, 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		for (No=0;;++No)
		{
			NameSize = TSIZEOF(Name);
			RegSize = sizeof(Path);
			if (RegEnumValue(Key,No,Name,&NameSize,NULL,&RegType,(LPBYTE)Path,&RegSize)!=ERROR_SUCCESS)
				break;
			if (RegType == REG_SZ)
				AddModule(Path,&KeyStamp);
		}
		RegCloseKey(Key);
	}

	if (KeyStamp)
	{
		// delete unused stamps
		ArrayClear(List);
		RegSize = TSIZEOF(Path);
		for (No=0;RegEnumValue(KeyStamp,No,Path,&RegSize,NULL,NULL,NULL,NULL)==ERROR_SUCCESS;++No)
		{
			if (!NodeFindModule(Path,0))
				ArrayAppend(List,Path,(tcslen(Path)+1)*sizeof(tchar_t),256);
			RegSize = TSIZEOF(Path);
		}

		for (s=ARRAYBEGIN(*List,tchar_t);s!=ARRAYEND(*List,tchar_t);s+=tcslen(s)+1)
			RegDeleteValue(KeyStamp,s);

		RegCloseKey(KeyStamp);
	}

	ArrayClear(List);
	NodeBase(0,Path,TSIZEOF(Path));
	if (RegOpenKeyEx(HKEY_ROOT, Path, 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		RegSize = TSIZEOF(Name);
		for (No=0;RegEnumKeyEx(Key,No,Name,&RegSize,NULL,NULL,NULL,NULL)==ERROR_SUCCESS;++No)
		{
			if (tcslen(Name)==4)
			{
				Class = StringToFourCC(Name,0);
				ArrayAppend(List,&Class,sizeof(Class),128);
			}
			RegSize = TSIZEOF(Name);
		}
		RegCloseKey(Key);
	}
}

static void LoadValueString(HKEY Key,int Class,int Id)
{
	uint8_t Buffer[MAXDATA];
	if (LoadValue(Key,Id,Buffer,sizeof(Buffer),TYPE_STRING))
		StringAdd(1,Class,Id,(tchar_t*)Buffer);
}

void Plugins_Init()
{
	tchar_t Path[MAXPATH];
	array List = {NULL};
	nodedef Def;
	int* i;

	FindPlugins(&List);

	for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
	{
		int Class = *i;
		HKEY Key;
		tchar_t Base[256];

		NodeBase(Class,Base,TSIZEOF(Base));
		if (RegOpenKeyEx(HKEY_ROOT, Base, 0, KEY_READ, &Key) == ERROR_SUCCESS)
		{
			if (LoadValue(Key,NODE_MODULE_PATH,Path,sizeof(Path),TYPE_STRING) && NodeFindModule(Path,0))
			{
				memset(&Def,0,sizeof(Def));
				Def.Class = Class;
				Def.Flags = 0;
				Def.ParentClass = 0;
				Def.Priority = PRI_DEFAULT;
				LoadValue(Key,NODE_PARENT,&Def.ParentClass,sizeof(int),TYPE_INT);
				LoadValue(Key,NODE_PRIORITY,&Def.Priority,sizeof(int),TYPE_INT);
				LoadValue(Key,NODE_FLAGS,&Def.Flags,sizeof(int),TYPE_INT);
				LoadValueString(Key,Class,NODE_NAME);
				LoadValueString(Key,Class,NODE_CONTENTTYPE);
				LoadValueString(Key,Class,NODE_EXTS);
				LoadValueString(Key,Class,NODE_PROBE);
				NodeLoadClass(&Def,Path,0);

			}
			RegCloseKey(Key);
		}
	}

	ArrayClear(&List);
}

void Plugins_Done()
{
}

#endif
