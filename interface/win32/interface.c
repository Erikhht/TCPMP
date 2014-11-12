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
 * $Id: interface.c 624 2006-02-01 17:48:42Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#include "../win.h"
#include "interface.h"
#include "resource.h"
#include "openfile_win32.h"
#include "../about.h"
#include "../benchresult.h"
#include "../mediainfo.h"
#include "../settings.h"
#include "playlst.h"
#include "../skin.h"
#include "../../config.h"

#define CORETHEQUE_UI_ID			FOURCC('C','T','Q','U')

#if defined(TARGET_WINCE) || defined(TARGET_WIN32)

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#if _MSC_VER > 1000
#pragma warning(disable : 4201)
#endif
#include <commctrl.h>

#define REG_INITING		0x2400

#define TIMER_CLIPPING	500
#define TIMER_INITING	501
#define TIMER_SLEEP		502
#define TIMER_KEYINSEEK	503
#define TIMER_SLIDERINSEEK 504
#define TIMER_SLIDERINVOL 505
#define TIMER_CLIPPING2	506
#define TIMER_TITLESCROLL 507

#define SLIDERINSEEK_CYCLE 250
#define SLIDERINVOL_CYCLE 100
#define KEYINSEEK_START 1000
#define KEYINSEEK_REPEAT 500
#define CLIPPING_CYCLE	200
#define INITING_CYCLE	3000
#define SLEEP_CYCLE		5000
#define CLIPPING2_CYCLE	200
#define TITLESCROLL_CYCLE 150
#define TITLESCROLL_WAIT (4000/TITLESCROLL_CYCLE)

#define TRACKMAX		30000
#define TRACKHEIGHT		16
#define TRACKTHUMB		12

#define TITLEHEIGHT		16
#define TITLEFONT		11

#define VOLMINWIDTH		35
#define VOLTHUMB		12

#define IF_AUDIO		50000
#define IF_VIDEO		51000
#define IF_VIDEOACCEL	52000
#define IF_CHAPTER		53000
#define IF_STREAM_AUDIO	54000
#define IF_STREAM_VIDEO	55000
#define IF_STREAM_SUBTITLE 56000

static int HotKey[] = 
{
	IF_FILE_OPENFILE,
	IF_FILE_PLAYLIST,
	IF_PLAY,
	IF_PLAYPAUSE,
	IF_PLAY_FULLSCREEN,
	IF_STOP,
	IF_MOVE_FFWD,
	IF_MOVE_BACK,
	IF_NEXT,			
	IF_PREV,
	IF_PREV_SMART,
	IF_NEXT2,			
	IF_PREV_SMART2,
	IF_FASTFORWARD,
	IF_OPTIONS_VOLUME_UP,
	IF_OPTIONS_VOLUME_DOWN,
	IF_OPTIONS_VOLUME_UP_FINE,
	IF_OPTIONS_VOLUME_DOWN_FINE,
	IF_OPTIONS_MUTE,
	IF_OPTIONS_EQUALIZER,
	IF_OPTIONS_FULLSCREEN,
	IF_OPTIONS_ZOOM_FIT,
	IF_OPTIONS_ZOOM_IN,
	IF_OPTIONS_ZOOM_OUT,
	IF_OPTIONS_ROTATE,
	IF_OPTIONS_BRIGHTNESS_UP,
	IF_OPTIONS_BRIGHTNESS_DOWN,
	IF_OPTIONS_CONTRAST_UP,
	IF_OPTIONS_CONTRAST_DOWN,
	IF_OPTIONS_SATURATION_UP,
	IF_OPTIONS_SATURATION_DOWN,
	IF_OPTIONS_TOGGLE_VIDEO,
	IF_OPTIONS_TOGGLE_AUDIO,
	//!SUBTITLE IF_OPTIONS_TOGGLE_SUBTITLE,
	IF_OPTIONS_AUDIO_STEREO_TOGGLE,
	IF_OPTIONS_SCREEN,
	IF_FILE_EXIT,
};

#define HOTKEYCOUNT (sizeof(HotKey)/sizeof(int))

typedef struct intface
{
	win Win;
	DWORD ThreadId;
	player* Player;
	node* Color;
	node* Equalizer;
	WNDPROC DefTrackProc;
	WNDPROC DefVolProc;
	HWND WndTrack;
	HWND WndTitle;
	HWND WndVol;
	HWND WndVolBack;
	int TrackThumb;
	RECT ClientRect;
	rect SkinViewport;
	rect SkinArea;
	rect Viewport;
	int InSkin;
	bool_t InVol;
	bool_t InSeek;
	bool_t Capture;
	bool_t ForceFullScreen;
	bool_t TrackBar;
	bool_t TitleBar;
	bool_t TaskBar;
	bool_t Buttons;
	bool_t Focus;
	bool_t Clipping;
	bool_t Overlap;
	bool_t VolResizeNeeded;
	bool_t VolResizeNeeded2;
	bool_t Exited;
	POINT Offset;
	bool_t Bench;
	bool_t Wait;
	bool_t CmdLineMode;
	int Vol;
	int HotKey[HOTKEYCOUNT];

	// refresh states
	int TrackSetPos;
	int TrackPos;
	bool_t Play;
	bool_t FFwd;
	bool_t FullScreen;
	bool_t Mute;

	HFONT TitleFont;
	int TitleFontSize;
	tick_t TitleTime;
	int TitleTimeWidth;
	int TitleNameWidth;
	int TitleNameSize;
	int TitleNameOffset;
	int ScrollMode;
	bool_t UpdateScroll;
	int TitleDurWidth;
	int TitleBorder;
	int TitleTop;
	int TitleWidth;
	int TitleHeight;
	HBRUSH TitleBrush;
	rgbval_t TitleFace;
	rgbval_t TitleText;
	tchar_t TitleName[256];
	tchar_t TitleDur[32];
	int MenuPreAmp;

	int MenuStreams;
	int MenuAStream;
	int MenuVStream;
	int MenuSubStream;

	array VOutput;
	array AOutput;

	bool_t AllKeys;
	bool_t AllKeysWarning;
	int SkinNo;
	skin Skin[MAX_SKIN];
	tchar_t SkinPath[MAXPATH];

} intface;

static void ResizeVolume(intface* p)
{
	if (p->WndVolBack)
	{
		RECT rv;
		RECT r;
		POINT o;
		int Width;
		int i;

		GetClientRect(p->Win.WndTB,&r);
		if (r.left<r.right)
		{
			SendMessage(p->Win.WndTB,TB_GETRECT,IF_OPTIONS_MUTE,(LPARAM)&rv);

#if defined(TARGET_WINCE)
			if (p->Win.ToolBarHeight) // decrease width by 'X' button on right
				r.right = max(r.left,r.right-(rv.right-rv.left));
#endif

			Width = r.right-rv.right;
			if (Width < 16)
				Width = 16;

			i = WinUnitToPixelX(&p->Win,50);
			if (Width > i)
				Width = i;

			o.x = o.y = 0;
			if (!p->Win.ToolBarHeight)
				ClientToScreen(p->Win.WndTB,&o);

			MoveWindow(p->WndVolBack,rv.right+o.x,o.y+1,Width,r.bottom-1,TRUE);

			GetClientRect(p->WndVolBack,&r);
			i = WinUnitToPixelX(&p->Win,2);
			MoveWindow(p->WndVol,-i,0,r.right+2*i,r.bottom,TRUE);

			SendMessage(p->WndVol, TBM_SETTHUMBLENGTH,WinUnitToPixelY(&p->Win,VOLTHUMB),0);
		}
	}
}

static bool_t Toggle(intface* p,int Param,int Force);
static void UpdateVolume(intface* p);
static void UpdateClipping(intface* p,bool_t Suggest,bool_t HotKey);

static void ShowVol(intface* p,bool_t Show)
{
	if (p->WndVolBack && !p->Win.ToolBarHeight)
	{
		if (Show)
			SetWindowPos(p->WndVolBack,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
		else
			ShowWindow(p->WndVolBack,SW_HIDE);

		if (p->VolResizeNeeded2>1)
			p->VolResizeNeeded2=1;

	}
}

static void Resize(intface* p)
{
	RECT r;
	bool_t Rotated = IsOrientationChanged();

	DEBUG_MSG(DEBUG_VIDEO,T("Resize begin"));

	if (Rotated)
	{
		p->Player->Set(p->Player,PLAYER_ROTATEBEGIN,NULL,0);
		p->Player->Set(p->Player,PLAYER_RESETVIDEO,NULL,0);
		if (!p->Win.ToolBarHeight)
		{
			p->VolResizeNeeded2 = 1; // toolbar position may change later
			if (p->WndVolBack && IsWindowVisible(p->WndVolBack))
			{
				ShowVol(p,0);
				p->VolResizeNeeded2 = 2;
			}
		}
		else
			p->VolResizeNeeded = 1;

		if (p->Win.FullScreen)
			PostMessage(p->Win.Wnd,MSG_PLAYER,PLAYER_FULLSCREEN,0);
	}

	p->Offset.x = 0;
	p->Offset.y = 0;
	ClientToScreen(p->Win.Wnd,&p->Offset);

	if (!p->Win.FullScreen)
	{
#if !defined(TARGET_WINCE) && defined(MAXIMIZE_FULLSCREEN)
		WINDOWPLACEMENT Place;
		Place.length = sizeof(Place);
		GetWindowPlacement(p->Win.Wnd,&Place);
		if (Place.showCmd != SW_MAXIMIZE)
#endif
		{
			GetClientRect(p->Win.Wnd,&r);

			if (r.right != p->ClientRect.right || r.bottom != p->ClientRect.bottom)
			{
				bool_t Skin = p->Skin[p->SkinNo].Valid;
				int TrackThumb;
				int TrackHeight = 0;
				p->TitleHeight = 0;
				p->ClientRect = r;

				r.top += p->Win.ToolBarHeight;

				if (p->WndTrack)
				{
					TrackHeight = WinUnitToPixelY(&p->Win,TRACKHEIGHT);
					r.bottom -= TrackHeight;
					MoveWindow(p->WndTrack,r.left,r.bottom,r.right,TrackHeight,TRUE);

					TrackThumb = WinUnitToPixelY(&p->Win,TRACKTHUMB);
					if (p->TrackThumb != TrackThumb)
					{
						p->TrackThumb = TrackThumb; // avoid calling this regulary because it shows the trackbar
						SendMessage(p->WndTrack, TBM_SETTHUMBLENGTH,TrackThumb,0);
					}
				}

				if (p->WndTitle)
				{
					p->TitleTimeWidth = 0;
					p->TitleFontSize = TITLEFONT;
					p->TitleFont = WinFont(&p->Win,&p->TitleFontSize,0);
					p->TitleHeight = WinUnitToPixelY(&p->Win,TITLEHEIGHT);
					p->TitleBorder = WinUnitToPixelX(&p->Win,3);
					p->TitleWidth = r.right - r.left;

					if (Skin) 
					{
						skinitem* i = &p->Skin[p->SkinNo].Item[SKIN_TITLE];
						p->TitleWidth = i->Rect.Width;
						p->TitleHeight = i->Rect.Height;
						MoveWindow(p->WndTitle,r.left + i->Rect.x,r.top + i->Rect.y,p->TitleWidth,p->TitleHeight,TRUE);
					}
					else
					{
						if (p->Win.ToolBarHeight)
						{
							r.bottom -= p->TitleHeight;
							MoveWindow(p->WndTitle,r.left,r.bottom,p->TitleWidth,p->TitleHeight,TRUE);
						}
						else
						{
							MoveWindow(p->WndTitle,r.left,r.top,p->TitleWidth,p->TitleHeight,TRUE);
							r.top += p->TitleHeight;
						}
					}

					p->TitleTop = (p->TitleHeight-WinUnitToPixelY(&p->Win,p->TitleFontSize))/2;
				}

				p->SkinArea.x = r.left;
				p->SkinArea.y = r.top;
				p->SkinArea.Width = r.right - r.left;
				p->SkinArea.Height = r.bottom - r.top;\

				if (!Skin)
					p->SkinViewport = p->SkinArea;
				else
				{
					p->SkinViewport = p->Skin[p->SkinNo].Item[SKIN_VIEWPORT].Rect;
					p->SkinViewport.x += p->SkinArea.x;
					p->SkinViewport.y += p->SkinArea.y;
				}

				if (p->Win.ToolBarHeight && !p->VolResizeNeeded)
					ResizeVolume(p);
			}

			p->Viewport = p->SkinViewport;
			p->Viewport.x += p->Offset.x;
			p->Viewport.y += p->Offset.y;
			p->Player->Set(p->Player,PLAYER_SKIN_VIEWPORT,&p->Viewport,sizeof(rect));
			p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0);

			if (p->VolResizeNeeded)
			{
				ResizeVolume(p);
				p->VolResizeNeeded = 0;
			}
		}
	}
	else
	{
		GetClientRect(p->Win.Wnd,&r);
		p->Viewport.x = r.left + p->Offset.x;
		p->Viewport.y = r.top + p->Offset.y;
		p->Viewport.Width = r.right - r.left;
		p->Viewport.Height = r.bottom - r.top;
	}

	if (Rotated)
		p->Player->Set(p->Player,PLAYER_ROTATEEND,NULL,0);

	DEBUG_MSG4(DEBUG_VIDEO,T("Resize end %d,%d,%d,%d"),p->Viewport.x,p->Viewport.y,p->Viewport.Width,p->Viewport.Height);
}

static BOOL CALLBACK EnumCheck(HWND Wnd, LPARAM lParam )
{
	intface* p = (intface*) lParam;
	RECT r;

	if (Wnd == p->Win.Wnd)
		return 0;

	if (IsWindowVisible(Wnd))
	{
		GetWindowRect(Wnd,&r);

		if (p->Viewport.x < r.right &&
			p->Viewport.x + p->Viewport.Width > r.left &&
			p->Viewport.y < r.bottom &&
			p->Viewport.y + p->Viewport.Height > r.top)
		{
			p->Overlap = 1;
			return 0;
		}
	}
	return 1;
}

static void UpdateHotKey(intface* p,bool_t State,bool_t NoKeep)
{
	int i;
	if (!p->AllKeys)
	{
		for (i=0;i<HOTKEYCOUNT;++i)
			if (p->HotKey[i])
			{
				if (State)
				{
					if (!WinEssentialKey(p->HotKey[i]))
						WinRegisterHotKey(&p->Win,HotKey[i],p->HotKey[i]);
				}
				else
				if (NoKeep || !(p->HotKey[i] & HOTKEY_KEEP))
					WinUnRegisterHotKey(&p->Win,HotKey[i]);
				else
				if (WinEssentialKey(p->HotKey[i])) //essential keys -> register in the background
					WinRegisterHotKey(&p->Win,HotKey[i],p->HotKey[i]);
			}
	}
	else
	{
		WinAllKeys(State);
		if (!State && !NoKeep)
			for (i=0;i<HOTKEYCOUNT;++i)
				if (p->HotKey[i] & HOTKEY_KEEP)
					WinRegisterHotKey(&p->Win,HotKey[i],p->HotKey[i]);
	}
}

