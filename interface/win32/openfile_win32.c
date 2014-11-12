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
 * $Id: openfile_win32.c 531 2006-01-04 12:56:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"
#include "openfile_win32.h"
#include "interface.h"
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

#define OPENFILE_TYPES		50000

#define EM_SETSYMBOLS       0x00DF
#define SYMBOLS				T("./:-1\\~?#")

#define MAXHISTORY		20

#define IMG_FILE		0
#define IMG_DIR			1
#define IMG_PLAYLIST	2
#define IMG_VIDEO		3
#define IMG_AUDIO		4
#define IMG_DIRUP		5

typedef struct openitem
{
	int Image;
	tchar_t FileName[MAXPATH];
	tchar_t Name[MAXPATH];
	tchar_t Ext[MAXPATH];
	filepos_t Size;
	int64_t Date;
	bool_t DisplayName;
	
} openitem;

typedef struct openfile
{
	win Win;
	HWND WndList;
	HWND WndURL;
	HWND WndGo;
	int* CurrentFileType;
	int FileType;
	int FileTypeSingle;
	int FileTypeSave;
	int SortCol;
	int Class;
	int Flags;
	int Top;
	int TypesMenu;
	bool_t SortDir;
	bool_t Multiple;
	bool_t SelectDir;
	bool_t OwnSelect;
	bool_t NextFocus;
	bool_t OnlyName;
	bool_t OnlyNameState;
	bool_t KeyClick;
	int InUpdate;
	int LastClick;
	int DlgWidth[4];
	tchar_t Path[MAXPATH];
	tchar_t Base[MAXPATH];
	tchar_t Exts[1024];
	tchar_t Last[MAXPATH];

	tchar_t SaveName[MAXPATH];
	array Types;
	int HistoryCount;
	tchar_t* History[MAXHISTORY];

} openfile;

static const datatable Params[] =
{
	{ OPENFILE_FLAGS,		TYPE_INT, DF_HIDDEN },
	{ OPENFILE_CLASS,		TYPE_INT, DF_HIDDEN },
	{ OPENFILE_MULTIPLE,	TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_FILETYPE,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_FILETYPE_SINGLE,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_FILETYPE_SAVE,TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_PATH,		TYPE_STRING, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_SORTCOL,		TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_SORTDIR,		TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_WIDTH_NAME,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_WIDTH_TYPE,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_WIDTH_SIZE,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_WIDTH_DATE,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_ONLYNAME,	TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ OPENFILE_HISTORYCOUNT,TYPE_INT, DF_SETUP|DF_HIDDEN },

	{ OPENFILE_SAVE_NAME, TYPE_STRING, DF_HIDDEN },
	{ OPENFILE_SAVE_TYPE, TYPE_INT, DF_ENUMCLASS|DF_HIDDEN  },
	{ OPENFILE_URL,		  TYPE_STRING, DF_HIDDEN },
	{ OPENFILE_LIST,	  0, DF_HIDDEN },

	DATATABLE_END(OPENFILE_ID)
};

static int Enum(openfile* p, int* No, datadef* Param )
{
	if (NodeEnumTable(No,Param,Params)==ERR_NONE)
	{
		if (Param->No == OPENFILE_SAVE_TYPE)
			Param->Format1 = p->Class;
		return ERR_NONE;
	}

	if (*No >= 0 && *No < p->HistoryCount)
	{
		memset(Param,0,sizeof(datadef));
		Param->No = OPENFILE_HISTORY + *No;
		Param->Type = TYPE_STRING;
		Param->Size = MAXDATA;
		Param->Flags = DF_HIDDEN | DF_SETUP;
		Param->Class = OPENFILE_ID;
		return ERR_NONE;
	}

	return ERR_INVALID_PARAM;
}

static int Get(openfile* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OPENFILE_MULTIPLE: GETVALUE(p->Multiple,bool_t); break;
	case OPENFILE_CLASS: GETVALUE(p->Class,int); break;
	case OPENFILE_FLAGS: GETVALUE(p->Flags,int); break;
	case OPENFILE_FILETYPE: GETVALUE(p->FileType,int); break;
	case OPENFILE_FILETYPE_SINGLE: GETVALUE(p->FileTypeSingle,int); break;
	case OPENFILE_FILETYPE_SAVE: GETVALUE(p->FileTypeSave,int); break;
	case OPENFILE_PATH: GETSTRING(p->Path); break;
	case OPENFILE_SORTCOL: GETVALUE(p->SortCol,int); break;
	case OPENFILE_SORTDIR: GETVALUE(p->SortDir,bool_t); break;
	case OPENFILE_WIDTH_NAME: GETVALUE(p->DlgWidth[0],int); break;
	case OPENFILE_WIDTH_TYPE: GETVALUE(p->DlgWidth[1],int); break;
	case OPENFILE_WIDTH_SIZE: GETVALUE(p->DlgWidth[2],int); break;
	case OPENFILE_WIDTH_DATE: GETVALUE(p->DlgWidth[3],int); break;
	case OPENFILE_HISTORYCOUNT: GETVALUE(p->HistoryCount,int); break;
	case OPENFILE_ONLYNAME: GETVALUE(p->OnlyName,bool_t); break;
	case OPENFILE_SAVE_NAME: GETSTRING(p->SaveName); break;
	case OPENFILE_SAVE_TYPE: GETVALUE(p->FileTypeSave,int); break;
	}
	
	if (No >= OPENFILE_HISTORY && No < OPENFILE_HISTORY+p->HistoryCount && p->History[No-OPENFILE_HISTORY])
		GETSTRING(p->History[No-OPENFILE_HISTORY]);

	return Result;
}

static void SetColWidth(openfile* p, int Index,int Width)
{
	ListView_SetColumnWidth(p->WndList,Index,WinUnitToPixelX(&p->Win,Width));
}

static int GetColWidth(openfile* p, int Index)
{
	if (p->Win.Smartphone)
		return p->DlgWidth[Index]; // no GUI update possible
	return WinPixelToUnitX(&p->Win,ListView_GetColumnWidth(p->WndList,Index));
}

static int UpdateOnlyName(openfile* p)
{
	int i;
	if (p->OnlyName != p->OnlyNameState)
	{
		WinMenuCheck(&p->Win,1,OPENFILE_ONLYNAME,p->OnlyName);
		p->OnlyNameState = p->OnlyName;

		if (p->OnlyName)
		{
			for (i=0;i<4;++i)
				p->DlgWidth[i] = GetColWidth(p,i);
			ShowWindow(p->WndList,SW_HIDE);
			SetColWidth(p,3,0);
			SetColWidth(p,2,0);
			SetColWidth(p,1,0);
			SetColWidth(p,0,300);
			ShowWindow(p->WndList,SW_SHOWNA);
		}
		else
		{
			ShowWindow(p->WndList,SW_HIDE);
			for (i=3;i>=0;--i)
				SetColWidth(p,i,max(15,p->DlgWidth[i]));
			ShowWindow(p->WndList,SW_SHOWNA);
		}
	}
	return ERR_NONE;
}

