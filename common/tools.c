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
 * $Id: tools.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#include "../splitter/asf.h"

void BuildChapter(tchar_t* s,int slen,int No,int64_t Time,int Rate)
{
	int Hour,Min,Sec;
	if (Time<0) Time=0;
	Time /= Rate;
	Hour = (int)(Time / 3600000);
	Time -= Hour * 3600000;
	Min = (int)(Time / 60000);
	Time -= Min * 60000;
	Sec = (int)(Time / 1000);
	Time -= Sec * 1000;
	stprintf_s(s,slen,T("CHAPTER%02d=%02d:%02d:%02d.%03d"),No,Hour,Min,Sec,(int)Time);
}

bool_t SetFileExt(tchar_t* URL, int URLLen, const tchar_t* Ext)
{
	tchar_t *p,*q;
	bool_t HasHost;

	p = (tchar_t*) GetMime(URL,NULL,0,&HasHost);
	q = p;
	
	p = tcsrchr(q,'\\');
	if (!p)
		p = tcsrchr(q,'/');
	if (p)
		q = p+1;
	else
	if (HasHost) // only hostname
		return 0;
	
	if (!q[0]) // no filename at all?
		return 0;

	p = tcsrchr(q,'.');
	if (p)
		*p = 0;

	tcscat_s(URL,URLLen,T("."));
	tcscat_s(URL,URLLen,Ext);
	return 1;
}

void SplitURL(const tchar_t* URL, tchar_t* Mime, int MimeLen, tchar_t* Dir, int DirLen, tchar_t* Name, int NameLen, tchar_t* Ext, int ExtLen)
{
	const tchar_t* p;
	const tchar_t* p2;
	bool_t HasHost;
	bool_t MergeMime = Mime && Mime == Dir;

	// mime 
	p = GetMime(URL,MergeMime?NULL:Mime,MimeLen,&HasHost);
	if (!MergeMime)
		URL = p;

	// dir
	p2 = tcsrchr(p,'\\');
	if (!p2)
		p2 = tcsrchr(p,'/');
	if (p2)
	{
		if (Dir)
			tcsncpy_s(Dir,DirLen,URL,p2-URL);
		URL = p2+1;
	}
	else
	if (HasHost) // no filename, only host
	{
		if (Dir)
			tcscpy_s(Dir,DirLen,URL);
		URL += tcslen(URL);
	}
	else // no directory
	{
		if (Dir)
			tcsncpy_s(Dir,DirLen,URL,p-URL);
		URL = p;
	}

	// name

	if (Name && Name == Ext)
		tcscpy_s(Name,NameLen,URL);
	else
	{
		p = tcsrchr(URL,'.');
		if (p)
		{
			if (Name)
				tcsncpy_s(Name,NameLen,URL,p-URL);
			if (Ext)
				tcscpy_s(Ext,ExtLen,p+1);
		}
		else
		{
			if (Name)
				tcscpy_s(Name,NameLen,URL);
			if (Ext)
				Ext[0] = 0;
		}
	}
}

void RelPath(tchar_t* Rel, int RelLen, const tchar_t* Any, const tchar_t* Base)
{
	size_t n;
	bool_t HasHost;
	const tchar_t* p = GetMime(Base,NULL,0,&HasHost);
	if (p != Base)
	{
		if (HasHost)
		{
			// include host name too
			tchar_t *a,*b;
			a = tcschr(p,'\\');
			b = tcschr(p,'/');
			if (!a || (b && b<a))
				a=b;
			if (a)
				p=a;
			else
				p+=tcslen(p);
		}

		// check if mime and host is the same
		n = p-Base;
		if (n<tcslen(Any) && (Any[n]=='\\' || Any[n]=='/') && tcsnicmp(Any,Base,n)==0)
		{
			Base += n;
			Any += n;
		}
	}

	n = tcslen(Base);
	if (n<tcslen(Any) && (Any[n]=='\\' || Any[n]=='/') && tcsnicmp(Any,Base,n)==0)
		Any += n+1;

	tcscpy_s(Rel,RelLen,Any);
}

bool_t UpperPath(tchar_t* Path, tchar_t* Last, int LastLen)
{
	tchar_t *a,*b,*c;
	bool_t HasHost;

	if (!*Path)
		return 0;

	c = (tchar_t*)GetMime(Path,NULL,0,&HasHost);
	
	a = tcsrchr(c,'\\');
	b = tcsrchr(c,'/');
	if (a<b)
		a=b;

	if (!a)
	{
		if (HasHost)
			return 0;
		a=c;
		if (!a[0]) // only mime left
			a=c=Path;
	}
	else
		++a;

	if (Last)
		tcscpy_s(Last,LastLen,a);

	if (a==c)
		*a = 0;
	while (--a>=c && (*a=='\\' || *a=='/'))
		*a = 0;

	return 1;
}