static void UpdateSleepTimer(intface* p)
{
#if defined(TARGET_WINCE)
	if (p->Play>0 || p->FFwd>0)
	{
		SleepTimerReset();
		SetTimer(p->Win.Wnd,TIMER_SLEEP,SLEEP_CYCLE,NULL);
	}
	else
		KillTimer(p->Win.Wnd,TIMER_SLEEP);
#endif
}

static void UpdateTitleScroll(intface* p)
{
	if (p->Focus && p->TitleNameSize < p->TitleNameWidth)
	{
		if (p->TitleNameOffset >= p->TitleNameWidth - p->TitleNameSize)
			p->TitleNameOffset = p->TitleNameWidth - p->TitleNameSize;
		SetTimer(p->Win.Wnd,TIMER_TITLESCROLL,TITLESCROLL_CYCLE,NULL);
	}
	else
	{
		p->TitleNameOffset = 0;
		p->ScrollMode = 0;
		KillTimer(p->Win.Wnd,TIMER_TITLESCROLL);
	}
}

static void TitleScroll(intface* p)
{
	if (p->ScrollMode<TITLESCROLL_WAIT)
		++p->ScrollMode;
	else
	if (p->ScrollMode==TITLESCROLL_WAIT)
	{
		p->TitleNameOffset += 3;
		if (p->TitleNameOffset >= p->TitleNameWidth - p->TitleNameSize)
		{
			p->TitleNameOffset = p->TitleNameWidth - p->TitleNameSize;
			++p->ScrollMode;
		}
		InvalidateRect(p->WndTitle,NULL,1); 
	}
	else
	if (p->ScrollMode<TITLESCROLL_WAIT*2+1)
		++p->ScrollMode;
	else
	{
		p->TitleNameOffset -= 3;
		if (p->TitleNameOffset<=0)
		{
			p->TitleNameOffset=0;
			p->ScrollMode = 0;
		}
		InvalidateRect(p->WndTitle,NULL,1); 
	}

	UpdateTitleScroll(p);
}

static bool_t ShowVideo(intface* p)
{
	bool_t b = p->Focus;
	if (!b)
		p->Player->Get(p->Player,PLAYER_SHOWINBACKGROUND,&b,sizeof(b));
	return b;
}

static void UpdateClippingTimer(intface* p)
{
	if (p->Clipping && ShowVideo(p))
		SetTimer(p->Win.Wnd,TIMER_CLIPPING,CLIPPING_CYCLE,NULL);
	else
		KillTimer(p->Win.Wnd,TIMER_CLIPPING);
}

static bool_t IsOverlay(intface* p)
{
	bool_t b = 0;
	p->Player->Get(p->Player,PLAYER_VIDEO_OVERLAY,&b,sizeof(b));
	return b;
}

static bool_t IsOverlapped(intface* p)
{
	if (!p->Win.FullScreen || p->Wait)
	{
		// check for overlapping top-level windows
		p->Overlap = 0;
		EnumWindows(EnumCheck,(LPARAM)p);
		if (p->Overlap)
			return 1;
	}
	return 0;
}

static void UpdateClipping(intface* p,bool_t Suggest,bool_t HotKey)
{
	if (p->Clipping != Suggest)
	{
		KillTimer(p->Win.Wnd,TIMER_CLIPPING2);

		if (!Suggest && IsOverlapped(p))
			return; // overlapping window -> clipping still needed

		p->Clipping = Suggest;
		p->Player->Set(p->Player,PLAYER_CLIPPING,&p->Clipping,sizeof(bool_t));

		if (HotKey)
			UpdateHotKey(p,!p->Clipping,0);

		UpdateClippingTimer(p);
	}
}

static void RefreshButton(intface* p,int No,bool_t* State,int Id,int Off,int On)
{
	int i;
	bool_t b;

	if (p->Player->Get(p->Player,No,&b,sizeof(b)) != ERR_NONE)
		b = -1;

	switch (No)
	{
	case PLAYER_PLAY: i = SKIN_PLAY; break;
	case PLAYER_FFWD: i = SKIN_FFWD; break;
	case PLAYER_MUTE: i = SKIN_MUTE; break;
	case PLAYER_FULLSCREEN: i = SKIN_FULLSCREEN; break;
	case PLAYER_REPEAT: i = SKIN_REPEAT; break;
	default: i = -1; break;
	}

	if (i>=0 && p->Skin[p->SkinNo].Item[i].Pushed != b)
	{
		p->Skin[p->SkinNo].Item[i].Pushed = b;
		SkinDrawItem(p->Skin+p->SkinNo,p->Win.Wnd,i,&p->SkinArea);
	}

	if (State && b != *State)
	{
		if (b == -1 || *State == -1)
			SendMessage(p->Win.WndTB,TB_ENABLEBUTTON,Id,MAKELPARAM(b!=-1,0));

		*State = b;

		if (b != -1)
		{
			SendMessage(p->Win.WndTB,TB_CHECKBUTTON,Id,MAKELPARAM(b,0));

			if (On>=0)
				SendMessage(p->Win.WndTB,TB_CHANGEBITMAP,Id,MAKELPARAM(p->Win.BitmapNo+(b?On:Off),0));
		}
	}
}

static void SetTrackThumb(HWND Wnd, bool_t State)
{
	int Style = GetWindowLong(Wnd,GWL_STYLE);

	if ((Style & TBS_NOTHUMB) != (State?0:TBS_NOTHUMB))
	{
		Style ^= TBS_NOTHUMB;
		SetWindowLong(Wnd,GWL_STYLE,Style);
	}
}

static void UpdateTitleTime(intface* p)
{
	if (p->WndTitle)
	{
		tick_t Time;
		if (p->Player->Get(p->Player,PLAYER_POSITION,&Time,sizeof(tick_t)) == ERR_NONE)
		{
			Time = (Time / TICKSPERSEC) * TICKSPERSEC;
			if (Time<0)
				Time=0;
		}
		else
			Time = -1;
	
		if (Time != p->TitleTime)
		{
			RECT r;

			r.left = p->TitleWidth - p->TitleTimeWidth - p->TitleDurWidth - p->TitleBorder;
			r.right = r.left + p->TitleTimeWidth;
			r.top = 0;
			r.bottom = p->TitleHeight;

			p->TitleTime = Time;
			InvalidateRect(p->WndTitle,&r,0);
		}
	}
}

static void UpdateMenuBool(intface* p,int MenuId,int Param)
{
	bool_t b = 0;
	WinMenuEnable(&p->Win,1,MenuId,p->Player->Get(p->Player,Param,&b,sizeof(b)) != ERR_NOT_SUPPORTED);
	WinMenuCheck(&p->Win,1,MenuId,b);
}

static void UpdateMenuInt(intface* p,int MenuId,int Param,int Value)
{
	int v = 0;
	WinMenuEnable(&p->Win,1,MenuId,p->Player->Get(p->Player,Param,&v,sizeof(v)) != ERR_NOT_SUPPORTED);
	WinMenuCheck(&p->Win,1,MenuId,v==Value);
}

static void UpdateMenuFrac(intface* p,int MenuId,int Param,int Num,int Den)
{
	fraction f = {0,0};
	WinMenuEnable(&p->Win,1,MenuId,p->Player->Get(p->Player,Param,&f,sizeof(f)) != ERR_NOT_SUPPORTED);
	WinMenuCheck(&p->Win,1,MenuId,f.Num*Den==f.Den*Num);
}

static void UpdateMenu(intface* p)
{
	packetformat PacketFormat;
	node* AOutput;
	bool_t Stereo;
	bool_t b;
	int* i;
	int No,Id;
	int PreAmp;
	bool_t Accel;
	tchar_t Name[20+1];
	int ACurrent = -1;
	int VCurrent = -1;
	int SubCurrent = -1;

	// remove old chapters
	if (!WinMenuEnable(&p->Win,0,IF_FILE_CHAPTERS_NONE,0))
	{
		WinMenuInsert(&p->Win,0,IF_CHAPTER+1,IF_FILE_CHAPTERS_NONE,LangStr(INTERFACE_ID,IF_FILE_CHAPTERS_NONE));

		for (No=1;No<MAXCHAPTER;++No)
			if (!WinMenuDelete(&p->Win,0,IF_CHAPTER+No))
				break;
	}

	// add new chapters
	for (No=1;No<MAXCHAPTER && PlayerGetChapter(p->Player,No,Name,TSIZEOF(Name))>=0;++No)
		WinMenuInsert(&p->Win,0,IF_FILE_CHAPTERS_NONE,IF_CHAPTER+No,Name);

	if (No>1)
		WinMenuDelete(&p->Win,0,IF_FILE_CHAPTERS_NONE);
	else
		WinMenuEnable(&p->Win,0,IF_FILE_CHAPTERS_NONE,0);

	// remove old streams
	if (p->MenuVStream >= 0)
	{
		WinMenuInsert(&p->Win,1,p->MenuVStream,IF_OPTIONS_VIDEO_STREAM_NONE, LangStr(INTERFACE_ID,IF_OPTIONS_VIDEO_STREAM_NONE));

		for (No=0;No<p->MenuStreams;++No)
			WinMenuDelete(&p->Win,1,IF_STREAM_VIDEO+No);
	}
	if (p->MenuAStream >= 0)
	{
		WinMenuInsert(&p->Win,1,p->MenuAStream,IF_OPTIONS_AUDIO_STREAM_NONE, LangStr(INTERFACE_ID,IF_OPTIONS_AUDIO_STREAM_NONE));

		for (No=0;No<p->MenuStreams;++No)
			WinMenuDelete(&p->Win,1,IF_STREAM_AUDIO+No);
	}

	if (p->MenuSubStream >= 0)
		for (No=0;No<p->MenuStreams;++No)
			WinMenuDelete(&p->Win,1,IF_STREAM_SUBTITLE+No);

	// add new streams
	p->MenuStreams = 0;
	p->MenuAStream = -1;
	p->MenuVStream = -1;
	p->MenuSubStream = -1;

	for (No=0;PlayerGetStream(p->Player,No,&PacketFormat,Name,TSIZEOF(Name),NULL);++No)
		switch (PacketFormat.Type)
		{
		case PACKET_VIDEO:
			p->MenuVStream = IF_STREAM_VIDEO+No;
			WinMenuInsert(&p->Win,1,IF_OPTIONS_VIDEO_STREAM_NONE,IF_STREAM_VIDEO+No,Name);
			break;
		case PACKET_AUDIO:
			p->MenuAStream = IF_STREAM_AUDIO+No;
			WinMenuInsert(&p->Win,1,IF_OPTIONS_AUDIO_STREAM_NONE,IF_STREAM_AUDIO+No,Name);
			break;
		case PACKET_SUBTITLE:
			p->MenuSubStream = IF_STREAM_SUBTITLE+No;
			WinMenuInsert(&p->Win,1,IF_OPTIONS_SUBTITLE_STREAM_NONE,IF_STREAM_SUBTITLE+No,Name);
			break;
		}

	p->MenuStreams = No;

	p->Player->Get(p->Player,PLAYER_VSTREAM,&VCurrent,sizeof(int));
	p->Player->Get(p->Player,PLAYER_ASTREAM,&ACurrent,sizeof(int));
	p->Player->Get(p->Player,PLAYER_SUBSTREAM,&SubCurrent,sizeof(int));

	if (p->MenuVStream<0)
		WinMenuEnable(&p->Win,1,IF_OPTIONS_VIDEO_STREAM_NONE,0);
	else
	{
		WinMenuCheck(&p->Win,1,IF_STREAM_VIDEO+VCurrent,1);
		WinMenuDelete(&p->Win,1,IF_OPTIONS_VIDEO_STREAM_NONE);
	}

	if (p->MenuAStream<0)
		WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STREAM_NONE,0);
	else
	{
		WinMenuCheck(&p->Win,1,IF_STREAM_AUDIO+ACurrent,1);
		WinMenuDelete(&p->Win,1,IF_OPTIONS_AUDIO_STREAM_NONE);
	}

	if (SubCurrent >= MAXSTREAM) 
		SubCurrent = -1;

	WinMenuEnable(&p->Win,1,IF_OPTIONS_SUBTITLE_STREAM_NONE,p->MenuSubStream>=0);
	WinMenuCheck(&p->Win,1,IF_OPTIONS_SUBTITLE_STREAM_NONE,SubCurrent<0);
	if (SubCurrent>=0)
		WinMenuCheck(&p->Win,1,IF_STREAM_SUBTITLE+SubCurrent,1);

	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_AUTO,PLAYER_ASPECT,0,1);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_SQUARE,PLAYER_ASPECT,1,1);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_4_3_SCREEN,PLAYER_ASPECT,-4,3);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_4_3_NTSC,PLAYER_ASPECT,10,11);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_4_3_PAL,PLAYER_ASPECT,12,11);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_16_9_SCREEN,PLAYER_ASPECT,-16,9);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_16_9_NTSC,PLAYER_ASPECT,40,33);
	UpdateMenuFrac(p,IF_OPTIONS_ASPECT_16_9_PAL,PLAYER_ASPECT,16,11);

	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_FIT_SCREEN,PLAYER_FULL_ZOOM,0,1);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_FIT_110,PLAYER_FULL_ZOOM,-11,10);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_FIT_120,PLAYER_FULL_ZOOM,-12,10);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_FIT_130,PLAYER_FULL_ZOOM,-13,10);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_FILL_SCREEN,PLAYER_FULL_ZOOM,-4,SCALE_ONE);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_STRETCH_SCREEN,PLAYER_FULL_ZOOM,-3,SCALE_ONE);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_50,PLAYER_FULL_ZOOM,1,2);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_100,PLAYER_FULL_ZOOM,1,1);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_150,PLAYER_FULL_ZOOM,3,2);
	UpdateMenuFrac(p,IF_OPTIONS_ZOOM_200,PLAYER_FULL_ZOOM,2,1);

	UpdateMenuBool(p,IF_OPTIONS_VIDEO_ZOOM_SMOOTH50,PLAYER_SMOOTH50);
	UpdateMenuBool(p,IF_OPTIONS_VIDEO_ZOOM_SMOOTHALWAYS,PLAYER_SMOOTHALWAYS);

	WinMenuCheck(&p->Win,1,IF_OPTIONS_VIEW_TITLEBAR,p->TitleBar);
	WinMenuCheck(&p->Win,1,IF_OPTIONS_VIEW_TRACKBAR,p->TrackBar);
	WinMenuCheck(&p->Win,1,IF_OPTIONS_VIEW_TASKBAR,p->TaskBar);

	//UpdateMenuInt(p,IF_OPTIONS_AUDIO_QUALITY_LOW,PLAYER_AUDIO_QUALITY,0);
	//UpdateMenuInt(p,IF_OPTIONS_AUDIO_QUALITY_MEDIUM,PLAYER_AUDIO_QUALITY,1);
	//UpdateMenuInt(p,IF_OPTIONS_AUDIO_QUALITY_HIGH,PLAYER_AUDIO_QUALITY,2);

	if (p->Player->Get(p->Player,PLAYER_PREAMP,&PreAmp,sizeof(PreAmp)) != ERR_NONE)
		PreAmp = 0;
	if (PreAmp != p->MenuPreAmp)
	{
		p->MenuPreAmp = PreAmp;
		stprintf_s(Name,TSIZEOF(Name),LangStr(INTERFACE_ID,IF_OPTIONS_AUDIO_PREAMP),PreAmp);
		WinMenuDelete(&p->Win,1,IF_OPTIONS_AUDIO_PREAMP);
		WinMenuInsert(&p->Win,1,IF_OPTIONS_AUDIO_PREAMP_INC,IF_OPTIONS_AUDIO_PREAMP,Name);
	}

	UpdateMenuInt(p,IF_OPTIONS_VIDEO_QUALITY_LOWEST,PLAYER_VIDEO_QUALITY,0);
	UpdateMenuInt(p,IF_OPTIONS_VIDEO_QUALITY_LOW,PLAYER_VIDEO_QUALITY,1);
	UpdateMenuInt(p,IF_OPTIONS_VIDEO_QUALITY_NORMAL,PLAYER_VIDEO_QUALITY,2);

	UpdateMenuInt(p,IF_OPTIONS_ROTATE_GUI,PLAYER_FULL_DIR,-1);
	UpdateMenuInt(p,IF_OPTIONS_ROTATE_0,PLAYER_FULL_DIR,0);
	UpdateMenuInt(p,IF_OPTIONS_ROTATE_90,PLAYER_FULL_DIR,DIR_SWAPXY | DIR_MIRRORLEFTRIGHT);
	UpdateMenuInt(p,IF_OPTIONS_ROTATE_270,PLAYER_FULL_DIR,DIR_SWAPXY | DIR_MIRRORUPDOWN);
	UpdateMenuInt(p,IF_OPTIONS_ROTATE_180,PLAYER_FULL_DIR,DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT);

	UpdateMenuFrac(p,IF_OPTIONS_SPEED_10,PLAYER_PLAY_SPEED,1,10);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_25,PLAYER_PLAY_SPEED,1,4);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_50,PLAYER_PLAY_SPEED,1,2);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_80,PLAYER_PLAY_SPEED,8,10);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_90,PLAYER_PLAY_SPEED,9,10);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_100,PLAYER_PLAY_SPEED,1,1);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_110,PLAYER_PLAY_SPEED,11,10);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_120,PLAYER_PLAY_SPEED,12,10);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_150,PLAYER_PLAY_SPEED,3,2);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_200,PLAYER_PLAY_SPEED,2,1);
	UpdateMenuFrac(p,IF_OPTIONS_SPEED_BENCHMARK,PLAYER_PLAY_SPEED,0,1);

	UpdateMenuBool(p,IF_PLAY,PLAYER_PLAY);

	WinMenuEnable(&p->Win,1,IF_OPTIONS_VIDEO_DITHER,p->Color->Get(p->Color,COLOR_DITHER,&b,sizeof(b)) != ERR_NOT_SUPPORTED);
	WinMenuCheck(&p->Win,1,IF_OPTIONS_VIDEO_DITHER,b);
	
	UpdateMenuInt(p,IF_OPTIONS_AUDIO_STEREO,PLAYER_STEREO,STEREO_NORMAL);
	UpdateMenuInt(p,IF_OPTIONS_AUDIO_STEREO_SWAPPED,PLAYER_STEREO,STEREO_SWAPPED);
	UpdateMenuInt(p,IF_OPTIONS_AUDIO_STEREO_JOINED,PLAYER_STEREO,STEREO_JOINED);
	UpdateMenuInt(p,IF_OPTIONS_AUDIO_STEREO_LEFT,PLAYER_STEREO,STEREO_LEFT);
	UpdateMenuInt(p,IF_OPTIONS_AUDIO_STEREO_RIGHT,PLAYER_STEREO,STEREO_RIGHT);

	UpdateMenuBool(p,IF_OPTIONS_FULLSCREEN,PLAYER_FULLSCREEN);
	UpdateMenuBool(p,IF_OPTIONS_REPEAT,PLAYER_REPEAT);
	UpdateMenuBool(p,IF_OPTIONS_SHUFFLE,PLAYER_SHUFFLE);

	p->Equalizer->Get(p->Equalizer,EQUALIZER_ENABLED,&b,sizeof(b));
	WinMenuCheck(&p->Win,1,IF_OPTIONS_EQUALIZER,b);

	p->Player->Get(p->Player,PLAYER_VIDEO_ACCEL,&Accel,sizeof(Accel));
	p->Player->Get(p->Player,PLAYER_VOUTPUTID,&Id,sizeof(Id));

	WinMenuCheck(&p->Win,1,IF_OPTIONS_VIDEO_TURNOFF,!Id);

	for (No=0,i=ARRAYBEGIN(p->VOutput,int);i!=ARRAYEND(p->VOutput,int);++i,++No)
	{
		WinMenuCheck(&p->Win,1,IF_VIDEO+No,*i==Id && !Accel);
		WinMenuCheck(&p->Win,1,IF_VIDEOACCEL+No,*i==Id && Accel);
	}

	p->Player->Get(p->Player,PLAYER_AOUTPUTID,&Id,sizeof(Id));
	WinMenuCheck(&p->Win,1,IF_OPTIONS_AUDIO_TURNOFF,!Id);

	for (No=0,i=ARRAYBEGIN(p->AOutput,int);i!=ARRAYEND(p->AOutput,int);++i,++No)
		WinMenuCheck(&p->Win,1,IF_AUDIO+No,*i==Id);

	Stereo = 1;
	p->Player->Get(p->Player,PLAYER_AOUTPUT,&AOutput,sizeof(AOutput));
	if (AOutput && AOutput->Get(AOutput,OUT_INPUT|PIN_FORMAT,&PacketFormat,sizeof(PacketFormat))==ERR_NONE && 
		PacketFormat.Type == PACKET_AUDIO && PacketFormat.Format.Audio.Channels<2)
		Stereo = 0;

	WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STEREO,Stereo);
	WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STEREO_SWAPPED,Stereo);
	WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STEREO_JOINED,Stereo);
	WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STEREO_LEFT,Stereo);
	WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STEREO_RIGHT,Stereo);
}