static int Set(openfile* p,int No,const void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OPENFILE_MULTIPLE: SETVALUE(p->Multiple,bool_t,ERR_NONE); break;
	case OPENFILE_FLAGS: SETVALUE(p->Flags,int,ERR_NONE); break;
	case OPENFILE_CLASS: SETVALUE(p->Class,int,ERR_NONE); break;
	case OPENFILE_FILETYPE: SETVALUE(p->FileType,int,ERR_NONE); break;
	case OPENFILE_FILETYPE_SINGLE: SETVALUE(p->FileTypeSingle,int,ERR_NONE); break;
	case OPENFILE_FILETYPE_SAVE: SETVALUE(p->FileTypeSave,int,ERR_NONE); break;
	case OPENFILE_PATH: SETSTRING(p->Path); break;
	case OPENFILE_SORTCOL: SETVALUE(p->SortCol,int,ERR_NONE); break;
	case OPENFILE_SORTDIR: SETVALUE(p->SortDir,bool_t,ERR_NONE); break;
	case OPENFILE_WIDTH_NAME: SETVALUE(p->DlgWidth[0],int,ERR_NONE); break;
	case OPENFILE_WIDTH_TYPE: SETVALUE(p->DlgWidth[1],int,ERR_NONE); break;
	case OPENFILE_WIDTH_SIZE: SETVALUE(p->DlgWidth[2],int,ERR_NONE); break;
	case OPENFILE_ONLYNAME: SETVALUE(p->OnlyName,bool_t,UpdateOnlyName(p)); break;
	case OPENFILE_HISTORYCOUNT: SETVALUE(p->HistoryCount,int,ERR_NONE); break;
	case OPENFILE_SAVE_NAME: SETSTRING(p->SaveName); break;
	case OPENFILE_SAVE_TYPE: SETVALUE(p->FileTypeSave,int,ERR_NONE); break;
	}

	if (No >= OPENFILE_HISTORY && No < OPENFILE_HISTORY+p->HistoryCount)
	{
		free(p->History[No-OPENFILE_HISTORY]);
		p->History[No-OPENFILE_HISTORY] = tcsdup((tchar_t*)Data);
		Result = ERR_NONE;
	}

	return Result;
}

static void AddCol(openfile* p, int Index, const tchar_t* Name, int Width, int Format )
{
	LVCOLUMN Col;
	Col.mask=LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	Col.cx=WinUnitToPixelX(&p->Win,Width);
	Col.pszText=(tchar_t*)Name;
	Col.cchTextMax=tcslen(Name)+1;
	Col.fmt=Format;

	ListView_InsertColumn(p->WndList,Index,&Col);
}

static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	openfile* p = (openfile*)lParamSort;
	openitem* a = (openitem*)lParam1;
	openitem* b = (openitem*)lParam2;
	int Result = 0;

	if (!a) return b?-1:0;
	if (!b) return 1;

	if (a->Image==IMG_DIR)
		if (b->Image==IMG_DIR)
			Result = tcsicmp(a->Name,b->Name);
		else
			Result = -1;
	else
	if (b->Image==IMG_DIR)
		Result = 1;
	else
	switch(p->SortCol)
	{
	case 0: 
		Result = tcsicmp(a->Name,b->Name); 
		break;
	case 1: 
		Result = tcsicmp(a->Ext,b->Ext); 
		if (!Result) 
			Result = tcsicmp(a->Name,b->Name);
		break;
	case 2: 
		if (a->Size == b->Size) 
			Result = tcsicmp(a->Name,b->Name);
		else
			Result = a->Size > b->Size ? 1:-1;
		break;
	case 3:
		if (a->Date == b->Date) 
			Result = tcsicmp(a->Name,b->Name);
		else
			Result = a->Date > b->Date ? 1:-1;
		break;
	}

	if (p->SortDir)
		Result = -Result;
	return Result;
}

static void SavePos(openfile* p)
{
	LVITEM Item;
	int n;

	p->Last[0] = 0;

	for (n=0;n<ListView_GetItemCount(p->WndList);++n)
		if (ListView_GetItemState(p->WndList,n,LVIS_FOCUSED)==LVIS_FOCUSED)
		{
			Item.iItem=n;
			Item.iSubItem = 0;
			Item.mask=LVIF_PARAM;
			ListView_GetItem(p->WndList,&Item);

			if (Item.lParam)
				tcscpy_s(p->Last,TSIZEOF(p->Last),((openitem*)Item.lParam)->Name);

			break;
		}
}

static void ClearList(openfile* p)
{
	LVITEM Item;
	int n;

	for (n=0;n<ListView_GetItemCount(p->WndList);++n)
	{
		Item.iItem=n;
		Item.iSubItem=0;
		Item.mask=LVIF_PARAM;
		ListView_GetItem(p->WndList,&Item);
		if (Item.lParam)
			free((void*)Item.lParam);
	}	

	ListView_DeleteAllItems(p->WndList);
}

static void SelectAll(openfile* p)
{
	LVITEM Item;
	int n;
	int State;

	p->OwnSelect = 1;

	for (n=0;n<ListView_GetItemCount(p->WndList);++n)
	{
		Item.iItem=n;
		Item.iSubItem=0;
		Item.mask=LVIF_PARAM;
		ListView_GetItem(p->WndList,&Item);

		State = 0;
		if (Item.lParam && (((openitem*)Item.lParam)->Image != IMG_DIR || p->SelectDir))
			State = LVIS_SELECTED;

		ListView_SetItemState(p->WndList,n,State,LVIS_SELECTED);
	}	

	p->OwnSelect = 0;
}

static void UpdateSelectDir(openfile* p)
{
	if (p->Flags & OPENFLAG_SINGLE) 
		return;

	if (p->Win.Smartphone)
	{
		WinMenuCheck(&p->Win,1,OPENFILE_SELECTDIR2,p->SelectDir);
	}
	else
	{
		TBBUTTONINFO Button;
		Button.cbSize = sizeof(Button);
		Button.dwMask = TBIF_STATE;
		Button.fsState = TBSTATE_ENABLED;
		if (p->SelectDir)
			Button.fsState |= TBSTATE_CHECKED;

		SendMessage(p->Win.WndTB,TB_SETBUTTONINFO,OPENFILE_SELECTDIR,(LPARAM)&Button);
	}
}

static void UpdateMultiple(openfile* p)
{
	int Count,i;

	if (p->Flags & OPENFLAG_SINGLE) 
		return;

	if (p->Win.Smartphone)
	{
		WinMenuCheck(&p->Win,1,OPENFILE_MORE,p->Multiple);
	}
	else
	{
		TBBUTTONINFO Button;
		Button.cbSize = sizeof(Button);
		Button.dwMask = TBIF_STATE;
		Button.fsState = TBSTATE_ENABLED;
		if (p->Multiple)
			Button.fsState |= TBSTATE_CHECKED;

		SendMessage(p->Win.WndTB,TB_SETBUTTONINFO,OPENFILE_MORE,(LPARAM)&Button);
	}

	p->OwnSelect = 1;

	Count = ListView_GetItemCount(p->WndList);
	for (i=0;i<Count;++i)
	{
		int State = 0;
		if (!p->Multiple && ListView_GetItemState(p->WndList,i,LVIS_FOCUSED)==LVIS_FOCUSED)
			State = LVIS_SELECTED;
		ListView_SetItemState(p->WndList,i,State,LVIS_SELECTED);
	}

	p->OwnSelect = 0;
}

