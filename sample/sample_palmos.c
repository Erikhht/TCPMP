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
 * $Id: sample_palmos.c 623 2006-02-01 13:19:15Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../common/palmos/pace.h"
#include "../common/palmos/dia.h"
#include "../interface/win.h"
#include "../interface/settings.h"
#include "../interface/benchresult.h"
#include "../interface/palmos/keys.h"
#include "../interface/skin.h"
#include "resources.h"
#include "events.h"

static int ProgramId = 'TPLY';
static const tchar_t ProgramName[] = T("TCPMP");
static const tchar_t ProgramVersion[] = 
#include "../version"
;

#ifdef HAVE_PALMONE_SDK
#include <Common/System/palmOneChars.h>
#include <68K/System/PmPalmOSNVFS.h>
#endif

static diastate MainState;
static int SIPState = 0;
static player* Player = NULL;
static bool_t HighDens = 0;
static bool_t RefreshResize = 0;
static int NoPaintResize = 0;
static bool_t AttnEvent = 0;
static bool_t FullScreen = 0;
static int InFullScreen = -1;
static bool_t MainDrawn = 0;
static FormEventHandlerType* MouseDown = NULL;
static UInt16 PrefCurrPage = PrefAudio;
static bool_t KeyTrackNext = 0;
static bool_t KeyTrackPrev = 0;
static bool_t KeySelectHold = 0;
static UInt16 LaunchCode = 0;
static MemPtr LaunchParameters = NULL;
static array VideoDriver;
static array VideoDriverNames;
static bool_t NoEventChecking = 0;
static void* SkinDb = NULL;
//static int InSkin = 0;
static UInt32 SkinNo = 0;
static skin Skin[MAX_SKIN];

static NOINLINE void GetObjectBounds(FormType* Form, int Index, RectangleType* Rect)
{
    WinHandle Old = WinSetDrawWindow(FrmGetWindowHandle(Form));
    FrmGetObjectBounds(Form,(UInt16)Index,Rect);
    if (Old)
        WinSetDrawWindow(Old);
}

static NOINLINE void SetObjectBounds(FormType* Form, int Index, const RectangleType* Rect)
{
    WinHandle Old = WinSetDrawWindow(FrmGetWindowHandle(Form));
    FrmSetObjectBounds(Form,(UInt16)Index,Rect);
    if (Old)
        WinSetDrawWindow(Old);
}

static NOINLINE void ObjectPosition(FormType* Form,UInt16 Id,int x1,int x2,int y1,int y2,int w,int h)
{
	RectangleType Rect;
	int Index = FrmGetObjectIndex(Form,Id);
	GetObjectBounds(Form,Index,&Rect);
	if (x1>=0 || x2>=0)
	{
		if (x1<0)
			Rect.topLeft.x = (Coord)(w - (160-x2) - Rect.extent.x);
		else
		{
			Rect.topLeft.x = (Coord)x1;
			if (x2>=0)
				Rect.extent.x = (Coord)(w - (160-x2) - x1);
		}
	}
	if (y1>=0 || y2>=0)
	{
		if (y1<0)
			Rect.topLeft.y = (Coord)(h - (160-y2) - Rect.extent.y);
		else
		{
			Rect.topLeft.y = (Coord)y1;
			if (y2>=0)
				Rect.extent.y = (Coord)(h - (160-y2) - y1);
		}
	}
	SetObjectBounds(Form,Index,&Rect);
}

static void HandleEvents();
static void Refresh(FormType* Form,bool_t Draw);
static bool_t Toggle(int Param,int Force);
static void SetFullScreen(bool_t State);

static NOINLINE bool_t IsVideoPrimary()
{
	node* VOutput;
	bool_t Primary;
	int CoverArt = -2;
	node* Format = NULL;
	int VStream;

	if (Player->Get(Player,PLAYER_VOUTPUT,&VOutput,sizeof(VOutput))!=ERR_NONE || !VOutput)
		return 0;

	if (VOutput->Get(VOutput,VOUT_PRIMARY,&Primary,sizeof(Primary))!=ERR_NONE || !Primary)
		return 0;

	if (Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format))==ERR_NONE && Format &&
		Format->Get(Format,FORMAT_COVERART,&CoverArt,sizeof(CoverArt))==ERR_NONE &&
		Player->Get(Player,PLAYER_VSTREAM,&VStream,sizeof(VStream))==ERR_NONE && VStream==CoverArt)
		return 0;

	return 1;
}

static NOINLINE void ShowObject(FormType* Form,UInt16 Id,bool_t State)
{
	UInt16 Index = FrmGetObjectIndex(Form,Id);
	if (Index != frmInvalidObjectId)
	{
		if (State)
			FrmShowObject(Form,Index);
		else
			FrmHideObject(Form,Index);
	}
}

static NOINLINE void SetClipping(bool_t b)
{
	if (b && IsVideoPrimary())
		Toggle(PLAYER_PLAY,0);
	Player->Set(Player,PLAYER_CLIPPING,&b,sizeof(b));
	if (b)
		SetDisplayPower(1,0);
}

static void MainFormResize(FormType* Form)
{
    Coord Width;
    Coord Height;
	RectangleType Bounds;
	rect Viewport;
	diastate State;

	if (!FullScreen)
		SIPState = DIAGet(DIA_ALL);

	DIAGetState(&State);
	if (MainState.Width != State.Width ||
		MainState.Height != State.Height ||
		MainState.Direction != State.Direction)
	{
		memcpy(&MainState,&State,sizeof(State));

		WinGetDisplayExtent(&Width, &Height);
		Bounds.topLeft.x = 0;
		Bounds.topLeft.y = 0;
		Bounds.extent.x = Width;
		Bounds.extent.y = Height;
		WinSetBounds(FrmGetWindowHandle(Form), &Bounds);

		if (FullScreen)
		{
			ObjectPosition(Form,MainViewport,0,160,0,160,Width,Height);

			Viewport.x = 0;
			Viewport.y = 0;
			Viewport.Width = (Coord)Width;
			Viewport.Height = (Coord)Height;
		}
		else
		{
			if (Skin[SkinNo].Valid)
			{
				Viewport = Skin[SkinNo].Item[SKIN_VIEWPORT].Rect;
				ObjectPosition(Form,MainViewport,Viewport.x,Viewport.y,Viewport.x+Viewport.Width,Viewport.y+Viewport.Height,Width,Height);
			}
			else
			{
				ObjectPosition(Form,MainViewport,0,160,15,130,Width,Height);
				ObjectPosition(Form,MainWinTitle,0,-1,0,-1,Width,Height);
				ObjectPosition(Form,MainTitle,45,159,2,-1,Width,Height);
				ObjectPosition(Form,MainPosition,1,129,-1,143,Width,Height);
				ObjectPosition(Form,MainTimer,-1,159,-1,143,Width,Height);

				ObjectPosition(Form,MainPrev,1,-1,-1,159,Width,Height);
				ObjectPosition(Form,MainPlay,31,-1,-1,159,Width,Height);
				ObjectPosition(Form,MainNext,71,-1,-1,159,Width,Height);
				ObjectPosition(Form,MainFullScreen,101,-1,-1,159,Width,Height);
				ObjectPosition(Form,MainPref,131,-1,-1,159,Width,Height);

				Viewport.x = 0;
				Viewport.y = 15;
				Viewport.Width = (Coord)(160+Width-160-Viewport.x);
				Viewport.Height = (Coord)(130+Height-160-Viewport.y);
			}
		}

		if (HighDens)
		{
			UInt16 OldCoord = WinSetCoordinateSystem(kCoordinatesNative);
			Viewport.Width = WinScaleCoord((Coord)(Viewport.x+Viewport.Width),1);
			Viewport.Height = WinScaleCoord((Coord)(Viewport.y+Viewport.Height),1);
			Viewport.x = WinScaleCoord((Coord)Viewport.x,0);
			Viewport.y = WinScaleCoord((Coord)Viewport.y,0);
			Viewport.Width -= Viewport.x;
			Viewport.Height -= Viewport.y;
			WinSetCoordinateSystem(OldCoord);
		}


		Player->Set(Player,PLAYER_ROTATEBEGIN,NULL,0);
		Player->Set(Player,PLAYER_RESETVIDEO,NULL,0);
		Player->Set(Player,PLAYER_SKIN_VIEWPORT,&Viewport,sizeof(rect));
		Player->Set(Player,PLAYER_FULLSCREEN,&FullScreen,sizeof(FullScreen));
		Player->Set(Player,PLAYER_UPDATEVIDEO,NULL,0);
		NoPaintResize = -1; // force redraw
	}

	if (NoPaintResize<=0)
	{
		if (FullScreen)
		{
			Player->Set(Player,PLAYER_ROTATEEND,NULL,0);
		}
		else
		{
			if (MainDrawn)
			{
				IndexedColorType BackColor;
				RectangleType Rect;
				UInt16 i,n = FrmGetNumberOfObjects(Form);
				WinHandle Old = WinSetDrawWindow(FrmGetWindowHandle(Form));

				if (!Skin[SkinNo].Valid)
				{
					WinGetBounds(FrmGetWindowHandle(Form), &Rect );

					BackColor = WinSetBackColor(UIColorGetTableEntryIndex(UIFormFill));
					Rect.topLeft.x = 0;
					Rect.topLeft.y = (Coord)(Rect.extent.y-160+130);
					Rect.extent.y = 160-130;
					WinEraseRectangle(&Rect,0);
					Rect.topLeft.y = 0;
					Rect.extent.y = 15;
					WinEraseRectangle(&Rect,0);
					WinSetBackColor(BackColor);

					for (i=0;i<n;++i)
						FrmShowObject(Form,i);
				}
				else
				{
					SkinDraw(&Skin[SkinNo],FrmGetWindowHandle(Form),NULL);
					ShowObject(Form,MainViewport,1);
				}

				if (Old)
					WinSetDrawWindow(Old);
			}
			else
			{
				if (Skin[SkinNo].Valid)
				{
					ShowObject(Form,MainTitle,0);
					ShowObject(Form,MainPosition,0);
					ShowObject(Form,MainTimer,0);
					ShowObject(Form,MainPrev,0);
					ShowObject(Form,MainNext,0);
					ShowObject(Form,MainPlay,0);
					ShowObject(Form,MainFullScreen,0);
					ShowObject(Form,MainPref,0);
				}
				FrmDrawForm(Form);
				if (Skin[SkinNo].Valid)
					SkinDraw(&Skin[SkinNo],FrmGetWindowHandle(Form),NULL);
				MainDrawn = 1;
			}

			if (Skin[SkinNo].Valid)
			{
				RectangleType Rect;
				WinGetBounds(FrmGetWindowHandle(Form), &Rect );
				if (Rect.extent.y > Skin[SkinNo].Bitmap[0].Height)
				{
					RGBColorType c,c0;
					c.index = 0;
					c.r = 0;
					c.g = 0;
					c.b = 0;
					WinSetBackColorRGB(&c,&c0);
					Rect.topLeft.x = 0;
					Rect.topLeft.y = (UInt16)Skin[SkinNo].Bitmap[0].Height;
					Rect.extent.y = (Int16)(Rect.extent.y - Skin[SkinNo].Bitmap[0].Height);
					WinEraseRectangle(&Rect,0);
					WinSetBackColorRGB(&c0,&c);
				}
			}
		}
	}

	NoPaintResize = 0;

	if (RefreshResize)
	{
		RefreshResize = 0;
		Refresh(Form,1);
		SetClipping(0);
	}
}

static void EnumDir(const tchar_t* Path,const tchar_t* Exts,int Deep)
{
	if (Deep>=0)
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
					EnumDir(FullPath,Exts,Deep-1);
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
}

static void GenExts(tchar_t* Exts,int ExtsLen)
{
	int* i;
	array List;
	Exts[0]=0;
	NodeEnumClass(&List,MEDIA_CLASS);
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
}

static void SearchFiles()
{
	tchar_t* Exts = malloc(1024*sizeof(tchar_t));
	if (Exts)
	{
		int n;
		GenExts(Exts,1024);

		n=0;
		Player->Set(Player,PLAYER_LIST_COUNT,&n,sizeof(n));

		EnumDir("",Exts,2);

		free(Exts);

		n=0;
		Player->Set(Player,PLAYER_LIST_CURRIDX,&n,sizeof(n));
	}
}

static NOINLINE bool_t Visible(FormType* Form)
{
	return !FullScreen && FrmGetActiveForm()==Form;
}

static void UpdatePosition(FormType* Form,bool_t Draw)
{
	static tchar_t Timer[40] = {0};

	tchar_t s[40];
	fraction Percent;
	UInt16 Pos;

	if (Player->Get(Player,PLAYER_PERCENT,&Percent,sizeof(Percent)) != ERR_NONE)
	{
		Percent.Num = 0;
		Percent.Den = 1;
	}

	if (Percent.Num>Percent.Den)
		Percent.Num=Percent.Den;
	if (Percent.Num<0)
		Percent.Num=0;

	Pos = (UInt16)Scale(16384,Percent.Num,Percent.Den);

	CtlSetSliderValues(FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,MainPosition)),NULL,NULL,NULL,&Pos);

	Player->Get(Player,PLAYER_TIMER,s,sizeof(s));
	if (tcscmp(s,Timer)!=0)
	{
		FieldType* Field = (FieldType*)FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,MainTimer));
		tcscpy_s(Timer,TSIZEOF(Timer),s);
		FldSetTextPtr(Field,Timer);
		if (Draw)
			FldDrawField(Field);
	}
}

