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
 * $Id: settings.c 345 2005-11-19 15:57:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "win.h"
#include "settings.h"

#define SETTINGS_PAGES		10000
#define MAXPAGE				64

typedef struct settings 
{
	win Win;
	node* Current;
	int Count;
	int Node[MAXPAGE];
	int Menu;

} settings;

static int Done(settings* p)
{
	if (p->Win.Flags & WIN_PROP_SOFTRESET)
		ShowMessage(LangStr(SETTINGS_ID,NODE_NAME),LangStr(SETTINGS_ID,SETTINGS_SOFTRESET));
	else
	if (p->Win.Flags & WIN_PROP_RESTART)
		ShowMessage(LangStr(SETTINGS_ID,NODE_NAME),LangStr(SETTINGS_ID,SETTINGS_RESTART));

	if (p->Win.Flags & WIN_PROP_RESYNC)
	{
		player* Player = (player*)Context()->Player;
		if (Player)
			Player->Set(Player,PLAYER_RESYNC,NULL,0);
	}

	if (p->Win.Flags & WIN_PROP_CHANGED)
	{
		p->Win.Flags &= ~WIN_PROP_CHANGED;
#ifdef REGISTRY_GLOBAL
		NodeRegSaveGlobal();
#else
		if (p->Current)
			NodeRegSave(p->Current);
#endif
	}

	NodeDelete(p->Current);
	p->Current = NULL;
	return ERR_NONE;
}

static void UpdatePage(settings* p)
{
	datadef DataDef;
	node* Node;
	bool_t Found = 0;
	int Class = Context()->SettingsPage;
	int No,i;
	winunit y;
	bool_t CheckList;
	tchar_t Data[MAXDATA/sizeof(tchar_t)];

#ifndef REGISTRY_GLOBAL
	if (p->Win.Flags & WIN_PROP_CHANGED)
	{
		p->Win.Flags &= ~WIN_PROP_CHANGED;
		if (p->Current)
			NodeRegSave(p->Current);
	}
#endif

	WinBeginUpdate(&p->Win);

	for (No=0;No<p->Count;++No)
	{
		if (p->Node[No]==Class)
			Found = 1;
		WinMenuCheck(&p->Win,p->Menu,SETTINGS_PAGES+No,p->Node[No]==Class);
	}

	if (!Found && p->Count>0)
		Class = p->Node[0];
	
	NodeDelete(p->Current);
	p->Current = Node = NodeCreate(Class);
	
	if (Node)
	{
		WinTitle(&p->Win,LangStr(Class,NODE_NAME));

		p->Win.LabelWidth = 
			(Class == PLATFORM_ID) ? 60:
			(p->Win.ScreenWidth < 130) ? 80:
			(Class == PLAYER_ID || Class == ADVANCED_ID) ? 120:90;

		CheckList = Class == ASSOCIATION_ID;
		y = 2;

		// if the menu is in bottom of screen. print a hint before platform settings (first page shown)
		if (Class == PLATFORM_ID && (p->Win.Flags & WIN_BOTTOMTOOLBAR) && !(p->Win.Flags & WIN_2BUTTON))
		{
			WinLabel(&p->Win,&y,-1,-1,LangStr(SETTINGS_ID,SETTINGS_HINT),PROPSIZE,0,NULL);
			y += 10;
		}

		for (No=0;NodeEnum(Node,No,&DataDef)==ERR_NONE;++No)
			if ((DataDef.Flags & DF_SETUP) && !(DataDef.Flags & DF_HIDDEN))
			{
				if (DataDef.Flags & DF_GAP)
					y += 7;

				if (!(DataDef.Flags & DF_RDONLY))
					WinPropValue(&p->Win,&y,Node,DataDef.No);
				else
				if (Node->Get(Node,DataDef.No,Data,DataDef.Size)==ERR_NONE)
				{
					switch (DataDef.Type)
					{
					case TYPE_LABEL:
						WinLabel(&p->Win,&y,-1,-1,DataDef.Name,PROPSIZE,0,NULL);
						break;

					case TYPE_TICK:
						TickToString(Data,TSIZEOF(Data),*(tick_t*)Data,0,1,0);
						WinPropLabel(&p->Win,&y,DataDef.Name,Data);
						break;

					case TYPE_INT:
						i = *(int*)Data;

						if (DataDef.Flags & DF_ENUMCLASS)
							tcscpy_s(Data,TSIZEOF(Data),LangStr(i,NODE_NAME));
						else
						if (DataDef.Flags & DF_ENUMSTRING)
							tcscpy_s(Data,TSIZEOF(Data),LangStr(DataDef.Format1,i));
						else
							Data[0] = 0;

						if (!Data[0])
							IntToString(Data,TSIZEOF(Data),i,(DataDef.Flags & DF_HEX)!=0);
						if (DataDef.Flags & DF_KBYTE)
							tcscat_s(Data,TSIZEOF(Data),T(" KB"));
						if (DataDef.Flags & DF_MHZ)
							tcscat_s(Data,TSIZEOF(Data),T(" Mhz"));
						WinPropLabel(&p->Win,&y,DataDef.Name,Data);
						break;

					case TYPE_STRING:
						WinPropLabel(&p->Win,&y,DataDef.Name,Data);
						break;
						
					case TYPE_HOTKEY:
						HotKeyToString(Data,TSIZEOF(Data),*(int*)Data);
						WinPropLabel(&p->Win,&y,DataDef.Name,Data);
						break;
						
					case TYPE_BOOL:
						WinPropLabel(&p->Win,&y,DataDef.Name,
							LangStr(PLATFORM_ID,*(bool_t*)Data ? PLATFORM_YES:PLATFORM_NO));
						break;

					default:
						WinPropLabel(&p->Win,&y,DataDef.Name,NULL);
						break;
					}
				}
			}
	}

	WinEndUpdate(&p->Win);
}