static void SetURLText(openfile* p,const tchar_t* s)
{
	int Len = tcslen(s);

	++p->InUpdate;
	SetWindowText(p->WndURL,s);
	if (p->Win.Smartphone)
		SendMessage(p->WndURL,EM_SETSEL,Len,Len);
	else
		SendMessage(p->WndURL,CB_SETEDITSEL,0,MAKEWORD(-1,-1));
	--p->InUpdate;
}

static void AddHistory(openfile* p,const tchar_t* s)
{
	int i;


	for (i=0;i<p->HistoryCount;++i)
		if (p->History[i] && tcsicmp(p->History[i],s)==0)
		{
			for (;i>0;--i)
				SwapPChar(&p->History[i],&p->History[i-1]);
			break;
		}

	if (i==p->HistoryCount)
	{
		if (i<MAXHISTORY)
			p->HistoryCount++;
		else
			free(p->History[--i]);

		for (;i>0;--i)
			p->History[i] = p->History[i-1];

		p->History[0] = tcsdup(s);
	}

	if (!p->Win.Smartphone)
	{
		int i;
		SendMessage(p->WndURL,CB_RESETCONTENT,0,0);
		for (i=0;i<p->HistoryCount;++i)
			if (p->History[i])
				SendMessage(p->WndURL,CB_ADDSTRING,0,(LPARAM)p->History[i]);
	}

	SetURLText(p,s);
}

static void UpdateList(openfile* p, bool_t Silent, int ListMode)
{
	streamdir DirItem;
	stream* Stream;
	int Result;
	openitem New;
	LVITEM Item;
	int No=0;
	int Pos=0;
	int State = LVIS_FOCUSED;
	int Len;
	const tchar_t* s;
	bool_t HasHost;

	if (!p->Multiple || (p->Flags & OPENFLAG_SINGLE))
		State |= LVIS_SELECTED; 

	Len = tcslen(p->Path);
	if (Len && (p->Path[Len-1] == '\\' || p->Path[Len-1] == '/'))
		p->Path[--Len] = 0;

	tcscpy_s(p->Base,TSIZEOF(p->Base),p->Path);
	s = p->Path[0] ? p->Path : T("\\");
	AddHistory(p,s);

	if (!ListMode)
		return;

	++p->InUpdate;
	WaitBegin();

	ShowWindow(p->WndList,SW_HIDE);
	ClearList(p);

	Item.mask=LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	Item.pszText=T("");
	Item.cchTextMax=1;
	Item.iSubItem=0;

	Stream = GetStream(p->Path,Silent);

	if (Stream)
	{
		s = GetMime(p->Path,NULL,0,&HasHost);
		if (*s)
		{
			Item.iImage=IMG_DIRUP;
			Item.iItem = No;
			Item.lParam = 0;

			ListView_InsertItem(p->WndList,&Item);
			ListView_SetItemText(p->WndList,No,0,T(".."));
			ListView_SetItemText(p->WndList,No,1,(tchar_t*)LangStr(OPENFILE_ID,OPENFILE_UP));
			++No;
		}

		Result = Stream->EnumDir(Stream,p->Path,p->Exts,*p->CurrentFileType!=-1,&DirItem);

		if (ListMode==2 && Result == ERR_FILE_NOT_FOUND && UpperPath(p->Path,p->Last,TSIZEOF(p->Last)))
		{
			WaitEnd();
			--p->InUpdate;

			p->LastClick = -1;
			UpdateList(p,Silent,ListMode);
			return;
		}

		if (Result == ERR_NOT_DIRECTORY && !Silent)
		{
			New.FileName[0] = 0;
			New.Ext[0] = 0;
			New.Image = IMG_FILE;
			New.Size = 0;

			Item.lParam = (DWORD)malloc(sizeof(openitem));
			if (Item.lParam)
			{
				p->Last[0] = 0;
				Pos = No;
				State = LVIS_SELECTED;
				PostMessage(p->Win.Wnd,WM_COMMAND,PLATFORM_OK,0);

				*(openitem*)Item.lParam = New;
				Item.iItem = No;
				Item.iImage = New.Image;
				ListView_InsertItem(p->WndList,&Item);
				++No;
			}
		}

		if (Result == ERR_NONE)
			Stream->Get(Stream,STREAM_BASE,p->Base,sizeof(p->Base));

		while (Result == ERR_NONE)
		{
			tchar_t Size[32];
			tchar_t Date[32];

			int i;
			for (i=0;i<No;++i)
			{
				LVITEM Item;
				Item.iItem=i;
				Item.iSubItem = 0;
				Item.mask=LVIF_PARAM;
				ListView_GetItem(p->WndList,&Item);

				if (Item.lParam && tcscmp(((openitem*)Item.lParam)->FileName,DirItem.FileName)==0)
					break;
			}

			if (i==No)
			{
				tchar_t DisplayName[MAXPATH];
				tcscpy_s(New.FileName,TSIZEOF(New.FileName),DirItem.FileName);
				New.DisplayName = DirItem.DisplayName[0] != 0;
				tcscpy_s(DisplayName,TSIZEOF(DisplayName),New.DisplayName?DirItem.DisplayName:DirItem.FileName);
				Size[0] = 0;
				Date[0] = 0;

				if (DirItem.Size < 0)
				{
					New.Image = IMG_DIR;
					New.Size = 0;
					New.Date = 0;
					tcscpy_s(New.Ext,TSIZEOF(New.Ext),LangStr(OPENFILE_ID,OPENFILE_DIR));
					tcscpy_s(New.Name,TSIZEOF(New.Name),DisplayName); //keep extension
				}
				else
				{
					switch (DirItem.Type)
					{
					case FTYPE_AUDIO: New.Image = IMG_AUDIO; break;
					case FTYPE_PLAYLIST: New.Image = IMG_PLAYLIST; break;
					case FTYPE_VIDEO: New.Image = IMG_VIDEO; break;
					default: New.Image = IMG_FILE; break;
					}
					New.Size = DirItem.Size;
					New.Date = DirItem.Date;

					SplitURL(DisplayName,NULL,0,NULL,0,New.Name,TSIZEOF(New.Name),New.Ext,TSIZEOF(New.Ext));
					tcsupr(New.Ext);
					if (New.Size >= 0)
					{
						if (New.Size < 10000*1024)
							stprintf_s(Size,TSIZEOF(Size),T("%d KB"),New.Size/1024);
						else
							stprintf_s(Size,TSIZEOF(Size),T("%d MB"),New.Size/(1024*1024));
					}
					if (New.Date >= 0)
					{
						FILETIME Time;
						SYSTEMTIME SysTime;
						Time.dwHighDateTime = (DWORD)(New.Date >> 32);
						Time.dwLowDateTime = (DWORD)(New.Date);
						if (FileTimeToSystemTime(&Time,&SysTime))
							GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&SysTime,NULL,Date,TSIZEOF(Date));
					}
				}
		
				Item.lParam = (DWORD)malloc(sizeof(openitem));
				if (Item.lParam)
				{
					*(openitem*)Item.lParam = New;
					Item.iItem = No;
					Item.iImage = New.Image;
					ListView_InsertItem(p->WndList,&Item);
					ListView_SetItemText(p->WndList,No,0,New.Name);
					ListView_SetItemText(p->WndList,No,1,New.Ext);
					ListView_SetItemText(p->WndList,No,2,Size);
					ListView_SetItemText(p->WndList,No,3,Date);
					++No;
				}
			}

			Result = Stream->EnumDir(Stream,NULL,NULL,0,&DirItem);
		}

		NodeDelete((node*)Stream);
	}

	ListView_SortItems(p->WndList,CompareProc,p);
	ShowWindow(p->WndList,SW_SHOWNA);

	if (p->Last[0])
	{
		int i;
		for (i=0;i<No;++i)
		{
			LVITEM Item;
			Item.iItem=i;
			Item.iSubItem = 0;
			Item.mask=LVIF_PARAM;
			ListView_GetItem(p->WndList,&Item);

			if (Item.lParam && tcsicmp(((openitem*)Item.lParam)->Name,p->Last)==0)
			{
				Pos = i;
				break;
			}
		}
	}

	SetFocus(p->WndList);
	
	p->OwnSelect = 1;
	ListView_SetItemState(p->WndList,Pos,State,State);
	p->OwnSelect = 0;

	ListView_EnsureVisible(p->WndList,Pos,TRUE);

	WaitEnd();
	--p->InUpdate;
}

