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
 * $Id: str.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

typedef struct stringdef
{	
	int Class;
	int Id;
	// tchar_t s[]
} stringdef;

#define TABLEINIT	3072
#define TABLEALIGN	1024

#ifdef TARGET_PALMOS
#define BUFFERSIZE	512*sizeof(tchar_t)
#else
#define BUFFERSIZE	2048*sizeof(tchar_t)
#endif

void StringAlloc()
{
	context* p = Context();
	if (p->Lang != LANG_DEFAULT)
		ArrayAlloc(&p->StrTable[0],TABLEINIT,TABLEALIGN);
	ArrayAlloc(&p->StrTable[1],TABLEINIT,TABLEALIGN);
	p->StrLock = LockCreate();
}

void StringFree()
{
	context* p = Context();
	array* i;
	for (i=ARRAYBEGIN(p->StrBuffer,array);i!=ARRAYEND(p->StrBuffer,array);++i)
		ArrayClear(i);
	ArrayClear(&p->StrTable[0]);
	ArrayClear(&p->StrTable[1]);
	ArrayClear(&p->StrBuffer);
	LockDelete(p->StrLock);
	p->StrLock = NULL;
}

static int CmpDef(const stringdef* const* pa, const stringdef* const* pb)
{
	const stringdef* a = *pa;
	const stringdef* b = *pb;
	if (a->Class < b->Class || (a->Class == b->Class && a->Id < b->Id))
		return -1;
	if (a->Class > b->Class || a->Id > b->Id)
		return 1;
	return 0;
}

static void AddDef(context* p, bool_t Default, const stringdef* Def)
{
	array* Table = &p->StrTable[Default?1:0];
	ArrayAdd(Table,ARRAYCOUNT(*Table,stringdef*),sizeof(stringdef*),&Def,(arraycmp)CmpDef,TABLEALIGN);
}

static void Filter(tchar_t* i)
{
	tchar_t* j=i;
	for (;*i;++i,++j)
	{
		if (i[0]=='\\' && i[1]=='n')
		{
			*j=10;
			++i;
		}
		else
			*j=*i;
	}
	*j=0;
}

void StringAddText(const char* p, int n)
{
	uint32_t UserLang = Context()->Lang;
	tchar_t* s = (tchar_t*)malloc(MAXLINE*sizeof(tchar_t));
	char* s8 = (char*)malloc(MAXLINE*sizeof(char));
	int CodePage = -1;
	uint32_t Lang = 0;
	tchar_t* q;
	int i,No;

	if (s && s8)
	{
		while (n>0)
		{
			for (;n>0 && (*p==9 || *p==32);++p,--n);
			for (i=0;n>0 && *p!=13 && *p!=10;++p,--n)
				if (i<MAXLINE-1)
					s8[i++] = *p;
			for (;n>0 && (*p==13 || *p==10);++p,--n)
			s8[i]=0;

			if (CodePage>=0 && s8[0]!='[' && s8[0]!=';')
				StrToTcsEx(s,MAXLINE,s8,CodePage);
			else
				AsciiToTcs(s,MAXLINE,s8);

			for (i=0;IsSpace(s[i]);++i);
			if (s[i]==0) continue;
			if (s[i]==';')
			{
				stscanf(s+i,T(";CODEPAGE = %d"),&CodePage);
				continue;
			}
			if (s[i]=='[')
			{
				++i;
				q = tcschr(s+i,']');
				if (!q || CodePage<0) break; // invalid language file
				*q = 0;

				Lang = StringToFourCC(s+i,1);
				if (Lang == FOURCC('D','E','F','A'))
					Lang = LANG_DEFAULT;

				if (Lang != LANG_DEFAULT)
				{
					if (Lang != UserLang)
						break;
					Context()->CodePage = CodePage;
				}
			}
			else
			{
				q = tcschr(s+i,'=');
				if (!q || !Lang) break; // invalid language file
				*q = 0;
				++q;

				if (tcslen(s+i)>4)
				{
					if (tcslen(s+i)<8)
						No = StringToFourCC(s+i+4,1);
					else
					{
						No = StringToInt(s+i+4,1);
						if (No >= 32768)
							No -= 65536;
					}
				}
				else
					No = 0;

				Filter(q);
				StringAdd(Lang==LANG_DEFAULT,StringToFourCC(s+i,1),No,q);
			}
		}
	}

	free(s);
	free(s8);
}

bool_t StringIsBinary(int Class,int Id)
{
	bool_t Result = 1;
	context* p = Context();
	array* i;
	const tchar_t* Def = LangStrDef(Class,Id);
	if (Def[0])
	{
		LockEnter(p->StrLock);
		for (i=ARRAYBEGIN(p->StrBuffer,array);i!=ARRAYEND(p->StrBuffer,array);++i)
			if (ARRAYBEGIN(*i,const tchar_t)<=Def && ARRAYEND(*i,const tchar_t)>Def)
			{
				Result = 0;
				break;
			}
		LockLeave(p->StrLock);
	}
	return Result;
}

