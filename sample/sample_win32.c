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
 * $Id: sample_win32.c 548 2006-01-08 22:41:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../interface/win.h"
#include "../interface/benchresult.h"
#include "dumpoutput.h"

const tchar_t URL[] = T("c:\\sample.avi");
//const tchar_t URL[] = T("F:\\samples\\h264.ref\\HCMP1_HHI_A.264");
//#define LOADALL		FOURCC('F','M','T','_')
//#define TIMELIMIT	TICKSPERSEC*5
#define BENCHMARK
//#define FULLSCREEN

static bool_t Play;

static void EnumDir(const tchar_t* Path,const tchar_t* Exts)
{
	streamdir DirItem;
	stream* Stream = GetStream(Path,1);
	if (Stream)
	{
		int Result = Stream->EnumDir(Stream,Path,Exts,1,&DirItem);
		
		while (Result == ERR_NONE)
		{
			tchar_t FullPath[MAXPATH];
			AbsPath(FullPath,TSIZEOF(FullPath),DirItem.FileName,Path);

			if (DirItem.Size < 0)
				EnumDir(FullPath,Exts);
			else
			if (DirItem.Type == FTYPE_AUDIO || DirItem.Type == FTYPE_VIDEO)
			{
				int n;
				node* Player = Context()->Player;
				Player->Get(Player,PLAYER_LIST_COUNT,&n,sizeof(n));
				Player->Set(Player,PLAYER_LIST_URL+n,FullPath,sizeof(FullPath));
			}

			Result = Stream->EnumDir(Stream,NULL,NULL,0,&DirItem);
		}

		NodeDelete((node*)Stream);
	}
}

int SilentError(void* p,int Param,int Param2)
{
	DebugMessage((const tchar_t*)Param2);
	return ERR_NONE;
}

int PlayerNotify(node* Player,int Param,int Param2)
{
	if (Param == PLAYER_BENCHMARK)
	{
		node* p;
		bool_t Bool = 0;
		Player->Set(Player,PLAYER_FULLSCREEN,&Bool,sizeof(Bool));

		p = NodeCreate(BENCHRESULT_ID);
		if (p) ((win*)p)->Popup((win*)p,NULL);
		NodeDelete(p);
	}

	if (Param == PLAYER_PLAY)
		Play = Param2;

	if (Param == PLAYER_PERCENT)
	{
#ifdef TIMELIMIT
		tick_t Pos;
		if (Player->Get(Player,PLAYER_POSITION,&Pos,sizeof(Pos))==ERR_NONE && Pos>TIMELIMIT)
		{
			int a,b;
			Player->Get(Player,PLAYER_LIST_CURRENT,&a,sizeof(a));
			Player->Get(Player,PLAYER_LIST_COUNT,&b,sizeof(b));
			if (a==b-1) 
				Play = 0;
			else
				Player->Set(Player,PLAYER_NEXT,NULL,0);
		}
#endif
	}

	return ERR_NONE;
}

void Main()
{
	int Int;
	notify Notify;
	bool_t Bool;
	node* Player = Context()->Player;

	DumpOutput_Init();

	Int = 0;//NULLAUDIO_ID;
	Player->Set(Player,PLAYER_AOUTPUTID,&Int,sizeof(Int));
	//Int = DUMPVIDEO_ID;
	//Player->Set(Player,PLAYER_VOUTPUTID,&Int,sizeof(Int));

	Notify.Func = PlayerNotify;
	Notify.This = Player;
	Player->Set(Player,PLAYER_NOTIFY,&Notify,sizeof(Notify));

	// empty saved playlist
	Int = 0;
	Player->Set(Player,PLAYER_LIST_COUNT,&Int,sizeof(Int));

	// turn off repeat
	Bool = 0;
	Player->Set(Player,PLAYER_REPEAT,&Bool,sizeof(Bool));

	Context_Wnd((void*)1); //fake window handle

#if defined(TARGET_WIN32) && !defined(TARGET_WINCE) && !defined(FULLSCREEN)
	{
		rect Rect;
		Rect.x = 600;
		Rect.y = 40;
		Rect.Width = 320;
		Rect.Height = 240;
		Player->Set(Player,PLAYER_SKIN_VIEWPORT,&Rect,sizeof(Rect));
	}
#else
	Bool = 1;
	Player->Set(Player,PLAYER_FULLSCREEN,&Bool,sizeof(Bool));
#endif

#ifdef LOADALL
	{
		int ExtsLen = 1024;
		tchar_t* Exts = malloc(ExtsLen*sizeof(tchar_t));
		if (Exts)
		{
			int* i;
			array List;

			Exts[0]=0;
			NodeEnumClass(&List,LOADALL);
			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
			{
				const tchar_t* s = LangStr(*i,NODE_EXTS);
				if (s[0])
				{
					if (Exts[0]) tcscat_s(Exts,ExtsLen,T(";"));
					tcscat_s(Exts,ExtsLen,s);
				}
			}
			ArrayClear(&List);
			EnumDir(T("d:\\samples"),Exts);
			free(Exts);
		}
		Context()->Error.Func = SilentError;
	}
#else
	Player->Set(Player,PLAYER_LIST_URL+0,URL,sizeof(URL));
#endif

	Int = 0;
	Player->Set(Player,PLAYER_LIST_CURRIDX,&Int,sizeof(Int));

	((player*)Player)->Paint(Player,NULL,0,0);

#ifdef BENCHMARK
	Bool = 1;
	Context()->Advanced->Set(Context()->Advanced,ADVANCED_BENCHFROMPOS,&Bool,sizeof(Bool));

	Int = 0;
	Player->Set(Player,PLAYER_BENCHMARK,&Int,sizeof(Int));
#else
	Bool = 1;
	Player->Set(Player,PLAYER_PLAY,&Bool,sizeof(Bool));
#endif

	Play = 1;
	while (Play)
	{
#ifndef MULTITHREAD
		if (((player*)Player)->Process(Player) == ERR_BUFFER_FULL)
			ThreadSleep(GetTimeFreq()/250);
#else
		ThreadSleep(GetTimeFreq()/10);
#endif
	}

	Context_Wnd(NULL);
	DumpOutput_Done();
}

#include <windows.h>

#if !defined(TARGET_WINCE) && defined(UNICODE)
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hParent,LPSTR CmdA,int CmdShow)
{
	WCHAR Cmd[MAXCMD];
	mbstowcs(Cmd,CmdA,sizeof(Cmd)/sizeof(WCHAR)); //!!!
#else
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hParent,TCHAR* Cmd,int CmdShow)
{
#endif

#ifndef NDEBUG
	DLLTest(); // just to help debugging plugins. comment out if not needed
	//DLLTest2(); // just to help debugging plugins. comment out if not needed
#endif

	if (Context_Init("sample","sample",3,Cmd,NULL))
	{
		Main();
		Context_Done();
	}
}