static void UpdateMenu(openfile* p)
{
	int No;
	int* i;

	p->TypesMenu = WinMenuFind(&p->Win,OPENFILE_ALL_FILES);
	WinMenuInsert(&p->Win,p->TypesMenu,OPENFILE_ALL_FILES,OPENFILE_TYPES-1,LangStr(p->Class,0));

	ArrayClear(&p->Types);
	NodeEnumClass(&p->Types,p->Class);
	for (No=0,i=ARRAYBEGIN(p->Types,int);i!=ARRAYEND(p->Types,int);++i,++No)
	{
		const tchar_t* Name = LangStr(*i,NODE_NAME);
		if (Name[0] && UniqueExts(ARRAYBEGIN(p->Types,int),i))
			WinMenuInsert(&p->Win,p->TypesMenu,OPENFILE_ALL_FILES,OPENFILE_TYPES+No,Name);
	}
}

static void UpdateSort(openfile* p)
{
	WinMenuCheck(&p->Win,1,OPENFILE_LABEL_NAME,p->SortCol==0);
	WinMenuCheck(&p->Win,1,OPENFILE_LABEL_TYPE,p->SortCol==1);
	WinMenuCheck(&p->Win,1,OPENFILE_LABEL_SIZE,p->SortCol==2);
	WinMenuCheck(&p->Win,1,OPENFILE_LABEL_DATE,p->SortCol==3);
	ListView_SortItems(p->WndList, CompareProc, (LPARAM)p);
}

static void UpdateFileType(openfile* p,int ListMode)
{
	int No;
	int* i;
	int FileType;

	if (p->InUpdate)
		return;

	FileType = *p->CurrentFileType;
	if (FileType && FileType!=-1 && !NodeEnumClass(NULL,FileType))
		FileType = *p->CurrentFileType = 0;

	WinMenuCheck(&p->Win,p->TypesMenu,OPENFILE_TYPES-1,FileType==0);
	WinMenuCheck(&p->Win,p->TypesMenu,OPENFILE_ALL_FILES,FileType==-1);

	p->Exts[0]=0;

	for (No=0,i=ARRAYBEGIN(p->Types,int);i!=ARRAYEND(p->Types,int);++i,++No)
	{
		WinMenuCheck(&p->Win,p->TypesMenu,OPENFILE_TYPES+No,FileType==*i);
		if (FileType==*i || FileType==0 || FileType==-1)
		{
			const tchar_t *Exts = LangStr(*i,NODE_EXTS);
			if (Exts[0])
			{
				if (p->Exts[0]) tcscat_s(p->Exts,TSIZEOF(p->Exts),T(";"));
				tcscat_s(p->Exts,TSIZEOF(p->Exts),Exts);
			}
		}
	}

	p->Last[0] = 0;
	UpdateList(p,1,ListMode);
}

static void SetSaveName(openfile* p,int Pos)
{
	LVITEM Item;
	Item.iItem=Pos;
	Item.iSubItem=0;
	Item.mask=LVIF_PARAM;
	if (ListView_GetItem(p->WndList,&Item) && Item.lParam && ((openitem*)Item.lParam)->Image != IMG_DIR)
	{
		int* i;
		SplitURL(((openitem*)Item.lParam)->FileName,NULL,0,NULL,0,p->SaveName,TSIZEOF(p->SaveName),p->SaveName,TSIZEOF(p->SaveName));

		for (i=ARRAYBEGIN(p->Types,int);i!=ARRAYEND(p->Types,int);++i)
			if (CheckExts(p->SaveName,LangStr(*i,NODE_EXTS)))
			{
				tchar_t *s = tcsrchr(p->SaveName,'.');
				if (s) *s = 0;
				p->FileTypeSave = *i;
				WinPropUpdate(&p->Win,&p->Win.Node,OPENFILE_FILETYPE_SAVE);
				break;
			}

		WinPropUpdate(&p->Win,&p->Win.Node,OPENFILE_SAVE_NAME);
		WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_SAVE_NAME,1);
	}
}

static void Save(openfile* p)
{
	player* Player = (player*)Context()->Player;
	if (Player)
	{	
		tchar_t URL[MAXPATH];
		tchar_t Exts[MAXPATH];

		if (!p->SaveName[0])
		{
			ShowError(0,OPENFILE_ID,OPENFILE_EMPTYNAME);
			WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_SAVE_NAME,1);
			return;
		}
		else
		{
			AbsPath(URL,TSIZEOF(URL),p->SaveName,p->Base);
			SplitURL(p->SaveName,NULL,0,NULL,0,NULL,0,Exts,TSIZEOF(Exts));

			if (!Exts[0])
			{
				tchar_t* s;
				tcscpy_s(Exts,TSIZEOF(Exts),LangStr(p->FileTypeSave,NODE_EXTS));
				s = tcschr(Exts,':');
				if (s) *s = 0;
				SetFileExt(URL,TSIZEOF(URL),Exts);
			}

			PlayerSaveList(Player,URL,p->FileTypeSave);
		}
	}
	WinClose(&p->Win);
}

