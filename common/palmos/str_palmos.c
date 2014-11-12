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
 * $Id: str_palmos.c 432 2005-12-28 16:39:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"

void StringAlloc();
void StringFree();

void String_Init()
{
	MemHandle Resource;
	DmResID Id;
	context* p = Context();
	StringAlloc();

	for (Id=1000;Id<1000+32;++Id)
	{
		Resource = DmGetResource('lang',Id);
		if (Resource)
		{
			int Size = MemHandleSize(Resource);
			void* Data = MemHandleLock(Resource);
			if (Size && Data && StringAddBinary(Data,Size))
				ArrayAppend(&p->StrModule,&Resource,sizeof(Resource),16);
			else
			{
				if (Data)
					MemHandleUnlock(Resource);
				DmReleaseResource(Resource);
			}
		}
	}
}

void String_Done()
{
	MemHandle *i;
	context* p = Context();

	StringFree();

	for (i=ARRAYBEGIN(p->StrModule,MemHandle);i!=ARRAYEND(p->StrModule,MemHandle);++i)
	{
		MemHandleUnlock(*i);
		DmReleaseResource(*i);
	}
	ArrayClear(&p->StrModule);
}

void AsciiToTcs(tchar_t* Out,size_t OutLen,const char* In)
{
	StrToTcs(Out,OutLen,In);
}

void TcsToAscii(char* Out,size_t OutLen,const tchar_t* In)
{
	TcsToStr(Out,OutLen,In);
}

void UTF8ToTcs(tchar_t* Out,size_t OutLen,const char* In)
{
	StrToTcs(Out,OutLen,In); //todo: fix
}

void TcsToUTF8(char* Out,size_t OutLen,const tchar_t* In)
{
	TcsToStr(Out,OutLen,In); //todo: fix
}

void StrToTcsEx(tchar_t* Out,size_t OutLen,const char* In,int CodePage)
{
	tcscpy_s(Out,OutLen,In);
}

void TcsToStrEx(char* Out,size_t OutLen,const tchar_t* In,int CodePage)
{
	tcscpy_s(Out,OutLen,In);
}

void WcsToTcs(tchar_t* Out,size_t OutLen,const uint16_t* In)
{
	if (OutLen>0)
	{
		for (;*In && OutLen>1;++Out,++In,--OutLen)
			*Out = (tchar_t)((*In < 256) ? *In:'?');
		*Out = 0;
	}
}

int tcsicmp(const tchar_t* a,const tchar_t* b) 
{
	for (;*a && *b;++a,++b)
		if (toupper(*a) != toupper(*b))
			break;
	if (*a == *b)
		return 0;
	return (toupper(*a)>toupper(*b)) ? 1:-1;
}

int tcsnicmp(const tchar_t* a,const tchar_t* b,size_t n) 
{
	for (;n>0 && *a && *b;++a,++b,--n)
		if (toupper(*a) != toupper(*b))
			break;
	if (n<=0 || *a == *b)
		return 0;
	return (toupper(*a)>toupper(*b)) ? 1:-1;
}

int GetCodePage(const tchar_t* ContentType)
{
	return Context()->CodePage;
}

#endif
