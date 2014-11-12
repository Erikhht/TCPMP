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
 * $Id: association_win32.c 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#define	REG_ASSOCIATIONS	0x2C00

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

static void SetURLProtocol(const tchar_t* Base)
{
	const tchar_t* Name = T("URL Protocol");
	tchar_t Value[MAXPATH];
	DWORD RegType;
	DWORD RegSize;
	DWORD Disp=0;
	HKEY Key;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
	{
		RegSize=sizeof(Value);
		if (RegQueryValueEx(Key, NULL, 0, &RegType, (LPBYTE)Value, &RegSize) != ERROR_SUCCESS || 
			tcsnicmp(Value,T("URL:"),4)!=0)
		{
			stprintf_s(Value,TSIZEOF(Value),T("URL:%s protocol"),Base);
			RegSetValueEx(Key, NULL, 0, REG_SZ, (LPBYTE)Value, (DWORD)((tcslen(Value)+1)*sizeof(tchar_t)));
		}

		RegSize=sizeof(Value);
		if (RegQueryValueEx(Key, Name, 0, &RegType, (LPBYTE)Value, &RegSize) != ERROR_SUCCESS)
			RegSetValueEx(Key, Name, 0, REG_SZ, (LPBYTE)T(""), sizeof(tchar_t));

		RegCloseKey(Key);
	}
}

static void SetEditFlags(const tchar_t* Base,DWORD Value)
{
	DWORD Disp=0;
	HKEY Key;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
	{
		RegSetValueEx(Key, T("EditFlags"), 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
		RegCloseKey(Key);
	}
	//BrowserFlags=8 (?)
}

static void SetReg(const tchar_t* Base,const tchar_t* New,bool_t State)
{
	tchar_t Old[MAXPATH];
	tchar_t Backup[MAXPATH];
	DWORD Disp=0;
	HKEY Key;
	DWORD RegSize;
	DWORD RegType;

	stprintf_s(Backup,TSIZEOF(Backup),T("%s.bak"),Context()->ProgramName);

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, Base, 0, KEY_READ|KEY_WRITE, &Key) == ERROR_SUCCESS)
	{
		RegSize=sizeof(Old);
		if (RegQueryValueEx(Key, NULL, 0, &RegType, (LPBYTE)Old, &RegSize) != ERROR_SUCCESS)
			Old[0] = 0;

		if (tcsicmp(Old,New)!=0)
		{
			if (State)
			{
				RegSetValueEx(Key, NULL, 0, REG_SZ, (LPBYTE)New, (DWORD)((tcslen(New)+1)*sizeof(tchar_t)));
				RegSetValueEx(Key, Backup, 0, REG_SZ, (LPBYTE)Old, RegSize);
			}
		}
		else 
		if (!State)
		{
			RegSize = sizeof(Old);
			if (RegQueryValueEx(Key, Backup, 0, &RegType, (LPBYTE)Old, &RegSize) == ERROR_SUCCESS)
			{
				RegSetValueEx(Key, NULL, 0, REG_SZ, (LPBYTE)Old, RegSize);
				RegDeleteValue(Key, Backup);
			}
		}

		RegCloseKey(Key);
	}
	else 
	if (State)
	{
		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
		{
			RegSetValueEx(Key, NULL, 0, REG_SZ, (LPBYTE)New, (DWORD)((tcslen(New)+1)*sizeof(New)));
			RegSetValueEx(Key, Backup, 0, REG_SZ, (LPBYTE)T(""), sizeof(tchar_t));
			RegCloseKey(Key);
		}
	}
}

static bool_t CmpReg(const tchar_t* Base, const tchar_t* Value)
{
	bool_t Result = 0;
	HKEY Key;
	DWORD RegSize;
	DWORD RegType;
	tchar_t s[MAXPATH];

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, Base, 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		RegSize = sizeof(s);
		if (RegQueryValueEx(Key, NULL, 0, &RegType, (LPBYTE)s, &RegSize) == ERROR_SUCCESS && RegType == REG_SZ)
			Result = tcsicmp(s,Value)==0;

		RegCloseKey(Key);
	}
	return Result;
}

