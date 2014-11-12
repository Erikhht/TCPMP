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
 * $Id: asx.c 346 2005-11-21 22:20:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "asx.h"

static const guid ASFHeader = { 0x75B22630, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }};

static int ReadListRef(playlist* p,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length)
{
	int i;
	tchar_t s[MAXLINE];

	DispName[0] = 0;
	*Length = -1;

	while (ParserLine(&p->Parser,s,MAXLINE))
	{
		tcsuprto(s,'=');
		if (stscanf(s,T("REF%d ="),&i)==1)
		{
			tchar_t* p = tcschr(s,'=');
			if (p++)
			{
				while (IsSpace(*p)) ++p;
				tcscpy_s(Path,PathLen,p);
				return ERR_NONE;
			}
		}
	}

	return ERR_END_OF_FILE;
}

static int ReadList(playlist* p,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length)
{
	tchar_t Token[MAXTOKEN];

	Path[0] = 0;
	DispName[0] = 0;
	*Length = -1;

	while (ParserIsElement(&p->Parser,Token,MAXTOKEN))
	{
		if (tcsicmp(Token,T("/ENTRY"))==0)
		{
			if (Path[0])
				return ERR_NONE;
			DispName[0] = 0;
		}
		else
		if (tcsicmp(Token,T("TITLE"))==0)
			ParserElementContent(&p->Parser,DispName,DispNameLen);
		else
		if (tcsicmp(Token,T("REF"))==0 && !Path[0])
		{
			while (ParserIsAttrib(&p->Parser,Token,MAXTOKEN))
			{
				if (tcsicmp(Token,T("HREF"))==0)
					ParserAttribString(&p->Parser,Path,PathLen);
				else
					ParserAttribSkip(&p->Parser);
			}
		}
		else
			ParserElementSkip(&p->Parser);

	}

	return ERR_END_OF_FILE;
}

static int UpdateStream(playlist* p)
{
	p->ReadList = (playreadlist)ReadList;

	if (p->Stream)
	{
		// detect asf files (some files have wrong extensions...)
		const uint8_t* Data = ParserPeek(&p->Parser,16);
		if (Data)
		{
			guid Id;
			Id.v1 = Data[0] | (Data[1] << 8) | (Data[2] << 16) | (Data[3] << 24);
			Data += 4;
			Id.v2 = (uint16_t)(Data[0] | (Data[1] << 8));
			Data += 2;
			Id.v3 = (uint16_t)(Data[0] | (Data[1] << 8));
			Data += 2;
			memcpy(Id.v4,Data,8);

			if (memcmp(&Id, &ASFHeader, sizeof(guid))==0)
			{
				ParserStream(&p->Parser,NULL);
				return ERR_INVALID_DATA;
			}
		}

		// detect web reference files
		if (ParserIsToken(&p->Parser,T("[Reference]")))
			p->ReadList = (playreadlist)ReadListRef;
	}
	return ERR_NONE;
}

static int WriteList(playlist* p, const tchar_t* Path,const tchar_t* DispName,tick_t Length)
{
	if (p->No<0)
	{
		p->No=0;
		StreamPrintf(p->Stream,T("<ASX version = \"3.0\">\n"));
		StreamPrintf(p->Stream,T("  <PARAM NAME = \"Encoding\" VALUE = \"UTF-8\" />\n"));
	}

	if (Path)
	{
	    StreamPrintf(p->Stream,T("  <ENTRY>\n"));
		if (DispName)
		    StreamPrintfEx(p->Stream,1,T("    <TITLE>%s</TITLE>\n"),DispName);
	    StreamPrintfEx(p->Stream,1,T("    <REF HREF = \"%s\" />\n"),Path);
	    StreamPrintf(p->Stream,T("  </ENTRY>\n"));
	}
	else
		StreamPrintf(p->Stream,T("</ASX>\n"));

	return ERR_NONE;
}

static int Create(playlist* p)
{
	p->UpdateStream = (nodefunc)UpdateStream;
	p->ReadList = (playreadlist)ReadList;
	p->WriteList = (playwritelist)WriteList;
	return ERR_NONE;
}

static const nodedef ASX =
{
	sizeof(playlist),
	ASX_ID,
	PLAYLIST_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void ASX_Init()
{
	NodeRegisterClass(&ASX);
}

void ASX_Done()
{
	NodeUnRegisterClass(ASX_ID);
}
