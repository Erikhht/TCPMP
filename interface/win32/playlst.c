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
 * $Id: playlist.c 332 2005-11-06 14:31:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"
#include "playlst.h"
#include "openfile_win32.h"
#include "resource.h"

#if defined(TARGET_WINCE) || defined(TARGET_WIN32)

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#if _MSC_VER > 1000
#pragma warning(disable : 4201)
#endif
#include <commctrl.h>

#define MSG_PLAYER2						WM_APP + 0x300

typedef struct playlistwin
{
	win Win;
	HWND WndList;
	int DlgWidth[3];
	player* Player;
	DWORD ThreadId;
	int Pos;
	int Current;
	bool_t InUpdate;

} playlistwin;

static const datatable Params[] =
{
	{ PLAYLIST_WIDTH_NAME,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYLIST_WIDTH_LENGTH,TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYLIST_WIDTH_URL,	TYPE_INT, DF_SETUP|DF_HIDDEN },

	DATATABLE_END(PLAYLIST_ID)
};

static void UpdateList(playlistwin* p)
{
	tchar_t s[256];
	tchar_t s2[256];
	LVITEM Item;
	tick_t Length;
	int i;
	int Count=0;
	p->Current = -1;

	p->Player->Get(p->Player,PLAYER_LIST_COUNT,&Count,sizeof(int));
	p->Player->Get(p->Player,PLAYER_LIST_CURRENT,&p->Current,sizeof(int));

	i = ListView_GetItemCount(p->WndList);
	while (i > Count)
		ListView_DeleteItem(p->WndList,--i);

	Item.mask=LVIF_TEXT;
	Item.pszText=T("");
	Item.cchTextMax=1;
	Item.iSubItem=0;

	while (i < Count)
	{
		Item.iItem = i++;
		ListView_InsertItem(p->WndList,&Item);
	}

	for (i=0;i<Count;++i)
	{
		s[0] = 0;
		p->Player->Get(p->Player,PLAYER_LIST_AUTOTITLE+i,s,sizeof(s));

		ListView_GetItemText(p->WndList,i,0,s2,256);
		if (tcscmp(s,s2)!=0)
			ListView_SetItemText(p->WndList,i,0,s);

		Length = -1;
		p->Player->Get(p->Player,PLAYER_LIST_LENGTH+i,&Length,sizeof(int));
		if (Length>=0)
			TickToString(s,TSIZEOF(s),Length,0,0,0);
		else
			s[0] = 0;
		ListView_GetItemText(p->WndList,i,1,s2,256);
		if (tcscmp(s,s2)!=0)
			ListView_SetItemText(p->WndList,i,1,s);

		s[0] = 0;
		p->Player->Get(p->Player,PLAYER_LIST_URL+i,s,sizeof(s));
		ListView_GetItemText(p->WndList,i,2,s2,256);
		if (tcscmp(s,s2)!=0)
			ListView_SetItemText(p->WndList,i,2,s);
	}
}

static int Enum(playlistwin* p, int* No, datadef* Param )
{
	return NodeEnumTable(No,Param,Params);
}

static int Get(playlistwin* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLAYLIST_WIDTH_NAME: GETVALUE(p->DlgWidth[0],int); break;
	case PLAYLIST_WIDTH_LENGTH: GETVALUE(p->DlgWidth[1],int); break;
	case PLAYLIST_WIDTH_URL: GETVALUE(p->DlgWidth[2],int); break;
	}
	return Result;
}

static int Set(playlistwin* p,int No,const void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case PLAYLIST_WIDTH_NAME: SETVALUE(p->DlgWidth[0],int,ERR_NONE); break;
	case PLAYLIST_WIDTH_LENGTH: SETVALUE(p->DlgWidth[1],int,ERR_NONE); break;
	case PLAYLIST_WIDTH_URL: SETVALUE(p->DlgWidth[2],int,ERR_NONE); break;
	}
	return Result;
}

static int GetColWidth(playlistwin* p, int Index)
{
	int cx = ListView_GetColumnWidth(p->WndList,Index);
	return WinPixelToUnitX(&p->Win,cx);
}

static void AddCol(playlistwin* p, int Index, const tchar_t* Name, int Width, int Format )
{
	LVCOLUMN Col;
	Col.mask=LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	Col.cx=WinUnitToPixelX(&p->Win,Width);
	Col.pszText=(tchar_t*)Name;
	Col.cchTextMax=tcslen(Name)+1;
	Col.fmt=Format;

	ListView_InsertColumn(p->WndList,Index,&Col);
}