static void SetFileAssociation(const tchar_t* Ext,bool_t State,bool_t MIME,bool_t PlayList)
{
	tchar_t Base[64];
	tchar_t Type[64];
	tchar_t Path[MAXPATH];
	tchar_t Open[MAXPATH];
	tchar_t Icon[MAXPATH];

	GetModuleFileName(NULL,Path,MAXPATH);
	if (tcschr(Path,' '))
		stprintf_s(Open,TSIZEOF(Open),T("\"%s\" \"%%1\""),Path);
	else
		stprintf_s(Open,TSIZEOF(Open),T("%s \"%%1\""),Path);
	stprintf_s(Icon,TSIZEOF(Icon),T("%s, -%d"),Path,1000);

	stprintf_s(Base,TSIZEOF(Base),T(".%s"),Ext);
	stprintf_s(Type,TSIZEOF(Type),MIME?T("%s"):T("%sFile"),Ext);
	tcsupr(Type);

	if (!MIME)
	{
		SetReg(Base,Type,State);
		if (State)
			SetEditFlags(Type,PlayList?0x10010000:0x10000); 
	}
	else
	if (State)
		SetURLProtocol(Type);

	stprintf_s(Base,TSIZEOF(Base),T("%s\\DefaultIcon"),Type);
	SetReg(Base,Icon,State);

	stprintf_s(Base,TSIZEOF(Base),T("%s\\Shell\\Open\\Command"),Type);
	SetReg(Base,Open,State);
}

#if defined(NDEBUG) && !defined(TARGET_WIN32)
static bool_t GetReg(const tchar_t* Base, tchar_t* Value,DWORD ValueSize)
{
	bool_t Result = 0;
	HKEY Key;
	DWORD RegType;

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, Base, 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(Key, NULL, 0, &RegType, (LPBYTE)Value, &ValueSize) == ERROR_SUCCESS && RegType == REG_SZ && Value[0])
			Result = 1;

		RegCloseKey(Key);
	}
	return Result;
}

static bool_t EmptyFileAssociation(const tchar_t* Ext,bool_t MIME)
{
	tchar_t Base[MAXPATH];
	tchar_t Type[MAXPATH];

	stprintf_s(Base,TSIZEOF(Base),T(".%s"),Ext);
	if (MIME || GetReg(Base,Type,sizeof(Type)))
	{
		if (MIME)
			tcscpy_s(Type,TSIZEOF(Type),Ext);
		stprintf_s(Base,TSIZEOF(Base),T("%s\\Shell\\Open\\Command"),Type);
		if (GetReg(Base,Type,sizeof(Type)))
			return 0;
	}
	return 1;
}
#endif

static bool_t GetFileAssociation(const tchar_t* Ext,bool_t MIME)
{
	tchar_t Base[64];
	tchar_t Type[64];

	stprintf_s(Base,TSIZEOF(Base),T(".%s"),Ext);
	stprintf_s(Type,TSIZEOF(Type),MIME?T("%s"):T("%sFile"),Ext);
	tcsupr(Type);

	if (MIME || CmpReg(Base,Type))
	{
		tchar_t Path[MAXPATH];
		tchar_t Open[MAXPATH];

		GetModuleFileName(NULL,Path,MAXPATH);
		if (tcschr(Path,' '))
			stprintf_s(Open,TSIZEOF(Open),T("\"%s\" \"%%1\""),Path);
		else
			stprintf_s(Open,TSIZEOF(Open),T("%s \"%%1\""),Path);

		stprintf_s(Base,TSIZEOF(Base),T("%s\\Shell\\Open\\Command"),Type);
		if (CmpReg(Base,Open))
			return 1;
	}

	return 0;
}

static int Enum( node* p, int* No, datadef* Param )
{
	array List;

	NodeEnumClass(&List,MEDIA_CLASS);
	if (*No>=0 && *No<ARRAYCOUNT(List,int))
	{
		memset(Param,0,sizeof(datadef));
		Param->No = ARRAYBEGIN(List,int)[*No];
		Param->Name = LangStr(Param->No,NODE_NAME);
		Param->Type = TYPE_BOOL;
		Param->Size = sizeof(bool_t);
		Param->Flags = DF_SETUP|DF_NOSAVE|DF_CHECKLIST;
		Param->Class = ASSOCIATION_ID;
		if (!UniqueExts(ARRAYBEGIN(List,int),ARRAYBEGIN(List,int)+*No))
			Param->Flags = DF_HIDDEN;
		ArrayClear(&List);
		return ERR_NONE;
	}
	*No -= ARRAYCOUNT(List,int);
	ArrayClear(&List);

	NodeEnumClass(&List,STREAM_CLASS);
	if (*No>=0 && *No<ARRAYCOUNT(List,int))
	{
		tchar_t Mime[MAXPATH];
		bool_t HasHost;

		memset(Param,0,sizeof(datadef));
		Param->No = ARRAYBEGIN(List,int)[*No];
		Param->Name = LangStr(Param->No,NODE_NAME);
		Param->Type = TYPE_BOOL;
		Param->Size = sizeof(bool_t);
		Param->Flags = DF_SETUP|DF_NOSAVE|DF_CHECKLIST;
		Param->Class = ASSOCIATION_ID;

		stprintf_s(Mime,TSIZEOF(Mime),T("%s://"),LangStr(Param->No,NODE_CONTENTTYPE));
		GetMime(Mime,NULL,0,&HasHost);
		if (!HasHost || tcsicmp(Mime,T("://"))==0 || tcsicmp(Mime,T("http://"))==0)
			Param->Flags = DF_HIDDEN;
		ArrayClear(&List);
		return ERR_NONE;
	}
	*No -= ARRAYCOUNT(List,int);
	ArrayClear(&List);

	return ERR_INVALID_PARAM;
}