static void UpdateTitle(FormType* Form,bool_t Draw)
{
	static tchar_t Title[40] = {0};
	tchar_t s[40];

	Player->Get(Player,PLAYER_TITLE,s,sizeof(s));
	if (tcscmp(s,Title)!=0)
	{
		FormType* Form = FrmGetFormPtr(MainForm);
		FieldType* Field = (FieldType*)FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,MainTitle));
		tcscpy_s(Title,TSIZEOF(Title),s);
		FldSetTextPtr(Field,Title);
		if (Draw)
			FldDrawField(Field);
	}
}

static void UpdatePlayButton(FormType* Form)
{
	bool_t Play;
	Player->Get(Player,PLAYER_PLAY,&Play,sizeof(Play));
	CtlSetValue(FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,MainPlay)),(Int16)Play);
}

static void Refresh(FormType* Form,bool_t Draw)
{
	if (Visible(Form))
	{
		UpdatePosition(Form,Draw);
		UpdatePlayButton(Form);
		UpdateTitle(Form,Draw);
	}
}

static void BenchmarkVerify()
{
	node* Node;
	Player->Get(Player,PLAYER_VOUTPUT,&Node,sizeof(Node));
	if (Node)
	{
		int Dropped = 0;
		Node->Get(Node,OUT_DROPPED,&Dropped,sizeof(int));
		if (Dropped > 2)
			ShowError(0,BENCHRESULT_ID,BENCHRESULT_INVALID);
	}
}

static NOINLINE bool_t IsAudioPlayed()
{
	bool_t Play,FFwd;
	node* AOutput;
	Player->Get(Player,PLAYER_PLAY,&Play,sizeof(Play));
	Player->Get(Player,PLAYER_FFWD,&FFwd,sizeof(FFwd));
	return Player->Get(Player,PLAYER_AOUTPUT,&AOutput,sizeof(AOutput))==ERR_NONE && AOutput && (Play || FFwd);
}

static NOINLINE bool_t IsPlayList()
{
	int Count;
	return Player->Get(Player,PLAYER_LIST_COUNT,&Count,sizeof(Count))==ERR_NONE && Count>1;
}

static int PlayerNotify(FormType* Form,int Id,int Value)
{
	if (Id == PLAYER_PERCENT && Value==1)
	{
		GetDisplayPower(); // udpate if display has turned on somehow
		if (!NoEventChecking)
			HandleEvents(); //EvtGetEvent() has to be called (alarm) even if the main loop is busy with decoding
	}

	if (Id == PLAYER_FULLSCREEN)  
		SetFullScreen(Value); 

	if (Visible(Form))
	{
		if (Id == PLAYER_PERCENT)
			UpdatePosition(Form,1);

		if (Id == PLAYER_PLAY)
			UpdatePlayButton(Form);

		if (Id == PLAYER_TITLE)
			UpdateTitle(Form,1);
	}

	if (Id == PLAYER_BENCHMARK)
	{
		EventType Event;

		Toggle(PLAYER_PLAY,0);
		SetFullScreen(0);

		BenchmarkVerify();

		memset(&Event,0,sizeof(Event));
		Event.eType = menuEvent;
		Event.data.menu.itemID = MainBenchmarkForm;
		_EvtAddUniqueEventToQueue(&Event, 0, 1);
	}

	if (Id == PLAYER_POWEROFF && !Value)
	{
		if (Form != FrmGetActiveForm()) 
			SetClipping(1); // probably keylock window

		if ((FullScreen?0:SIPState)!=DIAGet(DIA_ALL))
		{
			// overlay will be reinited in a different mode
			// we need to clear MainState so next resize will 
			// reinit overlay when dia is restored
			memset(&MainState,0,sizeof(MainState));
		}
	}

	return ERR_NONE;
}

static void UpdateSkin(bool_t Init);

static void MainFormInit(FormType* Form)
{
	notify Notify;
	UInt32 Version;

	Notify.This = Form;
	Notify.Func = (notifyfunc)PlayerNotify;
	Player->Set(Player,PLAYER_NOTIFY,&Notify,sizeof(Notify));

	HighDens = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &Version)==errNone && Version>=4;

	SkinDb = SkinLoad(Skin,Form,NULL);
	UpdateSkin(1);
	FtrGet(Context()->ProgramId,20000,&SkinNo);
}

static void UpdateControls(bool_t State)
{
	static const UInt16 Controls[] = { 
		MainTitle, MainPosition, MainTimer, MainPrev,
		MainPlay, MainNext, MainFullScreen, MainPref, 0 };

	FormType* Form = FrmGetFormPtr(MainForm);
	const UInt16* i;
	if (Skin[SkinNo].Valid)
		State = 0;
	for (i=Controls;*i;++i)
	{
		UInt16 Index = FrmGetObjectIndex(Form,*i);
		if (Index != frmInvalidObjectId)
			CtlSetUsable(FrmGetObjectPtr(Form,Index),(Boolean)State);
	}
}

static void ApplicationPostEvent();
static Boolean ApplicationHandleEvent(EventPtr Event);

static Boolean PlatformHandleEvent(EventPtr Event)
{
	if (Event->eType == keyDownEvent && Event->data.keyDown.chr == vchrPowerOff && !GetDisplayPower())
	{
		SetDisplayPower(1,0);
		SetKeyboardBacklight(1);
		return 1;
	}
	else
	if ((Event->eType == penDownEvent || Event->eType == penUpEvent) && 
		FrmGetActiveForm() == FrmGetFormPtr(MainForm) && !GetDisplayPower())
		return 1;

	if (Event->eType == keyUpEvent)
		GetDisplayPower(); // refresh, if neccessary disable led blinking

	return 0;
}

static NOINLINE void PushReDraw()
{
	EventType e;
	memset(&e,0,sizeof(e));
	e.eType = winDisplayChangedEvent;
	_EvtAddUniqueEventToQueue(&e, 0, 1);
}

static void HandleEvents()
{
	EventType event;
	for (;;)
	{
		_EvtGetEvent(&event,0);
		if (event.eType == nilEvent)
			break;

		if (!PlatformHandleEvent(&event))
			if (!ApplicationHandleEvent(&event)) // important: before SysHandleEvent
				OSHandleEvent(&event);

		ApplicationPostEvent();
	}
}

static void UpdateSkin(bool_t Init)
{
	if (SkinNo>0 && !Skin[SkinNo].Valid)
		SkinNo=0;
	if (!Init || Skin[SkinNo].Valid)
		UpdateControls(!FullScreen);
}

static void SetFullScreen(bool_t State)
{
	if (InFullScreen>=0)
	{
		InFullScreen=State;
		return;
	}

	InFullScreen = State;
	while (FullScreen != InFullScreen)
	{
		Player->Set(Player,PLAYER_ROTATEBEGIN,NULL,0);

		FullScreen = (bool_t)InFullScreen;
		memset(&MainState,0,sizeof(MainState));

		if (FullScreen)
		{
			UpdateControls(0);
			DIASet(0,DIA_ALL);
			SetKeyboardBacklight(0);
		}
		else
		{
			Refresh(FrmGetFormPtr(MainForm),0);
			DIASet(SIPState,DIA_ALL);

			SetKeyboardBacklight(1);
			UpdateControls(1);
		}

		PushReDraw();
		HandleEvents();
	}
	InFullScreen = -1;
}

static NOINLINE bool_t Toggle(int Param,int Force)
{
	bool_t State = 0;
	if (Player->Get(Player,Param,&State,sizeof(State))==ERR_NONE && State!=Force)
	{
		State = !State;
		Player->Set(Player,Param,&State,sizeof(State));
		Player->Get(Player,Param,&State,sizeof(State));
	}
	return State;
}

static NOINLINE int Delta(int Param,int d,int Min,int Max)
{
	int State;
	if (Player && Player->Get(Player,Param,&State,sizeof(State))==ERR_NONE)
	{
		State += d;
		if (State < Min)
			State = Min;
		if (State > Max)
			State = Max;

		Player->Set(Player,Param,&State,sizeof(State));

		if (Param == PLAYER_VOLUME && State > Min) 
			Toggle(PLAYER_MUTE,0);
	}
	return State;
}

static Boolean ViewportEventHandler(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP)
{
	if (cmd == formGadgetDrawCmd)
	{
		diastate State;
		DIAGetState(&State);
		if (MainState.Width == State.Width &&
			MainState.Height == State.Height &&
			MainState.Direction == State.Direction)
		{
			gadgetP->attr.visible = true;
			Player->Set(Player,PLAYER_ROTATEEND,NULL,0);
		}
		return 1;
	}
	return 0;
}

static Boolean WinTitleEventHandler(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP)
{
	if (cmd == formGadgetDrawCmd)
	{
		IndexedColorType BackColor,TextColor;
		FontID Font;

		BackColor = WinSetBackColor(UIColorGetTableEntryIndex(UIFormFrame));
		TextColor = WinSetTextColor(UIColorGetTableEntryIndex(UIFormFill));
		Font = FntSetFont(boldFont);
		WinEraseRectangle(&gadgetP->rect,0);
		WinDrawChars(Context()->ProgramName,(Int16)tcslen(Context()->ProgramName),
			(Coord)(gadgetP->rect.topLeft.x+2),(Coord)(gadgetP->rect.topLeft.y+2));
		WinSetBackColor(BackColor);
		WinSetTextColor(TextColor);
		FntSetFont(Font);

		gadgetP->attr.visible = true;
		return 1;
	}
	return 0;
}

static void OpenResize(Coord Width,Coord Height);

static void ResizeForm(FormType* Form)
{
    Coord Width;
    Coord Height;
	RectangleType Bounds;
	WinHandle Win = FrmGetWindowHandle(Form);
	WinGetDisplayExtent(&Width, &Height);
	WinGetBounds(Win, &Bounds );
	if (Bounds.topLeft.x!=0 || Bounds.topLeft.y!=0 ||
		Bounds.extent.x!=Width || Bounds.extent.y!=Height)
	{
		Bounds.topLeft.x = 0;
		Bounds.topLeft.y = 0;
		Bounds.extent.x = Width;
		Bounds.extent.y = Height;
		WinSetBounds(Win, &Bounds);

		switch (FrmGetFormId(Form))
		{
		case PrefForm:
			ObjectPosition(Form,PrefGeneral,-1,-1,-1,143+15,Width,Height);
			ObjectPosition(Form,PrefVideo,-1,-1,-1,143+15,Width,Height);
			ObjectPosition(Form,PrefAudio,-1,-1,-1,143+15,Width,Height);
			ObjectPosition(Form,PrefClose,-1,-1,-1,143+15,Width,Height);
			break;
		case OpenForm:
			OpenResize(Width,Height);
			break;
		case EqForm:
			ObjectPosition(Form,EqReset,-1,-1,-1,146+12,Width,Height);
			//no break
		default:
			ObjectPosition(Form,DummyOk,-1,-1,-1,146+12,Width,Height);
			break;
		}
	}

	FrmDrawForm(Form);
}

static Boolean DummyHandler( FormType* Form, EventPtr Event )
{
	if (DIAHandleEvent(Form,Event))
		return 1;

	switch (Event->eType)
	{
		case frmOpenEvent:
			SetClipping(1);
			PushReDraw();
			break;

		case frmUpdateEvent:
			PushReDraw();
			return 1;

 		case winDisplayChangedEvent:
			ResizeForm(Form);
			return 1;

		case winEnterEvent:
			if (Event->data.winEnter.enterWindow == (WinHandle)Form && Form == FrmGetActiveForm())
				PushReDraw();
			return 1;

		case winExitEvent:
			return 1;

		case ctlSelectEvent:
			if (Event->data.ctlSelect.controlID == DummyOk || 
				Event->data.ctlSelect.controlID == DummyCancel)
			{
				FrmReturnToForm(MainForm);
				return 1;
			}
			break;

		default:
			break;
	}
	return 0;
}

static NOINLINE void UpdateVideoCaps(FormType* Form)
{
	int i;
	bool_t b;
	node* Color = NodeEnumObject(NULL,COLOR_ID);

	b = Color->Get(Color,COLOR_BRIGHTNESS,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoBrightness,b);
	ShowObject(Form,PrefVideoBrightnessLabel,b);
	ShowObject(Form,PrefVideoBrightnessValue,b);

	b = Color->Get(Color,COLOR_CONTRAST,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoContrast,b);
	ShowObject(Form,PrefVideoContrastLabel,b);
	ShowObject(Form,PrefVideoContrastValue,b);

	b = Color->Get(Color,COLOR_SATURATION,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoSaturation,b);
	ShowObject(Form,PrefVideoSaturationLabel,b);
	ShowObject(Form,PrefVideoSaturationValue,b);

	b = Color->Get(Color,COLOR_R_ADJUST,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoRed,b);
	ShowObject(Form,PrefVideoRedLabel,b);
	ShowObject(Form,PrefVideoRedValue,b);

	b = Color->Get(Color,COLOR_G_ADJUST,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoGreen,b);
	ShowObject(Form,PrefVideoGreenLabel,b);
	ShowObject(Form,PrefVideoGreenValue,b);

	b = Color->Get(Color,COLOR_B_ADJUST,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoBlue,b);
	ShowObject(Form,PrefVideoBlueLabel,b);
	ShowObject(Form,PrefVideoBlueValue,b);

	b = Color->Get(Color,COLOR_DITHER,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoDither,b);

	b = Player->Get(Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i))==ERR_NONE;
	ShowObject(Form,PrefVideoQuality,b);
}