static void CreateButtons(playlistwin* p)
{
	if (QueryPlatform(PLATFORM_TYPENO) != TYPE_SMARTPHONE)
	{
		WinBitmap(&p->Win,IDB_PLAYLIST16,IDB_PLAYLIST32,5);
		WinAddButton(&p->Win,-1,-1,NULL,0);
		WinAddButton(&p->Win,PLAYLIST_ADD_FILES,2,NULL,0);
		WinAddButton(&p->Win,PLAYLIST_UP,0,NULL,0);
		WinAddButton(&p->Win,PLAYLIST_DOWN,1,NULL,0);
		WinAddButton(&p->Win,PLAYLIST_DELETE,3,NULL,0);
		//WinAddButton(&p->Win,PLAYLIST_PLAY,4,NULL,0);
	}
}

static void Resize(playlistwin* p)
{
	RECT r;
	GetClientRect(p->Win.Wnd,&r);

	r.top += p->Win.ToolBarHeight;
	MoveWindow(p->WndList,r.left,r.top,r.right-r.left,r.bottom-r.top,0);
}

static int GetPos(playlistwin* p)
{
	int n;
	for (n=0;n<ListView_GetItemCount(p->WndList);++n)
		if (ListView_GetItemState(p->WndList,n,LVIS_FOCUSED)==LVIS_FOCUSED)
			return n;
	return -1;
}

static int ListNotify(playlistwin* p,int Param,int Param2)
{
	if (p->Win.Wnd)
		PostMessage(p->Win.Wnd,MSG_PLAYER2,0,0);
	return ERR_NONE;
}

static void BeginUpdate(playlistwin* p)
{
	notify Notify;
	Notify.Func = NULL;
	Notify.This = 0;

	p->Player->Set(p->Player,PLAYER_LIST_NOTIFY,&Notify,sizeof(Notify));

	p->Pos = GetPos(p);
	p->InUpdate = 1;
}

static void EndUpdate(playlistwin* p)
{
	int n;
	notify Notify;
	Notify.Func = ListNotify;
	Notify.This = p;
	p->Player->Set(p->Player,PLAYER_LIST_NOTIFY,&Notify,sizeof(Notify));

	UpdateList(p);
	
	for (n=0;n<ListView_GetItemCount(p->WndList);++n)
		ListView_SetItemState(p->WndList,n,n==p->Pos?LVIS_FOCUSED|LVIS_SELECTED:0,LVIS_FOCUSED|LVIS_SELECTED);

	if (p->Pos>=n || p->Pos<0)
	{
		ListView_SetItemState(p->WndList,0,LVIS_FOCUSED,LVIS_FOCUSED);
		p->Pos = 0;
	}

	ListView_EnsureVisible(p->WndList,p->Pos,TRUE);
	p->InUpdate = 0;
}

static void Play(playlistwin* p, int i)
{
	bool_t b;
	node* Format;

	p->Player->Set(p->Player,PLAYER_LIST_CURRENT,&i,sizeof(i));

	// only play when loading succeeded
	if (p->Player->Get(p->Player,PLAYER_FORMAT,&Format,sizeof(Format)) == ERR_NONE && Format)
	{
		b = 1;
		p->Player->Set(p->Player,PLAYER_PLAY,&b,sizeof(b));
	}

	WinClose(&p->Win);
}

