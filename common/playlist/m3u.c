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
 * $Id: m3u.c 346 2005-11-21 22:20:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "m3u.h"

static int ReadList(playlist* p,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length)
{
	tchar_t s[MAXLINE];

	DispName[0] = 0;
	*Length = -1;

	while (ParserLine(&p->Parser,s,MAXLINE))
	{
		if (s[0]=='#')
		{
			tcsuprto(s,':');
			if (stscanf(s,T("#EXTINF: %d ,"),Length)==1)
			{
				tchar_t* p = tcschr(s,',');
				if (p++)
				{
					while (IsSpace(*p)) ++p;
					tcscpy_s(DispName,DispNameLen,p);
				}
				if (*Length >= 0)
					*Length *= TICKSPERSEC;
			}
		}
		else
		if (s[0])
		{
			tcscpy_s(Path,PathLen,s);
			return ERR_NONE;
		}
	}

	return ERR_END_OF_FILE;
}

static int WriteList(playlist* p, const tchar_t* Path,const tchar_t* DispName,tick_t Length)
{
	if (Path)
	{
		if (p->No++<0)
			StreamPrintf(p->Stream,T("#EXTM3U\n"));

		if (Length >= 0 || DispName[0])
			StreamPrintf(p->Stream,T("#EXTINF:%d,%s\n"),Length/TICKSPERSEC,DispName);

		StreamPrintf(p->Stream,T("%s\n"),Path);
	}
	return ERR_NONE;
}

static int Create(playlist* p)
{
	p->ReadList = (playreadlist)ReadList;
	p->WriteList = (playwritelist)WriteList;
	return ERR_NONE;
}

static const nodedef M3U =
{
	sizeof(playlist),
	M3U_ID,
	PLAYLIST_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void M3U_Init()
{
	NodeRegisterClass(&M3U);
}

void M3U_Done()
{
	NodeUnRegisterClass(M3U_ID);
}