static void UpdatePosition(intface* p);

static int Delta(intface* p,node* Node,int Param,int d,int Min,int Max)
{
	int State;
	if (Node && Node->Get(Node,Param,&State,sizeof(State))==ERR_NONE)
	{
		State += d;
		if (State < Min)
			State = Min;
		if (State > Max)
			State = Max;

		Node->Set(Node,Param,&State,sizeof(State));

		if (Param == PLAYER_VOLUME)
		{
			if (State > Min) Toggle(p,PLAYER_MUTE,0);
			UpdateVolume(p);
		}
	}
	return State;
}

static bool_t IsCoverAtr(player *Player)
{
	node* Format;
	int CoverArt;
	int VStream;
	return Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format))==ERR_NONE && Format &&
		Format->Get(Format,FORMAT_COVERART,&CoverArt,sizeof(CoverArt))==ERR_NONE &&
		Player->Get(Player,PLAYER_VSTREAM,&VStream,sizeof(VStream))==ERR_NONE && VStream==CoverArt;
}

static bool_t ToggleFullScreen(intface* p,int Force,int CoverArtFullScreen)
{
	bool_t State = 0;
	if (p->Player->Get(p->Player,PLAYER_FULLSCREEN,&State,sizeof(State))==ERR_NONE)
	{
		node* VOutput;
		bool_t Primary;
		packetformat Format;

		if (State==Force)
			return State;
		State = !State;

		if (p->Player->Get(p->Player,PLAYER_VOUTPUT,&VOutput,sizeof(VOutput))!=ERR_NONE ||
			!VOutput ||
			VOutput->Get(VOutput,VOUT_PRIMARY,&Primary,sizeof(Primary))!=ERR_NONE || !Primary ||
			(!CoverArtFullScreen && IsCoverAtr(p->Player)) ||
			(!p->ForceFullScreen && VOutput->Get(VOutput,OUT_INPUT|PIN_FORMAT,&Format,sizeof(Format))!=ERR_NONE) ||
			(!p->ForceFullScreen && !Format.Format.Video.Width))
			State = 0;

		UpdatePosition(p); // before returning from fullscreen

		if (!State)
		{
			p->Player->Set(p->Player,PLAYER_FULLSCREEN,&State,sizeof(State));
			p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0);
			RefreshButton(p,PLAYER_FULLSCREEN,&p->FullScreen,IF_OPTIONS_FULLSCREEN,-1,-1);
		}

		if (p->Skin[p->SkinNo].Valid) p->Skin[p->SkinNo].Hidden = State;
		if (p->WndTrack) ShowWindow(p->WndTrack,State ? SW_HIDE:SW_SHOWNA);
		if (p->WndTitle) ShowWindow(p->WndTitle,State ? SW_HIDE:SW_SHOWNA);
		WinSetFullScreen(&p->Win,State);
		if (p->Win.WndTB) UpdateWindow(p->Win.WndTB);
		ShowVol(p,!State); // called after WinSetFullScreen so TOPMOST is over taskbar

		if (State)
		{
			p->Player->Set(p->Player,PLAYER_FULLSCREEN,&State,sizeof(State));
			p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0);
			RefreshButton(p,PLAYER_FULLSCREEN,&p->FullScreen,IF_OPTIONS_FULLSCREEN,-1,-1);
		}
	}
	return State;
}

static bool_t Toggle(intface* p,int Param,int Force)
{
	bool_t State = 0;
	if (p->Player->Get(p->Player,Param,&State,sizeof(State))==ERR_NONE)
	{
		assert(Param != PLAYER_FULLSCREEN);
		if (State==Force)
			return State;
		State = !State;
		p->Player->Set(p->Player,Param,&State,sizeof(State));
		if (Param == PLAYER_PLAY)
			RefreshButton(p,PLAYER_PLAY,&p->Play,IF_PLAY,0,1);
		else
		if (Param == PLAYER_FFWD)
			RefreshButton(p,PLAYER_FFWD,&p->FFwd,IF_FASTFORWARD,4,1);
		else
			RefreshButton(p,Param,NULL,0,0,0);
	}
	return State;
}

static bool_t IsAutoRun(intface* p, const tchar_t* CmdLine)
{
	while (IsSpace(*CmdLine))
		++CmdLine;

	return tcsnicmp(CmdLine,T("-autorun"),8)==0;
}

static void EnumDir(const tchar_t* Path,const tchar_t* Exts,player* Player)
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
				EnumDir(FullPath,Exts,Player);
			else
			if (DirItem.Type == FTYPE_AUDIO || DirItem.Type == FTYPE_VIDEO)
			{
				int n;
				Player->Get(Player,PLAYER_LIST_COUNT,&n,sizeof(n));
				Player->Set(Player,PLAYER_LIST_URL+n,FullPath,sizeof(FullPath));
			}

			Result = Stream->EnumDir(Stream,NULL,NULL,0,&DirItem);
		}

		NodeDelete((node*)Stream);
	}
}

void ProcessCmdLine( intface* p, const tchar_t* CmdLine)
{
	int n;
	tchar_t URL[MAXPATH];

	if (IsAutoRun(p,CmdLine))
	{
		// search for media files on SD card
		tchar_t Base[MAXPATH];
		int* i;
		array List;
		tchar_t* s;
		int ExtsLen = 1024;
		tchar_t* Exts = malloc(ExtsLen*sizeof(tchar_t));
		if (!Exts)
			return;

		Exts[0]=0;
		NodeEnumClass(&List,FORMAT_CLASS);
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

		n = 0;
		p->Player->Set(p->Player,PLAYER_LIST_COUNT,&n,sizeof(n));

		GetModuleFileName(GetModuleHandle(NULL),Base,MAXPATH);
		s = tcsrchr(Base,'\\');
		if (s) *s=0;
		s = tcsrchr(Base,'\\');
		if (s) *s=0;

		EnumDir(Base,Exts,p->Player);
		free(Exts);
	}
	else
	{
		while (IsSpace(*CmdLine))
			++CmdLine;

		if (*CmdLine == '"')
		{
			tchar_t* ch = tcschr(++CmdLine,'"');
			if (ch) *ch=0;
		}

		tcscpy_s(URL,TSIZEOF(URL),CmdLine);

		p->CmdLineMode = 1;

		n = 1;
		p->Player->Set(p->Player,PLAYER_LIST_COUNT,&n,sizeof(n));

		PlayerAdd(p->Player,0,URL,NULL);
	}

	n = 0;
	p->Player->Set(p->Player,PLAYER_LIST_CURRIDX,&n,sizeof(n));

	if (!IsWindowEnabled(p->Win.Wnd))
		Toggle(p,PLAYER_PLAY,0);
}

void SetTrackPos(intface* p,int x)
{
	fraction Percent;
	RECT r,rt;
	int Adj;

	if (p->TrackSetPos == x)
		return;

	p->TrackSetPos = x;
	SendMessage(p->WndTrack,TBM_GETCHANNELRECT,0,(LONG)&r);
	SendMessage(p->WndTrack,TBM_GETTHUMBRECT,0,(LONG)&rt);
	Adj = (rt.right-rt.left) >> 1;
	r.left += Adj;
	r.right -= (rt.right-rt.left)-Adj;

	if (x > 32768) x -= 65536;
	if (r.right <= r.left) r.right = r.left+1;
	if (x < r.left) x = r.left;
	if (x > r.right) x = r.right;
	x = Scale(TRACKMAX,x-r.left,r.right-r.left);

	Percent.Num = x;
	Percent.Den = TRACKMAX;
	p->Player->Set(p->Player,PLAYER_PERCENT,&Percent,sizeof(Percent));
}