static bool_t IsSelected(openfile* p)
{
	int Count,i;
	Count = ListView_GetItemCount(p->WndList);

	for (i=0;i<Count;++i)
		if (ListView_GetItemState(p->WndList,i,LVIS_SELECTED)==LVIS_SELECTED)
		{
			LVITEM Item;
			Item.iItem=i;
			Item.iSubItem=0;
			Item.mask=LVIF_PARAM;
			if (ListView_GetItem(p->WndList,&Item) && Item.lParam && (((openitem*)Item.lParam)->Image != IMG_DIR || p->SelectDir))
				break;
		}

	return i<Count;
}

static void SelectIfNone(openfile* p,int Pos)
{
	if (!IsSelected(p))
	{
		p->OwnSelect = 1;
		ListView_SetItemState(p->WndList,Pos,LVIS_SELECTED,LVIS_SELECTED);
		p->OwnSelect = 0;
	}
}

static void Add(openfile* p)
{
	player* Player = (player*)Context()->Player;
	bool_t Add = (p->Flags & OPENFLAG_ADD) != 0;
	bool_t b;
	int Count,n,i;

	if (Player && IsSelected(p))
	{
		if (p->Class != SKIN_CLASS)
		{
			if (Add)
			{
				Player->Get(Player,PLAYER_LIST_COUNT,&n,sizeof(n));
				Add = n>0;
			}
			else
			{
				b = 0;
				Player->Set(Player,PLAYER_PLAY,&b,sizeof(b));
				n = 0;
				Player->Set(Player,PLAYER_LIST_COUNT,&n,sizeof(n));
			}
		}

		Count = ListView_GetItemCount(p->WndList);

		for (i=0;i<Count;++i)
			if (ListView_GetItemState(p->WndList,i,LVIS_SELECTED)==LVIS_SELECTED)
			{
				LVITEM Item;
				Item.iItem=i;
				Item.iSubItem=0;
				Item.mask=LVIF_PARAM;
				if (ListView_GetItem(p->WndList,&Item) && Item.lParam && (((openitem*)Item.lParam)->Image != IMG_DIR || p->SelectDir))
				{
					tchar_t URL[MAXPATH];
					if (((openitem*)Item.lParam)->FileName[0])
						AbsPath(URL,TSIZEOF(URL),((openitem*)Item.lParam)->FileName,p->Base);
					else
						tcscpy_s(URL,TSIZEOF(URL),p->Path);

					if (p->Class == SKIN_CLASS)
					{
						p->Win.Parent->Node.Set(&p->Win.Parent->Node,IF_SKINPATH,URL,TSIZEOF(URL));
					}
					else
					if (((openitem*)Item.lParam)->Image == IMG_DIR)
						n = PlayerAddDir(Player,n,URL,p->Exts,*p->CurrentFileType!=-1,0);
					else
						n = PlayerAdd(Player,n,URL,((openitem*)Item.lParam)->DisplayName?((openitem*)Item.lParam)->Name:NULL);
				}
			}

		if (!Add)
			p->Win.Result = 2;
	}

	WinClose(&p->Win);
}

static void Resize(openfile* p)
{
	RECT r;
	int BorderX = WinUnitToPixelX(&p->Win,1);
	int BorderY = WinUnitToPixelY(&p->Win,1);
	int URLHeight;

	GetClientRect(p->Win.Wnd,&r);

	r.left += BorderX;
	r.right -= BorderX;

	if (p->Win.Smartphone)
	{
		URLHeight = WinUnitToPixelY(&p->Win,15);
		MoveWindow(p->WndURL,r.left,p->Top+BorderY,r.right-r.left,URLHeight-2*BorderY,0);
	}
	else
	{
		RECT Combo;
		int ButtonWidth = WinUnitToPixelX(&p->Win,30);

		SendMessage(p->WndURL,CB_SHOWDROPDOWN,0,0);
		r.right -= ButtonWidth;
		MoveWindow(p->WndURL,r.left,p->Top+BorderY,r.right-r.left-BorderX,WinUnitToPixelY(&p->Win,200),0);
		GetWindowRect(p->WndURL,&Combo);
		URLHeight = Combo.bottom - Combo.top + BorderY*2;
		MoveWindow(p->WndGo,r.right,p->Top+BorderY,ButtonWidth,URLHeight-BorderY*2,0);
	}

	GetClientRect(p->Win.Wnd,&r);

	r.top += p->Win.ToolBarHeight;
	MoveWindow(p->Win.WndDialog,r.left,r.top,r.right-r.left,URLHeight+p->Top,0);

	r.top += p->Top+URLHeight;
	MoveWindow(p->WndList,r.left,r.top,r.right-r.left,r.bottom-r.top,0);
}

static void InitMenu(openfile* p)
{
	int i;

	if (!WinMenuEnable(&p->Win,1,OPENFILE_EMPTY,0))
	{
		WinMenuInsert(&p->Win,1,OPENFILE_HISTORY+1,OPENFILE_EMPTY,LangStr(OPENFILE_ID,OPENFILE_EMPTY));

		for (i=1;i<MAXHISTORY;++i)
			WinMenuDelete(&p->Win,1,OPENFILE_HISTORY+i);
	}

	if (p->HistoryCount>1)
	{
		for (i=1;i<MAXHISTORY;++i)
			WinMenuInsert(&p->Win,1,OPENFILE_EMPTY,OPENFILE_HISTORY+i,p->History[i]);

		WinMenuDelete(&p->Win,1,OPENFILE_EMPTY);
	}
	else
		WinMenuEnable(&p->Win,1,OPENFILE_EMPTY,0);
}

static bool_t DialogProc(openfile* p,int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	int i;

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
			PostMessage(p->Win.Wnd,WM_COMMAND,PLATFORM_OK,0);
		break;

	case WM_COMMAND:

		if (p->WndGo == (HWND)lParam && HIWORD(wParam)==BN_CLICKED && !p->InUpdate)
			SendMessage(p->Win.Wnd,WM_COMMAND,OPENFILE_GO,0);

		if (p->WndURL == (HWND)lParam)
		{
			if (p->Win.Smartphone)
			{
				switch (HIWORD(wParam))
				{
				case EN_SETFOCUS:
					WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_URL,0);
					p->Win.CaptureKeys = 1;
					break;
				case EN_KILLFOCUS:
					p->Win.CaptureKeys = 0;
					break;
				}
			}
			else
			{
				switch (HIWORD(wParam))
				{
				case CBN_CLOSEUP:
					if (!p->InUpdate)
					{
						i = SendMessage(p->WndURL,CB_GETCURSEL,0,0);
						if (i!=CB_ERR && i>0 && i < p->HistoryCount)
						{
							SetURLText(p,p->History[i]);					
							PostMessage(p->Win.Wnd,WM_COMMAND,OPENFILE_GO,0);
						}
					}
					break;

				case CBN_SETFOCUS:
					WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_URL,0);
					p->Win.CaptureKeys = 1;
					break;

				case CBN_KILLFOCUS:
					p->Win.CaptureKeys = 0;
					break;
				}
			}
		}
	}
	return 0;
}