static bool_t Proc(playlistwin* p,int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	LPNMLISTVIEW Notify;
	int i,j,n;
	int Style;
	bool_t b;
	node* Node;

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			WinClose(&p->Win);
		break;

	case MSG_PLAYER2:
		if (p->WndList)
		{
			p->InUpdate = 1;
			UpdateList(p);
			p->InUpdate = 0;
		}
		return 1;

	case WM_COMMAND:

		i = LOWORD(wParam);

		switch (i)
		{
		case PLAYLIST_DELETE:
			BeginUpdate(p);

			if (p->Player->Get(p->Player,PLAYER_LIST_COUNT,&j,sizeof(j))==ERR_NONE)
			{
				for (n=ListView_GetItemCount(p->WndList)-1;n>=0;--n)
					if (ListView_GetItemState(p->WndList,n,LVIS_SELECTED)==LVIS_SELECTED)
					{
						for (i=n;i<j-1;++i)
							p->Player->ListSwap(p->Player,i,i+1);
						--j;
						if (p->Pos>n || p->Pos>=j) --p->Pos;
					}

				p->Player->Set(p->Player,PLAYER_LIST_COUNT,&j,sizeof(j));
			}

			EndUpdate(p);
			break;

		case PLAYLIST_UP:
			BeginUpdate(p);
			if (p->Pos>0)
			{
				p->Player->ListSwap(p->Player,p->Pos,p->Pos-1);
				--p->Pos;
			}
			EndUpdate(p);
			break;

		case PLAYLIST_DOWN:
			BeginUpdate(p);
			if (p->Pos>=0 && p->Player->Get(p->Player,PLAYER_LIST_COUNT,&j,sizeof(j))==ERR_NONE && p->Pos<j-1)
			{
				p->Player->ListSwap(p->Player,p->Pos,p->Pos+1);
				++p->Pos;
			}
			EndUpdate(p);
			break;

		case PLAYLIST_PLAY:
			Play(p,GetPos(p));
			break;

		case PLAYLIST_SORTBYNAME:
			BeginUpdate(p);
			if (p->Player->Get(p->Player,PLAYER_LIST_COUNT,&j,sizeof(j))==ERR_NONE)
			{
				// bubble sort sucks :)
				do
				{
					b = 0;
					for (i=0;i<j-1;++i)
					{
						tchar_t URL1[MAXPATH];
						tchar_t URL2[MAXPATH];

						if (p->Player->Get(p->Player,PLAYER_LIST_URL+i,URL1,sizeof(URL1))==ERR_NONE &&
							p->Player->Get(p->Player,PLAYER_LIST_URL+i+1,URL2,sizeof(URL2))==ERR_NONE &&
							tcsicmp(URL1,URL2)>0)
						{
							if (p->Pos == i) 
								p->Pos = i+1;
							else
							if (p->Pos == i+1)
								p->Pos = i;

							p->Player->ListSwap(p->Player,i,i+1);
							b = 1;
						}
					}
				}
				while (b);
			}
			EndUpdate(p);
			break;

		case PLAYLIST_DELETE_ALL:
			BeginUpdate(p);
			i = 0;
			p->Player->Set(p->Player,PLAYER_LIST_COUNT,&i,sizeof(i));
			EndUpdate(p);
			break;

		case PLAYLIST_SAVE:
			BeginUpdate(p);

			Node = NodeCreate(OPENFILE_ID);
			if (Node)
			{
				i = OPENFLAG_SINGLE | OPENFLAG_SAVE;
				Node->Set(Node,OPENFILE_FLAGS,&i,sizeof(i));
				i = PLAYLIST_CLASS;
				Node->Set(Node,OPENFILE_CLASS,&i,sizeof(i));
				((win*)Node)->Popup((win*)Node,&p->Win);
				NodeDelete(Node);
			}

			EndUpdate(p);
			break;

		case PLAYLIST_LOAD:
			BeginUpdate(p);

			Node = NodeCreate(OPENFILE_ID);
			if (Node)
			{
				i = OPENFLAG_SINGLE;
				Node->Set(Node,OPENFILE_FLAGS,&i,sizeof(i));
				i = PLAYLIST_CLASS;
				Node->Set(Node,OPENFILE_CLASS,&i,sizeof(i));
				i = ((win*)Node)->Popup((win*)Node,&p->Win);
				NodeDelete(Node);
				if (i==2)
				{
					UpdateWindow(p->Win.Wnd);
					i=0;
					p->Player->Set(p->Player,PLAYER_LIST_CURRIDX,&i,sizeof(i));
				}
			}

			p->Pos = -1;
			p->Player->Get(p->Player,PLAYER_LIST_CURRENT,&p->Pos,sizeof(int));
			EndUpdate(p);
			break;

		case PLAYLIST_ADD_FILES:
			BeginUpdate(p);

			Node = NodeCreate(OPENFILE_ID);
			if (Node)
			{
				i = OPENFLAG_ADD;
				Node->Set(Node,OPENFILE_FLAGS,&i,sizeof(i));
				i = MEDIA_CLASS;
				Node->Set(Node,OPENFILE_CLASS,&i,sizeof(i));
				((win*)Node)->Popup((win*)Node,&p->Win);
				NodeDelete(Node);
			}

			p->Pos = -1;
			p->Player->Get(p->Player,PLAYER_LIST_CURRENT,&p->Pos,sizeof(int));
			EndUpdate(p);
			break;
		}
		break;

	case WM_CREATE:

		p->InUpdate = 1;
		p->ThreadId = GetCurrentThreadId();
		p->Player = (player*)Context()->Player;

		p->Pos = -1;
		p->Player->Get(p->Player,PLAYER_LIST_CURRENT,&p->Pos,sizeof(int));

		Style = WS_TABSTOP|WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_HSCROLL|LVS_REPORT|LVS_SHOWSELALWAYS;
		if (p->Win.ScreenWidth < 130)
			Style |= LVS_NOCOLUMNHEADER;

		p->WndList = CreateWindow(T("SysListView32"), NULL, Style,
			0,0,20,20, p->Win.Wnd, NULL, p->Win.Module, NULL);

		ListView_SetExtendedListViewStyle(p->WndList,LVS_EX_FULLROWSELECT);

		AddCol(p,0,LangStr(PLAYLIST_ID,PLAYLIST_LABEL_NAME),max(15,p->DlgWidth[0]),LVCFMT_LEFT);
		AddCol(p,1,LangStr(PLAYLIST_ID,PLAYLIST_LABEL_LENGTH),max(15,p->DlgWidth[1]),LVCFMT_LEFT);
		AddCol(p,2,LangStr(PLAYLIST_ID,PLAYLIST_LABEL_URL),max(15,p->DlgWidth[2]),LVCFMT_LEFT);

		CreateButtons(p);
		Resize(p);

		EndUpdate(p);
		break;

	case WM_SETFOCUS:
		SetFocus(p->WndList);
		break;

	case WM_DESTROY:
		BeginUpdate(p);
		p->DlgWidth[0] = GetColWidth(p,0);
		p->DlgWidth[1] = GetColWidth(p,1);
		p->DlgWidth[2] = GetColWidth(p,2);
		p->ThreadId = 0;
		p->WndList = NULL;
		break;

	case WM_SIZE:
		Resize(p);
		break;

	case WM_NOTIFY:
		Notify = (LPNMLISTVIEW) lParam;

		if (Notify->hdr.hwndFrom == p->WndList && !p->InUpdate)
		{
			if (Notify->hdr.code==LVN_ITEMACTIVATE)
				Play(p,Notify->iItem);

			if (Notify->hdr.code==LVN_KEYDOWN && ((LPNMLVKEYDOWN)lParam)->wVKey == VK_ESCAPE)
				WinClose(&p->Win);
		}
		break;
	}
	return 0;
}