LRESULT CALLBACK TitleProc(HWND WndTitle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	intface* p = (intface*) WinGetObject(WndTitle);
	if (p)
	{
		PAINTSTRUCT Paint;
		RECT r;
		tchar_t Time[32];

		switch (Msg)
		{
		case WM_ERASEBKGND:
			GetClientRect(WndTitle,&r);
			FillRect((HDC)wParam,&r,p->TitleBrush ? p->TitleBrush:GetSysColorBrush(COLOR_BTNFACE));
			break;

		case WM_SIZE:
			p->UpdateScroll = 1;
			break;

		case WM_LBUTTONDOWN:
			break;

		case WM_MOUSEMOVE:
			break;

		case WM_LBUTTONUP:
			break;

		case WM_PAINT:
			BeginPaint(WndTitle,&Paint);

			SelectObject(Paint.hdc,p->TitleFont);
			SetBkColor(Paint.hdc,p->TitleFace);
			SetTextColor(Paint.hdc,p->TitleText);
#if !defined(TARGET_WINCE)
			SetTextAlign(Paint.hdc,TA_LEFT);
#endif

			if (!p->TitleTimeWidth)
			{
				SIZE Size;
				tchar_t* s;
				tchar_t Dur[32];
				tick_t Duration;
				tchar_t TitleName[256];
				node* Format;

				if (p->Player->Get(p->Player,PLAYER_FORMAT,&Format,sizeof(Format)) == ERR_NONE && !Format)
					p->TitleTime = -1;

				p->Player->Get(p->Player,PLAYER_TITLE,TitleName,sizeof(TitleName));
				if (tcscmp(p->TitleName,TitleName)!=0)
				{
					tcscpy_s(p->TitleName,TSIZEOF(p->TitleName),TitleName);
					GetTextExtentPoint(Paint.hdc,p->TitleName,tcslen(p->TitleName),&Size);
					p->TitleNameWidth = Size.cx;
					p->TitleNameOffset = 0;
					p->ScrollMode = 0;
					p->UpdateScroll = 1;
				}

				s = Dur;
				Dur[0] = 0;
				if (p->Player->Get(p->Player,PLAYER_DURATION,&Duration,sizeof(Duration)) == ERR_NONE && Duration>=0)
				{
					TickToString(Dur,TSIZEOF(Dur),Duration,0,0,1);
					if (p->TitleTime<0)
						p->TitleTime = 0;
				}
				else
					s = T("0:00:00");

				GetTextExtentPoint(Paint.hdc,s,tcslen(s),&Size);
				p->TitleTimeWidth = Size.cx;
				p->TitleDurWidth = 0;
				p->TitleDur[0] = 0;

				if (Dur[0])
				{
					stprintf_s(p->TitleDur,TSIZEOF(p->TitleDur),T("/%s"),Dur);
					GetTextExtentPoint(Paint.hdc,p->TitleDur,tcslen(p->TitleDur),&Size);
					p->TitleDurWidth = Size.cx;
					p->UpdateScroll = 1;
				}
			}

			if (p->TitleTime>=0)
				TickToString(Time,TSIZEOF(Time),p->TitleTime,0,0,1);
			else
				Time[0] = 0;

			r.top = p->TitleTop;
			r.bottom = p->TitleHeight - p->TitleTop;
			r.left = p->TitleBorder;
			r.right = p->TitleWidth - p->TitleTimeWidth - p->TitleDurWidth - p->TitleBorder * 2;
			p->TitleNameSize  = r.right - r.left;

			if (p->UpdateScroll)
			{
				p->UpdateScroll = 0;
				UpdateTitleScroll(p);
			}

			ExtTextOut(Paint.hdc,r.left-p->TitleNameOffset,r.top,ETO_CLIPPED,&r,p->TitleName,tcslen(p->TitleName),NULL);


			r.left = r.right + p->TitleBorder;
			r.right = r.left + p->TitleTimeWidth;
			//SetBkColor(Paint.hdc,0xFFFFFF);
			ExtTextOut(Paint.hdc,r.left,r.top,ETO_CLIPPED|ETO_OPAQUE,&r,Time,tcslen(Time),NULL);
			//SetBkColor(Paint.hdc,0x00FF00);

			if (p->TitleDurWidth)
			{
				r.left = r.right;
				r.right = r.left + p->TitleDurWidth;
				ExtTextOut(Paint.hdc,r.left,r.top,ETO_CLIPPED,&r,p->TitleDur,tcslen(p->TitleDur),NULL);
			}

			EndPaint(WndTitle,&Paint);
			break;
		}
	}
	return DefWindowProc(WndTitle,Msg,wParam,lParam);
}

static void SetKeyInSeek(intface* p)
{
	bool_t First = !p->InSeek;
	p->InSeek = 1;
	p->Player->Set(p->Player,PLAYER_INSEEK,&p->InSeek,sizeof(p->InSeek));
	SetTimer(p->Win.Wnd,TIMER_KEYINSEEK,First?KEYINSEEK_START:KEYINSEEK_REPEAT,NULL);
}

static void StopSeek(intface* p)
{
	p->InSeek = 0;
	p->Player->Set(p->Player,PLAYER_INSEEK,&p->InSeek,sizeof(p->InSeek));
	KillTimer(p->Win.Wnd,TIMER_KEYINSEEK);
	if (p->Capture)
	{
		ReleaseCapture();
		p->Capture = 0;
		KillTimer(p->Win.Wnd,TIMER_SLIDERINSEEK);
	}
}

LRESULT CALLBACK TrackProc(HWND WndTrack, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	intface* p = (intface*) WinGetObject(WndTrack);
	if (Msg == WM_LBUTTONDOWN)
	{
		p->InSeek = 1;
		p->Capture = 1;
		p->TrackSetPos = -1;
		p->Player->Set(p->Player,PLAYER_INSEEK,&p->InSeek,sizeof(p->InSeek));
		SetTrackPos(p,LOWORD(lParam));
		SetCapture(WndTrack);
		SetTimer(p->Win.Wnd,TIMER_SLIDERINSEEK,SLIDERINSEEK_CYCLE,NULL);
		return 0;
	}
	if (Msg == WM_MOUSEMOVE)
	{
		if (p->InSeek && (wParam & MK_LBUTTON))
			SetTrackPos(p,LOWORD(lParam));
		return 0;
	}
	if (Msg == WM_LBUTTONUP)
	{
		if (p->InSeek)
		{
			SetTrackPos(p,LOWORD(lParam));
			StopSeek(p);
		}
		return 0;
	}
	return CallWindowProc(p->DefTrackProc,WndTrack,Msg,wParam,lParam);
}

void UpdateVol(intface* p)
{
	if (p->Vol >= 0)
	{
		SendMessage(p->WndVol,TBM_SETPOS,1,p->Vol);
		p->Player->Set(p->Player,PLAYER_VOLUME,&p->Vol,sizeof(p->Vol));
		p->Vol = -1;

		if (p->Skin[p->SkinNo].Valid)
			SkinUpdate(&p->Skin[p->SkinNo],p->Player,p->Win.Wnd,&p->SkinArea);
	}
}

void SetVolPos(intface* p,int x)
{
	RECT r;
	int Adj;
	SendMessage(p->WndVol,TBM_GETCHANNELRECT,0,(LONG)&r);
	Adj = WinUnitToPixelX(&p->Win,VOLTHUMB)/4;
	r.left += Adj;
	r.right -= Adj;

	if (x > 32768) x -= 65536;
	if (r.right <= r.left) r.right = r.left+1;
	if (x < r.left) x = r.left;
	if (x > r.right) x = r.right;

	p->Vol = (100*(x-r.left))/(r.right-r.left);
}

LRESULT CALLBACK VolBackProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//DEBUG_MSG1(DEBUG_WIN,T("MSG %04x"),Msg);

	switch (Msg)
	{
#if defined(TARGET_WINCE)
	case WM_CTLCOLORSTATIC:
		return (LRESULT) GetSysColorBrush(COLOR_BTNFACE);
#endif
	}
	return DefWindowProc(Wnd,Msg,wParam,lParam);
}

LRESULT CALLBACK VolProc(HWND WndVol, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	intface* p = (intface*) WinGetObject(WndVol);

	switch (Msg)
	{
	case WM_MOUSEMOVE:
		if ((wParam & MK_LBUTTON)==0)
			break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:

		SetVolPos(p,LOWORD(lParam));
		if (!p->InVol)
		{
			p->InVol = 1;
			SetForegroundWindow(p->Win.Wnd);
			SetCapture(WndVol);
			SetTimer(p->Win.Wnd,TIMER_SLIDERINVOL,SLIDERINVOL_CYCLE,NULL);
			UpdateVol(p);
		}
		return 0;

	case WM_LBUTTONUP:
		if (!p->InVol)
			SetForegroundWindow(p->Win.Wnd);
		else
		{
			p->InVol = 0;
			ReleaseCapture();
			KillTimer(p->Win.Wnd,TIMER_SLIDERINVOL);
		}
		SetVolPos(p,LOWORD(lParam));
		UpdateVol(p);
		return 0;
	}

	return CallWindowProc(p->DefVolProc,WndVol,Msg,wParam,lParam);
}

void CreateDeviceMenu(intface* p)
{
	int No;
	int *i;

	NodeEnumClass(&p->VOutput,VOUT_CLASS);
	NodeEnumClass(&p->AOutput,AOUT_CLASS);

	for (No=0,i=ARRAYBEGIN(p->VOutput,int);i!=ARRAYEND(p->VOutput,int);++i,++No)
	{
		if (VOutIDCT(*i))
		{
			tchar_t s[256];
			stprintf_s(s,TSIZEOF(s),LangStr(INTERFACE_ID,IF_OPTIONS_VIDEO_ACCEL),LangStr(*i,NODE_NAME));
			WinMenuInsert(&p->Win,1,IF_OPTIONS_VIDEO_TURNOFF,IF_VIDEOACCEL+No,s);
		}
		WinMenuInsert(&p->Win,1,IF_OPTIONS_VIDEO_TURNOFF,IF_VIDEO+No,LangStr(*i,NODE_NAME));
	}

	for (No=0,i=ARRAYBEGIN(p->AOutput,int);i!=ARRAYEND(p->AOutput,int);++i,++No)
		WinMenuInsert(&p->Win,1,IF_OPTIONS_AUDIO_TURNOFF,IF_AUDIO+No,LangStr(*i,NODE_NAME));
}

static menudef MenuDef[] =
{
	{ 0,INTERFACE_ID, IF_FILE },
	{ 1,INTERFACE_ID, IF_FILE_OPENFILE },
	{ 1,INTERFACE_ID, IF_FILE_CORETHEQUE },
	{ 1,INTERFACE_ID, IF_FILE_PLAYLIST },
	{ 1,-1,-1 },
	{ 1,INTERFACE_ID, IF_PLAY },
	{ 1,INTERFACE_ID, IF_NEXT },
	{ 1,INTERFACE_ID, IF_PREV },
	{ 1,-1,-1 },
	{ 1,INTERFACE_ID, IF_FILE_CHAPTERS },
	{ 2,INTERFACE_ID, IF_FILE_CHAPTERS_NONE },
	{ 1,INTERFACE_ID, IF_FILE_MEDIAINFO },
	{ 1,INTERFACE_ID, IF_FILE_BENCHMARK },
	{ 1,INTERFACE_ID, IF_FILE_ABOUT },
	{ 1,-1,-1 },
	{ 1,INTERFACE_ID, IF_FILE_EXIT },
	{ 0,INTERFACE_ID, IF_OPTIONS },
	{ 1,INTERFACE_ID, IF_OPTIONS_SPEED },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_10 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_25 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_50 }, 
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_80 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_90 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_100 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_110 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_120 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_150 },
	{ 2,INTERFACE_ID, IF_OPTIONS_SPEED_200 },
	{ 1,INTERFACE_ID, IF_OPTIONS_VIEW, MENU_SMALL },	
	{ 2,INTERFACE_ID, IF_OPTIONS_ZOOM },
#if !defined(SH3)
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_FIT_SCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_FILL_SCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_STRETCH_SCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_FIT_110 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_FIT_120 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_FIT_130 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_50 }, 
#endif
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_100 },
#if !defined(SH3)
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_150 },
#endif
	{ 3,INTERFACE_ID, IF_OPTIONS_ZOOM_200 },
#if !defined(SH3)
	{ 2,INTERFACE_ID, IF_OPTIONS_ASPECT },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_AUTO },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_SQUARE },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_4_3_SCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_4_3_NTSC },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_4_3_PAL },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_16_9_SCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_16_9_NTSC },
	{ 3,INTERFACE_ID, IF_OPTIONS_ASPECT_16_9_PAL },
#endif
	{ 2,INTERFACE_ID, IF_OPTIONS_ORIENTATION },
	{ 3,INTERFACE_ID, IF_OPTIONS_ORIENTATION_FULLSCREEN },
	{ 3,INTERFACE_ID, IF_OPTIONS_ROTATE_GUI },
	{ 3,INTERFACE_ID, IF_OPTIONS_ROTATE_0 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ROTATE_90 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ROTATE_270 },
	{ 3,INTERFACE_ID, IF_OPTIONS_ROTATE_180 },
	{ 2,INTERFACE_ID, IF_OPTIONS_VIEW, MENU_NOTSMALL },	
	{ 3,INTERFACE_ID, IF_OPTIONS_VIEW_TITLEBAR },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIEW_TRACKBAR }, 
	{ 3,INTERFACE_ID, IF_OPTIONS_VIEW_TASKBAR }, 
	{ 1,INTERFACE_ID, IF_OPTIONS_VIDEO },
#if !defined(SH3) && !defined(MIPS)
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_ZOOM_SMOOTH50 },
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_ZOOM_SMOOTHALWAYS },
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_DITHER },
#endif
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_QUALITY },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIDEO_QUALITY_LOWEST },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIDEO_QUALITY_LOW },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIDEO_QUALITY_NORMAL }, 
	{ 2,-1,-1 },
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_DEVICE, MENU_SMALL|MENU_GRAYED },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIDEO_TURNOFF },
	{ 2,-1,-1 },
	{ 2,INTERFACE_ID, IF_OPTIONS_VIDEO_STREAM, MENU_SMALL|MENU_GRAYED },
	{ 3,INTERFACE_ID, IF_OPTIONS_VIDEO_STREAM_NONE },

	{ 1,INTERFACE_ID, IF_OPTIONS_AUDIO },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_CHANNELS, MENU_SMALL },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STEREO },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STEREO_SWAPPED },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STEREO_JOINED },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STEREO_LEFT },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STEREO_RIGHT },
//	{ 2,-1,-1 },
//	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_QUALITY },
//	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_QUALITY_LOW },
//	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_QUALITY_MEDIUM },
//	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_QUALITY_HIGH }, 
	{ 2,-1,-1 },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_PREAMP_DEC },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_PREAMP },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_PREAMP_INC }, 
	{ 2,-1,-1 },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_DEVICE, MENU_SMALL|MENU_GRAYED },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_TURNOFF },
	{ 2,-1,-1 },
	{ 2,INTERFACE_ID, IF_OPTIONS_AUDIO_STREAM, MENU_SMALL|MENU_GRAYED },
	{ 3,INTERFACE_ID, IF_OPTIONS_AUDIO_STREAM_NONE },