static int Init(settings* p)
{
	array List;
	int* i;

	p->Menu = WinMenuFind(&p->Win,SETTINGS_PAGES-1);
	p->Win.Flags &= ~(WIN_PROP_RESTART|WIN_PROP_SOFTRESET|WIN_PROP_RESYNC|WIN_PROP_CHANGED);

	NodeEnumClass(&List,NODE_CLASS);
	for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
		if ((NodeClassDef(*i)->Flags & (CF_GLOBAL|CF_SETTINGS))==(CF_GLOBAL|CF_SETTINGS))
		{
			WinMenuInsert(&p->Win,p->Menu,SETTINGS_PAGES-1,SETTINGS_PAGES+p->Count,LangStr(*i,NODE_NAME));
			p->Node[p->Count++] = *i;
			if (p->Count==MAXPAGE)
				break;
		}

	ArrayClear(&List);

	WinMenuDelete(&p->Win,p->Menu,SETTINGS_PAGES-1);
	UpdatePage(p);
	return ERR_NONE;
}

static int Command(settings* p,int Cmd)
{
	if (Cmd >= SETTINGS_PAGES && Cmd < SETTINGS_PAGES+p->Count)
	{
		Context()->SettingsPage = p->Node[Cmd-SETTINGS_PAGES];
		UpdatePage(p);
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}

static const menudef MenuDef[] =
{
	{ 0, PLATFORM_ID, PLATFORM_OK },
	{ 0, SETTINGS_ID, SETTINGS_SELECTPAGE },
	{ 1, SETTINGS_ID, SETTINGS_PAGES-1 }, // just place holder (will be deleted)

	MENUDEF_END
};

WINCREATE(Settings)

static int Create(settings* p)
{
	SettingsCreate(&p->Win);
#ifdef TARGET_WIN32
	p->Win.WinWidth = 190;
#else
	p->Win.WinWidth = 180;
#endif
	p->Win.WinHeight = 240;
	p->Win.Flags |= WIN_DIALOG;
	p->Win.MenuDef = MenuDef;
	p->Win.Init = (nodefunc)Init;
	p->Win.Done = (nodefunc)Done;
	p->Win.Command = (wincommand)Command;
	return ERR_NONE;
}

static const nodedef Settings =
{
	sizeof(settings),
	SETTINGS_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create
};

void Settings_Init()
{
	NodeRegisterClass(&Settings);
}

void Settings_Done()
{
	NodeUnRegisterClass(SETTINGS_ID);
}
