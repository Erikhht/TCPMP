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
 * $Id: str_win32.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../gzip.h"

#define MAXTEXT		20000

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void GetModulePath(tchar_t* Path,const tchar_t* Module);
void FindFiles(const tchar_t* Path, const tchar_t* Mask,void(*Process)(const tchar_t*,void*),void* Param);
void StringAlloc();
void StringFree();

#if !defined(NO_LANG) || defined(NO_PLUGINS)
static void LoadTGZ(stream* p,void* Buffer)
{
	gzreader Reader;
	if (GZInit(&Reader,p))
	{
		int Size;
		tchar_t StrSize[16];
		tarhead Head;

		while (GZRead(&Reader,&Head,sizeof(Head))==sizeof(Head) && Head.size[0])
		{
			GetAsciiToken(StrSize,16,Head.size,sizeof(Head.size));
			if (stscanf(StrSize,T("%o"),&Size)!=1 || Size>MAXTEXT)
				break;
			assert(Size<MAXTEXT);
			StringAddText(Buffer,GZRead(&Reader,Buffer,Size));
			if (Size & 511)
				GZRead(&Reader,Buffer,512-(Size & 511));
		}

		GZDone(&Reader);
	}
}
#endif

#ifndef NO_LANG
static void LoadTXT(const tchar_t* FileName,void* Buffer)
{
	stream* p = StreamOpen(FileName,0);
	if (p)
	{
		StringAddText(Buffer,p->Read(p,Buffer,MAXTEXT));
		StreamClose(p);
	}
}
#endif

void String_Init()
{
	void* Buffer;

	StringAlloc();

	Buffer = malloc(MAXTEXT);
	if (Buffer)
	{
#ifdef NO_PLUGINS
		{
			int n;
			HANDLE Module = GetModuleHandle(NULL);
			HRSRC Rsrc = FindResource(Module,MAKEINTRESOURCE(2000),T("LANGTAR"));
			if (Rsrc)
			{
				int Size = SizeofResource(Module,Rsrc);
				HGLOBAL Global = LoadResource(Module,Rsrc);
				if (Global)
				{
					void* p = LockResource(Global);
					if (p)
					{
						stream* Stream = StreamOpenMem(p,Size);
						if (Stream)
						{
							LoadTGZ(Stream,Buffer);
							StreamCloseMem(Stream);
						}
					}
				}
			}
			for (n=2000;(Rsrc = FindResource(Module,MAKEINTRESOURCE(n),T("LANG")))!=NULL;++n)
			{
				int Size = SizeofResource(Module,Rsrc);
				HGLOBAL Global = LoadResource(Module,Rsrc);
				if (Global)
				{
					void* p = LockResource(Global);
					if (p)
						StringAddText(p,Size);
				}
			}
		}

#endif
#ifndef NO_LANG
		{
			tchar_t Path[MAXPATH];
			stream* p;

			GetModulePath(Path,T("common.dll"));
			tcscat_s(Path,TSIZEOF(Path),T("language.tgz"));
			p = StreamOpen(Path,0);
			if (p)
			{
				LoadTGZ(p,Buffer);
				StreamClose(p);
			}

			GetModulePath(Path,T("common.dll"));
			FindFiles(Path,T("*.txt"),LoadTXT,Buffer);
		}
#endif
		free(Buffer);
	}
}

void String_Done()
{
	StringFree();
}

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
void AsciiToTcs(tchar_t* Out,size_t OutLen,const char* In)
{
#ifdef UNICODE
	if (!MultiByteToWideChar(CP_ACP,0,In,-1,Out,OutLen))
		StrToTcs(Out,OutLen,In);
#else
	StrToTcs(Out,OutLen,In);
#endif
}

void TcsToAscii(char* Out,size_t OutLen,const tchar_t* In)
{
#ifdef UNICODE
	if (!WideCharToMultiByte(CP_ACP,0,In,-1,Out,OutLen,0,0))
		TcsToStr(Out,OutLen,In);
#else
	TcsToStr(Out,OutLen,In);
#endif
}