void AbsPath(tchar_t* Abs, int AbsLen, const tchar_t* Any, const tchar_t* Base)
{
	if (GetMime(Base,NULL,0,NULL)!=Base && (Any[0] == '/' || Any[0] == '\\'))
	{
		tchar_t* s;
		bool_t HasHost;

		tcscpy_s(Abs,AbsLen,Base);
		s = (tchar_t*)GetMime(Abs,NULL,0,&HasHost);
		if (!HasHost)
		{
			// keep "mime://" from Base
			++Any;
			*s = 0;
		}
		else
		{
			// keep "mime://host" from Base
			tchar_t *a,*b;
			a = tcschr(s,'\\');
			b = tcschr(s,'/');
			if (!a || (b && b<a))
				a=b;
			if (a)
				*a=0;
		}
	}
	else
	if (GetMime(Any,NULL,0,NULL)==Any && Any[0] != '/' && Any[0] != '\\' &&
		!(Any[0] && Any[1]==':' && Any[2]=='\\'))
	{	
		const tchar_t* MimeEnd = GetMime(Base,NULL,0,NULL);
		tcscpy_s(Abs,AbsLen,Base);

#if defined(TARGET_WIN32) || defined(TARGET_WINCE) || defined(TARGET_SYMBIAN)
		if (MimeEnd==Base)
			tcscat_s(Abs,AbsLen,T("\\"));
		else
#endif
		if (MimeEnd[0])
			tcscat_s(Abs,AbsLen,T("/"));
	}
	else
		Abs[0] = 0;

	tcscat_s(Abs,AbsLen,Any);

	if (GetMime(Abs,NULL,0,NULL)!=Abs)
		for (;*Abs;++Abs)
			if (*Abs == '\\')
				*Abs = '/';
}

bool_t CheckContentType(const tchar_t* s, const tchar_t* List)
{
	size_t n = tcslen(s);
	if (n)
	{
		while (List)
		{
			if (tcsnicmp(List,s,n)==0 && (!List[n] || List[n]==',' || List[n]==' '))
				return 1;
			List = tcschr(List,',');
			if (List) ++List;
		}
	}
	return 0;
}

bool_t UniqueExts(const int* Begin,const int* Pos)
{
	const tchar_t* Exts = LangStr(*Pos,NODE_EXTS);
	if (!Exts[0])
		return 0;

	for (;Begin != Pos;++Begin)
		if (tcsicmp(Exts,LangStr(*Begin,NODE_EXTS))==0)
			return 0;
	return 1;
}

int CheckExts(const tchar_t* URL, const tchar_t* Exts)
{
	tchar_t Ext[MAXPATH];
	tchar_t* Tail;

	SplitURL(URL,NULL,0,NULL,0,NULL,0,Ext,TSIZEOF(Ext));
	Tail = tcschr(Ext,'?');
	if (Tail) *Tail = 0;

	while (Exts)
	{
		const tchar_t* p = tcschr(Exts,':');
		if (p && tcsnicmp(Ext,Exts,p-Exts)==0)
			return p[1]; // return type char
		Exts = tcschr(Exts,';');
		if (Exts) ++Exts;
	}
	return 0;
}

void GetAsciiToken(tchar_t* Out,size_t OutLen,const char* In,size_t InLen)
{
	char* Tmp = alloca(OutLen);
	size_t i,n = min(InLen,OutLen-1);
	for (i=0;i<n && (unsigned char)In[i]<128;++i)
		Tmp[i] = In[i];
	Tmp[i] = 0;
	AsciiToTcs(Out,OutLen,Tmp);
}

void ShowError(int Sender, int Class, int No,...)
{
	tchar_t s[1024];
	const tchar_t* Msg;

	if (Sender)
		stprintf_s(s,TSIZEOF(s), T("%s: "),LangStr(Sender,0));
	else
		s[0] = 0;

	Msg = LangStr(Class,No);
	if (Msg[0])
	{
		va_list Args;
		va_start(Args,No);
		vstprintf_s(s+tcslen(s),TSIZEOF(s)-tcslen(s), Msg, Args);
		va_end(Args);
	}
	else
	{
		FourCCToString(s+tcslen(s),TSIZEOF(s)-tcslen(s),Class);
		stcatprintf_s(s,TSIZEOF(s), T("%04X"), No);
	}

	if (Context()->Error.Func)
		Context()->Error.Func(Context()->Error.This,Sender,(int)s);
	else
		ShowMessage(LangStr(PLATFORM_ID,PLATFORM_ERROR),s);
}

void DebugBinary(const tchar_t* Msg,const void* Data,int Length)
{
	const uint8_t* p = (const uint8_t*)Data;
	int i;
	tchar_t s[256];
	while (Length>0)
	{
		tcscpy_s(s,TSIZEOF(s),Msg);
		for (i=0;Length>0 && i<16;++i,--Length,++p)
			stcatprintf_s(s,TSIZEOF(s),T(" %02x"),*p);
		DebugMessage(s);
	}
}

const nodedef WMVF = 
{
	0, // parent size
	WMVF_ID,
	ASF_ID,
	PRI_DEFAULT,
};

const nodedef WMAF = 
{
	0, // parent size
	WMAF_ID,
	ASF_ID,
	PRI_DEFAULT,
};

