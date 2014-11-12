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
 * $Id: about.c 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "win.h"
#include "about.h"
#include "../config.h"

typedef struct about
{
	win Win;
	tchar_t Title[64];
	tchar_t Head[64];

} about;

static int Command(about* p,int Cmd)
{
	stream* File;
	tchar_t Path[MAXPATH];

	switch (Cmd)
	{
#ifdef WINSHOWHTML
	case ABOUT_COPYRIGHT:
		WinShowHTML(T("tcpmp.htm"));
		break;
#endif

	case ABOUT_DUMP:
		GetDebugPath(Path,TSIZEOF(Path),T("dump.txt"));
		File = StreamOpen(Path,1);
		if (File)
		{
			NodeDump(File);
			StreamClose(File);
			ShowMessage(LangStr(PLATFORM_ID,PLATFORM_DUMP_TITLE),
						LangStr(PLATFORM_ID,PLATFORM_DUMP_MESSAGE));
		}
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}

static int Init(about* p)
{
	const tchar_t* Translate;
	winunit y = 4;

	WinTitle(&p->Win,p->Title);
	WinLabel(&p->Win,&y,-1,-1,T("The Core Pocket Media Player"),12,LABEL_BOLD|LABEL_CENTER,NULL);
	WinLabel(&p->Win,&y,-1,-1,p->Head,11,LABEL_CENTER,NULL);

#if defined(CONFIG_DEMO)
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,T("FOR DEMONSTRATION PURPOSES ONLY"),11,LABEL_BOLD|LABEL_CENTER,NULL);
	y+=3;
#endif

	y+=4;
	WinLabel(&p->Win,&y,-1,-1,T("Copyright (C) 2002-2006, CoreCodec, Inc."),10,LABEL_CENTER,NULL);
	WinLabel(&p->Win,&y,-1,-1,T("All Rights Reserved."),10,LABEL_CENTER,NULL);
	WinLabel(&p->Win,&y,-1,-1,T("http://www.tcpmp.com"),11,LABEL_BOLD|LABEL_CENTER,NULL);
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,T("CoreCodec Audio / Video Project"),11,LABEL_CENTER,NULL);
	WinLabel(&p->Win,&y,-1,-1,T("http://www.corecodec.org"),11,LABEL_BOLD|LABEL_CENTER,NULL);
	y+=7;

	Translate = LangStr(ABOUT_ID,ABOUT_TRANSLATION);
	if (Translate[0])
	{
		y += 6;
		WinLabel(&p->Win,&y,-1,-1,Translate,11,LABEL_CENTER|LABEL_BOLD,NULL);
	}

#if defined(CONFIG_GPL)
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_LICENSE),11,0,NULL);
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_LICENSE2),11,0,NULL);
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_LIBSUSED),11,0,NULL);
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_LIBS),11,LABEL_BOLD,NULL);
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_THANKS),11,0,NULL);
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_THANKSLIBS),11,LABEL_BOLD,NULL);
#endif
	y+=7;
	WinLabel(&p->Win,&y,-1,-1,LangStr(ABOUT_ID,ABOUT_FORUM),11,0,NULL);

	return ERR_NONE;
}

static const menudef MenuDef[] =
{
	{ 0, PLATFORM_ID, PLATFORM_DONE },
	{ 0, ABOUT_ID, ABOUT_DUMP },
#ifdef WINSHOWHTML
	{ 0, ABOUT_ID, ABOUT_COPYRIGHT },
#endif	
	MENUDEF_END
};

#ifdef WINSHOWHTML
static const menudef MenuDef2[] =
{
	{ 0, PLATFORM_ID, PLATFORM_DONE },
	{ 0, ABOUT_ID, ABOUT_OPTIONS },
	{ 1, ABOUT_ID, ABOUT_DUMP },
	{ 1, ABOUT_ID, ABOUT_COPYRIGHT },
	
	MENUDEF_END
};
#endif

WINCREATE(About)

static int Create(about* p)
{
	context* c = Context();
	AboutCreate(&p->Win);

	p->Win.WinWidth = 180;
	p->Win.WinHeight = 240;
	p->Win.Flags |= WIN_DIALOG;
#ifdef WINSHOWHTML
	p->Win.MenuDef = (p->Win.Flags & WIN_2BUTTON) ? MenuDef2 : MenuDef;
#else
	p->Win.MenuDef = MenuDef;
#endif
	p->Win.Init = (nodefunc)Init;
	p->Win.Command = (wincommand)Command;

	stprintf_s(p->Title,TSIZEOF(p->Title),LangStr(ABOUT_ID,ABOUT_TITLE),c->ProgramName);
	stprintf_s(p->Head,TSIZEOF(p->Head),LangStr(ABOUT_ID,ABOUT_HEAD),c->ProgramName,c->ProgramVersion);
	return ERR_NONE;
}

static const nodedef About =
{
	sizeof(about),
	ABOUT_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create
};

void About_Init()
{
	NodeRegisterClass(&About);
}

void About_Done()
{
	NodeUnRegisterClass(ABOUT_ID);
}