void UTF8ToTcs(tchar_t* Out,size_t OutLen,const char* In)
{
#ifdef UNICODE
	if (!MultiByteToWideChar(CP_UTF8,0,In,-1,Out,OutLen))
		StrToTcs(Out,OutLen,In);
#else
	WCHAR Temp[512];
	if (!MultiByteToWideChar(CP_UTF8,0,In,-1,Temp,512) ||
		!WideCharToMultiByte(CP_ACP,0,Temp,-1,Out,OutLen,0,0))
		StrToTcs(Out,OutLen,In);
#endif
}

void TcsToUTF8(char* Out,size_t OutLen,const tchar_t* In)
{
#ifdef UNICODE
	if (!WideCharToMultiByte(CP_UTF8,0,In,-1,Out,OutLen,0,0))
		TcsToStr(Out,OutLen,In);
#else
	WCHAR Temp[512];
	if (!MultiByteToWideChar(CP_ACP,0,In,-1,Temp,512) ||
		!WideCharToMultiByte(CP_UTF8,0,Temp,-1,Out,OutLen,0,0))
		TcsToStr(Out,OutLen,In);
#endif
}

void TcsToStrEx(char* Out,size_t OutLen,const tchar_t* In,int CodePage)
{
#ifdef UNICODE
	if (!WideCharToMultiByte(CodePage,0,In,-1,Out,OutLen,0,0) && OutLen>0)
	{
		for (;OutLen>1 && *In;++In,--OutLen,++Out)
			*Out = (char)(*In>255?'?':*In);
		*Out = 0;
	}
#else
	WCHAR Temp[512];
	if (CodePage==Context()->CodePage ||
		!MultiByteToWideChar(CP_ACP,0,In,-1,Temp,512) ||
		!WideCharToMultiByte(CodePage,0,Temp,-1,Out,OutLen,0,0))
		tcscpy_s(Out,OutLen,In);
#endif
}

void StrToTcsEx(tchar_t* Out,size_t OutLen,const char* In,int CodePage)
{
#ifdef UNICODE
	if (!MultiByteToWideChar(CodePage,0,In,-1,Out,OutLen) && OutLen>0)
	{
		for (;OutLen>1 && *In;++In,--OutLen,++Out)
			*Out = (uint8_t)*In;
		*Out = 0;
	}
#else
	WCHAR Temp[512];
	if (CodePage==Context()->CodePage ||
		!MultiByteToWideChar(CodePage,0,In,-1,Temp,512) ||
		!WideCharToMultiByte(CP_ACP,0,Temp,-1,Out,OutLen,0,0))
		tcscpy_s(Out,OutLen,In);
#endif
}

void WcsToTcs(tchar_t* Out,size_t OutLen,const uint16_t* In)
{
#ifdef UNICODE
	tcscpy_s(Out,OutLen,In);
#else
	if (!WideCharToMultiByte(Context()->CodePage,0,In,-1,Out,OutLen,0,0) && OutLen>0)
	{
		for (;OutLen>1 && *In;++In,--OutLen,++Out)
			*Out = (char)(*In>255?'?':*In);
		*Out = 0;
	}
#endif
}

int tcsicmp(const tchar_t* a,const tchar_t* b) 
{
#ifdef UNICODE
	return _wcsicmp(a,b);
#else
	return _stricmp(a,b);
#endif
}

int tcsnicmp(const tchar_t* a,const tchar_t* b,size_t n) 
{
#ifdef UNICODE
	return _wcsnicmp(a,b,n);
#else
	return _strnicmp(a,b,n);
#endif
}

int GetCodePage(const tchar_t* ContentType)
{
	const tchar_t* p = tcsstr(ContentType,T("CHARSET="));
	if (p)
	{
		p += 8;
		if (tcsncmp(p,T("UTF-8"),5)==0)
			return CP_UTF8;
	}
	return Context()->CodePage;
}

#endif