//!SUBTITLE	{ 1,INTERFACE_ID, IF_OPTIONS_SUBTITLE_STREAM, MENU_GRAYED },
//!SUBTITLE	{ 2,INTERFACE_ID, IF_OPTIONS_SUBTITLE_STREAM_NONE },

	{ 1,-1,-1 },
	{ 1,INTERFACE_ID, IF_OPTIONS_REPEAT },
	{ 1,INTERFACE_ID, IF_OPTIONS_SHUFFLE },
	{ 1,INTERFACE_ID, IF_OPTIONS_EQUALIZER },
	{ 1,INTERFACE_ID, IF_OPTIONS_FULLSCREEN },
#if defined(CONFIG_SKIN)
	{ 1,INTERFACE_ID, IF_OPTIONS_SKIN },
#endif
	{ 1,-1,-1 },
	{ 1,INTERFACE_ID, IF_OPTIONS_SETTINGS },

	MENUDEF_END
};

void CreateButtons(intface* p)
{
	if (QueryPlatform(PLATFORM_TYPENO) != TYPE_SMARTPHONE)
	{
		bool_t FullScreenButton = 1;
		RECT r,rv;

		WinBitmap(&p->Win,IDB_TOOLBAR16,IDB_TOOLBAR32,10);
		WinAddButton(&p->Win,-1,-1,NULL,0);
		WinAddButton(&p->Win,IF_PLAY,0,NULL,1);
		WinAddButton(&p->Win,IF_FASTFORWARD,4,NULL,1);
		WinAddButton(&p->Win,IF_STOP,5,NULL,0);
		WinAddButton(&p->Win,-1,-1,NULL,0);
		WinAddButton(&p->Win,IF_OPTIONS_FULLSCREEN,6,NULL,1);
		WinAddButton(&p->Win,IF_OPTIONS_MUTE,7,NULL,1);

		GetClientRect(p->Win.WndTB,&r);
		SendMessage(p->Win.WndTB,TB_GETRECT,IF_OPTIONS_MUTE,(LPARAM)&rv);
		if (r.right-rv.right < VOLMINWIDTH)
		{
			WinDeleteButton(&p->Win,IF_FASTFORWARD);
			SendMessage(p->Win.WndTB,TB_GETRECT,IF_OPTIONS_MUTE,(LPARAM)&rv);
			if (r.right-rv.right < VOLMINWIDTH)
			{
				WinDeleteButton(&p->Win,IF_OPTIONS_FULLSCREEN);
				FullScreenButton = 0;
			}
		}

		WinMenuDelete(&p->Win,0,IF_PLAY);
		if (FullScreenButton)
			WinMenuDelete(&p->Win,1,IF_OPTIONS_FULLSCREEN);
		WinMenuDelete(&p->Win,1,IF_OPTIONS_MUTE);
	}
}

static void UpdateVolume(intface* p)
{
	int x;

	if (p->Skin[p->SkinNo].Valid)
		SkinUpdate(&p->Skin[p->SkinNo],p->Player,p->Win.Wnd,&p->SkinArea);

	if (p->WndVol)
	{
		bool_t Enable = p->Player->Get(p->Player,PLAYER_VOLUME,&x,sizeof(x))==ERR_NONE;
		EnableWindow(p->WndVol,Enable);
		SetTrackThumb(p->WndVol,Enable);
		if (Enable)
			SendMessage(p->WndVol,TBM_SETPOS,1,x);
	}
	RefreshButton(p,PLAYER_MUTE,&p->Mute,IF_OPTIONS_MUTE,7,8);
}

static void CreateVolumeTrack(intface* p)
{
	if (QueryPlatform(PLATFORM_TYPENO) != TYPE_SMARTPHONE)
	{
		WNDCLASS WinClass;

		memset(&WinClass,0,sizeof(WinClass));
		WinClass.style			= CS_HREDRAW | CS_VREDRAW;
		WinClass.lpfnWndProc	= (WNDPROC) VolBackProc;
		WinClass.cbClsExtra		= 0;
		WinClass.cbWndExtra		= 0;
		WinClass.hInstance		= p->Win.Module;
		WinClass.hCursor		= WinCursorArrow();
		WinClass.hbrBackground	= GetSysColorBrush(COLOR_BTNFACE);
		WinClass.lpszMenuName	= 0;
		WinClass.lpszClassName	= T("VolumeBack");
		RegisterClass(&WinClass);

		p->WndVolBack = CreateWindowEx( 
			WS_EX_TOPMOST,                    
			WinClass.lpszClassName,       
			Context()->ProgramName,   
			(p->Win.ToolBarHeight ? WS_CHILD:WS_POPUP) | WS_VISIBLE | 
			TBS_HORZ|TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH, 
			200, 0,    
			40, 20,   
			(p->Win.ToolBarHeight ? p->Win.WndTB:p->Win.Wnd),	
			NULL,						
			p->Win.Module,                     
			NULL                      
			); 

		p->WndVol = CreateWindowEx( 
			0,                    
			TRACKBAR_CLASS,       
			NULL,   
			WS_CHILD | WS_VISIBLE | 
			TBS_HORZ|TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH, 
			0, 0,    
			40, 20,   
			p->WndVolBack,
			NULL,						
			p->Win.Module,                     
			NULL                      
			); 

		p->DefVolProc = (WNDPROC)GetWindowLong(p->WndVol,GWL_WNDPROC);
		SetWindowLong(p->WndVol,GWL_WNDPROC,(LONG)VolProc);
		WinSetObject(p->WndVol,&p->Win);

		SendMessage(p->WndVol, TBM_SETRANGE, FALSE,MAKELONG(0,100));
		SendMessage(p->WndVol, TBM_SETPAGESIZE,0,10);
		SendMessage(p->WndVol, TBM_SETLINESIZE,0,10);

		ResizeVolume(p);
		UpdateVolume(p);
	}
}

static int UpdateTitleBar(intface* p,bool_t DoResize)
{
	if (p->TitleBar && !p->WndTitle && p->Win.Wnd)
	{
		WNDCLASS WinClass;

		memset(&WinClass,0,sizeof(WinClass));
		WinClass.style			= CS_HREDRAW | CS_VREDRAW;
		WinClass.lpfnWndProc	= (WNDPROC) TitleProc;
		WinClass.cbClsExtra		= 0;
		WinClass.cbWndExtra		= 0;
		WinClass.hInstance		= p->Win.Module;
		WinClass.hCursor		= WinCursorArrow();
		WinClass.hbrBackground	= NULL;
		WinClass.lpszMenuName	= 0;
		WinClass.lpszClassName	= T("TitleBar");
		RegisterClass(&WinClass);

		p->WndTitle = CreateWindowEx( 
			0,                    
			WinClass.lpszClassName,       
			T("Time"),   
			WS_CHILD | WS_VISIBLE, 
			0, 0,    
			200, 20,   
			p->Win.Wnd,	
			NULL,						
			p->Win.Module,                     
			NULL                      
			); 

		WinSetObject(p->WndTitle,&p->Win);
		p->ClientRect.right = -1;
		if (DoResize) Resize(p);
	}
	if (!p->TitleBar && p->WndTitle)
	{
		DestroyWindow(p->WndTitle);
		p->WndTitle = NULL;
		p->ClientRect.right = -1;
		if (DoResize) Resize(p);
	}
	return ERR_NONE;
}

static NOINLINE void UpdateTrackPos(intface* p)
{
	if (p->Skin[p->SkinNo].Valid)
		SkinUpdate(p->Skin+p->SkinNo,p->Player,p->Win.Wnd,&p->SkinArea);

	if (p->WndTrack)
	{
		fraction f;
		int TrackPos;
		if (p->Player->Get(p->Player,PLAYER_PERCENT,&f,sizeof(fraction)) == ERR_NONE)
			TrackPos = Scale(TRACKMAX,f.Num,f.Den);
		else
			TrackPos = -1;

		if (TrackPos != p->TrackPos)
		{
			if (TrackPos == -1)
			{
				SetTrackThumb(p->WndTrack,0);
				EnableWindow(p->WndTrack,0);
			}
			if (p->TrackPos == -1)
			{
				SetTrackThumb(p->WndTrack,1);
				EnableWindow(p->WndTrack,1);
			}
			p->TrackPos = TrackPos;
			SendMessage(p->WndTrack,TBM_SETPOS,1,TrackPos);
		}
	}
}

static int UpdateTrackBar(intface* p,bool_t DoResize)
{
	bool_t TrackBar = p->TrackBar && !p->Skin[p->SkinNo].Valid;
	if (TrackBar && !p->WndTrack && p->Win.Wnd)
	{
		p->WndTrack = CreateWindowEx( 
			0,                    
			TRACKBAR_CLASS,       
			T("Time"),   
			WS_CHILD | WS_VISIBLE | 
			TBS_HORZ|TBS_BOTH|TBS_NOTICKS|TBS_FIXEDLENGTH, 
			0, 0,    
			200, 20,   
			p->Win.Wnd,	
			NULL,						
			p->Win.Module,                     
			NULL                      
			); 

		p->DefTrackProc = (WNDPROC)GetWindowLong(p->WndTrack,GWL_WNDPROC);
		SetWindowLong(p->WndTrack,GWL_WNDPROC,(LONG)TrackProc);
		WinSetObject(p->WndTrack,&p->Win);

		SendMessage(p->WndTrack, TBM_SETRANGE, FALSE,MAKELONG(0,TRACKMAX));
		SendMessage(p->WndTrack, TBM_SETPAGESIZE,0,TRACKMAX/20);
		SendMessage(p->WndTrack, TBM_SETLINESIZE,0,TRACKMAX/100);

		p->TrackPos = 0;
		UpdateTrackPos(p);
		p->ClientRect.right = -1;
		if (DoResize) Resize(p);
	}
	if (!TrackBar && p->WndTrack)
	{
		DestroyWindow(p->WndTrack);
		p->WndTrack = NULL;
		p->ClientRect.right = -1;
		if (DoResize) Resize(p);
	}
	p->TrackThumb = -1;
	return ERR_NONE;
}

static void BeforeExit(intface* p)
{
	if (Context()->Wnd)
	{
		NodeRegSaveValue(0,REG_INITING,NULL,0,TYPE_INT);
		NodeRegSave((node*)p);
		UpdateHotKey(p,0,1);
		if (p->WndVolBack)
		{
			DestroyWindow(p->WndVolBack);
			p->WndVolBack = NULL;
		}
		Context_Wnd(NULL);
	}
}

