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
 * $Id: parser2.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

void ParserDataFeed(parser* p,const void* Ptr,size_t Len)
{
	BufferWrite(&p->Buffer,Ptr,Len,4096);
}

void ParserStream(parser* p, stream* Stream)
{
	p->Stream = Stream;
	if (Stream)
	{
		if (!p->Buffer.Data)
		{
			BufferAlloc(&p->Buffer,4096,1);
			BufferStream(&p->Buffer,p->Stream);
		}
	}
	else
		BufferClear(&p->Buffer);
}

const uint8_t* ParserPeek(parser* p, size_t Len)
{
	if (p->Buffer.WritePos < p->Buffer.ReadPos+(int)Len) //buffer!
	{
		BufferStream(&p->Buffer,p->Stream);
		if (p->Buffer.WritePos < p->Buffer.ReadPos+(int)Len) //buffer!
			return NULL;
	}
	return p->Buffer.Data + p->Buffer.ReadPos;
}

static void SkipSpace(parser* p)
{
	const uint8_t* i;
	while ((i = ParserPeek(p,1))!=NULL)
	{
		if (!IsSpace(*i) && *i!=10 && *i!=13)
			break;
		++p->Buffer.ReadPos;
	}
}

static bool_t SkipAfter(parser* p, char ch)
{
	const char* i;
	while ((i = (const char*)ParserPeek(p,1))!=NULL)
	{
		++p->Buffer.ReadPos;
		if (*i == ch)
			return 1;
	}
	return 0;
}

bool_t ParserIsToken(parser* p, const tchar_t* Token)
{
	size_t n = tcslen(Token);
	tchar_t* Tmp = alloca(sizeof(tchar_t)*(n+1));
	const char* i;

	SkipSpace(p);
	if ((i=(const char*)ParserPeek(p,n))!=NULL)
	{
		GetAsciiToken(Tmp,n+1,i,n);
		if (tcsicmp(Tmp,Token)==0)
		{
			p->Buffer.ReadPos += n;
			return 1;
		}
	}
	return 0;
}

static int Read(parser* p, tchar_t* Out, int OutLen, const char* Delimiter, bool_t Keep)
{
	char* s = alloca(OutLen);
	int n=0;

	while (ParserPeek(p,1))
	{
		char ch = p->Buffer.Data[p->Buffer.ReadPos++];
		if (strchr(Delimiter,ch))
		{
			if (Keep)
				--p->Buffer.ReadPos;
			goto readok;
		}
		else
		if (ch!=13 && ++n<OutLen)
			s[n-1] = ch;
	}
	if (!n)
		return -1;

readok:
	if (OutLen>0)
	{
		s[min(n,OutLen-1)]=0;
		StrToTcs(Out,OutLen,s);
	}
	return n;
}

bool_t ParserLine(parser* p, tchar_t* Out, size_t OutLen)
{
	return Read(p,Out,OutLen,"\n",0)>=0;
}

bool_t ParserIsElement(parser* p, tchar_t* Name, size_t NameLen)
{
	if (!SkipAfter(p,'<'))
		return 0;
	if (ParserIsToken(p,T("/")) && NameLen>0)
	{
		*(Name++) = '/';
		--NameLen;
	}
	return Read(p,Name,NameLen," \t\n/>",1)>0;
}

void ParserElementSkip(parser *p)
{
	while (ParserIsAttrib(p,NULL,0))
		ParserAttribSkip(p);
}

bool_t ParserElementContent(parser* p, tchar_t* Out, size_t OutLen)
{
	ParserElementSkip(p);
	return Read(p,Out,OutLen,"<",1)>=0;
}

bool_t ParserIsAttrib(parser* p, tchar_t* Name, size_t NameLen)
{
	SkipSpace(p);
	if (ParserIsToken(p,T(">")) || ParserIsToken(p,T("/>")))
		return 0;
	return Read(p,Name,NameLen,"= \t\n/>",1)>0;
}

int ParserAttribInt(parser* p)
{
	tchar_t Token[MAXTOKEN];
	if (!ParserAttribString(p,Token,MAXTOKEN))
		return 0;
	if (Token[0]=='0' && Token[1]=='x')
		return StringToInt(Token+2,1);
	else
		return StringToInt(Token,0);
}

bool_t ParserAttribString(parser* p, tchar_t* Out, size_t OutLen)
{
	if (!ParserIsToken(p,T("=")))
		return 0;
	SkipSpace(p);
	if (ParserIsToken(p,T("\"")))
		return Read(p,Out,OutLen,"\"",0)>=0;
	else
		return Read(p,Out,OutLen," \t\n/>",1)>=0;
}

void ParserAttribSkip(parser* p)
{
	ParserAttribString(p,NULL,0);
}