static const menudef MenuDef[] =
{
	{ 0,PLATFORM_ID, PLATFORM_OK },
	{ 0,OPENFILE_ID, OPENFILE_FILTER },
	{ 1,OPENFILE_ID, OPENFILE_ALL_FILES },
	{ 0,PLATFORM_ID, PLATFORM_CANCEL },
	{ 0,-1,-1 },
	{ 0,OPENFILE_ID, OPENFILE_MORE },
	{ 0,-1,-1 },
	{ 0,OPENFILE_ID, OPENFILE_SELECTALL },
	{ 0,-1,-1 },
	{ 0,OPENFILE_ID, OPENFILE_SELECTDIR },

	MENUDEF_END
};

static const menudef MenuDefSingle[] =
{
	{ 0,PLATFORM_ID, PLATFORM_OK },
	{ 0,OPENFILE_ID, OPENFILE_FILTER },
	{ 1,OPENFILE_ID, OPENFILE_ALL_FILES },
	{ 0,PLATFORM_ID, PLATFORM_CANCEL },

	MENUDEF_END
};

static const menudef MenuDefSmart[] =
{
	{ 0,PLATFORM_ID, PLATFORM_OK },
	{ 0,OPENFILE_ID, OPENFILE_OPTIONS },
	{ 1,OPENFILE_ID, OPENFILE_ENTERURL },
	{ 1,OPENFILE_ID, OPENFILE_HISTORYMENU },
	{ 2,OPENFILE_ID, OPENFILE_EMPTY },
	{ 1,OPENFILE_ID, OPENFILE_MORE },
	{ 1,OPENFILE_ID, OPENFILE_SELECTALL },
	{ 1,OPENFILE_ID, OPENFILE_SELECTDIR2 },
	{ 1,OPENFILE_ID, OPENFILE_SORT },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_NAME },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_TYPE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_SIZE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_DATE },
	{ 1,OPENFILE_ID, OPENFILE_FILTER },
	{ 2,OPENFILE_ID, OPENFILE_ALL_FILES },
	{ 1,OPENFILE_ID, OPENFILE_ONLYNAME },
	{ 1,PLATFORM_ID, PLATFORM_CANCEL },

	MENUDEF_END
};

static const menudef MenuDefSmartSingle[] =
{
	{ 0,PLATFORM_ID, PLATFORM_OK },
	{ 0,OPENFILE_ID, OPENFILE_OPTIONS },
	{ 1,OPENFILE_ID, OPENFILE_ENTERURL },
	{ 1,OPENFILE_ID, OPENFILE_HISTORYMENU },
	{ 2,OPENFILE_ID, OPENFILE_EMPTY },
	{ 1,OPENFILE_ID, OPENFILE_SORT },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_NAME },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_TYPE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_SIZE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_DATE },
	{ 1,OPENFILE_ID, OPENFILE_FILTER },
	{ 2,OPENFILE_ID, OPENFILE_ALL_FILES },
	{ 1,OPENFILE_ID, OPENFILE_ONLYNAME },
	{ 1,PLATFORM_ID, PLATFORM_CANCEL },

	MENUDEF_END
};

static const menudef MenuDefSmartSingleSave[] =
{
	{ 0,OPENFILE_ID, OPENFILE_SAVE },
	{ 0,OPENFILE_ID, OPENFILE_OPTIONS },
	{ 1,OPENFILE_ID, OPENFILE_ENTERNAME },
	{ 1,OPENFILE_ID, OPENFILE_ENTERURL },
	{ 1,OPENFILE_ID, OPENFILE_HISTORYMENU },
	{ 2,OPENFILE_ID, OPENFILE_EMPTY },
	{ 1,OPENFILE_ID, OPENFILE_SORT },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_NAME },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_TYPE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_SIZE },
	{ 2,OPENFILE_ID, OPENFILE_LABEL_DATE },
	{ 1,OPENFILE_ID, OPENFILE_FILTER },
	{ 2,OPENFILE_ID, OPENFILE_ALL_FILES },
	{ 1,OPENFILE_ID, OPENFILE_ONLYNAME },
	{ 1,PLATFORM_ID, PLATFORM_CANCEL },

	MENUDEF_END
};

static int Command(openfile* p,int i)
{
	if (i>=OPENFILE_HISTORY && i<OPENFILE_HISTORY+p->HistoryCount && !p->InUpdate)
	{
		SetURLText(p,p->History[i-OPENFILE_HISTORY]);					
		PostMessage(p->Win.Wnd,WM_COMMAND,OPENFILE_GO,0);
	}
	else
	switch (i)
	{
	case OPENFILE_SAVE:
		Save(p);
		break;

	case PLATFORM_OK:
		if (p->Win.Focus && p->Win.Focus->Pin.No == OPENFILE_URL)
			SendMessage(p->Win.Wnd,WM_COMMAND,OPENFILE_GO,0);
		else
		if (p->Flags & OPENFLAG_SAVE)
			Save(p);
		else
			Add(p); 
		return ERR_NONE;

	case OPENFILE_TYPES-1:
		*p->CurrentFileType = 0;
		UpdateFileType(p,1);
		break;
	case OPENFILE_ALL_FILES:
		*p->CurrentFileType = -1;
		UpdateFileType(p,1);
		break;

	case OPENFILE_ONLYNAME:
		p->OnlyName = !p->OnlyName;
		UpdateOnlyName(p);
		break;

	case OPENFILE_LABEL_NAME:
	case OPENFILE_LABEL_TYPE:
	case OPENFILE_LABEL_SIZE:
	case OPENFILE_LABEL_DATE:
		if (i == OPENFILE_LABEL_DATE)
			i = 3;
		else
			i -= OPENFILE_LABEL_NAME;
		if (p->SortCol != i)
			p->SortDir = 0;
		else
			p->SortDir = !p->SortDir;
		p->SortCol = i;
		UpdateSort(p);
		break;

	case OPENFILE_GO:
		if (!p->InUpdate)
		{
			tcscpy_s(p->Last,TSIZEOF(p->Last),p->Path);
			GetWindowText(p->WndURL,p->Path,TSIZEOF(p->Path));
			if (tcsicmp(p->Last,p->Path)==0)
				SavePos(p);
			else
				p->Last[0] = 0;

			UpdateList(p,0,1);
		}
		break;

	case OPENFILE_ENTERNAME:
		WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_SAVE_NAME,1);
		break;

	case OPENFILE_ENTERURL:
		SendMessage(p->WndURL,EM_SETSYMBOLS,0,(LPARAM)SYMBOLS);
		WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_URL,1);
		break;

	case OPENFILE_SELECTDIR:
	case OPENFILE_SELECTDIR2:
		p->SelectDir = !p->SelectDir;
		UpdateSelectDir(p);
		break;

	case OPENFILE_SELECTALL:
		p->Multiple = 1;
		UpdateMultiple(p);
		SelectAll(p);
		break;

	case OPENFILE_MORE:
		p->Multiple = !p->Multiple;
		UpdateMultiple(p);
		break;
	}

	if (i >= OPENFILE_TYPES && i < OPENFILE_TYPES+ARRAYCOUNT(p->Types,int))
	{
		*p->CurrentFileType = ARRAYBEGIN(p->Types,int)[i-OPENFILE_TYPES];
		UpdateFileType(p,1);
	}

	return ERR_INVALID_PARAM;
}