static int Get(node* p, int No, void* Data, int Size)
{
	if (NodeIsClass(No,STREAM_CLASS))
	{
		assert(Size==sizeof(bool_t));
		*(bool_t*)Data = GetFileAssociation(LangStr(No,NODE_CONTENTTYPE),1);
		return ERR_NONE;
	}
	else
	if (NodeIsClass(No,MEDIA_CLASS))
	{
		const tchar_t* Exts = LangStr(No,NODE_EXTS);
		if (Exts && Exts[0])
		{
			tchar_t s[16];
			const tchar_t *r,*q;

			assert(Size==sizeof(bool_t));
			*(bool_t*)Data = 1;
			for (r=Exts;r;)
			{
				q = tcschr(r,':');
				if (q)
				{
					tcsncpy_s(s,TSIZEOF(s),r,q-r);
					if (!GetFileAssociation(s,0))
					{
						*(bool_t*)Data = 0;
						break;
					}
				}
				r = tcschr(r,';');
				if (r) ++r;
			}
			return ERR_NONE;
		}
	}
	return ERR_INVALID_PARAM;
}

static int Set(node* p, int No, const void* Data, int Size)
{
	if (NodeIsClass(No,STREAM_CLASS))
	{
		assert(Size==sizeof(bool_t));
		SetFileAssociation(LangStr(No,NODE_CONTENTTYPE),*(bool_t*)Data,1,0);
		return ERR_NONE;
	}
	else
	if (NodeIsClass(No,MEDIA_CLASS))
	{
		const tchar_t* Exts = LangStr(No,NODE_EXTS);
		if (Exts && Exts[0])
		{
			tchar_t s[16];
			const tchar_t *r,*q;
			bool_t State;

			assert(Size==sizeof(bool_t));
			State = *(bool_t*)Data;

			for (r=Exts;r;)
			{
				q = tcschr(r,':');
				if (q)
				{
					tcsncpy_s(s,TSIZEOF(s),r,q-r);
					SetFileAssociation(s,State,0,q[1]==FTYPE_PLAYLIST);
				}
				r = tcschr(r,';');
				if (r) ++r;
			}
			return ERR_NONE;
		}
	}
	return ERR_INVALID_PARAM;
}

static void AssignEmpty(node* p)
{
#if defined(NDEBUG) && !defined(TARGET_WIN32)
	int v;
	if (!NodeRegLoadValue(0,REG_ASSOCIATIONS,&v,sizeof(v),TYPE_INT) || !v)
	{
		// set default associations for empty extensions
		array List;
		int* i;

		NodeEnumClass(&List,MEDIA_CLASS);

		for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
		{
			const tchar_t* Exts = LangStr(*i,NODE_EXTS);
			if (Exts && Exts[0])
			{
				bool_t Empty = 1;
				tchar_t s[16];
				const tchar_t *r,*q;

				for (r=Exts;r;)
				{
					q = tcschr(r,':');
					if (q)
					{
						tcsncpy_s(s,TSIZEOF(s),r,q-r);
						if (!EmptyFileAssociation(s,0))
						{
							Empty = 0;
							break;
						}
					}
					r = tcschr(r,';');
					if (r) ++r;
				}

				if (Empty)
				{
					bool_t True = 1;
					Set(p,*i,&True,sizeof(True));
				}
			}
		}

		ArrayClear(&List);

		v = 1;
		NodeRegSaveValue(0,REG_ASSOCIATIONS,&v,sizeof(v),TYPE_INT);
	}
#endif
}

static int Create(node* p)
{
	p->Enum = (nodeenum)Enum;
	p->Get = (nodeget)Get;
	p->Set = (nodeset)Set;
	AssignEmpty(p);
	return ERR_NONE;
}

static const nodedef Association =
{
	sizeof(node)|CF_GLOBAL|CF_SETTINGS,
	ASSOCIATION_ID,
	NODE_CLASS,
	PRI_MAXIMUM+500,
	(nodecreate)Create,
};

void Association_Init()
{
	NodeRegisterClass(&Association);
}

void Association_Done()
{
	NodeUnRegisterClass(ASSOCIATION_ID);
}

#endif