static void PrefPage(FormType* Form,UInt16 Id)
{
	static const UInt16 Group[] = 
	{
		PrefGeneralRepeat,PrefGeneral,
		PrefGeneralShuffle,PrefGeneral,
		PrefGeneralSpeedLabel,PrefGeneral,
		PrefGeneralSpeedValue,PrefGeneral,
		PrefGeneralSpeed,PrefGeneral,
		PrefGeneralSpeedOrig,PrefGeneral,
		PrefGeneralSpeedCustom,PrefGeneral,
		PrefGeneralMoveFFwdLabel,PrefGeneral,
		PrefGeneralMoveFFwd,PrefGeneral,
		PrefGeneralMoveFFwdLabel2,PrefGeneral,
		PrefGeneralMoveBackLabel,PrefGeneral,
		PrefGeneralMoveBack,PrefGeneral,
		PrefGeneralMoveBackLabel2,PrefGeneral,
		PrefGeneralAVOffsetLabel,PrefGeneral,
		PrefGeneralAVOffset,PrefGeneral,
		PrefGeneralAVOffsetLabel2,PrefGeneral,

		PrefAudioDisable,PrefAudio,
		PrefAudioVolumeLabel,PrefAudio,
		PrefAudioVolumeValue,PrefAudio,
		PrefAudioVolume,PrefAudio,
		PrefAudioPreampLabel,PrefAudio,
		PrefAudioPreampValue,PrefAudio,
		PrefAudioPreamp,PrefAudio,
		PrefAudioPreampReset,PrefAudio,
		PrefAudioPanLabel,PrefAudio,
		PrefAudioPanLabel2,PrefAudio,
		PrefAudioPanValue,PrefAudio,
		PrefAudioPan,PrefAudio,
		PrefAudioPanReset,PrefAudio,
		PrefAudioStereo,PrefAudio,

		PrefVideoDriver,PrefVideo,
		PrefVideoAspect,PrefVideo,
		PrefVideoFullLabel,PrefVideo,
		PrefVideoSkinLabel,PrefVideo,
		PrefVideoZoomLabel,PrefVideo,
		PrefVideoRotateLabel,PrefVideo,
		PrefVideoFullZoom,PrefVideo,
		PrefVideoSkinZoom,PrefVideo,
		PrefVideoFullRotate,PrefVideo,
		PrefVideoSkinRotate,PrefVideo,
		PrefVideoDither,PrefVideo,
		PrefVideoQuality,PrefVideo,
		PrefVideoBrightness,PrefVideo,
		PrefVideoContrast,PrefVideo,
		PrefVideoSaturation,PrefVideo,
		PrefVideoRed,PrefVideo,
		PrefVideoGreen,PrefVideo,
		PrefVideoBlue,PrefVideo,
		PrefVideoBrightnessLabel,PrefVideo,
		PrefVideoContrastLabel,PrefVideo,
		PrefVideoSaturationLabel,PrefVideo,
		PrefVideoRedLabel,PrefVideo,
		PrefVideoGreenLabel,PrefVideo,
		PrefVideoBlueLabel,PrefVideo,
		PrefVideoBrightnessValue,PrefVideo,
		PrefVideoContrastValue,PrefVideo,
		PrefVideoSaturationValue,PrefVideo,
		PrefVideoRedValue,PrefVideo,
		PrefVideoGreenValue,PrefVideo,
		PrefVideoBlueValue,PrefVideo,

		0,0
	};

	const UInt16* i;
	RectangleType r;

	//FrmGlueNavRemoveFocusRing(Form); //crashes in longterm at program exit!

	if (WinGetDrawWindow() == FrmGetWindowHandle(Form))
	{
		r.topLeft.x = 0;
		r.topLeft.y = 16;
		r.extent.x = 160;
		r.extent.y = 126;
		WinEraseRectangle(&r,0);
	}

	for (i=Group;i[0];i+=2)
		if (Id != i[1])
			ShowObject(Form,i[0],0);

	for (i=Group;i[0];i+=2)
		if (Id == i[1])
			ShowObject(Form,i[0],1);

	if (Id == PrefVideo)
		UpdateVideoCaps(Form);

	PrefCurrPage = Id;

	/* //crashes in longterm at program exit!
	switch (PrefCurrPage)
	{
	case PrefGeneral: FrmGlueNavObjectTakeFocus(Form, PrefGeneralSpeedOrig); break;
	case PrefVideo: FrmGlueNavObjectTakeFocus(Form, PrefVideoSkinZoom); break;
	case PrefAudio: FrmGlueNavObjectTakeFocus(Form, PrefAudioVolume); break;
	}
	*/
}

static NOINLINE void SetList(FormType* Form,int Id,int IdList,int Value,bool_t OnlyLabel)
{
	void* List = FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,(UInt16)IdList));
	if (!OnlyLabel) 
		LstSetSelection(List,(Int16)Value);
	CtlSetLabel(FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,(UInt16)Id)),LstGetSelectionText(List,(Int16)Value));
}

static NOINLINE bool_t GetFieldInt(FormType* Form,UInt16 Id,int* Value)
{
	FieldType* Field = (FieldType*)FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,Id));
	const tchar_t* s = FldGetTextPtr(Field);
	return s && stscanf(s,T("%d"),Value)==1;
}

static NOINLINE void SetFieldInt(FormType* Form,UInt16 Id,int Value,bool_t Draw)
{
	FieldType* Field = (FieldType*)FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,Id));
    MemHandle h0,h = MemHandleNew(12*sizeof(tchar_t));
	tchar_t *s;

	if (h)
	{
		s=MemHandleLock(h);
		IntToString(s,12,Value,0);
		MemHandleUnlock(h);

		h0 = FldGetTextHandle(Field);
		FldSetTextHandle(Field,h);
		if (h0)
			MemHandleFree(h0);
		if (Draw)
			FldDrawField(Field);
	}
}

static NOINLINE int FromDir(int i)
{
	switch (i)
	{
	case 0: return 1;
	case DIR_SWAPXY | DIR_MIRRORLEFTRIGHT: return 2;
	case DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT: return 3;
	case DIR_SWAPXY | DIR_MIRRORUPDOWN: return 4;
	}
	return 0;
}

static NOINLINE int ToDir(int i)
{
	switch (i)
	{
	case 1: return 0;
	case 2: return DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
	case 3: return DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT;
	case 4: return DIR_SWAPXY | DIR_MIRRORUPDOWN;
	}
	return -1;
}

static const fraction ZoomTab[10] =
{
	{0,0},
	{-4,SCALE_ONE},
	{-3,SCALE_ONE},
	{-11,10},
	{-12,10},
	{-13,10},
	{1,2},
	{1,1},
	{3,2},
	{2,1},
};

static NOINLINE void ToZoom(int i,fraction* f)
{
	*f = ZoomTab[i];
}

static NOINLINE int FromZoom(const fraction* f)
{
	int i;
	if (f->Num)
		for (i=1;i<10;++i)
			if (EqFrac(f,ZoomTab+i)) 
				return i;
	return 0;
}

static const fraction AspectTab[8] =
{
	{0,1},
	{1,1},
	{-4,3},
	{10,11},
	{12,11},
	{-16,9},
	{40,33},
	{16,11},
};

static NOINLINE void ToAspect(int i,fraction* f)
{
	*f = AspectTab[i];
}

static NOINLINE int FromAspect(const fraction* f)
{
	int i;
	if (f->Num)
		for (i=1;i<8;++i)
			if (EqFrac(f,AspectTab+i))
				return i;
	return 0;
}

static NOINLINE int TickConvert(tick_t t,int n)
{
	int Adjust = TICKSPERSEC/(n*2);
	if (t<0)
		t -= Adjust;
	else
		t += Adjust;
	return Scale(t,n,TICKSPERSEC);
}

static NOINLINE void SetValue(FormType* Form,int Id,int Value)
{
	CtlSetValue(FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,(UInt16)Id)),(Int16)Value);
}

static NOINLINE int GetValue(FormType* Form,int Id)
{
	return CtlGetValue(FrmGetObjectPtr(Form,FrmGetObjectIndex(Form,(UInt16)Id)));
}