bool_t StringAddBinary(const void* Data, size_t Length)
{
	context* p = Context();
	const uint8_t* Def = (const uint8_t*)Data + 2*sizeof(int);
	uint32_t Lang = *(const uint32_t*)Data;
	size_t Len;

	if (Lang != LANG_DEFAULT && Lang != p->Lang)
		return 0;

	LockEnter(p->StrLock);
	while (Length>sizeof(stringdef))
	{
		AddDef(p,Lang==LANG_DEFAULT,(const stringdef*)Def);

		Len = (tcslen((const tchar_t*)(Def+sizeof(stringdef)))+1)*sizeof(tchar_t);
		Len = sizeof(stringdef)+((Len+sizeof(int)-1) & ~(sizeof(int)-1));
		Def += Len;
		Length -= Len;
	}
	LockLeave(p->StrLock);
	return 1;
}

void StringAdd(bool_t Default, int Class, int Id, const tchar_t* s)
{
	context* p = Context();
	int Pos,Len,Total;
	bool_t Found;
	array* Last = &p->StrTable[Default?1:0];
	stringdef Def;
	stringdef* Ptr = &Def;
	Def.Class = Class;
	Def.Id = Id;

	if (!s)	s = T("");

	LockEnter(p->StrLock);
	// already the same?
	Pos = ArrayFind(Last,ARRAYCOUNT(*Last,stringdef*),sizeof(stringdef*),&Ptr,(arraycmp)CmpDef,&Found);
	if (Found && tcscmp(s,(tchar_t*)(ARRAYBEGIN(*Last,stringdef*)[Pos]+1))==0)
	{
		LockLeave(p->StrLock);
		return;
	}

	// add to buffer
	Len = (tcslen(s)+1)*sizeof(tchar_t);
	Total = sizeof(stringdef)+((Len+sizeof(int)-1) & ~(sizeof(int)-1));
	Last = ARRAYEND(p->StrBuffer,array)-1;

	if (ARRAYEMPTY(p->StrBuffer) || ARRAYCOUNT(*Last,uint8_t)+Total > ARRAYALLOCATED(*Last,uint8_t))
	{
		// add new buffer
		if (!ArrayAppend(&p->StrBuffer,NULL,sizeof(array),128))
		{
			LockLeave(p->StrLock);
			return;
		}
		Last = ARRAYEND(p->StrBuffer,array)-1;
		memset(Last,0,sizeof(array));
		if (!ArrayAlloc(Last,BUFFERSIZE,1))
		{
			LockLeave(p->StrLock);
			return;
		}
	}

	// no allocation needed here (can't fail)
	Ptr = (stringdef*)ARRAYEND(*Last,uint8_t);
	ArrayAppend(Last,&Def,sizeof(stringdef),1);
	ArrayAppend(Last,s,Len,1);
	ArrayAppend(Last,NULL,Total-Len-sizeof(stringdef),1);

	AddDef(p,Default,Ptr);
	LockLeave(p->StrLock);
}

void StringAddPrint(bool_t Default, int Class,int No,const tchar_t* Msg, ...)
{
	tchar_t s[256];
	va_list Arg;
	va_start(Arg, Msg);
	vstprintf_s(s,TSIZEOF(s), Msg, Arg);
	va_end(Arg);
	StringAdd(Default,Class,No,s);
}

int LangEnum(int Class, int No)
{
	int Result = 0;
	context* p = Context();
	stringdef **i;
	LockEnter(p->StrLock);

	for (i=ARRAYBEGIN(p->StrTable[1],stringdef*);i!=ARRAYEND(p->StrTable[1],stringdef*);++i)
		if ((*i)->Class==Class && No--==0)
		{
			Result = (*i)->Id;
			break;
		}

	LockLeave(p->StrLock);
	return Result;
}

const tchar_t* LangStr(int Class, int Id)
{
	int n;
	context* p = Context();
	bool_t Found;
	stringdef Def;
	stringdef* Ptr = &Def;
	Def.Class = Class;
	Def.Id = Id;

	LockEnter(p->StrLock);
	for (n=0;n<2;++n)
	{
		int Pos = ArrayFind(&p->StrTable[n],ARRAYCOUNT(p->StrTable[n],stringdef*),
		                    sizeof(stringdef*),&Ptr,(arraycmp)CmpDef,&Found);
		if (Found)
		{
			LockLeave(p->StrLock);
			return (tchar_t*)(ARRAYBEGIN(p->StrTable[n],stringdef*)[Pos]+1);
		}
	}
	LockLeave(p->StrLock);
	return T("");
}

const tchar_t* LangStrDef(int Class, int Id)
{
	int n;
	context* p = Context();
	bool_t Found;
	stringdef Def;
	stringdef* Ptr = &Def;
	Def.Class = Class;
	Def.Id = Id;

	LockEnter(p->StrLock);
	for (n=1;n>=0;--n)
	{
		int Pos = ArrayFind(&p->StrTable[n],ARRAYCOUNT(p->StrTable[n],stringdef*),
		                    sizeof(stringdef*),&Ptr,(arraycmp)CmpDef,&Found);
		if (Found)
		{
			LockLeave(p->StrLock);
			return (tchar_t*)(ARRAYBEGIN(p->StrTable[n],stringdef*)[Pos]+1);
		}
	}
	LockLeave(p->StrLock);
	return T("");
}