static bool_t Proc(openfile* p,int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	LPNMLISTVIEW Notify;
	int Style;
	int FontSize;
	wincontrol* Control;
	bool_t HasHost;
	tchar_t Mime[MAXMIME];

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			SendMessage(p->Win.Wnd,WM_COMMAND,OPENFILE_GO,0);
			return 1;
		}

		if (wParam == VK_ESCAPE)
			PostMessage(p->Win.Wnd,WM_CLOSE,0,0);
		break;

	case WM_INITMENUPOPUP:
		InitMenu(p);
		break;

	case MSG_PREPARE:
		p->CurrentFileType = (p->Flags & OPENFLAG_SINGLE) ? &p->FileTypeSingle : &p->FileType;
		p->Last[0] = 0;

		if (p->Win.Smartphone)
			if (p->Flags & OPENFLAG_SAVE)
				p->Win.MenuDef = MenuDefSmartSingleSave;
			else
				p->Win.MenuDef = (p->Flags & OPENFLAG_SINGLE) ? MenuDefSmartSingle : MenuDefSmart;
		else
			p->Win.MenuDef = (p->Flags & OPENFLAG_SINGLE) ? MenuDefSingle : MenuDef;
		break;

	case WM_CREATE:
		if (p->Win.Smartphone)
		{
			p->WndGo = NULL;
			p->WndURL = CreateWindow(T("EDIT"), NULL, WS_TABSTOP|WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL, 
				0,0,10,10,p->Win.WndDialog, NULL, p->Win.Module, NULL);
			SetWindowLong(p->WndURL,GWL_USERDATA,1); // send back key to control
			SendMessage(p->WndURL,EM_SETSYMBOLS,0,(LPARAM)SYMBOLS);
		}
		else
		{
			p->WndURL = CreateWindow(T("COMBOBOX"), NULL, WS_TABSTOP|WS_VISIBLE|WS_CHILD|CBS_DROPDOWN|CBS_AUTOHSCROLL, 
				0,0,10,10,p->Win.WndDialog, NULL, p->Win.Module, 0L);

			p->WndGo = CreateWindow(T("BUTTON"),NULL,WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_NOTIFY,
				0,0,10,10,p->Win.WndDialog, NULL, p->Win.Module, NULL );

			SetWindowText(p->WndGo,LangStr(OPENFILE_ID,OPENFILE_GO));

			FontSize = 11;
			SendMessage(p->WndGo,WM_SETFONT,(WPARAM)WinFont(&p->Win,&FontSize,0),0);
		}

		FontSize = 12;
		SendMessage(p->WndURL,WM_SETFONT,(WPARAM)WinFont(&p->Win,&FontSize,0),0);

		Style = WS_TABSTOP|WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_HSCROLL|LVS_REPORT|LVS_SHOWSELALWAYS;
		if (p->Win.Smartphone)
			Style |= LVS_NOCOLUMNHEADER;

		p->WndList = CreateWindow(T("SysListView32"), NULL, Style,
			0,0,20,20, p->Win.Wnd, NULL, p->Win.Module, NULL);

		ListView_SetExtendedListViewStyle(p->WndList,LVS_EX_FULLROWSELECT);

		if (WinUnitToPixelX(&p->Win,72) >= 96*2)
			ListView_SetImageList(p->WndList,
				ImageList_LoadBitmap(p->Win.Module,MAKEINTRESOURCE(IDB_FICON32),32,0,0xFF00FF), LVSIL_SMALL);
		else
			ListView_SetImageList(p->WndList,
				ImageList_LoadBitmap(p->Win.Module,MAKEINTRESOURCE(IDB_FICON16),16,0,0xFF00FF), LVSIL_SMALL);

		AddCol(p,0,LangStr(OPENFILE_ID,OPENFILE_LABEL_NAME),max(15,p->DlgWidth[0]),LVCFMT_LEFT);
		AddCol(p,1,LangStr(OPENFILE_ID,OPENFILE_LABEL_TYPE),max(15,p->DlgWidth[1]),LVCFMT_LEFT);
		AddCol(p,2,LangStr(OPENFILE_ID,OPENFILE_LABEL_SIZE),max(15,p->DlgWidth[2]),LVCFMT_RIGHT);
		AddCol(p,3,LangStr(OPENFILE_ID,OPENFILE_LABEL_DATE),max(15,p->DlgWidth[3]),LVCFMT_LEFT);
		p->OnlyNameState = 0;

		p->LastClick = -1;
		p->Top = 0;
		p->Win.LabelWidth = 40;

		if (p->Flags & OPENFLAG_SAVE)
		{
			if (!p->FileTypeSave)
				p->FileTypeSave = NodeEnumClass(NULL,p->Class);

			p->Top += 2;
			WinPropValue(&p->Win,&p->Top,&p->Win.Node,OPENFILE_SAVE_NAME);
			p->Top += 2;
			WinPropValue(&p->Win,&p->Top,&p->Win.Node,OPENFILE_SAVE_TYPE);
			p->Top = WinUnitToPixelY(&p->Win,p->Top+2);
		}

		Control = WinPropNative(&p->Win,&p->Win.Node,OPENFILE_LIST);
		Control->Control = p->WndList;
		Control->ListView = 1;
		Control->External = 1;

		Control = WinPropNative(&p->Win,&p->Win.Node,OPENFILE_URL);
		Control->Control = p->WndURL;
		Control->External = 1;

		p->SelectDir = 0;

		Resize(p);

		GetMime(p->Path,Mime,TSIZEOF(Mime),&HasHost);

		UpdateMenu(p);
		UpdateFileType(p,HasHost?0:2);
		UpdateSort(p);
		UpdateMultiple(p);
		UpdateSelectDir(p);
		UpdateOnlyName(p);

		if (p->Flags & OPENFLAG_SAVE)
			WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_SAVE_NAME,1);
		else
		if (HasHost)
			WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_URL,1);
		break;

	case WM_DESTROY:
		if (!p->OnlyNameState)
		{
			p->DlgWidth[0] = GetColWidth(p,0);
			p->DlgWidth[1] = GetColWidth(p,1);
			p->DlgWidth[2] = GetColWidth(p,2);
			p->DlgWidth[3] = GetColWidth(p,3);
		}
		ClearList(p);
		break;

	case WM_SIZE:
		Resize(p);
		break;

	case WM_NOTIFY:
		Notify = (LPNMLISTVIEW) lParam;

		if (Notify->hdr.hwndFrom == p->WndList)
		{
			if (Notify->hdr.code == NM_SETFOCUS)
				WinPropFocus(&p->Win,&p->Win.Node,OPENFILE_LIST,0);

			// mouse double click in multiple mode
			if (p->Multiple && !(p->Flags & OPENFLAG_SINGLE) && 
				Notify->hdr.code==LVN_ITEMACTIVATE && Notify->iItem==p->LastClick &&
				Notify->ptAction.x >= 0 && Notify->ptAction.y >= 0)
			{
				LVITEM Item;
				Item.iItem=Notify->iItem;
				Item.iSubItem=0;
				Item.mask=LVIF_PARAM;

				if (ListView_GetItem(p->WndList,&Item) && Item.lParam && (((openitem*)Item.lParam)->Image != IMG_DIR || p->SelectDir))
				{
					SelectIfNone(p,Notify->iItem);
					PostMessage(p->Win.Wnd,WM_COMMAND,PLATFORM_OK,0);
					return 0;
				}
			}
		
			if (Notify->hdr.code==LVN_ITEMACTIVATE || Notify->hdr.code==NM_CLICK)
			{
				LVITEM Item;
				Item.iItem=Notify->iItem;
				Item.iSubItem=0;
				Item.mask=LVIF_PARAM;

				p->LastClick = Notify->iItem;

				if (ListView_GetItem(p->WndList,&Item))
				{
					tchar_t Path[MAXPATH];

					if (!Item.lParam) // ".."
					{
						if (UpperPath(p->Path,p->Last,TSIZEOF(p->Last))) 
						{
							p->LastClick = -1;
							UpdateList(p,1,1);
						}
					}
					else
					if (((openitem*)Item.lParam)->Image==IMG_DIR && !p->SelectDir) // directory
					{
						AbsPath(Path,TSIZEOF(Path),((openitem*)Item.lParam)->FileName,p->Base);
						tcscpy_s(p->Path,TSIZEOF(p->Path),Path);
						p->Last[0] = 0;
						p->LastClick = -1;
						UpdateList(p,1,1);
					}
					else
					{
						if (p->Multiple && !(p->Flags & OPENFLAG_SINGLE))
						{
							int State = ListView_GetItemState(p->WndList,Notify->iItem,LVIS_SELECTED) & LVIS_SELECTED;

							p->OwnSelect = 1;
							ListView_SetItemState(p->WndList,Notify->iItem,(State ^ LVIS_SELECTED)|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
							p->OwnSelect = 0;

							if (p->KeyClick)
								keybd_event(VK_DOWN,1,0,0);
						}
						else
						{
							if (p->Flags & OPENFLAG_SAVE)
								SetSaveName(p,Notify->iItem);
							else
							{
								SelectIfNone(p,Notify->iItem);
								PostMessage(p->Win.Wnd,WM_COMMAND,PLATFORM_OK,0);
							}
						}
					}
				}

				p->KeyClick = 0;
			}

			if (Notify->hdr.code==LVN_KEYDOWN && ((LPNMLVKEYDOWN)lParam)->wVKey == VK_ESCAPE)
				PostMessage(p->Win.Wnd,WM_CLOSE,0,0);

			if (p->Multiple && !(p->Flags & OPENFLAG_SINGLE))
			{
				// manual focus and selection

				if (Notify->hdr.code==LVN_KEYDOWN)
				{
					// pass one focus change after keyboard
					
					LPNMLVKEYDOWN Key = (LPNMLVKEYDOWN) lParam;

					if (Key->wVKey == VK_RETURN)
						p->KeyClick = 1;

					if (Key->wVKey == VK_UP ||
						Key->wVKey == VK_PRIOR  ||
						Key->wVKey == VK_HOME)
					{
						if (ListView_GetItemState(p->WndList,0,LVIS_FOCUSED)!=LVIS_FOCUSED)
							p->NextFocus = 1;
					}

					if (Key->wVKey == VK_DOWN ||
						Key->wVKey == VK_NEXT ||
						Key->wVKey == VK_END)
					{
						if (ListView_GetItemState(p->WndList,ListView_GetItemCount(p->WndList)-1,LVIS_FOCUSED)!=LVIS_FOCUSED)
							p->NextFocus = 1;
					}
				}

				if (p->NextFocus && Notify->hdr.code==LVN_ITEMCHANGED && (Notify->uChanged & LVIF_STATE) && 
					((Notify->uOldState ^ Notify->uNewState) & LVIS_FOCUSED))
				{
					// clear NextFocus if the new focus has been set
					if (Notify->uNewState & LVIS_FOCUSED)
						p->NextFocus = 0;

					if ((Notify->uOldState ^ Notify->uNewState) & LVIS_SELECTED)
					{
						// restore old selection
						p->OwnSelect = 1;
						ListView_SetItemState(p->WndList,Notify->iItem,Notify->uOldState & LVIS_SELECTED,LVIS_SELECTED);
						p->OwnSelect = 0;
					}
				}

				if (!p->OwnSelect && Notify->hdr.code==LVN_ITEMCHANGING && (Notify->uChanged & LVIF_STATE))
				{
					// let focus change pass (if NextFocus is set)
					if (!p->NextFocus || !((Notify->uOldState ^ Notify->uNewState) & LVIS_FOCUSED))
					{
						*Result = 1;
						return 1;
					}
				}
			}

			if (Notify->hdr.code==LVN_COLUMNCLICK)
			{
				if (p->SortCol != Notify->iSubItem)
					p->SortDir = 0;
				else
					p->SortDir = !p->SortDir;
				p->SortCol = Notify->iSubItem;

				UpdateSort(p);
			}
		}
		break;
	}
	return 0;
}