static void PrefInit(FormType* Form)
{
	UInt16 Idx;
	ListType* ListControl;
	array List;
	bool_t b;
	RectangleType ListRect,ItemRect;
	fraction f;
	const tchar_t *s;
	int *q;
	int i,j;
	tick_t t;
	node* Color = NodeEnumObject(NULL,COLOR_ID);

	SetValue(Form,PrefCurrPage,1);
	PrefPage(Form,PrefCurrPage);

	Player->Get(Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
	SetFieldInt(Form,PrefGeneralSpeedValue,Scale(f.Num,100,f.Den),0);
	SetValue(Form,PrefGeneralSpeedOrig,f.Den == f.Num);
	SetValue(Form,PrefGeneralSpeedCustom,f.Den != f.Num);
	SetValue(Form,PrefGeneralSpeed,(f.Num>f.Den?Scale(512,f.Num,f.Den)-512:Scale(600,f.Num,f.Den)-600)+512);
	Player->Get(Player,PLAYER_REPEAT,&b,sizeof(b));
	SetValue(Form,PrefGeneralRepeat,b);
	Player->Get(Player,PLAYER_SHUFFLE,&b,sizeof(b));
	SetValue(Form,PrefGeneralShuffle,b);
	Context()->Advanced->Get(Context()->Advanced,ADVANCED_AVOFFSET,&t,sizeof(t));
	SetFieldInt(Form,PrefGeneralAVOffset,TickConvert(t,1000),0);
	Player->Get(Player,PLAYER_MOVEBACK_STEP,&t,sizeof(t));
	SetFieldInt(Form,PrefGeneralMoveBack,TickConvert(t,1),0);
	Player->Get(Player,PLAYER_MOVEFFWD_STEP,&t,sizeof(t));
	SetFieldInt(Form,PrefGeneralMoveFFwd,TickConvert(t,1),0);

	Player->Get(Player,PLAYER_VOLUME,&i,sizeof(i));
	SetValue(Form,PrefAudioVolume,i);
	SetFieldInt(Form,PrefAudioVolumeValue,i,0);
	Player->Get(Player,PLAYER_PAN,&i,sizeof(i));  
	SetFieldInt(Form,PrefAudioPanValue,i,0); i>>=1;
	SetValue(Form,PrefAudioPan,i+50); 
	Player->Get(Player,PLAYER_PREAMP,&i,sizeof(i)); 
	SetFieldInt(Form,PrefAudioPreampValue,i,0); i>>=1;
	SetValue(Form,PrefAudioPreamp,i+50); 
	Player->Get(Player,PLAYER_AOUTPUTID,&i,sizeof(i));
	SetValue(Form,PrefAudioDisable,i==0);
	Player->Get(Player,PLAYER_STEREO,&i,sizeof(i));
	SetList(Form,PrefAudioStereo,PrefAudioStereoList,i,0);

	Color->Get(Color,COLOR_BRIGHTNESS,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoBrightness,i+64);
	SetFieldInt(Form,PrefVideoBrightnessValue,i,0);
	Color->Get(Color,COLOR_CONTRAST,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoContrast,i+64);
	SetFieldInt(Form,PrefVideoContrastValue,i,0);
	Color->Get(Color,COLOR_SATURATION,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoSaturation,i+64);
	SetFieldInt(Form,PrefVideoSaturationValue,i,0);
	Color->Get(Color,COLOR_R_ADJUST,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoRed,i+32);
	SetFieldInt(Form,PrefVideoRedValue,i,0);
	Color->Get(Color,COLOR_G_ADJUST,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoGreen,i+32);
	SetFieldInt(Form,PrefVideoGreenValue,i,0);
	Color->Get(Color,COLOR_B_ADJUST,&i,sizeof(i)); i>>=1;
	SetValue(Form,PrefVideoBlue,i+32);
	SetFieldInt(Form,PrefVideoBlueValue,i,0);
	Color->Get(Color,COLOR_DITHER,&b,sizeof(b));
	SetValue(Form,PrefVideoDither,b);
	Player->Get(Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i));
	SetList(Form,PrefVideoQuality,PrefVideoQualityList,i,0);

	ArrayClear(&VideoDriver);
	ArrayClear(&VideoDriverNames);

	NodeEnumClass(&List,VOUT_CLASS);
	j = 0;

	Player->Get(Player,PLAYER_VOUTPUTID,&i,sizeof(i));
	Player->Get(Player,PLAYER_VIDEO_ACCEL,&b,sizeof(b));

	for (q=ARRAYBEGIN(List,int);q!=ARRAYEND(List,int);++q)
	{
		if (VOutIDCT(*q))
		{
			if (i==*q && b)
				j = ARRAYCOUNT(VideoDriverNames,tchar_t*);
			s = LangStr(VOUT_IDCT_CLASS(*q),NODE_NAME);
			ArrayAppend(&VideoDriver,q,sizeof(int),32);
			ArrayAppend(&VideoDriverNames,&s,sizeof(tchar_t*),32);
		}

		if (i==*q && !b)
			j = ARRAYCOUNT(VideoDriverNames,tchar_t*);
		s = LangStr(*q,NODE_NAME);
		ArrayAppend(&VideoDriver,q,sizeof(int),32);
		ArrayAppend(&VideoDriverNames,&s,sizeof(tchar_t*),32);
	}

	ArrayClear(&List);

	if (i==0)
		j = ARRAYCOUNT(VideoDriverNames,tchar_t*);
	s = T("Disabled video");
	ArrayAppend(&VideoDriver,q,sizeof(int),32);
	ArrayAppend(&VideoDriverNames,&s,sizeof(tchar_t*),32);

	Idx = FrmGetObjectIndex(Form,PrefVideoDriverList);
	ListControl = (ListType*)FrmGetObjectPtr(Form,Idx);
	LstSetListChoices(ListControl,
		ARRAYBEGIN(VideoDriverNames,tchar_t*),(UInt16)ARRAYCOUNT(VideoDriverNames,tchar_t*));
	LstSetHeight(ListControl,(UInt16)(min(6,ARRAYCOUNT(VideoDriverNames,tchar_t*))));
	GetObjectBounds(Form,Idx,&ListRect);
	GetObjectBounds(Form,FrmGetObjectIndex(Form,PrefVideoDriver),&ItemRect);
	ListRect.topLeft.y = (Coord)(ItemRect.topLeft.y + ItemRect.extent.y - ListRect.extent.y);
	SetObjectBounds(Form,Idx,&ListRect);
	SetList(Form,PrefVideoDriver,PrefVideoDriverList,j,0);

	Player->Get(Player,PLAYER_ASPECT,&f,sizeof(f));
	SetList(Form,PrefVideoAspect,PrefVideoAspectList,FromAspect(&f),0);
	Player->Get(Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
	SetList(Form,PrefVideoSkinZoom,PrefVideoSkinZoomList,FromZoom(&f),0);
	Player->Get(Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
	SetList(Form,PrefVideoFullZoom,PrefVideoFullZoomList,FromZoom(&f),0);
	Player->Get(Player,PLAYER_SKIN_DIR,&i,sizeof(i));
	SetList(Form,PrefVideoSkinRotate,PrefVideoSkinRotateList,FromDir(i),0);
	Player->Get(Player,PLAYER_FULL_DIR,&i,sizeof(i));
	SetList(Form,PrefVideoFullRotate,PrefVideoFullRotateList,FromDir(i),0);
}

static Boolean MediaInfoHandler( EventPtr Event )
{
	return DummyHandler(FrmGetFormPtr(MediaInfoForm),Event);
}

static Boolean BenchmarkHandler( EventPtr Event )
{
	return DummyHandler(FrmGetFormPtr(BenchmarkForm),Event);
}

static Boolean AboutHandler( EventPtr Event )
{
	return DummyHandler(FrmGetFormPtr(AboutForm),Event);
}

static bool_t Tweak(int Class,int No,bool_t* State,bool_t Set)
{
	node* p = NodeEnumObject(NULL,Class);
	if (p)
	{
		bool_t v;
		datadef Def;

		if (!NodeDataDef(p,No,&Def) || (Def.Flags & DF_HIDDEN))
			return 0;

		if (p->Get(p,No,&v,sizeof(bool_t))==ERR_NONE)
		{
			if (!Set)
				*State = v;
			else
				p->Set(p,No,State,sizeof(bool_t));
			return 1;
		}
	}
	return 0;
}

static Boolean TweakHandler( EventPtr Event )
{
	FormType* Form = FrmGetFormPtr(TweakForm);
	if (Event->eType == ctlSelectEvent && Event->data.ctlSelect.controlID == DummyOk)
	{
		bool_t Memory = GetValue(Form,TweakMemory);
		bool_t Borderless = GetValue(Form,TweakBorderless);
		bool_t TrippleBuffer = GetValue(Form,TweakTrippleBuffer);
		bool_t Battery = GetValue(Form,TweakBattery);

		FrmReturnToForm(MainForm);

		Tweak(ADVANCED_ID,ADVANCED_MEMORYOVERRIDE,&Memory,1);
		Tweak(HIRES_ID,HIRES_USEBORDER,&Borderless,1);
		Tweak(HIRES_ID,HIRES_TRIPLEBUFFER,&TrippleBuffer,1);
		EvtOverride((Boolean)Battery);
		
		return 1;
	}
	return DummyHandler(Form,Event);
}

static void EqInit(FormType* Form,bool_t Draw);
static Boolean EqHandler( EventPtr Event )
{
	bool_t b;
	int n,i;
	node* Eq = NodeEnumObject(NULL,EQUALIZER_ID);
	FormType* Form = FrmGetFormPtr(EqForm);
	if (Event->eType == ctlSelectEvent && Event->data.ctlSelect.controlID == EqReset)
	{
		i=0;
		for (n=0;n<10;++n)
			Eq->Set(Eq,EQUALIZER_1+n,&i,sizeof(i));
		Eq->Set(Eq,EQUALIZER_PREAMP,&i,sizeof(i));
		EqInit(Form,1);
	}
	if (Event->eType == ctlSelectEvent)
	{
		switch (Event->data.ctlSelect.controlID)
		{
		case EqEnable:
			b = GetValue(Form,EqEnable);
			Eq->Set(Eq,EQUALIZER_ENABLED,&b,sizeof(b));
			break;
		case EqAttenuate:
			b = GetValue(Form,EqAttenuate);
			Eq->Set(Eq,EQUALIZER_ATTENUATE,&b,sizeof(b));
			break;
		}
	}
	if (Event->eType == ctlRepeatEvent)
	{
		int n = Event->data.ctlRepeat.controlID;
		switch (n)
		{
		case EqPreamp:
			i = (Int16)Event->data.ctlRepeat.value-20;
			SetFieldInt(Form,EqPreampValue,i,1);
			Eq->Set(Eq,EQUALIZER_PREAMP,&i,sizeof(i));
			break;

		case Eq1:
		case Eq2:
		case Eq3:
		case Eq4:
		case Eq5:
		case Eq6:
		case Eq7:
		case Eq8:
		case Eq9:
		case Eq10:
			i = (Int16)Event->data.ctlRepeat.value-20;
			SetFieldInt(Form,(UInt16)(Eq1Value+(n-Eq1)),i,1);
			Eq->Set(Eq,EQUALIZER_1+(n-Eq1),&i,sizeof(i));
			break;
		}
	}
	return DummyHandler(Form,Event);
}

static Boolean AdvHandler( EventPtr Event )
{
	FormType* Form = FrmGetFormPtr(AdvForm);
	if (Event->eType == ctlSelectEvent && Event->data.ctlSelect.controlID == DummyOk)
	{
		bool_t b,b2;
		node* Advanced = Context()->Advanced;

		b = GetValue(Form,AdvNoBatteryWarning);
		Advanced->Set(Advanced,ADVANCED_NOBATTERYWARNING,&b,sizeof(b));

		b = GetValue(Form,AdvNoEventChecking);
		Advanced->Set(Advanced,ADVANCED_NOEVENTCHECKING,&b,sizeof(b));
		NoEventChecking = b;

		b = GetValue(Form,AdvCardPlugins);
		Advanced->Get(Advanced,ADVANCED_CARDPLUGINS,&b2,sizeof(b2));
		if (b != b2)
		{
			Advanced->Set(Advanced,ADVANCED_CARDPLUGINS,&b,sizeof(b));
			ShowMessage(NULL,LangStr(SETTINGS_ID,SETTINGS_RESTART));
		}

		b = GetValue(Form,AdvKeyFollowDir);
		Advanced->Set(Advanced,ADVANCED_KEYFOLLOWDIR,&b,sizeof(b));

		b = GetValue(Form,AdvAVIFrameRate);
		Advanced->Set(Advanced,ADVANCED_AVIFRAMERATE,&b,sizeof(b));

		b = GetValue(Form,AdvBenchFromPos);
		Advanced->Set(Advanced,ADVANCED_BENCHFROMPOS,&b,sizeof(b));

		b = GetValue(Form,AdvNoDeblocking);
		Advanced->Set(Advanced,ADVANCED_NODEBLOCKING,&b,sizeof(b));

		b = GetValue(Form,AdvBlinkLED);
		Advanced->Set(Advanced,ADVANCED_BLINKLED,&b,sizeof(b));

		b = GetValue(Form,AdvSmoothZoom);
		Player->Set(Player,PLAYER_SMOOTHALWAYS,&b,sizeof(b));

		b = GetValue(Form,AdvPreRotate);
		Player->Set(Player,PLAYER_AUTOPREROTATE,&b,sizeof(b));

		FrmReturnToForm(MainForm);
		return 1;
	}
	return DummyHandler(Form,Event);
}

static Boolean PrefHandler( EventPtr Event )
{
	FormType* Form = FrmGetFormPtr(PrefForm);
	node* Color;
	fraction f;
	bool_t b;
	tick_t t;
	int i;

	if (DummyHandler(Form,Event))
		return 1;

	if (Event->eType == ctlRepeatEvent)
	{
		Color = NodeEnumObject(NULL,COLOR_ID);
		switch (Event->data.ctlRepeat.controlID)
		{
		case PrefVideoBrightness:
			i = (Int16)Event->data.ctlRepeat.value-64;
			SetFieldInt(Form,PrefVideoBrightnessValue,i,1); i*=2;
			Color->Set(Color,COLOR_BRIGHTNESS,&i,sizeof(i));
			break;

		case PrefVideoContrast:
			i = (Int16)Event->data.ctlRepeat.value-64;
			SetFieldInt(Form,PrefVideoContrastValue,i,1); i*=2;
			Color->Set(Color,COLOR_CONTRAST,&i,sizeof(i));
			break;

		case PrefVideoSaturation:
			i = (Int16)Event->data.ctlRepeat.value-64;
			SetFieldInt(Form,PrefVideoSaturationValue,i,1); i*=2;
			Color->Set(Color,COLOR_SATURATION,&i,sizeof(i));
			break;

		case PrefVideoRed:
			i = (Int16)Event->data.ctlRepeat.value-32;
			SetFieldInt(Form,PrefVideoRedValue,i,1); i*=2;
			Color->Set(Color,COLOR_R_ADJUST,&i,sizeof(i));
			break;

		case PrefVideoGreen:
			i = (Int16)Event->data.ctlRepeat.value-32;
			SetFieldInt(Form,PrefVideoGreenValue,i,1); i*=2;
			Color->Set(Color,COLOR_G_ADJUST,&i,sizeof(i));
			break;

		case PrefVideoBlue:
			i = (Int16)Event->data.ctlRepeat.value-32;
			SetFieldInt(Form,PrefVideoBlueValue,i,1); i*=2;
			Color->Set(Color,COLOR_B_ADJUST,&i,sizeof(i));
			break;

		case PrefAudioVolume:
			i = (Int16)Event->data.ctlRepeat.value;
			Player->Set(Player,PLAYER_VOLUME,&i,sizeof(i));
			SetFieldInt(Form,PrefAudioVolumeValue,i,1);
			break;

		case PrefAudioPan:
			i = (Int16)Event->data.ctlRepeat.value; i-=50; i*=2;
			Player->Set(Player,PLAYER_PAN,&i,sizeof(i));
			SetFieldInt(Form,PrefAudioPanValue,i,1);
			break;

		case PrefAudioPreamp:
			i = (Int16)Event->data.ctlRepeat.value; i-=50; i*=2;
			Player->Set(Player,PLAYER_PREAMP,&i,sizeof(i));
			SetFieldInt(Form,PrefAudioPreampValue,i,1);
			break;

		case PrefGeneralSpeed:
			f.Num = (Int16)Event->data.ctlRepeat.value - 512;
			if (f.Num<0)
			{
				f.Num += 600;
				f.Den = 600;
			}
			else
			{
				f.Num += 512;
				f.Den = 512;
			}
			Player->Set(Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
			SetValue(Form,PrefGeneralSpeedCustom,1);
			SetValue(Form,PrefGeneralSpeedOrig,0);
			SetFieldInt(Form,PrefGeneralSpeedValue,Scale(f.Num,100,f.Den),1);
			break;
		}
	}

	if (Event->eType == popSelectEvent)
	{
		switch (Event->data.popSelect.listID)
		{
		case PrefAudioStereoList:
			i = Event->data.popSelect.selection;
			Player->Set(Player,PLAYER_STEREO,&i,sizeof(i));
			SetList(Form,PrefAudioStereo,PrefAudioStereoList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoQualityList:
			i = Event->data.popSelect.selection;
			Player->Set(Player,PLAYER_VIDEO_QUALITY,&i,sizeof(i));
			SetList(Form,PrefVideoQuality,PrefVideoQualityList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoFullRotateList:
			i = ToDir(Event->data.popSelect.selection);
			Player->Set(Player,PLAYER_FULL_DIR,&i,sizeof(i));
			SetList(Form,PrefVideoFullRotate,PrefVideoFullRotateList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoSkinRotateList:
			i = ToDir(Event->data.popSelect.selection);
			Player->Set(Player,PLAYER_SKIN_DIR,&i,sizeof(i));
			SetList(Form,PrefVideoSkinRotate,PrefVideoSkinRotateList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoSkinZoomList:
			ToZoom(Event->data.popSelect.selection,&f);
			Player->Set(Player,PLAYER_SKIN_ZOOM,&f,sizeof(f));
			SetList(Form,PrefVideoSkinZoom,PrefVideoSkinZoomList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoFullZoomList:
			ToZoom(Event->data.popSelect.selection,&f);
			Player->Set(Player,PLAYER_FULL_ZOOM,&f,sizeof(f));
			SetList(Form,PrefVideoFullZoom,PrefVideoFullZoomList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoAspectList:
			ToAspect(Event->data.popSelect.selection,&f);
			Player->Set(Player,PLAYER_ASPECT,&f,sizeof(f));
			SetList(Form,PrefVideoAspect,PrefVideoAspectList,Event->data.popSelect.selection,1);
			break;

		case PrefVideoDriverList:

			i = Event->data.popSelect.selection;
			b = 0;
			
			if (i<0 || i>=ARRAYCOUNT(VideoDriver,int)-1)
				i = 0;
			else
			{
				b = ARRAYBEGIN(VideoDriver,int)[i+1] == ARRAYBEGIN(VideoDriver,int)[i];
				i = ARRAYBEGIN(VideoDriver,int)[i];
			}

			Player->Set(Player,PLAYER_VIDEO_ACCEL,&b,sizeof(b));
			Player->Set(Player,PLAYER_VOUTPUTID,&i,sizeof(i));

			UpdateVideoCaps(Form);
			SetList(Form,PrefVideoDriver,PrefVideoDriverList,Event->data.popSelect.selection,1);
			break;
		}
	}

	if (Event->eType == frmCloseEvent)
	{
		ArrayClear(&VideoDriver);
		ArrayClear(&VideoDriverNames);
	}

	if (Event->eType == ctlSelectEvent)
	{
		switch (Event->data.ctlSelect.controlID)
		{
		case PrefGeneral:
		case PrefAudio:
		case PrefVideo:
			if (PrefCurrPage != Event->data.ctlSelect.controlID)
				PrefPage(Form,Event->data.ctlSelect.controlID);
			return 1;

		case PrefClose:
			if (GetFieldInt(Form,PrefGeneralMoveBack,&i))
			{
				t = i*TICKSPERSEC;
				Player->Set(Player,PLAYER_MOVEBACK_STEP,&t,sizeof(t));
			}
			if (GetFieldInt(Form,PrefGeneralMoveFFwd,&i))
			{
				t = i*TICKSPERSEC;
				Player->Set(Player,PLAYER_MOVEFFWD_STEP,&t,sizeof(t));
			}
			if (GetFieldInt(Form,PrefGeneralAVOffset,&i))
			{
				t = Scale(i,TICKSPERSEC,1000);
				Context()->Advanced->Set(Context()->Advanced,ADVANCED_AVOFFSET,&t,sizeof(t));
			}

			ArrayClear(&VideoDriver);
			ArrayClear(&VideoDriverNames);

			FrmReturnToForm(MainForm);
			return 1;

		case PrefVideoDither:
			Color = NodeEnumObject(NULL,COLOR_ID);
			b = CtlGetValue(Event->data.ctlSelect.pControl) != 0;
			Color->Set(Color,COLOR_DITHER,&b,sizeof(b));
			return 1;

		case PrefAudioDisable:
			b = CtlGetValue(Event->data.ctlSelect.pControl) != 0;
			if (b)
				i=0;
			else
				i=NodeEnumClass(NULL,AOUT_CLASS);
			Player->Set(Player,PLAYER_AOUTPUTID,&i,sizeof(i));
			return 1;
			
		case PrefGeneralSpeedOrig:
			f.Num = 1;
			f.Den = 1;
			Player->Set(Player,PLAYER_PLAY_SPEED,&f,sizeof(f));
			SetValue(Form,PrefGeneralSpeed,512);
			SetFieldInt(Form,PrefGeneralSpeedValue,Scale(f.Num,100,f.Den),1);
			return 1;

		case PrefGeneralRepeat:
			b = CtlGetValue(Event->data.ctlSelect.pControl) != 0;
			Player->Set(Player,PLAYER_REPEAT,&b,sizeof(b));
			return 1;

		case PrefGeneralShuffle:
			b = CtlGetValue(Event->data.ctlSelect.pControl) != 0;
			Player->Set(Player,PLAYER_SHUFFLE,&b,sizeof(b));
			return 1;

		case PrefAudioPreampReset:
			i = 0;
			Player->Set(Player,PLAYER_PREAMP,&i,sizeof(i));
			SetFieldInt(Form,PrefAudioPreampValue,i,1); i>>=1;
			SetValue(Form,PrefAudioPreamp,i+50); 
			return 1;

		case PrefAudioPanReset:
			i = 0;
			Player->Set(Player,PLAYER_PAN,&i,sizeof(i));
			SetFieldInt(Form,PrefAudioPanValue,i,1); i>>=1;
			SetValue(Form,PrefAudioPan,i+50); 
			return 1;
		}
	}

	return 0;
}

typedef struct openitem
{
	bool_t Checked;
	bool_t Selected;
	int Image;
	tchar_t FileName[MAXPATH];
	tchar_t Name[64];
	
} openitem;

typedef struct openfile
{
	array List;
	int Count;
	int First;
	int FirstMax;
	int PageSize;
	int ItemHeight;
	int Mouse;
	bool_t MouseDir;
	FormType* Form;
	tchar_t Path[MAXPATH];
	tchar_t Last[MAXPATH];
	tchar_t Exts[1024];
	MemPtr Checked[2];
	MemPtr Image[2];

} openfile;

static openfile* OpenFile = NULL;

static void OpenClear(openfile* p)
{
	ArrayDrop(&p->List);
	p->Count = 0;
	p->First = 0;
}

static void OpenDraw(openfile* p,int Pos)
{
	int n;
	RectangleType Rect;
	RectangleType OldClip;

	WinGetClip( &OldClip );
	WinPushDrawState();

	GetObjectBounds(p->Form,FrmGetObjectIndex(p->Form,OpenList),&Rect);
	Rect.extent.y = (Coord)p->ItemHeight;

	for (n=0;n<p->PageSize;++n)
	{
		if (Pos<0 || n+p->First==Pos)
		{
			UIColorTableEntries c;
			openitem* i = NULL;
			if (n+p->First < p->Count)
				i = ARRAYBEGIN(p->List,openitem)+n+p->First;

			WinSetClip(&Rect);

			if (i && i->Selected)
				c = UIMenuSelectedFill;
			else
			if (i && i->Checked)
				c = UIFieldTextHighlightBackground;
			else
				c = UIFormFill;

			WinSetBackColor(UIColorGetTableEntryIndex(c));
			WinEraseRectangle(&Rect,0);

			if (i)
			{
				if (i->Selected)
					c = UIMenuSelectedForeground;
				else
				if (i->Checked)
					c = UIFieldTextHighlightForeground;
				else
					c = UIFieldText;

				WinSetTextColor(UIColorGetTableEntryIndex(c));
				WinSetForeColor(UIColorGetTableEntryIndex(c));
				WinDrawChars(i->Name,(Int16)tcslen(i->Name),(Coord)(Rect.topLeft.x+24),Rect.topLeft.y);

				if (i->Image != OpenImageUp)
				{
					BitmapType* Bitmap = p->Checked[i->Checked];
					if (Bitmap)
    					WinDrawBitmap(Bitmap, Rect.topLeft.x, Rect.topLeft.y);
				}

				if (i->Image == OpenImageDir || i->Image == OpenImageUp)
				{
					BitmapType* Bitmap = NULL;
					switch (i->Image)
					{
					case OpenImageDir: Bitmap = p->Image[0]; break;
					case OpenImageUp: Bitmap = p->Image[1]; break;
					}
    				if (Bitmap)
						WinDrawBitmap(Bitmap, (Coord)(Rect.topLeft.x+12), (Coord)(Rect.topLeft.y));
				}
			}
		}
		Rect.topLeft.y = (Coord)(Rect.topLeft.y+p->ItemHeight);
	}

	WinPopDrawState();
	WinSetClip( &OldClip );
}

static Boolean OpenListHandler(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP)
{
	if (cmd == formGadgetDrawCmd && OpenFile)
	{
		OpenDraw(OpenFile,-1);
		return 1;
	}
	return 0;
}

static Boolean OpenLineHandler(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP)
{
	if (cmd == formGadgetDrawCmd && OpenFile)
	{
		RectangleType Rect;
		GetObjectBounds(OpenFile->Form,FrmGetObjectIndex(OpenFile->Form,OpenLine),&Rect);
		WinDrawRectangle(&Rect,0);
		return 1;
	}
	return 0;
}

static void OpenPost(int Cmd)
{
	EventType e;
	memset(&e,0,sizeof(e));
	e.eType = ctlSelectEvent;
	e.data.ctlSelect.controlID = (UInt16)Cmd;
	_EvtAddUniqueEventToQueue(&e, 0, 1);
}

static bool_t OpenIsSelected(openfile* p)
{
	openitem* i;
	for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
		if (i->Checked)
			return 1;
	return 0;
}

static void OpenPageSize(openfile* p)
{
	RectangleType Bounds;
	GetObjectBounds(p->Form,FrmGetObjectIndex(p->Form,OpenList),&Bounds);
	p->PageSize = Bounds.extent.y / p->ItemHeight;
}

static void OpenAdd(openfile* p)
{
	if (Player && OpenIsSelected(p))
	{
		bool_t b;
		int n;
		openitem* i;

		b = 0;
		Player->Set(Player,PLAYER_PLAY,&b,sizeof(b));
		n = 0;
		Player->Set(Player,PLAYER_LIST_COUNT,&n,sizeof(n));

		for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
			if (i->Checked)
			{
				tchar_t URL[MAXPATH];

				if (i->FileName[0])
					AbsPath(URL,TSIZEOF(URL),i->FileName,p->Path);
				else
					tcscpy_s(URL,TSIZEOF(URL),p->Path);

				if (i->Image == OpenImageDir)
					n = PlayerAddDir(Player,n,URL,p->Exts,1,0);
				else
					n = PlayerAdd(Player,n,URL,NULL);
			}

		n=0;
		Player->Set(Player,PLAYER_LIST_CURRIDX,&n,sizeof(n));
	}
}

static void OpenUpdate(openfile* p,bool_t Silent,int ListMode);
static void OpenSetScroll(openfile* p);

static void OpenInit(FormType* Form)
{
	openfile* p = OpenFile = malloc(sizeof(openfile));
	if (p)
	{
		bool_t HasHost;
		memset(p,0,sizeof(openfile));
		p->Form = Form;
		p->ItemHeight = 11;
		p->Mouse = -1;

		p->Checked[0] = MemHandleLock(DmGetResource(bitmapRsc,OpenImageUnchecked));
		p->Checked[1] = MemHandleLock(DmGetResource(bitmapRsc,OpenImageChecked));
		p->Image[0] = MemHandleLock(DmGetResource(bitmapRsc,OpenImageDir));
		p->Image[1] = MemHandleLock(DmGetResource(bitmapRsc,OpenImageUp));

		Player->Get(Player,PLAYER_CURRENTDIR,p->Path,sizeof(p->Path));
		GenExts(p->Exts,TSIZEOF(p->Exts));
		GetMime(p->Path,NULL,0,&HasHost);
		OpenPageSize(p);
		OpenUpdate(p,1,HasHost?0:2);

		FrmSetGadgetHandler(Form, FrmGetObjectIndex(Form, OpenList),OpenListHandler);
		FrmSetGadgetHandler(Form, FrmGetObjectIndex(Form, OpenLine),OpenLineHandler);
		//FrmGlueNavObjectTakeFocus(Form, OpenOk); //crashes in longterm at program exit!
	}
}

static void OpenDone(openfile* p)
{
	int i;
	for (i=0;i<2;++i)
		if (p->Checked[i]) 
			MemPtrUnlock(p->Checked[i]);
	for (i=0;i<2;++i)
		if (p->Image[i]) 
			MemPtrUnlock(p->Image[i]);

	Player->Set(Player,PLAYER_CURRENTDIR,p->Path,sizeof(p->Path));
	ArrayClear(&p->List);
}

static void OpenResize(Coord Width,Coord Height)
{
	openfile* p = OpenFile;
	if (p)
	{
		ObjectPosition(p->Form,OpenURL,2,158,16,-1,Width,Height);
		ObjectPosition(p->Form,OpenLine,0,152,28,-1,Width,Height);
		ObjectPosition(p->Form,OpenList,0,152,30,140,Width,Height);
		ObjectPosition(p->Form,OpenScroll,-1,159,30,140,Width,Height);

		ObjectPosition(p->Form,OpenOk,2,-1,-1,143+15,Width,Height);
		ObjectPosition(p->Form,OpenAll,42,-1,-1,143+15,Width,Height);
		ObjectPosition(p->Form,OpenNone,82,-1,-1,143+15,Width,Height);
		ObjectPosition(p->Form,OpenCancel,122,-1,-1,143+15,Width,Height);

		OpenPageSize(p);
		OpenSetScroll(p);
	}
}

static int OpenFindPos(openfile* p,int x,int y,bool_t* Dir)
{
	openitem* Item;
	int n;
	RectangleType Rect;
	GetObjectBounds(p->Form,FrmGetObjectIndex(p->Form,OpenList),&Rect);

	x -= Rect.topLeft.x;
	y -= Rect.topLeft.y;

	if (x<0 || x>=Rect.extent.x ||
		y<0 || y>=p->PageSize*p->ItemHeight)
		return -1;

	n = y/p->ItemHeight;
	n += p->First;
	if (n>=p->Count)
		return -1;

	Item = ARRAYBEGIN(p->List,openitem)+n;
	if (Dir)
	{
		if (Item->Image == OpenImageUp)
			*Dir = 1;
		else
			*Dir = Item->Image == OpenImageDir && (Rect.topLeft.x + 14 < x);
	}
	return n;
}

static Boolean OpenHandler( EventPtr Event )
{
	FormType* Form = FrmGetFormPtr(OpenForm);
	openfile* p = OpenFile;
	openitem* i;
	openitem* Item;
	bool_t Selected;

	if (DummyHandler(Form,Event))
		return 1;

	if (!p && Event->eType == ctlSelectEvent)
	{
		FrmReturnToForm(MainForm);
	}

	if (p)
		switch (Event->eType)
		{
			case keyUpEvent:
			case keyDownEvent:
				if (KeyUp(Event)>0)
				{
					p->First -= p->PageSize-1;
					OpenSetScroll(p);
					OpenDraw(p,-1);
					return 1;
				}

				if (KeyDown(Event)>0)
				{
					p->First += p->PageSize-1;
					OpenSetScroll(p);
					OpenDraw(p,-1);
					return 1;
				}
				break;

			case penUpEvent:
				if (p->Mouse>=0)
				{
					Item = ARRAYBEGIN(p->List,openitem)+p->Mouse;
					Selected = OpenFindPos(p,Event->screenX,Event->screenY,NULL) == p->Mouse;
					if (Selected != Item->Selected && !p->MouseDir)
						Item->Checked = !Item->Checked;
					Item->Selected = 0;
					OpenDraw(p,p->Mouse);

					p->Mouse = -1;
					if (Selected)
						SndPlaySystemSound(sndClick);

					if (Selected && p->MouseDir)
					{
						if (Item->Image == OpenImageUp)
						{
							if (UpperPath(p->Path,p->Last,TSIZEOF(p->Last))) 
								OpenUpdate(p,1,1);
						}
						else
						{
							tchar_t Path[MAXPATH];
							AbsPath(Path,TSIZEOF(Path),Item->FileName,p->Path);
							tcscpy_s(p->Path,TSIZEOF(p->Path),Path);
							p->Last[0] = 0;
							OpenUpdate(p,1,1);
						}
					}
					return 1;
				}
				break;

			case penMoveEvent:
				if (p->Mouse>=0)
				{
					Item = ARRAYBEGIN(p->List,openitem)+p->Mouse;
					Selected = OpenFindPos(p,Event->screenX,Event->screenY,NULL) == p->Mouse;
					if (Selected != Item->Selected)
					{
						if (!p->MouseDir) Item->Checked = !Item->Checked;
						Item->Selected = Selected;
						OpenDraw(p,p->Mouse);
					}
				}
				break;

			case penDownEvent:
				p->Mouse = OpenFindPos(p,Event->screenX,Event->screenY,&p->MouseDir);
				if (p->Mouse>=0)
				{
					Item = ARRAYBEGIN(p->List,openitem)+p->Mouse;
					Item->Selected = 1;
					if (!p->MouseDir) Item->Checked = !Item->Checked;
					OpenDraw(p,p->Mouse);
					return 1;
				}
				break;

			case sclRepeatEvent:
				p->First = Event->data.sclRepeat.newValue;
				OpenDraw(p,-1);
				break;

			case ctlSelectEvent:

				if (Event->data.ctlSelect.controlID == OpenAll)
				{
					bool_t All = 1;
					for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
						if (!i->Checked && i->Image != OpenImageUp && i->Image != OpenImageDir)
						{
							All = 0;
							break;
						}

					for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
						if (i->Image != OpenImageUp && (i->Image != OpenImageDir || All))
							i->Checked = 1;

					OpenDraw(p,-1);
				}

				if (Event->data.ctlSelect.controlID == OpenNone)
				{
					for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
						i->Checked = 0;
					OpenDraw(p,-1);
				}

				if (Event->data.ctlSelect.controlID == OpenOk || 
					Event->data.ctlSelect.controlID == OpenCancel)
				{
					if (Event->data.ctlSelect.controlID == OpenOk)
						OpenAdd(p);
				
					OpenDone(p);
					free(p);
					OpenFile = NULL;

					FrmReturnToForm(MainForm);
					return 1;
				}
				break;
			default:
				break;
		}

	return 0;
}

static int OpenCmp(const void* pa,const void* pb)
{
	const openitem* a = (const openitem*)pa;
	const openitem* b = (const openitem*)pb;
	int Result;

	if (a->Image==OpenImageUp) return -1;
	if (b->Image==OpenImageUp) return 1;

	if (a->Image==OpenImageDir)
		if (b->Image==OpenImageDir)
			Result = tcsicmp(a->Name,b->Name);
		else
			Result = -1;
	else
	if (b->Image==OpenImageDir)
		Result = 1;
	else
		Result = tcsicmp(a->Name,b->Name); 
	
	return Result;
}

static void OpenSetScroll(openfile* p)
{
	if (p->Count > p->PageSize)
		p->FirstMax = p->Count - p->PageSize;
	else
		p->FirstMax = 0;

	if (p->First < 0)
		p->First = 0;
	if (p->First > p->FirstMax)
		p->First = p->FirstMax;

	SclSetScrollBar(FrmGetObjectPtr(p->Form,FrmGetObjectIndex(p->Form,OpenScroll)),
		(Int16)p->First,0,(Int16)p->FirstMax,(Int16)(p->PageSize-1));
}

static void OpenUpdate(openfile* p,bool_t Silent,int ListMode)
{
	streamdir DirItem;
	stream* Stream;
	openitem New;
	int Result;
	bool_t Draw = FrmVisible(p->Form);

	FieldType* Field = (FieldType*)FrmGetObjectPtr(p->Form,FrmGetObjectIndex(p->Form,OpenURL));
	FldSetTextPtr(Field,p->Path[0] ? p->Path : T("/"));

	OpenClear(p);

	if (!ListMode)
	{
		if (Draw)
			FldDrawField(Field);
		return;
	}

	memset(&New,0,sizeof(New));

	Stream = GetStream(p->Path,Silent);

	if (Stream)
	{
		bool_t HasHost;
		if (*GetMime(p->Path,NULL,0,&HasHost) || (!HasHost && p->Path[0]))
		{
			New.Image = OpenImageUp;
			New.Checked = 0;
			tcscpy_s(New.FileName,TSIZEOF(New.FileName),T(".."));
			tcscpy_s(New.Name,TSIZEOF(New.Name),New.FileName);
			if (ArrayAppend(&p->List,&New,sizeof(New),4096))
				++p->Count;
		}

		Result = Stream->EnumDir(Stream,p->Path,p->Exts,1,&DirItem);

		if (ListMode==2 && Result == ERR_FILE_NOT_FOUND && UpperPath(p->Path,p->Last,TSIZEOF(p->Last)))
		{
			OpenUpdate(p,Silent,ListMode);
			return;
		}

		if (Result == ERR_NOT_DIRECTORY && !Silent)
		{
			tcscpy_s(New.FileName,TSIZEOF(New.FileName),DirItem.FileName);
			New.Image = OpenImageFile;
			New.Checked = 1;
			if (ArrayAppend(&p->List,&New,sizeof(New),4096))
				++p->Count;
			OpenPost(OpenOk);
		}
	
		while (Result == ERR_NONE)
		{
			tchar_t Name[MAXPATH];

			openitem* i;
			for (i=ARRAYBEGIN(p->List,openitem);i!=ARRAYEND(p->List,openitem);++i)
				if (tcscmp(i->FileName,DirItem.FileName)==0)
					break;

			if (i==ARRAYEND(p->List,openitem))
			{
				New.Checked = 0;
				tcscpy_s(New.FileName,TSIZEOF(New.FileName),DirItem.FileName);

				if (DirItem.Size < 0)
				{
					int Len;
					New.Image = OpenImageDir;
					tcscpy_s(Name,TSIZEOF(Name),New.FileName);

					Len = tcslen(Name);
					if (Len>=3 && tcscmp(Name+Len-3,T("://"))==0)
						Name[Len-3] = 0;
				}
				else
				{
					switch (DirItem.Type)
					{
					case FTYPE_AUDIO: New.Image = OpenImageAudio; break;
					case FTYPE_PLAYLIST: New.Image = OpenImagePlayList; break;
					case FTYPE_VIDEO: New.Image = OpenImageVideo; break;
					default: New.Image = OpenImageFile; break;
					}
					SplitURL(New.FileName,NULL,0,NULL,0,Name,TSIZEOF(Name),Name,TSIZEOF(Name));
				}

				tcscpy_s(New.Name,TSIZEOF(New.Name),Name);
		
				if (ArrayAppend(&p->List,&New,sizeof(New),4096))
					++p->Count;
			}
			Result = Stream->EnumDir(Stream,NULL,NULL,0,&DirItem);
		}

		NodeDelete((node*)Stream);
	}

	qsort(ARRAYBEGIN(p->List,openitem),p->Count,sizeof(openitem),OpenCmp);

	p->First = 0;
	OpenSetScroll(p);

	if (Draw)
	{
		FldDrawField(Field);
		OpenDraw(p,-1);
	}
}

static void EqInit(FormType* Form,bool_t Draw)
{
	node* Eq = NodeEnumObject(NULL,EQUALIZER_ID);\
	bool_t b;
	int n,i;
	for (n=0;n<10;++n)
	{
		Eq->Get(Eq,EQUALIZER_1+n,&i,sizeof(i));
		SetValue(Form,Eq1+n,i+20);
		SetFieldInt(Form,(UInt16)(Eq1Value+n),i,Draw);
	}

	Eq->Get(Eq,EQUALIZER_PREAMP,&i,sizeof(i));
	SetValue(Form,EqPreamp,i+20);
	SetFieldInt(Form,EqPreampValue,i,Draw);

	Eq->Get(Eq,EQUALIZER_ENABLED,&b,sizeof(b));
	SetValue(Form,EqEnable,b);

	Eq->Get(Eq,EQUALIZER_ATTENUATE,&b,sizeof(b));
	SetValue(Form,EqAttenuate,b);
}

static void AdvInit(FormType* Form)
{
	bool_t b;
	node* Advanced = Context()->Advanced;

	Advanced->Get(Advanced,ADVANCED_NOBATTERYWARNING,&b,sizeof(b));
	SetValue(Form,AdvNoBatteryWarning,b);

	Advanced->Get(Advanced,ADVANCED_KEYFOLLOWDIR,&b,sizeof(b));
	SetValue(Form,AdvKeyFollowDir,b);

	Advanced->Get(Advanced,ADVANCED_AVIFRAMERATE,&b,sizeof(b));
	SetValue(Form,AdvAVIFrameRate,b);

	Advanced->Get(Advanced,ADVANCED_BENCHFROMPOS,&b,sizeof(b));
	SetValue(Form,AdvBenchFromPos,b);

	Advanced->Get(Advanced,ADVANCED_NODEBLOCKING,&b,sizeof(b));
	SetValue(Form,AdvNoDeblocking,b);

	Advanced->Get(Advanced,ADVANCED_BLINKLED,&b,sizeof(b));
	SetValue(Form,AdvBlinkLED,b);

	Advanced->Get(Advanced,ADVANCED_CARDPLUGINS,&b,sizeof(b));
	SetValue(Form,AdvCardPlugins,b);

	SetValue(Form,AdvNoEventChecking,NoEventChecking);

	Player->Get(Player,PLAYER_SMOOTHALWAYS,&b,sizeof(b));
	SetValue(Form,AdvSmoothZoom,b);

	Player->Get(Player,PLAYER_AUTOPREROTATE,&b,sizeof(b));
	SetValue(Form,AdvPreRotate,b);

	//FrmGlueNavObjectTakeFocus(Form, DummyOk); //crashes in longterm at program exit!
}

static void TweakInit(FormType* Form)
{
	bool_t v;

	if (!Tweak(ADVANCED_ID,ADVANCED_MEMORYOVERRIDE,&v,0))
		ShowObject(Form,TweakMemory,0);
	else
		SetValue(Form,TweakMemory,v);

	if (!Tweak(HIRES_ID,HIRES_USEBORDER,&v,0))
	{
		ShowObject(Form,TweakBorderless,0);
		ShowObject(Form,TweakBorderlessLabel1,0);
		ShowObject(Form,TweakBorderlessLabel2,0);
	}
	else
		SetValue(Form,TweakBorderless,v);

	if (!Tweak(HIRES_ID,HIRES_TRIPLEBUFFER,&v,0))
	{
		ShowObject(Form,TweakTrippleBuffer,0);
		ShowObject(Form,TweakTrippleBufferLabel1,0);
	}
	else
		SetValue(Form,TweakTrippleBuffer,v);

	SetValue(Form,TweakBattery,EvtGetOverride());

	//FrmGlueNavObjectTakeFocus(Form, DummyOk); //crashes in longterm at program exit!
}

static void AboutInit(FormType* Form)
{
	int Mhz=0;
	tchar_t s[40];
	uint32_t Max = 0;
	uint32_t Free = 0;

	node* Platform = Context()->Platform;

	stprintf_s(s,TSIZEOF(s),T("%s Version %s"),Context()->ProgramName,Context()->ProgramVersion);
	FrmCopyLabel(Form,AboutVersion,s);
	tcscpy_s(s,TSIZEOF(s),T("PalmOS "));
	Platform->Get(Platform,PLATFORM_VERSION,s+tcslen(s),sizeof(s)-tcslen(s)*sizeof(tchar_t));
	FrmCopyLabel(Form,AboutOSVersion,s);
	Platform->Get(Platform,PLATFORM_OEMINFO,s,sizeof(s));
	FrmCopyLabel(Form,AboutOEMInfo,s);
	Platform->Get(Platform,PLATFORM_CPU,s,sizeof(s));
	Platform->Get(Platform,PLATFORM_CPUMHZ,&Mhz,sizeof(Mhz));
	if (Mhz)
		stcatprintf_s(s,TSIZEOF(s),T(" ~%d Mhz"),Mhz);
	FrmCopyLabel(Form,AboutCPU,s);

	if (MemHeapFreeBytes(MemHeapID(0,1),&Free,&Max)==errNone)
	{
		stprintf_s(s,TSIZEOF(s),T("Storage heap: %dKB"),(int)Free/1024);

#ifdef HAVE_PALMONE_SDK
		{
			UInt32 Version;
			if (FtrGet(sysFtrCreator,sysFtrNumDmAutoBackup,&Version)==errNone && Version==1 &&
				MemHeapFreeBytes(1|dbCacheFlag,&Free,&Max)==errNone)
				stcatprintf_s(s,TSIZEOF(s),T(" (%dKB)"),(int)Free/1024);
		}
#endif

		FrmCopyLabel(Form,AboutMem1,s);
	}
	if (MemHeapFreeBytes(MemHeapID(0,0),&Free,&Max)==errNone)
	{
		stprintf_s(s,TSIZEOF(s),T("Dynamic heap: %dKB"),(int)Free/1024);
		FrmCopyLabel(Form,AboutMem2,s);
	}

	//FrmGlueNavObjectTakeFocus(Form, DummyOk); //crashes in longterm at program exit!
}

static void MediaInfoInit(FormType* Form)
{
	tchar_t Rate[16];
	tchar_t s[40];
	node* Format = NULL;
	node* VOutput = NULL;
	packetformat Packet;
	pin Pin;
	int i,j;

	s[0] = 0;
	Player->Get(Player,PLAYER_LIST_CURRENT,&i,sizeof(i));
	Player->Get(Player,PLAYER_LIST_URL+i,s,sizeof(s));
	FrmCopyLabel(Form,MediaInfoURL,s);

	Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format));
	if (Format)
	{
		tcscpy_s(s,sizeof(s),LangStr(Format->Class,NODE_NAME));
		FrmCopyLabel(Form,MediaInfoFormat,s);
		
		Player->Get(Player,PLAYER_VSTREAM,&i,sizeof(i));
		if (i>=0 && Format->Get(Format,(FORMAT_STREAM+i)|PIN_FORMAT,&Packet,sizeof(Packet))==ERR_NONE &&
			Packet.Type == PACKET_VIDEO)
		{
			stprintf_s(s,TSIZEOF(s),"%dx%d ",Packet.Format.Video.Width,Packet.Format.Video.Height);
			if (Packet.PacketRate.Num)
			{
				FractionToString(Rate,TSIZEOF(Rate),&Packet.PacketRate,0,2);
				stcatprintf_s(s,TSIZEOF(s),"%s fps",Rate);
			}
			FrmCopyLabel(Form,MediaVideoFormat,s);
		}
		if (i>=0 && Format->Get(Format,(FORMAT_STREAM+i),&Pin,sizeof(Pin))==ERR_NONE && Pin.Node)
			FrmCopyLabel(Form,MediaVideoCodec,LangStr(Pin.Node->Class,NODE_NAME));

		Player->Get(Player,PLAYER_ASTREAM,&i,sizeof(i));
		if (i>=0 && Format->Get(Format,(FORMAT_STREAM+i)|PIN_FORMAT,&Packet,sizeof(Packet))==ERR_NONE &&
			Packet.Type == PACKET_AUDIO)
		{
			s[0]=0;
			if (Packet.Format.Audio.SampleRate)
				stcatprintf_s(s,TSIZEOF(s),"%dHz ",Packet.Format.Audio.SampleRate);
			if (Packet.Format.Audio.Bits)
				stcatprintf_s(s,TSIZEOF(s),"%dBits ",Packet.Format.Audio.Bits);
			if (Packet.Format.Audio.Channels==1) tcscat_s(s,TSIZEOF(s),T("Mono"));
			if (Packet.Format.Audio.Channels==2) tcscat_s(s,TSIZEOF(s),T("Stereo"));
			FrmCopyLabel(Form,MediaAudioFormat,s);
		}

		if (i>=0 && Format->Get(Format,(FORMAT_STREAM+i),&Pin,sizeof(Pin))==ERR_NONE && Pin.Node)
		{
			tcscpy_s(s,sizeof(s),LangStr(Pin.Node->Class,NODE_NAME));
			FrmCopyLabel(Form,MediaAudioCodec,s);
		}

		Player->Get(Player,PLAYER_VOUTPUT,&VOutput,sizeof(VOutput));
		if (VOutput)
		{
			VOutput->Get(VOutput,OUT_TOTAL,&i,sizeof(int));
			VOutput->Get(VOutput,OUT_DROPPED,&j,sizeof(int));

			stprintf_s(s,TSIZEOF(s),T("Played frames: %d"),i+j);
			FrmCopyLabel(Form,MediaVideoTotal,s);
			stprintf_s(s,TSIZEOF(s),T("Dropped frames: %d"),j);
			FrmCopyLabel(Form,MediaVideoDropped,s);
		}
	}

	//FrmGlueNavObjectTakeFocus(Form, DummyOk); //crashes in longterm at program exit!
}

static void BenchmarkInit(FormType* Form)
{
	tchar_t v[16];
	tchar_t s[40];
	tick_t OrigTick = -1;
	tick_t Tick = -1;
	fraction f;
	int Frames = 0;
	node* Node;
	int i;

	Player->Get(Player,PLAYER_BENCHMARK,&Tick,sizeof(Tick));
	TickToString(v,TSIZEOF(v),Tick,0,1,0);
	stprintf_s(s,TSIZEOF(s),T("Benchmark time: %s"),v);
	FrmCopyLabel(Form,BenchmarkTime,s);

	Player->Get(Player,PLAYER_AOUTPUT,&Node,sizeof(Node));
	if (Node)
	{
		packetformat Audio;
		Audio.Type = PACKET_NONE;
		Node->Get(Node,OUT_INPUT|PIN_FORMAT,&Audio,sizeof(Audio));
		if (Audio.Type == PACKET_AUDIO && Audio.Format.Audio.Bits && Audio.Format.Audio.Channels && Audio.Format.Audio.SampleRate)
		{
			int Samples = 0;
			Node->Get(Node,OUT_TOTAL,&Samples,sizeof(int));
			Samples /= Audio.Format.Audio.Bits/8;
			if (!(Audio.Format.Audio.Flags & PCM_PLANES))
				Samples /= Audio.Format.Audio.Channels;
			OrigTick = Scale(Samples,TICKSPERSEC,Audio.Format.Audio.SampleRate);
		}
	}

	Player->Get(Player,PLAYER_VOUTPUT,&Node,sizeof(Node));
	if (Node)
	{
		Node->Get(Node,OUT_TOTAL,&Frames,sizeof(int));

		if (Frames)
		{
			f.Num = Frames;
			f.Den = Tick;
			Simplify(&f,MAX_INT/TICKSPERSEC,MAX_INT);
			f.Num *= TICKSPERSEC;
			FractionToString(v,TSIZEOF(v),&f,0,2);
			stprintf_s(s,TSIZEOF(s),T("Benchmark FPS: %s"),v);
			FrmCopyLabel(Form,BenchmarkFPS,s);

			Player->Get(Player,PLAYER_VSTREAM,&i,sizeof(i));
			if (i>=0) 
			{
				node* Format;
				packetformat Video;
				Video.Type = PACKET_NONE;
				Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format));
				if (Format)
					Format->Get(Format,(FORMAT_STREAM+i)|PIN_FORMAT,&Video,sizeof(Video));
				if (Video.Type == PACKET_VIDEO && Video.PacketRate.Num)
				{
					OrigTick = Scale64(Frames,(int64_t)Video.PacketRate.Den*TICKSPERSEC,Video.PacketRate.Num);

					FractionToString(v,TSIZEOF(v),&Video.PacketRate,0,2);
					stprintf_s(s,TSIZEOF(s),T("Original FPS: %s"),v);
					FrmCopyLabel(Form,BenchmarkOrigFPS,s);
				}
			}
		}
	}
	
	if (OrigTick >= 0)
	{
		TickToString(v,TSIZEOF(v),OrigTick,0,1,0);
		stprintf_s(s,TSIZEOF(s),T("Original time: %s"),v);
		FrmCopyLabel(Form,BenchmarkOrigTime,s);

		f.Num = OrigTick;
		f.Den = Tick;
		FractionToString(v,TSIZEOF(v),&f,1,2);
		stprintf_s(s,TSIZEOF(s),T("Average speed: %s"),v);
		FrmCopyLabel(Form,BenchmarkSpeed,s);
	}

	//FrmGlueNavObjectTakeFocus(Form, DummyOk); //crashes in longterm at program exit!
}

static bool_t HitObject(EventType* Event,FormType* Form,UInt16 Id,int dx0,int dx1,int dy0,int dy1)
{
	RectangleType Rect;
	GetObjectBounds(Form,FrmGetObjectIndex(Form,Id),&Rect);
	return Event->screenX >= Rect.topLeft.x + dx0 &&
	       Event->screenX < Rect.topLeft.x + Rect.extent.x + dx1 &&
	       Event->screenY >= Rect.topLeft.y + dy0 &&
	       Event->screenY < Rect.topLeft.y + Rect.extent.y + dy1;
}

static Boolean PenMainDummy(EventPtr Event)
{
	return 1;
}

static bool_t ExitFullScreen()
{
	if (FullScreen)
	{
		Toggle(PLAYER_PLAY,0);
		SetFullScreen(0);
		return 1;
	}
	return 0;
}

static void PlayAndFullScreen(bool_t ExitFullScreen)
{
	bool_t b;
	Player->Get(Player,PLAYER_PLAY,&b,sizeof(b));
	if (b)
	{
		Toggle(PLAYER_PLAY,0);
		if (FullScreen && ExitFullScreen)
			SetFullScreen(0);
	}
	else
	{
		if (IsVideoPrimary())
			SetFullScreen(1);
		Toggle(PLAYER_PLAY,1);
	}
}

static Boolean PenMainViewport(EventPtr Event)
{
	if (Event && Event->eType == penUpEvent)
	{
		if (FullScreen)
		{
			Toggle(PLAYER_PLAY,0);
			SetFullScreen(0);
		}
		else
		if (HitObject(Event,FrmGetFormPtr(MainForm),MainViewport,0,0,0,0))
			PlayAndFullScreen(1);
	}
	return 1;
}

static Boolean PenMainWinTitle(EventPtr Event)
{
	if (Event && Event->eType == penUpEvent &&
		HitObject(Event,FrmGetFormPtr(MainForm),MainWinTitle,0,0,0,0))
		_EvtEnqueueKey( menuChr, 0, commandKeyMask );
	return 1;
}

static NOINLINE void InSeek(bool_t b)
{
	Player->Set(Player,PLAYER_INSEEK,&b,sizeof(b));
}

static Boolean PenMainPosition(EventPtr Event)
{
	bool_t b;

	if (Event && Event->eType == penDownEvent)
		InSeek(1);

	if (Event && Player->Get(Player,PLAYER_INSEEK,&b,sizeof(b))==ERR_NONE && b)
	{
		fraction Percent;
		RectangleType Rect;
		FormType* Form = FrmGetFormPtr(MainForm);
		GetObjectBounds(Form,FrmGetObjectIndex(Form,MainPosition),&Rect);

		Percent.Num = Event->screenX - Rect.topLeft.x-4;
		Percent.Den = Rect.extent.x-8;
		Player->Set(Player,PLAYER_PERCENT,&Percent,sizeof(Percent));
	}
	
	if (Event==NULL || Event->eType == penUpEvent)
		InSeek(0);

	return 1;
}

static void CancelMouseDown()
{
	if (MouseDown)
	{
		MouseDown(NULL);
		MouseDown = NULL;
	}
}

static NOINLINE int KeyDir(EventType* Event,bool_t* NoUp)
{
	int i,d;

	if ((d = KeyUp(Event))!=0)
		i = 1;
	else
	if ((d = KeyRight(Event))!=0)
		i = 2;
	else
	if ((d = KeyDown(Event))!=0)
		i = 3;
	else
	if ((d = KeyLeft(Event))!=0)
		i = 4;
	else
		return 0;

	if (QueryAdvanced(ADVANCED_KEYFOLLOWDIR))
	{
		int Dir;
		int Offset;

		Player->Get(Player,PLAYER_REL_DIR,&Dir,sizeof(Dir));
		
		if (Dir & DIR_SWAPXY)
			Offset = (Dir & DIR_MIRRORUPDOWN) ? 1:3;
		else
			Offset = (Dir & DIR_MIRRORUPDOWN) ? 2:0;

		i += Offset;
		if (i>4) i-=4;
	}

	if (NoUp)
		*NoUp = d>1;

	if (d<0)
		i = -i;
	return i;
}

static void PopupForm(UInt16 Id)
{
	int i;
	for (i=0;i<1000;++i)
	{
		if (AvailMemory()<40000)
		{
			if (!NodeHibernate())
				break;
		}
		else
		{
			_FrmPopupForm(Id);
			return;
		}
	}
	ShowOutOfMemory();
}

static void CheckClipping(FormType* Form)
{
	if (WinGetActiveWindow() == FrmGetWindowHandle(Form))
		SetClipping(0);
}

static NOINLINE void Launch(const tchar_t* URL)
{
	bool_t b,b0;
	int n;
	
	n = 1;
	Player->Set(Player,PLAYER_LIST_COUNT,&n,sizeof(n));

	PlayerAdd(Player,0,URL,NULL);

	b = 1;
	Player->Get(Player,PLAYER_PLAYATOPEN_FULL,&b0,sizeof(b0));
	Player->Set(Player,PLAYER_PLAYATOPEN_FULL,&b,sizeof(b));

	n = 0;
	Player->Set(Player,PLAYER_LIST_CURRIDX,&n,sizeof(n));

	Player->Set(Player,PLAYER_PLAYATOPEN_FULL,&b0,sizeof(b0));
}

static bool_t IsParams()
{
	return LaunchParameters && (LaunchCode == sysAppLaunchCmdOpenDB || LaunchCode == sysAppLaunchCmdCustomBase);
}

static void ProcessParams()
{
	if (LaunchParameters)
	{
		tchar_t URL[MAXPATH];
		if (LaunchCode == sysAppLaunchCmdOpenDB)
		{
			SysAppLaunchCmdOpenDBType2* Param = (SysAppLaunchCmdOpenDBType2*)LaunchParameters;
			if (DBFrom(Param->cardNo,Param->dbID[0]+((UInt32)Param->dbID[1]<<16),URL,TSIZEOF(URL)))
				Launch(URL);
		}
		else
		if (LaunchCode == sysAppLaunchCmdCustomBase)
		{
			vfspath* Param = (vfspath*)LaunchParameters;
			if (VFSFromVol(Param->volRefNum,Param->path,URL,TSIZEOF(URL)))
				Launch(URL);
		}
	}
}

static Boolean MainFormEventHandler(EventPtr Event)
{
	FormType* Form = FrmGetFormPtr(MainForm);
	tick_t t;
	int i;
	fraction f;
	UInt16 Id;
	EventType e;

	if (DIAHandleEvent(Form,Event))
		return 1;

	switch (Event->eType)
	{
	case frmOpenEvent:
		DIASet(0,DIA_SIP);
		SIPState = DIAGet(DIA_ALL);
		NoPaintResize = -1;
		PushReDraw();
		return 1;

	case frmUpdateEvent:
		NoPaintResize = -1;
		PushReDraw();
		return 1;

	case winDisplayChangedEvent:
		MainFormResize(Form);
		return 1;

	case winEnterEvent:
		if (Event->data.winEnter.enterWindow == (WinHandle)Form && Form == FrmGetActiveForm())
		{
			if (FullScreen)
				DIASet(0,DIA_ALL);
			else
			if (MainDrawn && DIAGet(DIA_ALL) != SIPState)
				DIASet(SIPState,DIA_ALL);

			if (Context()->Wnd == NULL)
			{
				HandleEvents(); // make sure form is drawn
				if (IsParams())
				{
					bool_t b = 1;
					Player->Set(Player,PLAYER_DISCARDLIST,&b,sizeof(b));
				}
				Context_Wnd(Form);
				NoEventChecking = QueryAdvanced(ADVANCED_NOEVENTCHECKING);
				ProcessParams();
				Refresh(Form,1);
				SetClipping(0);
			}
			else
			{
				if (NoPaintResize==0)
					NoPaintResize = 1;
				RefreshResize = 1;
				PushReDraw();
			}
			CancelMouseDown();
		}
		return 1;

	case winExitEvent:
		if (Event->data.winExit.exitWindow == (WinHandle)Form)
		{
			CancelMouseDown();
			SetClipping(1);
			RefreshResize = 0;
		}
		return 1;

    case frmCloseEvent:
		Player->Set(Player,PLAYER_NOTIFY,NULL,0);
		FtrSet(Context()->ProgramId,20000,SkinNo);
		SkinFree(Skin,SkinDb);
		SetFullScreen(0);
		Context_Wnd(NULL);
		break;

	case menuEvent:
		switch (Event->data.menu.itemID)
		{
		case MainClear:
			i = 0;
			Player->Set(Player,PLAYER_LIST_COUNT,&i,sizeof(i));
			Player->Set(Player,PLAYER_STOP,NULL,0);
			for (i=0;i<1000;++i)
				if (!NodeHibernate()) // free everything
					break;
			PushReDraw();
			return 1;
		case MainOpenFiles:
			PopupForm(OpenForm);
			return 1;
		case MainSearchFiles:
			SearchFiles();
			return 1;
		case MainQuit:
			memset(&e,0,sizeof(e));
			e.eType = appStopEvent;
			_EvtAddUniqueEventToQueue(&e,0,1);
			return 1;
		case MainMenuPref:
			PopupForm(PrefForm);
			break;
		case MainAbout:
			PopupForm(AboutForm);
			return 1;
		case MainMenuTweaks:
			PopupForm(TweakForm);
			return 1;
		case MainMenuAdv:
			PopupForm(AdvForm);
			return 1;
		case MainMenuEq:
			PopupForm(EqForm);
			return 1;
		case MainBenchmark:
			if (IsVideoPrimary())
				SetFullScreen(1);
			t = 0;
			Player->Set(Player,PLAYER_BENCHMARK,&t,sizeof(t));
			return 1;
		case MainBenchmarkForm:
			PopupForm(BenchmarkForm);
			return 1;
		case MainMediaInfo:
			PopupForm(MediaInfoForm);
			return 1;
		}
		break;

	case penUpEvent:
		if (MouseDown)
		{
			MouseDown(Event);
			MouseDown = NULL;
			return 1;
		}
		break;

	case penMoveEvent:
		if (MouseDown)
		{
			MouseDown(Event);
			return 1;
		}
		break;

	case penDownEvent:
		CheckClipping(Form);
		if (FullScreen || HitObject(Event,Form,MainViewport,0,0,0,-5)) 
		{
			MouseDown = PenMainViewport;
			return 1;
		}
		if (HitObject(Event,Form,MainWinTitle,0,0,0,0))
		{
			MouseDown = PenMainWinTitle;
			return 1;
		}
		if (HitObject(Event,Form,MainPosition,-2,2,-5,0))
		{
			MouseDown = PenMainPosition;
			PenMainPosition(Event);
			return 1;
		}
		if (HitObject(Event,Form,MainTimer,0,0,0,0))
		{
			Toggle(PLAYER_TIMER_LEFT,-1);
			UpdatePosition(Form,1);
			MouseDown = PenMainDummy;
			return 1;
		}
		break;

	case ctlSelectEvent: 
		Id = Event->data.ctlSelect.controlID;
		switch (Id)
		{
		case MainPref:
			PopupForm(PrefForm);
			break;
		case MainFullScreen:
			if (IsVideoPrimary())
				SetFullScreen(1);
			else
			if (IsAudioPlayed())
			{
				SetDisplayPower(0,0);
				SetKeyboardBacklight(0);
			}
			break;
		case MainPlay:
			PlayerNotify(Form,PLAYER_PLAY,Toggle(PLAYER_PLAY,-1));
			break;
		case MainNext: 
			Player->Set(Player,PLAYER_NEXT,NULL,0);
			break;
		case MainOpen:
			PopupForm(OpenForm);
			break;
		case MainPrev:
			Player->Set(Player,PLAYER_PREV,NULL,1);
			break;
		case MainStop:
			Player->Set(Player,PLAYER_STOP,NULL,0);
			f.Num = 0;
			f.Den = 1;
			Player->Set(Player,PLAYER_PERCENT,&f,sizeof(f));
			break;
		}
		break;

	default:
		break;
	}
	return 0;
}

static void ApplicationPostEvent()
{
	if (AttnEvent)
	{
		AttnEvent = 0;
		if (FrmGetActiveForm() == FrmGetFormPtr(MainForm))
		{
			SetClipping(0);
		}
	}
}

static Boolean ApplicationHandleEvent(EventPtr Event)
{
	bool_t NoUp;
	if (Event->eType == keyDownEvent || Event->eType == keyUpEvent || Event->eType == keyHoldEvent)
	{
		FormType* Form = FrmGetFormPtr(MainForm);
		if (WinGetActiveWindow() == FrmGetWindowHandle(Form))
		{
			int i;
			if (Event->eType == keyDownEvent)
			{
				if (Event->data.keyDown.chr == vchrCalc && QueryPlatform(PLATFORM_MODEL)==MODEL_ZODIAC)
					Event->data.keyDown.chr = vchrMenu;
					
				if (Event->data.keyDown.chr == vchrLowBattery && QueryAdvanced(ADVANCED_NOBATTERYWARNING))
				{
					UInt16 Level,Critical;
					Level = SysBatteryInfo(0,NULL,&Critical,NULL,NULL,NULL,NULL);
					if (Level > Critical)
						return 1;
				}

				if (Event->data.keyDown.chr == vchrMenu || //menu will be opened
					Event->data.keyDown.chr == vchrLowBattery ||
#ifdef HAVE_PALMONE_SDK
					Event->data.keyDown.chr == vchrSilkClock ||
					Event->data.keyDown.chr == vchrClock ||
					Event->data.keyDown.chr == vchrPopupBrightness ||
#endif
					Event->data.keyDown.chr == vchrLaunch) //PSHack will open a password window
					SetClipping(1);
				else
				if (Event->data.keyDown.chr == vchrAttnStateChanged ||
					Event->data.keyDown.chr == vchrAttnIndicatorTapped ||
					Event->data.keyDown.chr == vchrAttnUnsnooze)
				{
					AttnEvent = 1;
					SetClipping(1);
				}
				else
					SetClipping(0);
			}

			switch (KeyDir(Event,&NoUp))
			{
			case 1:
				Delta(PLAYER_VOLUME,5,0,100);
				return 1;
			case 3:
				Delta(PLAYER_VOLUME,-5,0,100);
				return 1;
			case 2:
				KeyTrackNext = IsPlayList() && !KeyHold(Event);
				if (!KeyTrackNext)
				{
					if (!NoUp) InSeek(1);
					Player->Set(Player,PLAYER_MOVEFFWD,NULL,0);
				}
				return 1;
			case 4:
				KeyTrackPrev = IsPlayList() && !KeyHold(Event);
				if (!KeyTrackPrev)
				{
					if (!NoUp) InSeek(1);
					Player->Set(Player,PLAYER_MOVEBACK,NULL,0);
				}
				return 1;
			case -2:
				if (KeyTrackNext)
					Player->Set(Player,PLAYER_NEXT,NULL,0);
				InSeek(0);
				return 1;
			case -4:
				if (KeyTrackPrev)
					Player->Set(Player,PLAYER_PREV,NULL,1);
				InSeek(0);
				return 1;
			}

			if (KeyMediaStop(Event)>0)
			{
				if (IsVideoPrimary())
					SetFullScreen(!FullScreen);
				return 1;
			}

			if (KeyMediaVolumeUp(Event)>0)
			{
				Delta(PLAYER_VOLUME,2,0,100);
				return 1;
			}

			if (KeyMediaVolumeDown(Event)>0)
			{
				Delta(PLAYER_VOLUME,-2,0,100);
				return 1;
			}

			if (KeyMediaNext(Event)>0)
			{
				Player->Set(Player,PLAYER_NEXT,NULL,0);
				return 1;
			}

			if (KeyMediaPrev(Event)>0)
			{
				Player->Set(Player,PLAYER_PREV,NULL,1);
				return 1;
			}

			i = KeySelect(Event);
			if (i==1)
			{
				KeySelectHold = KeyHold(Event);
				if (FullScreen && KeySelectHold)
				{
					ExitFullScreen();
					return 1;
				}
			}

			if (((i&2) && !KeySelectHold) || KeyMediaPlay(Event)>0)
			{
				PlayAndFullScreen(0);
				return 1;
			}
		}
	}

	if (Event->eType == frmLoadEvent)
	{
		UInt16 Id = Event->data.frmLoad.formID;
		FormPtr Form = FrmInitForm(Id);

		DIALoad(Form);

		switch (Id)
		{
			case OpenForm:
				OpenInit(Form);
				FrmSetEventHandler(Form, OpenHandler);
				break;
			
			case AboutForm:
				AboutInit(Form);
				FrmSetEventHandler(Form, AboutHandler);
				break;

			case TweakForm:
				TweakInit(Form);
				FrmSetEventHandler(Form, TweakHandler);
				break;

			case AdvForm:
				AdvInit(Form);
				FrmSetEventHandler(Form, AdvHandler);
				break;

			case EqForm:
				EqInit(Form,0);
				FrmSetEventHandler(Form, EqHandler);
				break;

			case PrefForm:
				PrefInit(Form);
				FrmSetEventHandler(Form, PrefHandler);
				break;

			case MediaInfoForm:
				MediaInfoInit(Form);
				FrmSetEventHandler(Form, MediaInfoHandler);
				break;

			case BenchmarkForm:
				BenchmarkInit(Form);
				FrmSetEventHandler(Form, BenchmarkHandler);
				break;

			case MainForm:
				memset(&MainState,0,sizeof(MainState));
				MainDrawn = 0;
				MainFormInit(Form);
				FrmSetEventHandler(Form, MainFormEventHandler);
				FrmSetGadgetHandler(Form, FrmGetObjectIndex(Form, MainViewport),ViewportEventHandler);
				FrmSetGadgetHandler(Form, FrmGetObjectIndex(Form, MainWinTitle),WinTitleEventHandler);
				break;
		}

		FrmSetActiveForm(Form);
		return 1;
	}

	return 0;
}

void Main()
{
	int Sleep = max(GetTimeFreq()/250,1);
	EventType event;

	FrmGotoForm(MainForm);

	Player = (player*)Context()->Player;
	if (Player)
	{
		event.eType = nilEvent;
		do
		{
			int Timeout;

			switch (Player->Process(Player))
			{
			case ERR_BUFFER_FULL:
				Timeout = NoEventChecking?evtNoWait:Sleep;
				break;
			case ERR_STOPPED:
				Timeout = evtWaitForever;
				break;
			default:
				Timeout = evtNoWait;
				break;
			}

			while (Timeout || _EvtEventAvail())
			{ 
				_EvtGetEvent(&event,Timeout);

				if (event.eType == nilEvent)
					break;

				if (!PlatformHandleEvent(&event))
					if (!ApplicationHandleEvent(&event)) // important: before SysHandleEvent
						OSHandleEvent(&event);

				ApplicationPostEvent();

				if (event.eType == appStopEvent)
					break;

				Timeout = 0;
			}
		} 
		while (event.eType != appStopEvent);
	}
	FrmCloseAllForms();
}

UInt32 PilotMain(UInt16 launchCode, MemPtr launchParameters, UInt16 launchFlags)
{
	if ((launchCode == sysAppLaunchCmdNormalLaunch ||
		 launchCode == sysAppLaunchCmdOpenDB ||
		 launchCode == sysAppLaunchCmdCustomBase) && 
		Context_Init(ProgramName,ProgramVersion,ProgramId,NULL,NULL))
	{
		LaunchCode = launchCode;
		LaunchParameters = launchParameters;
		Main();
		Context_Done();
	}
	else
	if (launchCode == sysAppLaunchCmdNotify)
		LaunchNotify((SysNotifyParamType*)launchParameters,FrmGetActiveForm() == FrmGetFormPtr(MainForm));
	return errNone;
}
