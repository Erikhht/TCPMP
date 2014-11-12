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
 * $Id: pls.c 346 2005-11-21 22:20:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "pls.h"

typedef struct pls
{
	playlist Playlist;

	tchar_t File[MAXLINE];
	tchar_t Title[MAXLINE];
	int Length;

} pls;

static int UpdateStream(pls* p)
{
	p->File[0] = 0;
	p->Title[0] = 0;
	p->Length = -1;
	return ERR_NONE;
}

static bool_t Fill(pls* p,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length)
{
	if (p->Playlist.No >= 0 && p->File[0])
	{
		tcscpy_s(Path,PathLen,p->File);
		tcscpy_s(DispName,DispNameLen,p->Title);
		if (p->Length>=0) p->Length *= TICKSPERSEC;
		*Length = p->Length;

		p->File[0] = 0;
		p->Title[0] = 0;
		p->Length = -1;
		return 1;
	}
	return 0;
}

static int ReadList(pls* p,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length)
{
	tchar_t s[MAXLINE];
	int No,Len;
	int Result = ERR_END_OF_FILE;

	while (Result==ERR_END_OF_FILE && ParserLine(&p->Playlist.Parser,s,MAXLINE))
	{
		tcsuprto(s,'=');
		if (stscanf(s,T("FILE%d ="),&No)==1)
		{
			if (No != p->Playlist.No && Fill(p,Path,PathLen,DispName,DispNameLen,Length))
				Result = ERR_NONE;
			p->Playlist.No = No;
			tcscpy_s(p->File,TSIZEOF(p->File),tcschr(s,'=')+1);
		}
		else
		if (stscanf(s,T("TITLE%d ="),&No)==1)
		{
			if (No != p->Playlist.No && Fill(p,Path,PathLen,DispName,DispNameLen,Length))
				Result = ERR_NONE;
			p->Playlist.No = No;
			tcscpy_s(p->Title,TSIZEOF(p->Title),tcschr(s,'=')+1);
		}
		else
		if (stscanf(s,T("LENGTH%d = %d"),&No,&Len)==2)
		{
			if (No != p->Playlist.No && Fill(p,Path,PathLen,DispName,DispNameLen,Length))
				Result = ERR_NONE;
			p->Playlist.No = No;
			p->Length = Len;
		}
	}

	if (Result==ERR_END_OF_FILE && Fill(p,Path,PathLen,DispName,DispNameLen,Length))
		Result = ERR_NONE;

	return Result;
}

static int WriteList(pls* p, const tchar_t* Path,const tchar_t* DispName,tick_t Length)
{
	if (p->Playlist.No<0)
	{
		p->Playlist.No=0;
		StreamPrintf(p->Playlist.Stream,T("[playlist]\n"));
	}

	if (Path)
	{
		++p->Playlist.No;
		StreamPrintf(p->Playlist.Stream,T("File%d=%s\n"),p->Playlist.No,Path);
		if (DispName[0]) StreamPrintf(p->Playlist.Stream,T("Title%d=%s\n"),p->Playlist.No,DispName);
		if (Length>=0) StreamPrintf(p->Playlist.Stream,T("Length%d=%d\n"),p->Playlist.No,Length/TICKSPERSEC);
	}
	else
	{
		StreamPrintf(p->Playlist.Stream,T("NumberOfEntries=%d\n"),p->Playlist.No);
		StreamPrintf(p->Playlist.Stream,T("Version=2\n"));
	}
	return ERR_NONE;
}

static int Create(pls* p)
{
	p->Playlist.UpdateStream = (nodefunc)UpdateStream;
	p->Playlist.ReadList = (playreadlist)ReadList;
	p->Playlist.WriteList = (playwritelist)WriteList;
	return ERR_NONE;
}

static const nodedef PLS =
{
	sizeof(pls),
	PLS_ID,
	PLAYLIST_CLASS, 
	PRI_DEFAULT+1, //default playlist format (form saving)
	(nodecreate)Create,
	NULL,
};

void PLS_Init()
{
	NodeRegisterClass(&PLS);
}

void PLS_Done()
{
	NodeUnRegisterClass(PLS_ID);
}