static menudef MenuDef[] =
{
	{ 0,PLATFORM_ID, PLATFORM_DONE },
	{ 0,PLAYLIST_ID, PLAYLIST_FILE },
	{ 1,PLAYLIST_ID, PLAYLIST_ADD_FILES },
	{ 1,-1,-1 },
	{ 1,PLAYLIST_ID, PLAYLIST_PLAY },
	{ 1,-1,-1 },
	{ 1,PLAYLIST_ID, PLAYLIST_DELETE },
	{ 1,PLAYLIST_ID, PLAYLIST_DELETE_ALL },
	{ 1,-1,-1 },
	{ 1,PLAYLIST_ID, PLAYLIST_SORTBYNAME },
	{ 1,PLAYLIST_ID, PLAYLIST_UP },
	{ 1,PLAYLIST_ID, PLAYLIST_DOWN },
	{ 1,-1,-1 },
	{ 1,PLAYLIST_ID, PLAYLIST_LOAD },
	{ 1,PLAYLIST_ID, PLAYLIST_SAVE },

	MENUDEF_END
};

WINCREATE(PlayList)

static int Create(playlistwin* p)
{
	PlayListCreate(&p->Win);

	if (!p->Win.Smartphone)
		p->DlgWidth[0] = 145;
	else
		p->DlgWidth[0] = 92;
	p->DlgWidth[1] = 40;
	p->DlgWidth[2] = 200;

	p->Win.WinWidth = 180;
	p->Win.WinHeight = 240;

	p->Win.MenuDef = MenuDef;
	p->Win.Proc = Proc;

	p->Win.Node.Enum = Enum;
	p->Win.Node.Get = Get;
	p->Win.Node.Set = Set;

	return ERR_NONE;
}

static void Delete(playlistwin* p)
{
#ifdef REGISTRY_GLOBAL
	NodeRegSave(&p->Win.Node);
#endif
}

static const nodedef PlayListWin =
{
	sizeof(playlistwin)|CF_GLOBAL,
	PLAYLIST_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void PlaylistWin_Init()
{
	NodeRegisterClass(&PlayListWin);
}

void PlaylistWin_Done()
{
	NodeUnRegisterClass(PLAYLIST_ID);
}

#endif