static void ShowError2(intface* p, const tchar_t* s)
{
	win* Win;

	if (p->Player)
	{
		p->Player->Set(p->Player,PLAYER_STOP,NULL,1); // full stop (no filling buffers)
		ToggleFullScreen(p,0,0); //ATI tweaks would have problem
	}
	UpdateClipping(p,1,1); // force clipping mode
	
	// find application foreground window
	Win = &p->Win;
	while (Win->Child) Win = Win->Child;

	MessageBox(Win->Wnd,s,LangStr(PLATFORM_ID,PLATFORM_ERROR),MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
}

static void ToggleStream(intface* p,int Id,int Type,bool_t UseNone)
{
	packetformat Format;
	int No,Current,First=-1;

	p->Player->Get(p->Player,Id,&Current,sizeof(int));

	if (Current == MAXSTREAM)
		Current = -1;

	for (No=0;PlayerGetStream(p->Player,No,&Format,NULL,0,NULL);++No)
		if (Format.Type == Type)
		{
			if (First < 0)
				First = No;
			if (Current == No)
				Current = -1;
			else
			if (Current == -1)
				Current = No;
		}

	if (Current == -1)
		Current = UseNone ? MAXSTREAM:First;

	if (Current >= 0)
	{
		p->Player->Set(p->Player,Id,&Current,sizeof(int));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
	}
}

static int UpdateTaskBar(intface* p)
{
	if (!p->Win.FullScreen)
		DIASet(p->TaskBar?DIA_TASKBAR:0,DIA_TASKBAR);
	return ERR_NONE;
}

static int UpdateSkin(intface* p,bool_t Others);
static int UpdateSkinFile(intface* p)
{
	if (p->Player)
	{
		SkinFree(p->Skin,NULL);
		SkinLoad(p->Skin,p->Win.Wnd,p->SkinPath);
		UpdateSkin(p,1);
	}
	return ERR_NONE;
}

static int Command(intface* p,int Cmd)
{
	node* Node;
	bool_t b;
	int i;
	fraction f;
	tick_t t;
	int Zero = 0;

	switch (Cmd)
	{
	case IF_SKIN0:
		p->SkinNo = 0;
		UpdateSkin(p,1);
		break;
	case IF_SKIN1:
		p->SkinNo = 1;
		UpdateSkin(p,1);
		break;
	case IF_OPTIONS_VOLUME_UP:
		Delta(p,(node*)p->Player,PLAYER_VOLUME,10,0,100);
		break;
	case IF_OPTIONS_VOLUME_DOWN:
		Delta(p,(node*)p->Player,PLAYER_VOLUME,-10,0,100);
		break;
	case IF_OPTIONS_VOLUME_UP_FINE:
		Delta(p,(node*)p->Player,PLAYER_VOLUME,3,0,100);
		break;
	case IF_OPTIONS_VOLUME_DOWN_FINE:
		Delta(p,(node*)p->Player,PLAYER_VOLUME,-3,0,100);
		break;
	case IF_OPTIONS_BRIGHTNESS_UP:
		Delta(p,p->Color,COLOR_BRIGHTNESS,6,-128,127);
		break;
	case IF_OPTIONS_BRIGHTNESS_DOWN:
		Delta(p,p->Color,COLOR_BRIGHTNESS,-6,-128,127);
		break;
	case IF_OPTIONS_CONTRAST_UP:
		Delta(p,p->Color,COLOR_CONTRAST,6,-128,127);
		break;
	case IF_OPTIONS_CONTRAST_DOWN:
		Delta(p,p->Color,COLOR_CONTRAST,-6,-128,127);
		break;
	case IF_OPTIONS_SATURATION_UP:
		Delta(p,p->Color,COLOR_SATURATION,6,-128,127);
		break;
	case IF_OPTIONS_SATURATION_DOWN:
		Delta(p,p->Color,COLOR_SATURATION,-6,-128,127);
		break;

	case IF_OPTIONS_SCREEN:
		SetDisplayPower(!GetDisplayPower(),1);
		break;

	case IF_OPTIONS_AUDIO_STEREO_TOGGLE:
		p->Player->Get(p->Player,PLAYER_STEREO,&i,sizeof(i));
		i = i==STEREO_LEFT?STEREO_RIGHT:STEREO_LEFT;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_TOGGLE_VIDEO:
		ToggleStream(p,PLAYER_VSTREAM,PACKET_VIDEO,0);
		break;

	case IF_OPTIONS_TOGGLE_SUBTITLE:
		ToggleStream(p,PLAYER_SUBSTREAM,PACKET_SUBTITLE,1);
		break;

	case IF_OPTIONS_TOGGLE_AUDIO:
		ToggleStream(p,PLAYER_ASTREAM,PACKET_AUDIO,0);
		break;

	case IF_OPTIONS_ZOOM_IN:
		p->Player->Get(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		if (f.Num <= 0) f.Num = f.Den = 1;
		if (f.Den != 100)
		{
			f.Num = f.Num * 100 / f.Den;
			f.Den = 100;
		}
		f.Num+=5;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_OUT:
		p->Player->Get(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		if (f.Num <= 0) f.Num = f.Den = 1;
		if (f.Den != 100)
		{
			f.Num = f.Num * 100 / f.Den;
			f.Den = 100;
		}
		if (f.Num>5) f.Num-=5;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FIT:
		p->Player->Get(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		f.Num = f.Num == -1 ? -2:-1;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_PLAY_FULLSCREEN:
		if (p->Player->Get(p->Player,PLAYER_STREAMING,&b,sizeof(b))==ERR_NONE && b)
			ToggleFullScreen(p,-1,0);
		else
		if (!(p->Player->Get(p->Player,PLAYER_FULLSCREEN,&b,sizeof(b))==ERR_NONE && b) &&
			!(p->Player->Get(p->Player,PLAYER_PLAY,&b,sizeof(b))==ERR_NONE && b))
		{
			ToggleFullScreen(p,1,0);
			Toggle(p,PLAYER_PLAY,1);
		}
		else
		{
			Toggle(p,PLAYER_PLAY,0);
			ToggleFullScreen(p,0,0);
		}
		break;

	case IF_MOVE_FFWD:
		SetKeyInSeek(p);
		p->Player->Set(p->Player,PLAYER_MOVEFFWD,NULL,0);
		break;

	case IF_MOVE_BACK:
		SetKeyInSeek(p);
		p->Player->Set(p->Player,PLAYER_MOVEBACK,NULL,0);
		break;

	case IF_NEXT2:
	case IF_NEXT:
		p->Player->Set(p->Player,PLAYER_NEXT,NULL,0);
		break;

	case IF_PREV_SMART:
	case IF_PREV_SMART2:
		p->Player->Set(p->Player,PLAYER_PREV,NULL,1);
		break;

	case IF_PREV:
		p->Player->Set(p->Player,PLAYER_PREV,NULL,0);
		break;

	case IF_PLAYPAUSE:
	case IF_PLAY:
		p->Play = -1; // force button refresh
		Toggle(p,PLAYER_PLAY,-1);
		break;

	case IF_FASTFORWARD:
		p->FFwd = -1; // force button refresh
		Toggle(p,PLAYER_FFWD,-1);
		break;

	case IF_FILE_BENCHMARK:
		if (!ToggleFullScreen(p,1,0))
		{
			if (p->WndTrack)
				SetTrackThumb(p->WndTrack,0);
			WaitBegin();
		}
		UpdateWindow(p->Win.Wnd);
		t = 0;
		p->Player->Set(p->Player,PLAYER_BENCHMARK,&t,sizeof(t));
		p->Bench = 1;
		break;

	case IF_STOP:
		p->Player->Set(p->Player,PLAYER_STOP,NULL,0);
		f.Num = 0;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_PERCENT,&f,sizeof(f));
		break;

	case IF_FILE_OPENFILE:
		p->CmdLineMode = 0;

		Node = NodeCreate(OPENFILE_ID);
		i = 0;
		Node->Set(Node,OPENFILE_FLAGS,&i,sizeof(i));
		i = MEDIA_CLASS;
		Node->Set(Node,OPENFILE_CLASS,&i,sizeof(i));

		UpdateHotKey(p,0,1);
		i = ((win*)Node)->Popup((win*)Node,&p->Win);
		NodeDelete(Node);
		if (i==2)
		{
			UpdateWindow(p->Win.Wnd);
			i=0;
			p->Player->Set(p->Player,PLAYER_LIST_CURRIDX,&i,sizeof(i));
		}
		break;

	case IF_FILE_CORETHEQUE:
		WinPopupClass(CORETHEQUE_UI_ID,&p->Win);
		
		p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0); 
		break;

	case IF_FILE_ABOUT:
		WinPopupClass(ABOUT_ID,&p->Win);
		break;

	case IF_OPTIONS_SETTINGS:
		UpdateHotKey(p,0,1);
		p->AllKeysWarning = 1;
		WinPopupClass(SETTINGS_ID,&p->Win);
		p->AllKeysWarning = 0;

		p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0); 
		break;

	case IF_FILE_PLAYLIST:
		WinPopupClass(PLAYLIST_ID,&p->Win);
		break;

	case IF_FILE_MEDIAINFO:
		WinPopupClass(MEDIAINFO_ID,&p->Win);
		break;

	case IF_FILE_EXIT:
		// just to be sure to stop overlay update (WM_DESTROY comes after window is hidden)
		BeforeExit(p);
		DestroyWindow(p->Win.Wnd);
		break;

	case IF_OPTIONS_REPEAT:
		Toggle(p,PLAYER_REPEAT,-1);
		break;

	case IF_OPTIONS_SHUFFLE:
		Toggle(p,PLAYER_SHUFFLE,-1);
		break;

	case IF_OPTIONS_EQUALIZER:
		p->Equalizer->Get(p->Equalizer,EQUALIZER_ENABLED,&b,sizeof(b));
		b = !b;
		p->Equalizer->Set(p->Equalizer,EQUALIZER_ENABLED,&b,sizeof(b));
		//p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
		break;

	case IF_OPTIONS_SKIN:

		Node = NodeCreate(OPENFILE_ID);
		if (Node)
		{
			i = OPENFLAG_SINGLE;
			Node->Set(Node,OPENFILE_FLAGS,&i,sizeof(i));
			i = SKIN_CLASS;
			Node->Set(Node,OPENFILE_CLASS,&i,sizeof(i));
			i = ((win*)Node)->Popup((win*)Node,&p->Win);
			NodeDelete(Node);
			if (i==1)
			{
				p->SkinPath[0] = 0;
				UpdateSkinFile(p);
			}
		}
		break;


	case IF_OPTIONS_FULLSCREEN:
		p->FullScreen = -1; // force button refresh
		ToggleFullScreen(p,-1,1);
		break;

	case IF_OPTIONS_MUTE:
		p->Mute = -1; // force button refresh
		if (p->Player->Get(p->Player,PLAYER_VOLUME,&i,sizeof(i))==ERR_NONE && i==0)
		{
			i = 1;
			p->Player->Set(p->Player,PLAYER_VOLUME,&i,sizeof(i));
		}
		Toggle(p,PLAYER_MUTE,-1);
		UpdateVolume(p);
		break;

	case IF_OPTIONS_VIDEO_DITHER:
		p->Color->Get(p->Color,COLOR_DITHER,&b,sizeof(b));
		b = !b;
		p->Color->Set(p->Color,COLOR_DITHER,&b,sizeof(b));
		break;

	case IF_OPTIONS_ROTATE:
		p->Player->Get(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		switch (i)
		{
		case 0:	i = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT; break;
		case DIR_SWAPXY | DIR_MIRRORLEFTRIGHT: i = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT; break;
		case DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT: i = DIR_SWAPXY | DIR_MIRRORUPDOWN; break;
		case DIR_SWAPXY | DIR_MIRRORUPDOWN:	i = 0; break;
		default: i = 0; break;
		}
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		p->Player->Get(p->Player,PLAYER_SKIN_DIR,&i,sizeof(i));
		switch (i)
		{
		case 0:	i = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT; break;
		case DIR_SWAPXY | DIR_MIRRORLEFTRIGHT: i = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT; break;
		case DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT: i = DIR_SWAPXY | DIR_MIRRORUPDOWN; break;
		case DIR_SWAPXY | DIR_MIRRORUPDOWN:	i = 0; break;
		default: i = 0; break;
		}
		p->Player->Set(p->Player,PLAYER_SKIN_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ROTATE_GUI:
		i = -1;
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ROTATE_0:
		i = 0;
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ROTATE_270:
		i = DIR_SWAPXY | DIR_MIRRORUPDOWN;
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ROTATE_180:
		i = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT;
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ROTATE_90:
		i = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
		p->Player->Set(p->Player,PLAYER_FULL_DIR,&i,sizeof(i));
		break;

	case IF_OPTIONS_ASPECT_AUTO:
		f.Num = 0;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_SQUARE:
		f.Num = 1;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_4_3_SCREEN:
		f.Num = -4;
		f.Den = 3;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_4_3_NTSC:
		f.Num = 10;
		f.Den = 11;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_4_3_PAL:
		f.Num = 12;
		f.Den = 11;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_16_9_SCREEN:
		f.Num = -16;
		f.Den = 9;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_16_9_NTSC:
		f.Num = 40;
		f.Den = 33;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ASPECT_16_9_PAL:
		f.Num = 16;
		f.Den = 11;
		p->Player->Set(p->Player,PLAYER_ASPECT,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FIT_SCREEN:
		f.Num = 0;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_STRETCH_SCREEN:
		f.Num = -3;
		f.Den = SCALE_ONE;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FILL_SCREEN:
		f.Num = -4;
		f.Den = SCALE_ONE;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_VIEW_TITLEBAR:
		p->TitleBar = !p->TitleBar;
		UpdateTitleBar(p,1);
		break;

	case IF_OPTIONS_VIEW_TASKBAR:
		p->TaskBar = !p->TaskBar;
		UpdateTaskBar(p);
		break;

	case IF_OPTIONS_VIEW_TRACKBAR:
		p->TrackBar = !p->TrackBar;
		UpdateTrackBar(p,1);
		break;

	case IF_OPTIONS_ZOOM_50:
		f.Num = 1;
		f.Den = 2;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_100:
		f.Num = 1;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FIT_110:
		f.Num = -11;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FIT_120:
		f.Num = -12;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_FIT_130:
		f.Num = -13;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_150:
		f.Num = 3;
		f.Den = 2;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_ZOOM_200:
		f.Num = 2;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
		p->Player->Set(p->Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
		break;

	case IF_OPTIONS_VIDEO_ZOOM_SMOOTH50:
		Toggle(p,PLAYER_SMOOTH50,-1);
		Toggle(p,PLAYER_SMOOTHALWAYS,0);
		break;

	case IF_OPTIONS_VIDEO_ZOOM_SMOOTHALWAYS:
		Toggle(p,PLAYER_SMOOTHALWAYS,-1);
		Toggle(p,PLAYER_SMOOTH50,0);
		break;

	case IF_OPTIONS_AUDIO_STEREO:
		i = STEREO_NORMAL;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_STEREO_SWAPPED:
		i = STEREO_SWAPPED;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_STEREO_JOINED:
		i = STEREO_JOINED;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_STEREO_LEFT:
		i = STEREO_LEFT;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_STEREO_RIGHT:
		i = STEREO_RIGHT;
		p->Player->Set(p->Player,PLAYER_STEREO,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_PREAMP_INC:
		p->Player->Get(p->Player,PLAYER_PREAMP,&i,sizeof(i));
		i += 25;
		if (i>200) i=200;
		p->Player->Set(p->Player,PLAYER_PREAMP,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_PREAMP_DEC:
		p->Player->Get(p->Player,PLAYER_PREAMP,&i,sizeof(i));
		i -= 25;
		if (i<-200) i=-200;
		p->Player->Set(p->Player,PLAYER_PREAMP,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_PREAMP:
		i = 0;
		p->Player->Set(p->Player,PLAYER_PREAMP,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_QUALITY_LOW:
		i = 0;
		p->Player->Set(p->Player,PLAYER_AUDIO_QUALITY,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_QUALITY_MEDIUM:
		i = 1;
		p->Player->Set(p->Player,PLAYER_AUDIO_QUALITY,&i,sizeof(i));
		break;

	case IF_OPTIONS_AUDIO_QUALITY_HIGH:
		i = 2;
		p->Player->Set(p->Player,PLAYER_AUDIO_QUALITY,&i,sizeof(i));
		break;

	case IF_OPTIONS_VIDEO_QUALITY_LOWEST:
		i = 0;
		p->Player->Set(p->Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
		break;

	case IF_OPTIONS_VIDEO_QUALITY_LOW:
		i = 1;
		p->Player->Set(p->Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
		break;

	case IF_OPTIONS_VIDEO_QUALITY_NORMAL:
		i = 2;
		p->Player->Set(p->Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
		break;

	case IF_OPTIONS_SPEED_10:
		f.Num = 1;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_25:
		f.Num = 1;
		f.Den = 4;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_50:
		f.Num = 1;
		f.Den = 2;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_100:
		f.Num = 1;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_150:
		f.Num = 3;
		f.Den = 2;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_200:
		f.Num = 2;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_80:
		f.Num = 8;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_90:
		f.Num = 9;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_110:
		f.Num = 11;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_120:
		f.Num = 12;
		f.Den = 10;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_SPEED_BENCHMARK:
		f.Num = 0;
		f.Den = 1;
		p->Player->Set(p->Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
		break;

	case IF_OPTIONS_VIDEO_TURNOFF:
		p->Player->Set(p->Player,PLAYER_VOUTPUTID,&Zero,sizeof(Zero));
		break;

	case IF_OPTIONS_AUDIO_TURNOFF:
		p->Player->Set(p->Player,PLAYER_AOUTPUTID,&Zero,sizeof(Zero));
		UpdateVolume(p);
		break;

	case IF_OPTIONS_SUBTITLE_STREAM_NONE:
		i = MAXSTREAM;
		p->Player->Set(p->Player,PLAYER_SUBSTREAM,&i,sizeof(i));
		break;
	}

	if (Cmd >= IF_CHAPTER && Cmd < IF_CHAPTER+100)
	{
		tick_t Time = PlayerGetChapter(p->Player,Cmd-IF_CHAPTER,NULL,0);
		if (Time>=0)
			p->Player->Set(p->Player,PLAYER_POSITION,&Time,sizeof(tick_t));
	}

	if (Cmd >= IF_STREAM_AUDIO && Cmd < IF_STREAM_AUDIO+MAXSTREAM)
	{
		i = Cmd-IF_STREAM_AUDIO;
		p->Player->Set(p->Player,PLAYER_ASTREAM,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
	}

	if (Cmd >= IF_STREAM_VIDEO && Cmd < IF_STREAM_VIDEO+MAXSTREAM)
	{
		i = Cmd-IF_STREAM_VIDEO;
		p->Player->Set(p->Player,PLAYER_VSTREAM,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
	}

	if (Cmd >= IF_STREAM_SUBTITLE && Cmd < IF_STREAM_SUBTITLE+MAXSTREAM)
	{
		i = Cmd-IF_STREAM_SUBTITLE;
		p->Player->Set(p->Player,PLAYER_SUBSTREAM,&i,sizeof(i));
		p->Player->Set(p->Player,PLAYER_RESYNC,NULL,0);
	}

	if (Cmd >= IF_VIDEO && Cmd < IF_VIDEO+ARRAYCOUNT(p->VOutput,int))
	{
		b = 0;
		p->Player->Set(p->Player,PLAYER_VIDEO_ACCEL,&b,sizeof(b));
		p->Player->Set(p->Player,PLAYER_VOUTPUTID,ARRAYBEGIN(p->VOutput,int)+(Cmd-IF_VIDEO),sizeof(int));
	}

	if (Cmd >= IF_VIDEOACCEL && Cmd < IF_VIDEOACCEL+ARRAYCOUNT(p->VOutput,int))
	{
		b = 1;
		p->Player->Set(p->Player,PLAYER_VIDEO_ACCEL,&b,sizeof(b));
		p->Player->Set(p->Player,PLAYER_VOUTPUTID,ARRAYBEGIN(p->VOutput,int)+(Cmd-IF_VIDEOACCEL),sizeof(int));
	}

	if (Cmd >= IF_AUDIO && Cmd < IF_AUDIO+ARRAYCOUNT(p->AOutput,int))
	{
		p->Player->Set(p->Player,PLAYER_AOUTPUTID,ARRAYBEGIN(p->AOutput,int)+(Cmd-IF_AUDIO),sizeof(int));
		UpdateVolume(p);
	}

	if (p->Focus && p->Clipping)
		UpdateClipping(p,0,1); // maybe overlapping window is gone

	return ERR_INVALID_PARAM;
}

static int PlayerNotify(intface* p,int Id,int Value)
{
	if (Id == PLAYER_TITLE)
	{
		p->TitleTimeWidth = 0;
		if (Value)
			p->TitleTime = -1;
		if (p->WndTitle && !p->FullScreen)
			InvalidateRect(p->WndTitle,NULL,1); // redraws timer too!
		if (!p->Win.FullScreen && !p->Bench && p->InSeek && p->Capture)
			PostMessage(p->Win.Wnd,MSG_PLAYER,PLAYER_PERCENT,Value);
	}
	else
	if (Id == PLAYER_PERCENT)
	{
		if (!p->Win.FullScreen && !p->Bench && (!p->InSeek || !p->Capture))
			PostMessage(p->Win.Wnd,MSG_PLAYER,PLAYER_PERCENT,Value);
	}
	else
		PostMessage(p->Win.Wnd,MSG_PLAYER,Id,Value);
	return ERR_NONE;
}

static int ErrorNotify(intface* p,int Param,int Param2)
{
	const tchar_t* Msg = (const tchar_t*)Param2;
	if (GetCurrentThreadId() != p->ThreadId)
		PostMessage(p->Win.Wnd,MSG_ERROR,0,(LPARAM)tcsdup(Msg));
	else
		ShowError2(p,Msg);
	return ERR_NONE;
}

static int KeyRotate(intface* p,int Code)
{
	int i;
	int Dir;

	if (!QueryAdvanced(ADVANCED_KEYFOLLOWDIR))
		return Code;

	switch (Code)
	{
	case VK_UP: i=1; break;
	case VK_RIGHT: i=2; break;
	case VK_DOWN: i=3; break;
	case VK_LEFT: i=4; break;
	default: return Code;
	}

	p->Player->Get(p->Player,PLAYER_REL_DIR,&Dir,sizeof(Dir));
	if (Dir & DIR_SWAPXY)
		i += (Dir & DIR_MIRRORUPDOWN) ? 1:3;
	else
		i += (Dir & DIR_MIRRORUPDOWN) ? 2:0;
	if (i>4) i-=4;

	switch (i)
	{
	case 1: Code = VK_UP; break;
	case 2: Code = VK_RIGHT; break;
	case 3: Code = VK_DOWN; break;
	case 4: Code = VK_LEFT; break;
	}

	return Code;
}

static void DefaultSkin(intface* p);

static NOINLINE void UpdatePosition(intface* p)
{
	UpdateTrackPos(p);
	UpdateTitleTime(p);
}

static const int SkinCmd[MAX_SKINITEM] =
{
	0,
	0,
	0,
	0,
	0,
	IF_PLAY,
	IF_STOP,
	IF_NEXT,
	IF_PREV,
	IF_OPTIONS_FULLSCREEN,
	IF_FASTFORWARD,
	IF_FILE_PLAYLIST,
	IF_FILE_OPENFILE,
	IF_OPTIONS_MUTE,
	IF_OPTIONS_REPEAT,
	IF_SKIN0,
	IF_SKIN1,
	IF_OPTIONS_ROTATE,
	0,
	IF_PLAY_FULLSCREEN,
	IF_FILE_EXIT,
};

static bool_t Proc(intface* p, int Msg, uint32_t wParam, uint32_t lParam, int* Result)
{
	tick_t t;
	bool_t b;
	int i,Key;
	PAINTSTRUCT Paint;
	notify Notify;

	switch (Msg) 
	{
	case WM_CANCELMODE:
		if (p->InSkin>0)
		{
			p->InSkin = 0;
			ReleaseCapture();
		}

		if (p->InVol)
		{
			p->InVol = 0;
			ReleaseCapture();
		}
		if (p->InSeek)
			StopSeek(p);
		if (IsOverlay(p))
		{
			//delayed clipping (don't want to turn off overlay, just because a toolbar button was pressed)
			SetTimer(p->Win.Wnd,TIMER_CLIPPING2,CLIPPING2_CYCLE,NULL);
			break;
		}
		// no break
#if !defined(TARGET_WINCE)
	case WM_INITMENU:
#endif
		UpdateClipping(p,1,1); // force clipping mode
		break;

	case WM_PAINT:
		BeginPaint(p->Win.Wnd,&Paint);
		if (p->Skin[p->SkinNo].Valid)
			SkinDraw(&p->Skin[p->SkinNo],Paint.hdc,&p->SkinArea);
		p->Player->Paint(p->Player,Paint.hdc,p->Offset.x,p->Offset.y);
		EndPaint(p->Win.Wnd,&Paint);
		break;

#if defined(TARGET_WINCE)
	case WM_CTLCOLORSTATIC:
		*Result = (LRESULT) GetSysColorBrush(COLOR_BTNFACE);
		return 1;
#endif

	case WM_SYSCHAR:
		if (wParam == 13) //Alt+Enter
			PostMessage(p->Win.Wnd,WM_COMMAND,IF_OPTIONS_FULLSCREEN,0);
		break;

	case WM_TIMER:
		if (wParam == TIMER_TITLESCROLL)
			TitleScroll(p);

		if (wParam == TIMER_SLIDERINVOL)
			UpdateVol(p);

		if (wParam == TIMER_SLIDERINSEEK && !p->Win.FullScreen)
			UpdatePosition(p);

		if (wParam == TIMER_KEYINSEEK)
			StopSeek(p);

		if (wParam == TIMER_CLIPPING2 && ShowVideo(p) && !p->Clipping && IsOverlapped(p))
			UpdateClipping(p,1,1);
		
		if (wParam == TIMER_CLIPPING && ShowVideo(p) && p->Clipping)
			UpdateClipping(p,0,1);

		if (wParam == TIMER_INITING)
		{
			NodeRegSaveValue(0,REG_INITING,NULL,0,TYPE_INT);
			KillTimer(p->Win.Wnd,TIMER_INITING);
		}

		if (wParam == TIMER_SLEEP && (p->Play>0 || p->FFwd>0))
			SleepTimerReset();

		break;

	case WM_INITMENUPOPUP:
		UpdateClipping(p,1,1); // force clipping mode
		UpdateMenu(p);
		break;

	case WM_KEYUP:
#if defined(TARGET_WINCE)
		if (!GetDisplayPower())
			SetDisplayPower(0,1);
#endif
		Key = WinKeyState(KeyRotate(p,wParam));
		for (i=0;i<HOTKEYCOUNT;++i)
			if (p->HotKey[i] && (p->HotKey[i] & ~HOTKEY_KEEP) == Key)
				return 0;

		if (p->AllKeys)
			ForwardMenuButtons(&p->Win,Msg,wParam,lParam);
		break;

	case WM_KEYDOWN:
		Key = WinKeyState(KeyRotate(p,wParam));
		for (i=0;i<HOTKEYCOUNT;++i)
			if (p->HotKey[i] && (p->HotKey[i] & ~HOTKEY_KEEP) == Key)
			{
				PostMessage(p->Win.Wnd,WM_COMMAND,HotKey[i],0);
				return 0;
			}

		if (p->AllKeys)
			ForwardMenuButtons(&p->Win,Msg,wParam,lParam);
		break;

	case WM_HOTKEY:
#if defined(TARGET_WINCE)
		if ((LOWORD(lParam) & MOD_KEYUP))
		{
			if (!GetDisplayPower())
				SetDisplayPower(0,1);
		}
		else
#endif
			PostMessage(p->Win.Wnd,WM_COMMAND,wParam,0);
		*Result = 0;
		return 1;
	
	case WM_MOUSEMOVE:
		if (p->InSkin>0 && (wParam & MK_LBUTTON))
			SkinMouse(&p->Skin[p->SkinNo],p->InSkin,(int16_t)LOWORD(lParam)-p->SkinArea.x,(int16_t)HIWORD(lParam)-p->SkinArea.y,p->Player,NULL,p->Win.Wnd,&p->SkinArea);
		return 0;

	case WM_LBUTTONUP:
		if (p->InSkin>0)
		{
			int Cmd = 0;
			SkinMouse(&p->Skin[p->SkinNo],p->InSkin,(int16_t)LOWORD(lParam)-p->SkinArea.x,(int16_t)HIWORD(lParam)-p->SkinArea.y,p->Player,&Cmd,p->Win.Wnd,&p->SkinArea);
			if (p->InSeek)
				StopSeek(p);

			p->InSkin = 0;
			if (p->Capture)
			{
				ReleaseCapture();
				p->Capture = 0;
			}

			if (Cmd==SKIN_VOLUME)
				UpdateVolume(p);

			if (Cmd>0 && SkinCmd[Cmd]>0)
				Command(p,SkinCmd[Cmd]);
		}
		*Result = 0;
		return 1;

	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		DEBUG_MSG(DEBUG_WIN,T("LBUTTON"));
		if ((wParam & MK_LBUTTON) && p->Focus)
		{
			tchar_t URL[MAXPATH];
			SetDisplayPower(1,0);

			if (p->Skin[p->SkinNo].Valid && !p->FullScreen)
				p->InSkin = SkinMouse(&p->Skin[p->SkinNo],0,(int16_t)LOWORD(lParam)-p->SkinArea.x,(int16_t)HIWORD(lParam)-p->SkinArea.y,p->Player,NULL,p->Win.Wnd,&p->SkinArea);
			else
				p->InSkin = -1;

			if (p->InSkin>0)
			{
				SetCapture(p->Win.Wnd);
				p->Capture = 1;
				if (p->InSkin == SKIN_SEEK)
					p->InSeek = 1;
			}
			else if (p->InSkin<0)
			{
				if (!p->FullScreen && p->Player->CommentByName(p->Player,-1,T("URL"),URL,TSIZEOF(URL)))
					WinShowHTML(URL);
				else
				if (p->Player->Get(p->Player,PLAYER_SINGLECLICKFULLSCREEN,&b,sizeof(b))==ERR_NONE && !b)
				{
					if (Msg == WM_LBUTTONDBLCLK)
						PostMessage(p->Win.Wnd,WM_COMMAND,IF_OPTIONS_FULLSCREEN,0);
					PostMessage(p->Win.Wnd,WM_COMMAND,IF_PLAY,0);
				}
				else
					PostMessage(p->Win.Wnd,WM_COMMAND,IF_PLAY_FULLSCREEN,0);
			}
		}
		SetForegroundWindow(p->Win.Wnd); //sometimes wince suck (triple click in opendialog)
		*Result = 0;
		return 1;

	case WM_MOVE:
		Resize(p);
		break;

	case WM_SETTINGCHANGE:
		if (wParam == SETTINGCHANGE_RESET) 
		{
			p->Player->Set(p->Player,PLAYER_RESETVIDEO,NULL,0);

			if (p->VolResizeNeeded2)
			{
				ResizeVolume(p);
				if (p->VolResizeNeeded2>1)
					ShowVol(p,1);
				p->VolResizeNeeded2 = 0;
			}
		}
		break;

	case WM_SIZE:
#ifdef MAXIMIZE_FULLSCREEN
		if (wParam == SIZE_MAXIMIZED)
		{
			ShowWindow(p->Win.Wnd,SW_SHOWNORMAL);
			ToggleFullScreen(p,1,1);
		}
		else
#endif
			Resize(p);
		break;

	case WM_SETFOCUS:
		if (!p->Focus || !p->WndVolBack || (HWND)wParam != p->WndVolBack)
		{
			DEBUG_MSG(DEBUG_WIN,T("SETFOCUS"));
			p->Focus = 1;
			DIASet(0,DIA_SIP);
			if (p->Wait)
			{
				WaitBegin();
				UpdateHotKey(p,1,0);
				// clipping still needed
			}
			else
				UpdateClipping(p,0,1); // turn off clipping when no overlapping windows
			UpdateClippingTimer(p);
			UpdateTitleScroll(p);
		}
		break;

	case WM_KILLFOCUS:
		if (!p->WndVolBack || (HWND)wParam != p->WndVolBack)
		{
			DEBUG_MSG(DEBUG_WIN,T("KILLFOCUS"));
			p->Focus = 0;
			if (!ShowVideo(p))
				UpdateClipping(p,1,1); // force clipping mode
			if (p->Wait)
				WaitEnd();
		}
		break;

	case WM_ACTIVATE:
		if (!p->WndVolBack || (HWND)lParam != p->WndVolBack)
		{
			if (LOWORD(wParam)==WA_INACTIVE) 
			{
#ifdef NDEBUG
				Toggle(p,PLAYER_FOREGROUND,0);
				ToggleFullScreen(p,0,0); // example ATI tweaks have problem
#endif
				ShowVol(p,0);
				if (p->InSeek) StopSeek(p);
				SetDisplayPower(1,0);
			}
			else
			{
				ShowVol(p,1);
				UpdateVolume(p);
				Toggle(p,PLAYER_FOREGROUND,1);
				if (p->ForceFullScreen)
					PostMessage(p->Win.Wnd,MSG_PLAYER,PLAYER_FULLSCREEN,1);
			}
		}
		break;

	case WM_DESTROY:
		BeforeExit(p);
		PostQuitMessage(0);

		SkinFree(p->Skin,NULL);

		Context()->Error.Func = NULL;
		ArrayClear(&p->VOutput);
		ArrayClear(&p->AOutput);
		break;

	case MSG_ERROR:
		ShowError2(p,(const tchar_t*)lParam);
		free((tchar_t*)lParam);
		return 1;

	case MSG_PLAYER:
		switch (wParam)
		{
		case PLAYER_EXIT_AT_END:
			if (p->CmdLineMode)
				PostMessage(p->Win.Wnd,WM_COMMAND,IF_FILE_EXIT,0);
			break;

		case PLAYER_PERCENT:
			UpdatePosition(p);
			break;

		case PLAYER_LOADMODE:
			if (p->Wait != (bool_t)lParam)
			{
				p->Wait = lParam;
				if (p->Wait)
				{
					if (WaitBegin())
						UpdateClipping(p,1,0);
				}
				else
					WaitEnd();
			}
			break;

		case PLAYER_FULLSCREEN:
			p->ForceFullScreen = lParam;
			if (GetForegroundWindow() == p->Win.Wnd || !lParam)
			{
				ToggleFullScreen(p,lParam,1);
				p->ForceFullScreen = 0;
			}
			break;

		case PLAYER_PLAY:
			RefreshButton(p,PLAYER_PLAY,&p->Play,IF_PLAY,0,1);
			RefreshButton(p,PLAYER_FFWD,&p->FFwd,IF_FASTFORWARD,4,1);
			UpdateSleepTimer(p);
			break;

		case PLAYER_BENCHMARK:
			if (p->Bench && p->Player->Get(p->Player,PLAYER_BENCHMARK,&t,sizeof(tick_t))==ERR_NONE)
			{
				p->Bench = 0;
				ToggleFullScreen(p,0,0);
				WaitEnd();
				if (p->WndTrack)
					SetTrackThumb(p->WndTrack,1);

				UpdateWindow(p->Win.Wnd);
				WinPopupClass(BENCHRESULT_ID,&p->Win);
			}
			break;
		}
		return 1;

	case WM_COPYDATA:
		ProcessCmdLine(p,(const tchar_t*)((COPYDATASTRUCT*)lParam)->lpData);
		return 1;

	case MSG_INIT:
		SetForegroundWindow(p->Win.Wnd);
		if (Context()->CmdLine[0] || NodeRegLoadValue(0,REG_INITING,&i,sizeof(i),TYPE_INT))
		{
			b = 1; // last time crashed -> discard saved playlist
			p->Player->Set(p->Player,PLAYER_DISCARDLIST,&b,sizeof(b));
		}

#ifdef NDEBUG
		i = 1;
		NodeRegSaveValue(0,REG_INITING,&i,sizeof(int),TYPE_INT);
		SetTimer(p->Win.Wnd,TIMER_INITING,INITING_CYCLE,NULL);
#endif

		Context_Wnd(p->Win.Wnd);

		if (Context()->CmdLine[0])
		{
			p->Player->Get(p->Player,PLAYER_PLAYATOPEN_FULL,&b,sizeof(b));
			if (!b)
			{
				UpdateWindow(p->Win.Wnd); //http connection may take a while
				if (p->Win.WndTB)
					UpdateWindow(p->Win.WndTB);
			}

			ProcessCmdLine(p,Context()->CmdLine);
		}

		if (LangStr(LANG_ID,LANG_DEFAULT)[0]==0 &&
			LangStr(LANG_ID,Context()->Lang)[0]==0)
			ShowMessage(NULL,T("Language files (*.txt,*.tgz) are missing!"));

		return 1;

	case WM_CREATE:

		WinTitle(&p->Win,Context()->ProgramName);

		p->ThreadId = GetCurrentThreadId();
		p->Player = (player*)Context()->Player;
		p->Color = NodeEnumObject(NULL,COLOR_ID);
		p->Equalizer = NodeEnumObject(NULL,EQUALIZER_ID);
		p->Focus = 0;
		p->Clipping = 0;

		Notify.This = p;
		Notify.Func = (notifyfunc)PlayerNotify;
		if (p->Player)
			p->Player->Set(p->Player,PLAYER_NOTIFY,&Notify,sizeof(Notify));

		Context()->Error.This = p;
		Context()->Error.Func = ErrorNotify;

		WinMenuEnable(&p->Win,1,IF_OPTIONS_VIDEO_STREAM,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_VIDEO_STREAM,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_STREAM,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_VIDEO_DEVICE,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_DEVICE,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_ORIENTATION_FULLSCREEN,0);
		WinMenuEnable(&p->Win,1,IF_OPTIONS_AUDIO_QUALITY,0);

		if (!NodeEnumClass(NULL,CORETHEQUE_UI_ID))
			WinMenuDelete(&p->Win,0,IF_FILE_CORETHEQUE);

		if (IsAutoRun(p,Context()->CmdLine))
			DefaultSkin(p);

		SkinLoad(p->Skin,p->Win.Wnd,p->SkinPath);

		if (!p->Skin[0].Valid)
			CreateButtons(p);
		UpdateSkin(p,0);
		UpdateTrackBar(p,0);
		UpdateTitleBar(p,0);
		CreateVolumeTrack(p);
		CreateDeviceMenu(p);
		Resize(p);

		// first finish window creation and postpone loading playlist and etc... 
		PostMessage(p->Win.Wnd,MSG_INIT,0,0);

		UpdateHotKey(p,1,0);
		break;
	}

	return 0;
}

static const datatable Params[] =
{
	{ IF_TITLEBAR,			TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ IF_TRACKBAR,			TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ IF_TASKBAR,			TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ IF_SKINNO,			TYPE_INT,  DF_SETUP|DF_HIDDEN },
	{ IF_SKINPATH,			TYPE_STRING,DF_SETUP|DF_HIDDEN },
#if defined(TARGET_WINCE)
	{ IF_ALLKEYS,			TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif

	DATATABLE_END(INTERFACE_ID)
};

static int Enum( intface* p, int* No, datadef* Param )
{
	if (NodeEnumTable(No,Param,Params)==ERR_NONE)
		return ERR_NONE;

	if (*No>=0 && *No<HOTKEYCOUNT)
	{
		memset(Param,0,sizeof(datadef));
		Param->No = HotKey[*No];
		Param->Type = TYPE_HOTKEY;
		Param->Size = sizeof(int);
		Param->Flags = DF_SETUP;
#if defined(TARGET_WINCE)
		if (*No==0) Param->Flags |= DF_GAP;
#endif
		Param->Class = INTERFACE_ID;
		Param->Name = LangStr(INTERFACE_ID,Param->No);
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}

static int Get(intface* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	int i;

	for (i=0;i<HOTKEYCOUNT;++i)
		if (HotKey[i]==No)
		{
			GETVALUE(p->HotKey[i],int); 
			break;
		}

	switch (No)
	{
	case IF_TRACKBAR: GETVALUE(p->TrackBar,bool_t); break;
	case IF_TITLEBAR: GETVALUE(p->TitleBar,bool_t); break;
	case IF_TASKBAR: GETVALUE(p->TaskBar,bool_t); break;
	case IF_SKINNO: GETVALUE(p->SkinNo,int); break;
	case IF_SKINPATH: GETSTRING(p->SkinPath); break;
	case IF_ALLKEYS: GETVALUE(p->AllKeys,bool_t); break;
	}
	return Result;
}

static int UpdateSkin(intface* p,bool_t Others)
{
	if (!p->Player)
		return ERR_NONE;

	if (p->SkinNo>0 && !p->Skin[p->SkinNo].Valid)
		p->SkinNo=0;

	if (Others)
	{
		if (p->Skin[p->SkinNo].Valid)
			SkinUpdate(&p->Skin[p->SkinNo],p->Player,p->Win.Wnd,&p->SkinArea);

		ShowVol(p,1);
		UpdateTrackBar(p,1);
		InvalidateRect(p->Win.Wnd,NULL,0);
		p->ClientRect.right = -1;
		Resize(p);
	}
	RefreshButton(p,PLAYER_REPEAT,NULL,0,0,0);
	RefreshButton(p,PLAYER_PLAY,NULL,0,0,0);
	RefreshButton(p,PLAYER_FFWD,NULL,0,0,0);
	RefreshButton(p,PLAYER_MUTE,NULL,0,0,0);
	RefreshButton(p,PLAYER_FULLSCREEN,NULL,0,0,0);
	if (p->TitleBrush)
	{
		DeleteObject(p->TitleBrush);
		p->TitleBrush = NULL;
	}
	if (p->Skin[p->SkinNo].Valid)
	{
		p->TitleBrush = CreateSolidBrush(p->Skin[p->SkinNo].Item[SKIN_TITLE].ColorFace);
		p->TitleFace = p->Skin[p->SkinNo].Item[SKIN_TITLE].ColorFace;
		p->TitleText = p->Skin[p->SkinNo].Item[SKIN_TITLE].ColorText;
	}
	else
	{
		p->TitleBrush = NULL;
		p->TitleFace = GetSysColor(COLOR_BTNFACE);
		p->TitleText = GetSysColor(COLOR_BTNTEXT);
	}
	return ERR_NONE;
}

static int UpdateAllKeys(intface* p)
{
	if (!p->AllKeys && p->AllKeysWarning)
	{
		int i;
		for (i=0;i<HOTKEYCOUNT;++i)
			if (p->HotKey[i] && WinNoHotKey(p->HotKey[i]))
			{
				ShowMessage(LangStr(p->Win.Node.Class,NODE_NAME),LangStr(INTERFACE_ID,IF_ALLKEYS_WARNING2));
				break;
			}
	}
	return ERR_NONE;
}

static void SetHotKey(intface* p,int i,int Key)
{
	if (p->Focus && p->HotKey[i] && !p->AllKeys)
		WinUnRegisterHotKey(&p->Win,HotKey[i]);

	if (!p->AllKeys && p->AllKeysWarning && WinNoHotKey(Key))
		ShowMessage(LangStr(p->Win.Node.Class,NODE_NAME),LangStr(INTERFACE_ID,IF_ALLKEYS_WARNING));

	p->HotKey[i] = Key;

	if (p->Focus && p->HotKey[i] && !p->AllKeys && !WinEssentialKey(p->HotKey[i]))
		WinRegisterHotKey(&p->Win,HotKey[i],p->HotKey[i]);
}

static int Set(intface* p,int No,const void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	int i,j;

	for (i=0;i<HOTKEYCOUNT;++i)
		if (HotKey[i]==No)
		{
			int Key = *(int*)Data;
			SetHotKey(p,i,0);
			if (Key)
			{
				for (j=0;j<HOTKEYCOUNT;++j)
					if ((p->HotKey[j] & ~HOTKEY_KEEP) == (Key & ~HOTKEY_KEEP))
						SetHotKey(p,j,0);
				SetHotKey(p,i,Key);
			}
			Result = ERR_NONE;
			break;
		}

	switch (No)
	{
	case IF_TRACKBAR: SETVALUE(p->TrackBar,bool_t,UpdateTrackBar(p,1)); break;
	case IF_TITLEBAR: SETVALUE(p->TitleBar,bool_t,UpdateTitleBar(p,1)); break;
	case IF_TASKBAR: SETVALUE(p->TaskBar,bool_t,UpdateTaskBar(p)); break;
	case IF_SKINNO: SETVALUE(p->SkinNo,int,UpdateSkin(p,1)); break;
	case IF_SKINPATH: SETSTRING(p->SkinPath); Result = UpdateSkinFile(p); break;
	case IF_ALLKEYS: SETVALUECMP(p->AllKeys,bool_t,UpdateAllKeys(p),EqBool); break;
	}
	return Result;
}

static void DefaultSkin(intface* p)
{
	tchar_t* s;
	GetModuleFileName(GetModuleHandle(NULL),p->SkinPath,MAXPATH);
	s = tcsrchr(p->SkinPath,'\\');
	if (s) s[1]=0;
	tcscat_s(p->SkinPath,TSIZEOF(p->SkinPath),T("skin.xml"));
	p->SkinNo = 0;
}

static int Create(intface* p)
{
	int Key;
	DefaultSkin(p);
	
	p->Win.WinWidth = 360;
	p->Win.WinHeight = 240;
	p->Win.Proc = Proc;
	p->Win.Command = (wincommand)Command;
	p->Win.MenuDef = MenuDef;
	p->Win.Node.Enum = Enum;
	p->Win.Node.Get = Get;
	p->Win.Node.Set = Set;

	p->Vol = -1;
	p->TaskBar = 1;
	p->TitleBar = 1;
	p->TrackBar = 1;
	p->MenuPreAmp = -1;
	p->MenuStreams = 0;
	p->MenuAStream = -1;
	p->MenuVStream = -1;
	p->MenuSubStream = -1;

	Key = VK_RIGHT;
	Set(p,IF_MOVE_FFWD,&Key,sizeof(Key));
	Key = VK_LEFT;
	Set(p,IF_MOVE_BACK,&Key,sizeof(Key));
	Key = VK_UP;
	Set(p,IF_OPTIONS_VOLUME_UP,&Key,sizeof(Key));
	Key = VK_DOWN;
	Set(p,IF_OPTIONS_VOLUME_DOWN,&Key,sizeof(Key));
	Key = VK_RETURN;
	Set(p,QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE?IF_PLAY_FULLSCREEN:IF_PLAY,&Key,sizeof(Key));
	Key = 0xB0; //VK_MEDIA_NEXT_TRACK;
	Set(p,IF_NEXT2,&Key,sizeof(Key));
	Key = 0xB1; //VK_MEDIA_PREV_TRACK;
	Set(p,IF_PREV_SMART2,&Key,sizeof(Key));
	Key = 0xB2; //VK_MEDIA_STOP;
	Set(p,IF_STOP,&Key,sizeof(Key));
	Key = 0xB3; //VK_MEDIA_PLAY_PAUSE;
	Set(p,IF_PLAYPAUSE,&Key,sizeof(Key));
#if defined(TARGET_WIN32)
	Key = VK_SPACE;
	Set(p,IF_PLAY,&Key,sizeof(Key));
#endif
	
	return ERR_NONE;
}

static const nodedef IntFace =
{
	sizeof(intface)|CF_GLOBAL|CF_SETTINGS,
	INTERFACE_ID,
	WIN_CLASS,
	PRI_MAXIMUM+570,
	(nodecreate)Create,
};

static const nodedef Skin =
{
	0,
	SKIN_CLASS,
	NODE_CLASS,
	PRI_MAXIMUM,
};

void Interface_Init()
{
	NodeRegisterClass(&IntFace);
	NodeRegisterClass(&Skin);
}

void Interface_Done()
{
	NodeUnRegisterClass(INTERFACE_ID);
	NodeUnRegisterClass(SKIN_CLASS);
}

#endif