static void Delete(openfile* p)
{
	int i;
#ifndef REGISTRY_GLOBAL
	NodeRegSave(&p->Win.Node);
#endif
	ArrayClear(&p->Types);
	for (i=0;i<MAXHISTORY;++i)
		free(p->History[i]);
}

static int Create(openfile* p)
{
	p->OnlyName = p->Win.Smartphone;
	p->DlgWidth[1] = 40;
	p->DlgWidth[2] = 46;
	p->DlgWidth[3] = 60;
	p->Class = MEDIA_CLASS;
	p->CurrentFileType = &p->FileType;

#ifdef TARGET_WIN32
	p->DlgWidth[0] = 150;
	p->Win.WinWidth = 320;
#else
	p->DlgWidth[0] = 90;
	p->Win.WinWidth = 180;
#endif
	p->Win.WinHeight = 240;
	p->Win.Flags |= WIN_DIALOG;
	p->Win.OwnDialogSize = 1;
	p->Win.Command = (wincommand)Command;
	p->Win.Proc = Proc;
	p->Win.DialogProc = DialogProc;
	p->Win.Node.Enum = Enum;
	p->Win.Node.Get = Get;
	p->Win.Node.Set = Set;
	return ERR_NONE;
}

static const nodedef OFile =
{
	sizeof(openfile)|CF_GLOBAL,
	OPENFILE_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void OpenFile_Init()
{
	NodeRegisterClass(&OFile);
}

void OpenFile_Done()
{
	NodeUnRegisterClass(OPENFILE_ID);
}

#endif
