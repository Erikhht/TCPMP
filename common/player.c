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
 * $Id: player.c 623 2006-02-01 13:19:15Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#include "playlist/m3u.h"
#include "../config.h"

#define VIDEO_STREAMING_UNDERRUN	(256*1024)/BLOCKSIZE

#define ALIGN_SIZE			4096
#define PROBE_SIZE			ALIGN_SIZE
#define MAXSKIP				16

typedef struct ref
{
	node* Node;
	int RefCount;

} ref;

typedef struct item
{
	tchar_t URL[MAXPATH];
	tchar_t Title[256];
	tick_t Length;
	bool_t Changed;
} item;

typedef struct player_base
{
	player Player;
	bool_t BufferWarning;
	int BufferSize2; //in blocksize
	int MDBufferSize2; //in blocksize
	int CurrBufferSize2; //in blocksize
	bool_t SoftVolume;
	bool_t Background;
	bool_t Foreground;
	bool_t PowerOff;
	bool_t PowerOffReSync;
	bool_t FlowBackground;
	bool_t MicroDrive;
	bool_t Repeat;
	bool_t Shuffle;
	bool_t PlayAtOpen;
	bool_t PlayAtOpenFull;
	bool_t ExitAtEnd;
	bool_t KeepPlayVideo;
	bool_t KeepPlayAudio;
	bool_t ShowInBackground;
	bool_t SingleClickFullScreen;
	bool_t KeepList;
	tick_t MoveBack;
	tick_t MoveFFwd;
	int BurstStart; // in blocksize
	int UnderRun; // percent
	int AudioUnderRun; //kbyte
	fraction FullZoom;
	fraction SkinZoom;
	fraction Aspect;
	int FullDir;
	int SkinDir;
	int RelDir;
	bool_t SmoothZoom50;
	bool_t SmoothZoomAlways;
	bool_t VideoAccel;
	bool_t AutoPreRotate;
	int SkipAccelFrom;
	int AudioQuality;
	int VideoQuality;
	int AOutputId;
	int VOutputId;
	int AOutputIdMax;
	int VOutputIdMax;
	fraction SeekAfterSync;
	bool_t SeekAfterSyncPrevKey;
	fraction InSeekPos;
	fraction PlaySpeed;
	fraction FFwdSpeed;
	int Volume;
	int Pan;
	int PreAmp;
	bool_t Mute;
	bool_t TimerLeft;
	node* Color;
	node* Equalizer;
	bool_t Eq;

	// states
	notify ListNotify;
	notify Notify;
	rect SkinViewport;
	void* Wnd;
	bool_t DiscardList;
	bool_t FullScreen;
	bool_t Clipping;
	bool_t Rotating;
	rect Viewport;
	bool_t Primary;
	bool_t Overlay;
	bool_t HasHost;
	bool_t Streaming; // HasHost and filesize==-1
	bool_t Timing;
	bool_t TemporaryHidden; // hidden till next sync
	bool_t FullScreenAfterSync;
	int Stereo;
	int StreamNo;

	// current media
	bool_t PreRotate;
	int BufferMax;
	stream* Input;
	stream* InputBase;
	node* Timer;
	node* AOutput;
	node* VOutput;
	node* VIDCT;
	node* FlowBuffer;
	format* Format;
	int Selected[PACKET_MAX]; // stream numbers

	// threads and timings
	bool_t NoMoreInput;
	bool_t Play;
	bool_t FFwd;
	bool_t Sync;
	bool_t Fill;
	bool_t LoadMode;
	bool_t Bench;
	bool_t SaveMicroDrive;
	bool_t WaitForProcess;
	bool_t TimerPlay;
	bool_t InSeek;
	bool_t UpdateStreamsNeeded;
	tick_t Position;
	tick_t PosNotify;
	fraction Speed;
	tick_t PosNotifyStep;
	tick_t BenchTime;
	point BenchSrc;
	point BenchDst;
	int BenchStart;
	int IOError;
	int UsedEnough2; //in blocksize
	int MinBuffer; //in blocksize
	uint32_t RndSeed;

	bool_t RunInput;
	bool_t RunProcess;

#ifdef MULTITHREAD
	int ProcessPriority;
	int InputPriority;

	int LockInputCount; // just hint to sleep in input thread so other other thread can lock it
	int LockProcessCount; // just hint to sleep in process thread so other other thread can lock it
	void* Lock;			 // parameter read/write
	void* LockComment;	
	void* LockInput;
	void* LockProcess;

	void* EventProcess; // ProcessThread has processed some packets (InputThread can continue)

	void* EventRunInput;	
	void* EventRunProcess; 
	
	void* ProcessThread;
	void* InputThread;

	int ProcessLocked;
	int InputLocked;
#endif

	// playlist
	int Current;
	bool_t CurrentChanged;
	int PlayListCount;
	item* PlayList;
	int* PlayIndex;

	// paint
	rgbval_t ColorKey; // colorkeybursh color
	void* ColorKeyBrush;
	void* BlackBrush;

	array Ref; // ref
	array Comment; // [Stream,Name,0,Value,0]
	bool_t StreamSkip[MAXSTREAM];
	tchar_t Title[256];
	tchar_t CurrentDir[MAXPATH];

	int CodecSkipCount;
	int CodecSkip[MAXSKIP];

} player_base;

static const datatable PlayerParams[] = 
{
	{ PLAYER_AUTOPREROTATE,	TYPE_BOOL, DF_SETUP },
	{ PLAYER_REPEAT,		TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ PLAYER_SHUFFLE,		TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ PLAYER_KEEPPLAY_AUDIO,TYPE_BOOL, DF_SETUP },
	{ PLAYER_KEEPPLAY_VIDEO,TYPE_BOOL, DF_SETUP },
	{ PLAYER_SHOWINBACKGROUND,TYPE_BOOL, DF_SETUP },
	{ PLAYER_SINGLECLICKFULLSCREEN, TYPE_BOOL, DF_SETUP },
	{ PLAYER_KEEPLIST,		TYPE_BOOL, DF_SETUP },
	{ PLAYER_PLAYATOPEN,	TYPE_BOOL, DF_SETUP },
	{ PLAYER_PLAYATOPEN_FULL,TYPE_BOOL, DF_SETUP },
	{ PLAYER_EXIT_AT_END,	TYPE_BOOL, DF_SETUP },
	{ PLAYER_FULL_ZOOM,		TYPE_FRACTION, DF_SETUP|DF_PERCENT|DF_HIDDEN },
	{ PLAYER_SKIN_ZOOM,		TYPE_FRACTION, DF_SETUP|DF_PERCENT|DF_HIDDEN },
	{ PLAYER_PLAY_SPEED,	TYPE_FRACTION, DF_SETUP|DF_PERCENT|DF_GAP },
	{ PLAYER_FFWD_SPEED,	TYPE_FRACTION, DF_SETUP|DF_PERCENT },
	{ PLAYER_MOVEFFWD_STEP,	TYPE_TICK, DF_SETUP|DF_MINMAX, 0, 600*TICKSPERSEC },
	{ PLAYER_MOVEBACK_STEP,	TYPE_TICK, DF_SETUP|DF_MINMAX, 0, 600*TICKSPERSEC },

	{ PLAYER_BUFFER_SIZE,	TYPE_INT, DF_SETUP|DF_KBYTE|DF_GAP|DF_HIDDEN, 512, 128*1024 },
	{ PLAYER_UNDERRUN,		TYPE_INT, DF_SETUP|DF_PERCENT|DF_HIDDEN,0,PERCENT_ONE },
	{ PLAYER_AUDIO_UNDERRUN,TYPE_INT, DF_SETUP|DF_KBYTE|DF_HIDDEN,32,128*1024 },

	{ PLAYER_MICRODRIVE,	TYPE_BOOL, DF_SETUP|DF_GAP|DF_HIDDEN },
	{ PLAYER_MD_BUFFER_SIZE,TYPE_INT, DF_SETUP|DF_KBYTE|DF_HIDDEN, 512, 128*1024 },
	{ PLAYER_BURSTSTART,	TYPE_INT, DF_SETUP|DF_KBYTE|DF_HIDDEN, 512, 128*1024 },

	{ PLAYER_AOUTPUTID,		TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_AOUTPUTID_MAX,	TYPE_INT, DF_SETUP|DF_HIDDEN }, // should be after PLAYER_AOUTPUTID
	{ PLAYER_VOUTPUTID,		TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_VOUTPUTID_MAX,	TYPE_INT, DF_SETUP|DF_HIDDEN }, // should be after PLAYER_VOUTPUTID
	{ PLAYER_VIDEO_ACCEL,	TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ PLAYER_LIST_COUNT,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_LIST_CURRENT,	TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_LIST_CURRIDX,	TYPE_INT, DF_HIDDEN },
	{ PLAYER_SMOOTH50,		TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ PLAYER_SMOOTHALWAYS,	TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ PLAYER_STEREO,		TYPE_INT,  DF_SETUP|DF_HIDDEN },
	{ PLAYER_ASPECT,		TYPE_FRACTION, DF_SETUP|DF_HIDDEN },
	{ PLAYER_FULL_DIR,		TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_SKIN_DIR,		TYPE_INT, DF_SETUP|DF_HIDDEN },
	{ PLAYER_AUDIO_QUALITY,	TYPE_INT, DF_SETUP|DF_ENUMSTRING|DF_HIDDEN, PLAYERQUALITY_ENUM },
	{ PLAYER_VIDEO_QUALITY,	TYPE_INT, DF_SETUP|DF_ENUMSTRING|DF_HIDDEN, PLAYERQUALITY_ENUM },
	{ PLAYER_ASTREAM,		TYPE_INT, DF_HIDDEN|DF_SETUP },
	{ PLAYER_VSTREAM,		TYPE_INT, DF_HIDDEN|DF_SETUP },
	{ PLAYER_SUBSTREAM,		TYPE_INT, DF_HIDDEN|DF_SETUP },
	{ PLAYER_PERCENT,		TYPE_FRACTION, DF_HIDDEN|DF_SETUP },
	{ PLAYER_VOLUME,		TYPE_INT, DF_MINMAX|DF_HIDDEN|DF_SETUP, 0, 100 },
	{ PLAYER_PAN,			TYPE_INT, DF_MINMAX|DF_HIDDEN|DF_SETUP, -100, 100 },
	{ PLAYER_PREAMP,		TYPE_INT, DF_MINMAX|DF_HIDDEN|DF_SETUP, -100, 100 },
	{ PLAYER_MUTE,			TYPE_BOOL, DF_HIDDEN|DF_SETUP },
	{ PLAYER_TIMER_LEFT,	TYPE_BOOL, DF_HIDDEN|DF_SETUP },

	{ PLAYER_BACKGROUND,	TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_FOREGROUND,	TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_PLAY,			TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_FFWD,			TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_POSITION,		TYPE_TICK, DF_HIDDEN },
	{ PLAYER_DURATION,		TYPE_TICK, DF_HIDDEN },
	{ PLAYER_TIMER,			TYPE_STRING, DF_HIDDEN },
	{ PLAYER_TITLE,			TYPE_STRING },
	{ PLAYER_INPUT,			TYPE_NODE, DF_HIDDEN, STREAM_CLASS },
	{ PLAYER_FORMAT,		TYPE_NODE, DF_HIDDEN, FORMAT_CLASS },
	{ PLAYER_AOUTPUT,		TYPE_NODE, DF_HIDDEN, AOUT_CLASS },
	{ PLAYER_VOUTPUT,		TYPE_NODE, DF_HIDDEN, VOUT_CLASS },
	{ PLAYER_SKIN_VIEWPORT,	TYPE_RECT, DF_HIDDEN },
	{ PLAYER_CLIPPING,		TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_FULLSCREEN,	TYPE_BOOL, DF_HIDDEN },
	{ PLAYER_LOADMODE,		TYPE_BOOL, DF_HIDDEN | DF_RDONLY },

	{ PLAYER_CURRENTDIR,	TYPE_STRING, DF_SETUP|DF_HIDDEN },

	DATATABLE_END(PLAYER_ID)
};

static const datatable BufferParams[] = 
{
	{ PLAYER_BUFFER_SIZE,	TYPE_INT, DF_SETUP|DF_KBYTE|DF_GAP, 512, 128*1024 },
	{ PLAYER_UNDERRUN,		TYPE_INT, DF_SETUP|DF_PERCENT,0,PERCENT_ONE },
	{ PLAYER_AUDIO_UNDERRUN,TYPE_INT, DF_SETUP|DF_KBYTE,32,128*1024 },

	{ PLAYER_MICRODRIVE,	TYPE_BOOL, DF_SETUP|DF_GAP },
	{ PLAYER_MD_BUFFER_SIZE,TYPE_INT, DF_SETUP|DF_KBYTE, 512, 128*1024 },
	{ PLAYER_BURSTSTART,	TYPE_INT, DF_SETUP|DF_KBYTE, 512, 128*1024 },

	DATATABLE_END(PLAYER_BUFFER_ID)
};

static int Enum(player_base* p, int* No, datadef* Param)
{
	if (NodeEnumTable(No,Param,PlayerParams) == ERR_NONE)
		return ERR_NONE;

	if (*No<p->PlayListCount*3)
	{
		memset(Param,0,sizeof(datadef));
		if (*No >= 2*p->PlayListCount)
		{
			Param->No = PLAYER_LIST_LENGTH + (*No - 2*p->PlayListCount);
			Param->Type = TYPE_TICK;
			Param->Size = sizeof(tick_t);
		}
		else
		{
			if (*No >= p->PlayListCount)
				Param->No = PLAYER_LIST_TITLE + (*No - p->PlayListCount);
			else
				Param->No = PLAYER_LIST_URL + *No;
			Param->Type = TYPE_STRING;
			Param->Size = MAXDATA;
		}
		Param->Flags = DF_HIDDEN | DF_SETUP;
		Param->Class = PLAYER_ID;
		return ERR_NONE;
	}

	*No -= p->PlayListCount*3;
	return ERR_INVALID_PARAM;
}

static NOINLINE int UpdateSpeed(player_base* p)
{
	if (p->Bench)
	{
		p->Speed.Num = 0;
		p->Speed.Den = 1;
	}
	else
	if (p->FFwd)
		p->Speed = p->FFwdSpeed;
	else
		p->Speed = p->PlaySpeed;

	p->PosNotifyStep = Scale(TICKSPERSEC,p->Speed.Num,p->Speed.Den);

	if (p->Timer)
		p->Timer->Set(p->Timer,TIMER_SPEED,&p->Speed,sizeof(p->Speed));

	return ERR_NONE;
}

static NOINLINE void NotifyList(player_base* p)
{
	if (p->ListNotify.Func)
		p->ListNotify.Func(p->ListNotify.This,0,0);
}

static NOINLINE void Notify(player_base* p,int Id,int Value)
{
	if (p->Notify.Func)
		p->Notify.Func(p->Notify.This,Id,Value);
}

static NOINLINE void UpdateListLength(player_base* p,tick_t Length)
{
	if (p->Current < p->PlayListCount && !p->PlayList[p->Current].Changed && 
		p->PlayList[p->Current].Length != Length)
	{
		p->PlayList[p->Current].Length = Length;
		NotifyList(p);
	}
}

#define INVALID_COMMENT -2
static INLINE int CommentStream(const uint8_t* p) { return *(tchar_t*)p; }
static INLINE const tchar_t* CommentName(const uint8_t* p) { return (const tchar_t*)(p+sizeof(tchar_t)); }
static INLINE const tchar_t* CommentValue(const uint8_t* p) { const tchar_t* i = CommentName(p); return i+tcslen(i)+1; }
static const uint8_t* CommentNext(const uint8_t* p) { const tchar_t* i = CommentValue(p); return (const uint8_t*)(i+tcslen(i)+1); }

static const tchar_t* GetFirstComment(player_base* p, int Id)
{
	const uint8_t *i;
	const tchar_t* Name = PlayerComment(Id);
	for (i=ARRAYBEGIN(p->Comment,uint8_t);i!=ARRAYEND(p->Comment,uint8_t);i=CommentNext(i))
		if (CommentStream(i)!=INVALID_COMMENT && tcsicmp(CommentName(i),Name)==0)
			return CommentValue(i);
	return T("");
}

static void UpdateListTitle(player_base* p)
{
	const tchar_t* Title;
	const tchar_t* Artist;

	LockEnter(p->LockComment);

	Title = GetFirstComment(p,COMMENT_TITLE);
	Artist = GetFirstComment(p,COMMENT_ARTIST);
	if (!Artist[0])
		Artist = GetFirstComment(p,COMMENT_AUTHOR);

	if (Artist[0] && Title[0])
		stprintf_s(p->Title,TSIZEOF(p->Title),T("%s - %s"),Artist,Title);
	else
	{
		if (!Title[0])
			Title = Artist;
		tcscpy_s(p->Title,TSIZEOF(p->Title),Title);
	}

	LockLeave(p->LockComment);
	Notify(p,PLAYER_TITLE,0);

	if (p->Current < p->PlayListCount && !p->PlayList[p->Current].Changed)
	{
		tcscpy_s(p->PlayList[p->Current].Title,TSIZEOF(p->PlayList[p->Current].Title),p->Title);
		NotifyList(p);
	}
}

static NOINLINE bool_t IsCoverArt(player_base* p)
{
	int CoverArt = -2;
	if (p->Format)
		p->Format->Get(p->Format,FORMAT_COVERART,&CoverArt,sizeof(CoverArt));
	return CoverArt==p->Selected[PACKET_VIDEO];
}

static NOINLINE bool_t IsVideo(player_base* p)
{
	return p->VOutput && !IsCoverArt(p);
}

static NOINLINE void UpdateTimeouts(player_base* p)
{
	bool_t KeepProcess = (p->Play || p->FFwd) && p->Format;
	if (!KeepProcess || !p->Sync) // VOutput not neccessary updated till first sync is over
		SleepTimeout(KeepProcess,p->Bench || (p->Primary && IsVideo(p)));
}

static NOINLINE void UpdateRunInput(player_base* p)
{
	// we don't want deadlocks so LoadMode will restart InputThread even with NoMoreInput
	p->RunInput = p->Format && p->Input && (!p->NoMoreInput || p->LoadMode);
#ifdef MULTITHREAD
	if (p->RunInput)
		EventSet(p->EventRunInput);
	else
		EventReset(p->EventRunInput);
#endif
}

static NOINLINE void UpdateRunProcess(player_base* p)
{
	p->RunProcess = p->Format && p->Input && p->Timer && (((p->FFwd || p->Play) && !p->LoadMode) || p->Sync || p->Fill);
#ifdef MULTITHREAD
	if (p->RunProcess)
		EventSet(p->EventRunProcess);
	else
		EventReset(p->EventRunProcess);
#endif
}

static NOINLINE void UpdatePriority(player_base* p)
{
#ifdef MULTITHREAD
	if (p->InputThread && p->ProcessThread)
	{
		int Priority;
		Priority = (!p->TimerPlay || !QueryAdvanced(ADVANCED_PRIORITY))?0:1;
		if (p->ProcessPriority != Priority)
		{
			p->ProcessPriority = Priority;
			ThreadPriority(p->ProcessThread,Priority);
		}
		Priority = (((p->Play || p->FFwd) && !p->Sync) || p->LoadMode)?0:1;
		if (p->InputPriority != Priority)
		{
			p->InputPriority = Priority;
			ThreadPriority(p->InputThread,Priority);
		}
	}
#endif
}

static NOINLINE void UpdatePlay(player_base* p)
{
	p->TimerPlay = (p->Play || p->FFwd) && !p->InSeek && !p->Sync && !p->Fill && !p->LoadMode;
	UpdatePriority(p);
	p->Timer->Set(p->Timer,TIMER_PLAY,&p->TimerPlay,sizeof(bool_t));
	if (p->VOutput)
		p->VOutput->Set(p->VOutput,VOUT_PLAY,&p->TimerPlay,sizeof(bool_t));
}

static int CmpRef(const ref* a, const ref* b)
{
	if (a->Node > b->Node) return 1;
	if (a->Node < b->Node) return -1;
	return 0;
}

static void Unlock(player_base* p, bool_t Input);
static void Lock(player_base* p, bool_t Input);

static NOINLINE bool_t Used(player_base* p,node* Node)
{
	const ref *i;
	for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
		if (i->Node == Node)
			return 1;
	return 0;
}

static NOINLINE void Flush(player_base* p)
{
	const ref *i;
	node* Skip = NULL;
	if (IsCoverArt(p))
		Skip = p->FlowBuffer;

	for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
		if (i->Node != Skip)
			i->Node->Set(i->Node,FLOW_FLUSH,NULL,0);

	if (p->FlowBuffer && p->FlowBuffer != Skip)
		p->FlowBuffer->Set(p->FlowBuffer,FLOW_FLUSH,NULL,0);
}

static NOINLINE int UpdateBackground(player_base* p)
{
	bool_t b;
#ifdef MULTITHREAD
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
#endif

	LockEnter(p->Lock);
	b = p->Background && !p->ShowInBackground && !p->Play && !p->FFwd;

#if !defined(TARGET_WINCE)
	if (p->PowerOff) b = 1;
#endif

	if (b != p->FlowBackground)
	{
		bool_t Locked = 0;
		const ref *i;
		p->FlowBackground = b;

		for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
			if (i->Node->Get(i->Node,FLOW_BACKGROUND,&b,sizeof(b))==ERR_NONE)
			{
				if (!Locked)
				{
					Lock(p,1);
					Locked = 1;
				}
				i->Node->Set(i->Node,FLOW_BACKGROUND,&p->FlowBackground,sizeof(p->FlowBackground));
			}

		if (Locked)
			Unlock(p,1);
	}
	LockLeave(p->Lock);
	return ERR_NONE;
}

static void ReSync(player_base* p,bool_t Seek);
static int UpdateFormat(player_base* p,bool_t BufferChanged);
static int SetPlay(player_base* p,bool_t Stop);
static int SetPlayListCount(player_base* p, int Count);
static void Paint(player_base* p,void* DC,int x0,int y0);

static int UpdateVideoEx(player_base* p,node* VOutput,int ForceRefresh,bool_t Active)
{
	int Caps;
	bool_t Dither;
	packetformat Mode;
	blitfx FX;
	bool_t FullScreen;
	bool_t Visible = (p->Foreground || p->ShowInBackground) && !p->Rotating && Active;
	bool_t Refresh;
	bool_t Updating;
	bool_t VOutVisible;
	bool_t Primary;
	bool_t Overlay;
	rect Viewport;
	int RelDir;
	
	if (VOutput)
	{
		VOutput->Get(VOutput,VOUT_PRIMARY,&Primary,sizeof(bool_t));

		Viewport = p->SkinViewport;
		FullScreen = p->FullScreen || !Primary;
		if (FullScreen && VOutput->Get(VOutput,OUT_OUTPUT|PIN_FORMAT,&Mode,sizeof(Mode))==ERR_NONE && Mode.Type == PACKET_VIDEO)
			PhyToVirt(NULL,&Viewport,&Mode.Format.Video);

		FX.Flags = 0;
		if (p->SmoothZoom50)
			FX.Flags |= BLITFX_ARITHSTRETCH50;
		if (p->SmoothZoomAlways)
			FX.Flags |= BLITFX_ARITHSTRETCHALWAYS;
		if (QueryAdvanced(ADVANCED_SLOW_VIDEO))
			FX.Flags |= BLITFX_ONLYDIFF;
		if (QueryAdvanced(ADVANCED_COLOR_LOOKUP))
			FX.Flags |= BLITFX_COLOR_LOOKUP;

		p->Color->Get(p->Color,COLOR_DITHER,&Dither,sizeof(Dither));
		if (Dither)
			FX.Flags |= BLITFX_DITHER;

		p->Color->Get(p->Color,COLOR_BRIGHTNESS,&FX.Brightness,sizeof(FX.Brightness));
		p->Color->Get(p->Color,COLOR_CONTRAST,&FX.Contrast,sizeof(FX.Contrast));
		p->Color->Get(p->Color,COLOR_SATURATION,&FX.Saturation,sizeof(FX.Saturation));
		p->Color->Get(p->Color,COLOR_R_ADJUST,&FX.RGBAdjust[0],sizeof(FX.RGBAdjust[0]));
		p->Color->Get(p->Color,COLOR_G_ADJUST,&FX.RGBAdjust[1],sizeof(FX.RGBAdjust[1]));
		p->Color->Get(p->Color,COLOR_B_ADJUST,&FX.RGBAdjust[2],sizeof(FX.RGBAdjust[2]));

		if (FullScreen)
		{
			FX.Direction = p->FullDir;
			FX.ScaleY = FX.ScaleX = Scale(SCALE_ONE,p->FullZoom.Num,p->FullZoom.Den);
		}
		else
		{
			FX.Direction = p->SkinDir;
			FX.ScaleY = FX.ScaleX = Scale(SCALE_ONE,p->SkinZoom.Num,p->SkinZoom.Den);
		}

		if (FX.Direction < 0)
			FX.Direction = 0;
		else
		if (Primary)
			FX.Direction = CombineDir(GetOrientation(),0,FX.Direction);

		RelDir = FX.Direction;

		VOutVisible = Visible && !p->TemporaryHidden;
		Updating = 1;
		VOutput->Set(VOutput,VOUT_UPDATING,&Updating,sizeof(bool_t));

		VOutput->Set(VOutput,VOUT_FULLSCREEN,&FullScreen,sizeof(bool_t));
		VOutput->Set(VOutput,VOUT_VIEWPORT,&Viewport,sizeof(rect));
		VOutput->Set(VOutput,VOUT_FX,&FX,sizeof(blitfx));
		VOutput->Set(VOutput,VOUT_ASPECT,&p->Aspect,sizeof(fraction));
		VOutput->Set(VOutput,VOUT_CLIPPING,&p->Clipping,sizeof(p->Clipping));
		VOutput->Set(VOutput,VOUT_VISIBLE,&VOutVisible,sizeof(VOutVisible));
		VOutput->Set(VOutput,VOUT_AUTOPREROTATE,&p->AutoPreRotate,sizeof(bool_t));

		Updating = 0;
		VOutput->Set(VOutput,VOUT_UPDATING,&Updating,sizeof(bool_t));
		VOutput->Get(VOutput,VOUT_OVERLAY,&Overlay,sizeof(bool_t));
	}
	else
	{
		Primary = 1;
		Overlay = 0;
		RelDir = 0;
		Viewport = p->SkinViewport;
	}

	if (Active)
	{
		p->RelDir = RelDir;
		p->Primary = Primary;
		p->Overlay = Overlay;
		p->Viewport = Viewport;

#ifdef TARGET_PALMOS
		if (!p->Clipping && Visible)
			Paint(p,p->Wnd,0,0);
#endif

		// try to redraw overlay
		Refresh = VOutput && ForceRefresh>=0 && !p->Sync;
		if (Refresh && p->FlowBuffer && !ForceRefresh)
			Refresh = p->FlowBuffer->Set(p->FlowBuffer,FLOW_RESEND,NULL,0) != ERR_NONE;
		if (Refresh && p->Format)
			ReSync(p,0); // redraw the hard way: resync

		if (!p->Wnd || !VOutput || VOutput->Get(VOutput,VOUT_CAPS,&Caps,sizeof(Caps))!=ERR_NONE)
			Caps = -1;
		p->Color->Set(p->Color,COLOR_CAPS,&Caps,sizeof(Caps));
	}

	return ERR_NONE;
}

static NOINLINE int UpdateVideo(player_base* p,int ForceRefresh)
{
	return UpdateVideoEx(p,p->VOutput,ForceRefresh,1);
}

static NOINLINE int UpdateBuffer(player_base* p)
{
	node* Node = p->AOutput;
	if (Node)
	{
		bool_t b = IsVideo(p);
		Node->Set(Node,AOUT_MODE,&b,sizeof(b));
	}

	p->MinBuffer = 0;
	if (p->Format)
		p->Format->Get(p->Format,FORMAT_MIN_BUFFER,&p->MinBuffer,sizeof(int));

	return ERR_NONE;
}

static NOINLINE player_base* RefreshAudio(player_base* p)
{
	node* Node = p->AOutput;
	if (Node && QueryAdvanced(ADVANCED_SYSTEMVOLUME))
	{
		Node->Get(Node,AOUT_VOLUME,&p->Volume,sizeof(int));
		Node->Get(Node,AOUT_MUTE,&p->Mute,sizeof(bool_t));
		Node->Get(Node,AOUT_PAN,&p->Pan,sizeof(int));
	}
	return p;
}

static NOINLINE int UpdateAudioEx(player_base* p,node* Node)
{
	if (Node)
	{
		Node->Set(Node,AOUT_STEREO,&p->Stereo,sizeof(int));
		Node->Set(Node,AOUT_QUALITY,&p->AudioQuality,sizeof(int));
		Node->Set(Node,AOUT_PREAMP,&p->PreAmp,sizeof(int));
		Node->Set(Node,AOUT_VOLUME,&p->Volume,sizeof(int));
		Node->Set(Node,AOUT_MUTE,&p->Mute,sizeof(bool_t));
		Node->Set(Node,AOUT_PAN,&p->Pan,sizeof(int));
	}
	return ERR_NONE;
}

static NOINLINE int UpdateAudio(player_base* p)
{
	return UpdateAudioEx(p,p->AOutput);
}

static NOINLINE int UpdateVolume(player_base* p)
{
	if (p->AOutput)
		p->AOutput->Set(p->AOutput,AOUT_VOLUME,&p->Volume,sizeof(int));
	return ERR_NONE;
}

static int UpdateStreams(player_base* p,int ForceRefresh,bool_t CodecSkip);

static NOINLINE int UpdatePowerOff(player_base* p)
{
	Notify(p,PLAYER_POWEROFF,p->PowerOff); // before poweroff is handled! (PalmOS may need to turn on clipping)

#ifdef TARGET_WINCE
	if (p->PowerOff)
	{
		bool_t b;

		Lock(p,0);

		if (p->VOutput && p->VOutput->Get(p->VOutput,FLOW_BACKGROUND,&b,sizeof(b))==ERR_NONE)
		{
			int Old = p->VOutputId;
			bool_t OldAccel = p->VideoAccel;

			p->VOutputId = 0;
			UpdateStreams(p,-1,0);

			p->VOutputId = Old;
			p->VideoAccel = OldAccel;
			p->PowerOffReSync = 1;
		}

		if (p->Play || p->FFwd)
		{
			Lock(p,1);
			p->Play = 0;
			p->FFwd = 0;
			p->Fill = 1;
			SetPlay(p,0);
			Unlock(p,1);
		}

		Unlock(p,0);
	}
	else
	{
		if (p->PowerOffReSync)
		{
			p->PowerOffReSync = 0;
			UpdateStreams(p,-1,0);
			ReSync(p,1);
		}
	}
#else
	if (p->PowerOff && (p->Play || p->FFwd || p->Fill))
	{
		Lock(p,1);
		p->Play = 0;
		p->FFwd = 0;
		p->Fill = 0; // do not fill, because acceleration devices could be turned off
		SetPlay(p,0); // calls UpdateBackground
		Unlock(p,1);
	}
	else
	if (!p->PowerOff && !p->Play && !p->FFwd && !p->Fill)
	{
		Lock(p,1);
		p->Fill = 1; // turn on fill
		SetPlay(p,0); // calls UpdateBackground
		Unlock(p,1);
	}
	else
		UpdateBackground(p);
#endif

	return ERR_NONE;
}

static NOINLINE int UpdateForeground(player_base* p)
{
	if (!p->Foreground)
		if ((IsVideo(p) && !p->KeepPlayVideo) ||
			(!IsVideo(p) && !p->KeepPlayAudio))
		{
			Lock(p,1);
			SetPlay(p,1);
			Unlock(p,1);
		}

	UpdateVideo(p,0);
	return ERR_NONE;
}

static int FindCurrentIndex(player_base* p)
{
	int Index;
	for (Index=0;Index<p->PlayListCount;++Index)
		if (p->PlayIndex[Index] == p->Current)
			return Index;

	return 0;
}

static uint32_t Rand(player_base* p)
{ 
	p->RndSeed = p->RndSeed*0x8088405U + 0x251001U;
	return p->RndSeed;
}

static int RandomizePlayIndex(player_base* p,bool_t CurrentAsFirst)
{
	int No,n;
	int* Index = p->PlayIndex;
	for (No=0;No<p->PlayListCount;++No)
		Index[No] = No;

	if (p->Shuffle)
	{
		for (n=0;n<4;++n)
			for (No=0;No<p->PlayListCount;++No)
				SwapInt(&Index[No],&Index[Rand(p) % p->PlayListCount]);

		if (CurrentAsFirst)
		{
			// make sure current media is the first
			// so the full range of playlist will be played before it's regenerated
			No = FindCurrentIndex(p);
			if (No>0)
				SwapInt(&Index[No],&Index[0]);
		}
	}

	return ERR_NONE;
}

static int SetPlayListCount(player_base* p, int Count)
{
	int i,j;
	int* Index = p->PlayIndex;
	item* List = (item*)realloc(p->PlayList,sizeof(item)*Count);
	if (!List && Count)
		return ERR_OUT_OF_MEMORY;

	if (p->PlayListCount > Count)
	{
		// remove unwanted indexes
		for (i=0,j=0;i<p->PlayListCount;++i)
			if (Index[i]<Count)
				Index[j++] = Index[i];
		assert(j==Count);
	}

	// realloc index
	Index = (int*)realloc(Index,sizeof(int)*Count);
	if (!Index && Count)
	{
		SetPlayListCount(p,0);
		return ERR_OUT_OF_MEMORY;
	}

	// add new items
	for (i=p->PlayListCount;i<Count;++i)
	{
		memset(List+i,0,sizeof(item));
		List[i].Length = -1;
		List[i].Changed = 1;
		Index[i] = i;
		if (p->Shuffle)
			SwapInt(&Index[i],&Index[Rand(p) % (i+1)]);
	}

	p->PlayIndex = Index;
	p->PlayList = List;
	p->PlayListCount = Count;

	if (p->Current >= Count)
	{
		p->Current = 0;
		p->CurrentChanged = 1;
	}

	NotifyList(p);
	return ERR_NONE;
}

static void StopLoadMode(player_base* p)
{
	if (p->LoadMode)
	{
		DEBUG_MSG(DEBUG_TEST,T("LoadMode Leaving"));

		p->LoadMode = 0;
		UpdatePlay(p); // UpdateInputPriority(p) called too
		UpdateRunProcess(p);
		UpdateRunInput(p);
		Notify(p,PLAYER_LOADMODE,p->LoadMode);
	}
}

static void StopBench(player_base* p)
{
	p->BenchTime = Scale(GetTimeTick() - p->BenchStart,TICKSPERSEC,GetTimeFreq());
	p->Bench = 0;
	p->MicroDrive = p->SaveMicroDrive;

	p->BenchSrc.x = p->BenchSrc.y = 0;
	p->BenchDst.x = p->BenchDst.y = 0;

	if (p->VOutput)
	{
		rect Rect;
		packetformat Format;
		if (p->VOutput->Get(p->VOutput,OUT_INPUT|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE)
		{
			p->BenchSrc.x = Format.Format.Video.Width;
			p->BenchSrc.y = Format.Format.Video.Height;
		}
		if (p->VOutput->Get(p->VOutput,VOUT_OUTPUTRECT,&Rect,sizeof(Rect))==ERR_NONE)
		{
			p->BenchDst.x = Rect.Width;
			p->BenchDst.y = Rect.Height;
		}
	}

	UpdateFormat(p,0);
	UpdatePlay(p);
	UpdateSpeed(p);
	
	Flush(p);

	Notify(p,PLAYER_BENCHMARK,p->BenchTime);
}

static void UpdateKeepAlive(player_base* p)
{
	bool_t KeepAlive = p->FFwd || p->Play;
	if (p->VOutput)
		p->VOutput->Set(p->VOutput,OUT_KEEPALIVE,&KeepAlive,sizeof(KeepAlive));
	if (p->AOutput)
		p->AOutput->Set(p->AOutput,OUT_KEEPALIVE,&KeepAlive,sizeof(KeepAlive));
}

static int SetPlay(player_base* p,bool_t Stop)
{
	bool_t Refresh;

	if (Stop)
	{
		p->Play = 0;
		p->FFwd = 0;
		p->Fill = 0;
		p->Sync = 0;
		p->InSeek = 0;

		if (p->TemporaryHidden)
		{
			p->TemporaryHidden = 0;
			UpdateVideo(p,-1);
		}
	}

	DEBUG_MSG2(DEBUG_PLAYER,T("Play %d,%d"),p->Play,p->FFwd);

	StopLoadMode(p);

	if (p->Bench && !p->Play)
		StopBench(p);

	if (p->Format && p->Input && p->Timer)
	{
		if (p->NoMoreInput && (p->Play || p->FFwd) && p->IOError)
		{
			p->Fill = 1;
			p->NoMoreInput = 0;
			p->IOError = 0;
			UpdateRunInput(p);
		}

		if (p->TimerPlay)
		{
			UpdateKeepAlive(p);
			UpdatePlay(p);
			UpdateSpeed(p);
		}
		else
		{
			UpdateSpeed(p);
			UpdatePlay(p);
			UpdateKeepAlive(p);
		}

		UpdateRunProcess(p);

		// make sure current frame is show after pausing a dropping period
		Refresh = p->VOutput && !p->Play && !p->FFwd && !p->Sync && p->Fill;
		if (Refresh && p->FlowBuffer)
			Refresh = p->FlowBuffer->Set(p->FlowBuffer,FLOW_RESEND,NULL,0) != ERR_NONE;
		if (Refresh)
			ReSync(p,0);
	}
	else
		UpdateKeepAlive(p);

	UpdateTimeouts(p);
	UpdateBackground(p);
	Notify(p,PLAYER_PLAY,p->Play);

	return ERR_NONE;
}

static int Seek2(player_base* p,tick_t Time,const fraction* Percent,bool_t PrevKey)
{
	tick_t Duration;
	int Result = ERR_INVALID_DATA;

	if (p->Format && p->Input && p->Timer)
	{
		int FileSize;
		int FilePos = -1;

		if (Percent)
		{
			if (p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))==ERR_NONE)
			{
				Time = Scale(Duration,Percent->Num,Percent->Den);
				if (Time < 0)
					Time = 0;
			}
			else
			if (p->Input->Get(p->Input,STREAM_LENGTH,&FileSize,sizeof(FileSize))==ERR_NONE)
			{
				Time = -1;
				FilePos = Scale(FileSize,Percent->Num,Percent->Den);
				if (FilePos < 0)
					FilePos = 0;
			}
			else
				return ERR_NOT_SUPPORTED;
		}

		p->SeekAfterSync.Num = -1;

		DEBUG_MSG2(DEBUG_PLAYER,T("Sync time:%d filepos:%d"),Time,FilePos);
		Result = p->Format->Sync(p->Format,Time,FilePos,PrevKey);

		if (Result == ERR_LOADING_HEADER && p->Sync && Percent)
		{
			p->SeekAfterSync = *Percent;
			p->SeekAfterSyncPrevKey = PrevKey;
			Result = ERR_NONE;
		}
		else
		if (Result == ERR_NONE)
		{
			Flush(p);

			p->NoMoreInput = 0;
			p->BufferMax = p->CurrBufferSize2;
			p->Position = Time;
			p->Sync = 1;
			p->PosNotify = -1;

			StopLoadMode(p);
			UpdatePlay(p);
			UpdateRunProcess(p);
			UpdateRunInput(p);

			// wakeup input threads (if needed)
#ifdef MULTITHREAD
			EventSet(p->EventProcess); 
#else
			p->WaitForProcess = 0;
#endif
		}
		Notify(p,PLAYER_PERCENT,0);
	}
	else
	if (!p->Wnd && Percent)
	{
		p->SeekAfterSync = *Percent;
		p->SeekAfterSyncPrevKey = 1;
		Result = ERR_NONE;
	}

	return Result;
}

static int InSeek(player_base* p,tick_t Time,const fraction* Percent,bool_t PrevKey)
{
	int Result;
#ifdef MULTITHREAD
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
#endif

	LockEnter(p->Lock);

	if (p->InSeek)
	{
		tick_t Duration;
		fraction Pos;
		if (Percent)
			Pos = *Percent;
		else
		if (p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))==ERR_NONE)
		{
			Pos.Num = Time;
			Pos.Den = Duration;
		}
		else
		{
			Pos.Num = -1;
			Pos.Den = 0;
		}

		if (EqFrac(&p->InSeekPos,&Pos))
		{
			LockLeave(p->Lock);
			return ERR_NONE;
		}

		p->InSeekPos = Pos;
		if (p->InSeekPos.Num>=0 && (p->Sync || !IsVideo(p)))
		{
			p->SeekAfterSync = p->InSeekPos;
			p->SeekAfterSyncPrevKey = PrevKey;
			Notify(p,PLAYER_PERCENT,0);

			LockLeave(p->Lock);
			return ERR_NONE;
		}
	}

	Lock(p,1);
	Result = Seek2(p,Time,Percent,PrevKey);
	Unlock(p,1);

	LockLeave(p->Lock);
	return Result;
}

static void ReSync(player_base* p,bool_t DoSeek)
{
	if (p->Format)
	{
		if (DoSeek && p->Position>=0)
			Seek2(p,p->Position,NULL,1);
		else
		if (p->Format->Sync(p->Format,-1,-1,0) == ERR_NONE)
		{
			p->Sync = 1;
			p->PosNotify = -1;

			StopLoadMode(p);
			UpdatePlay(p);
			UpdateRunProcess(p);

			// wakeup input threads, just in case
#ifdef MULTITHREAD
			EventSet(p->EventProcess); 
#else
			p->WaitForProcess = 0;
#endif
		}
	}
}

static bool_t SupportVQNode(node* Node)
{
	int Shift;
	bool_t Buffered;
	if (NodeIsClass(Node->Class,IDCT_CLASS))
	{
		if (Node->Get(Node,IDCT_SHIFT,&Shift,sizeof(Shift))!=ERR_NONE)
			return 0;
	}
	else
	if (Node->Get(Node,FLOW_BUFFERED,&Buffered,sizeof(Buffered))==ERR_NONE && Buffered)
		return 0;

	return 1;
}

static bool_t SupportVQ(player_base* p)
{
	const ref *i;
	for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
		if (!SupportVQNode(i->Node))
			return 0;
	if (p->FlowBuffer && !SupportVQNode(p->FlowBuffer)) // flowbuffer may not be in ref array (accel. idct)
		return 0;
	return 1;
}

static int UpdateVideoQuality(player_base* p,node* Node)
{
	if (NodeIsClass(Node->Class,IDCT_CLASS))
	{
		int Shift = 2-p->VideoQuality;
		Node->Set(Node,IDCT_SHIFT,&Shift,sizeof(Shift));
	}
	return ERR_NONE;
}

static void ClearStats(player_base* p)
{
	int Zero=0;
	if (p->VOutput)
	{
		p->VOutput->Set(p->VOutput,OUT_TOTAL,&Zero,sizeof(Zero));
		p->VOutput->Set(p->VOutput,OUT_DROPPED,&Zero,sizeof(Zero));
	}

	if (p->AOutput)
	{
		p->AOutput->Set(p->AOutput,OUT_TOTAL,&Zero,sizeof(Zero));
		p->AOutput->Set(p->AOutput,OUT_DROPPED,&Zero,sizeof(Zero));
	}
}

static void StartBench(player_base* p)
{
	int Used;
	p->SaveMicroDrive = p->MicroDrive;
	p->MicroDrive = 0;
	p->InSeek = 0;

	UpdateFormat(p,0);

	if (!QueryAdvanced(ADVANCED_BENCHFROMPOS))
		Seek2(p,0,NULL,0);

	p->BufferMax = p->CurrBufferSize2;
	while (p->Format->Read(p->Format,p->BufferMax,&Used) == ERR_NONE &&
		Used < p->UsedEnough2);

	ClearStats(p);
				
	p->Bench = 1;
	p->BenchStart = GetTimeTick();
	p->Play = 1;
	p->FFwd = 0;
	p->Fill = 0;
	SetPlay(p,0);
}

static void SetTimerNode(player_base* p,node* Timer,bool_t SaveTime)
{
	tick_t Time = 0;

	if (p->Timer && SaveTime)
		p->Timer->Get(p->Timer,TIMER_TIME,&Time,sizeof(Time));

	p->Timer = Timer;

	if (p->Timer)
	{
		p->Timer->Set(p->Timer,TIMER_SPEED,&p->Speed,sizeof(p->Speed));
		p->Timer->Set(p->Timer,TIMER_TIME,&Time,sizeof(Time));
		UpdatePlay(p);
	}

	UpdateRunProcess(p);
}

static node* AddRef(player_base* p,node* Node);
static void ReleaseRef(player_base* p,node* Node);

static bool_t UpdateNode(player_base* p,node* Node);
static void ReleaseNode(player_base* p,node* Node);

static bool_t UpdateOutput(player_base* p,node* From,const datadef* FromDef,int* FoundFirst);

static NOINLINE void NullPin(node* p,int No)
{
	pin Pin;
	Pin.Node = NULL;
	Pin.No = 0;
	p->Set(p,No,&Pin,sizeof(Pin));
}

static void TryComment(player_base* p,node* From,const datadef* FromDef)
{
	pin Pin;
	Pin.Node = (node*)p;
	Pin.No = PLAYER_COMMENT+p->StreamNo;
	From->Set(From,FromDef->No,&Pin,sizeof(Pin));
}

static void ReleaseOutput(player_base* p,node* Node,datadef* DataDef)
{
	node* Null = NULL;
	pin Pin;
	node* Ptr;
	bool_t RdOnly = (DataDef->Flags & DF_RDONLY)!=0;

	switch (DataDef->Type)
	{
	case TYPE_NODE:
		if (Node->Get(Node,DataDef->No,&Ptr,sizeof(Ptr))==ERR_NONE && Ptr)
		{
			ReleaseNode(p,Ptr);
			if (!RdOnly)
			{
				Node->Set(Node,DataDef->No,&Null,sizeof(Null));
				ReleaseRef(p,Ptr);
			}
		}
		break;

	case TYPE_COMMENT:
		if (!RdOnly)
			NullPin(Node,DataDef->No);
		break;

	case TYPE_PACKET:
		if (Node->Get(Node,DataDef->No,&Pin,sizeof(Pin))==ERR_NONE && Pin.Node)
		{
			ReleaseNode(p,Pin.Node);
			if (!RdOnly)
			{
				Disconnect(Node,DataDef->No,Pin.Node,Pin.No);
				ReleaseRef(p,Pin.Node);
			}
		}
	}
}

static void ReleaseNode(player_base* p,node* Node)
{
	if (Node)
	{
		bool_t False = 0;
		datadef DataDef;
		int No;

		if (p->VOutput == Node)
		{
			p->VOutput = NULL;
			p->VIDCT = NULL;
		}

		if (p->AOutput == Node)
		{
			SetTimerNode(p,NodeEnumObject(NULL,SYSTIMER_ID),1); // restore system timer
			p->AOutput = NULL;
		}

		if (p->FlowBuffer == Node)
			p->FlowBuffer = NULL;

		Node->Set(Node,FLOW_BACKGROUND,&False,sizeof(bool_t));

		for (No=0;NodeEnum(Node,No,&DataDef)==ERR_NONE;++No)
			if (DataDef.Flags & DF_OUTPUT)
				ReleaseOutput(p,Node,&DataDef);
	}
}

static node* GetNode(player_base* p,int Class)
{
	node* Node = NULL;
	bool_t ReUse = 0;
	bool_t IDCT = 0;

	if (Class == (int)VOUT_IDCT_CLASS(p->VOutputId) && NodeIsClass(Class,IDCT_CLASS))
	{
		Class = p->VOutputId;
		IDCT = 1;
	}

	if (Class == p->AOutputId || Class == p->VOutputId)
	{
		ref *i;
		for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
			if (i->Node->Class == Class)
			{
				Node = i->Node;
				ReUse = 1;
				break;
			}
	}

	if (!Node)
	{
		Node = NodeCreate(Class);
		if (Node)
		{
			if (NodeIsClass(Class,VOUT_CLASS))
				UpdateVideoEx(p,Node,-1,0);
			if (NodeIsClass(Class,AOUT_CLASS))
				UpdateAudioEx(p,Node);
			UpdateVideoQuality(p,Node);
		}
	}

	if (Node)
	{
		pin Pin;

		if (IDCT)
		{
			node* i;
			Node->Get(Node,VOUT_IDCT,&i,sizeof(i));
			if (!i)
			{
				if (!ReUse) 
					NodeDelete(Node);
				return NULL;
			}
			Node = i;
		}

		Pin.No = PLAYER_NOT_SUPPORTED_DATA;
		Pin.Node = (node*)p;
		Node->Set(Node,FLOW_NOT_SUPPORTED,&Pin,sizeof(Pin));
		AddRef(p,Node);
	}

	return Node;
}

static bool_t TryPin(player_base* p,node* From,int FromNo,int ToClass,int* FoundFirst,int Failed)
{
	node* To;

	if (ToClass == Failed)
		return 0;

	To = GetNode(p,ToClass);
	if (To)
	{
		packetformat Format;
		datadef ToDef;
		int No;
		int Result;

		for (No=0;NodeEnum(To,No,&ToDef)==ERR_NONE;++No)
			if (ToDef.Type == TYPE_PACKET && (ToDef.Flags & DF_INPUT) && !(ToDef.Flags & DF_RDONLY))
			{
				if (To->Get(To,ToDef.No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE && Format.Type != PACKET_NONE)
					continue; // already connected

				Result = ConnectionUpdate(From,FromNo,To,ToDef.No);
				
				if (Result == ERR_NOT_SUPPORTED && FoundFirst)
					*FoundFirst = 2; // found codec, but this type is not supported
				else			
				if (Result == ERR_NONE)
				{
					if (FoundFirst)
						*FoundFirst = 1;

					if (UpdateNode(p,To))
						return 1;
				}

				Disconnect(From,FromNo,To,ToDef.No);
			}

		ReleaseRef(p,To);
	}
	return 0;
}

static bool_t IsCodecSkip(player_base* p,int Id)
{
	int i;
	for (i=0;i<p->CodecSkipCount;++i)
		if (p->CodecSkip[i]==Id)
			return 1;
	return 0;
}

static bool_t UpdateOutput(player_base* p,node* From,const datadef* FromDef,int* FoundFirst)
{
	packetformat Format;
	bool_t EqMissing;
	node* Null = NULL;
	node* To;
	node* Old = NULL;
	int Force = 0;
	int Skip = 0;
	pin Pin;
	bool_t RdOnly = (FromDef->Flags & DF_RDONLY)!=0;
	int Failed = 0;

	if (FoundFirst)
		*FoundFirst = 0;

	switch (FromDef->Type)
	{
	case TYPE_NODE:

		if (!RdOnly)
			switch (FromDef->Format1)
			{
			case IDCT_CLASS:
				if (VOutIDCT(p->VOutputId))
				{
					int IDCT = VOUT_IDCT_CLASS(p->VOutputId);
					if (p->VideoAccel && p->Timing && p->SkipAccelFrom != From->Class)
						Force = IDCT;
					else
						Skip = IDCT;
				}
				break;
			case VOUT_CLASS:
				Force = p->VOutputId;
				break;
			case AOUT_CLASS:
				Force = p->AOutputId;
				break;
			}

		// already connected pointer?
		if (From->Get(From,FromDef->No,&Old,sizeof(Old))==ERR_NONE && Old)
		{
			if (FoundFirst)
				*FoundFirst = 1;

			if ((!Force || Old->Class == Force) && (!Skip || Old->Class != Skip))
			{
				if (UpdateNode(p,Old))
					return 1;

				Failed = Old->Class;
			}
			else
				ReleaseNode(p,Old);
		}

		if (RdOnly)
			return 0;

		if (Force)
		{
			To = GetNode(p,Force);
			if (To && From->Set(From,FromDef->No,&To,sizeof(To))==ERR_NONE)
			{
				ReleaseRef(p,Old);
				Old = To;

				if (FoundFirst)
					*FoundFirst = 1;

				if (UpdateNode(p,To))
					return 1;
			}
			else
				ReleaseRef(p,To);

			if (FromDef->Format1 == IDCT_CLASS)
				p->SkipAccelFrom = From->Class;

			if (Old && !Failed && UpdateNode(p,Old))
				return 1;
		}

		if (FromDef->Format1 != VOUT_CLASS && FromDef->Format1 != AOUT_CLASS)
		{
			int* i;
			array List;
			NodeEnumClass(&List,FromDef->Format1);

			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
				if (Failed != *i && (To = GetNode(p,*i))!=NULL)
				{
					if (From->Set(From,FromDef->No,&To,sizeof(To))==ERR_NONE)
					{
						ReleaseRef(p,Old);
						Old = To;

						if (FoundFirst)
							*FoundFirst = 1;

						if (UpdateNode(p,To))
						{
							ArrayClear(&List);
							return 1;
						}
					}
					else
						ReleaseRef(p,To);
				}

			ArrayClear(&List);
		}

		From->Set(From,FromDef->No,&Null,sizeof(Null));
		ReleaseRef(p,Old);
		break;

	case TYPE_COMMENT:
		if (!RdOnly)
			TryComment(p,From,FromDef);
		return 1;

	case TYPE_PACKET:

		EqMissing = p->Eq && !Used(p,p->Equalizer);

		// already connected pin?
		if (From->Get(From,FromDef->No,&Pin,sizeof(Pin))==ERR_NONE && Pin.Node)
		{
			if (!IsCodecSkip(p,Pin.Node->Class) && (!NodeIsClass(Pin.Node->Class,OUT_CLASS) || Pin.Node->Class == p->VOutputId || 
				(Pin.Node->Class == p->AOutputId && !EqMissing)))
			{
				if (FoundFirst)
					*FoundFirst = 1;

				if (UpdateNode(p,Pin.Node))
					return 1;

				Failed = Pin.Node->Class; // don't try with class again
			}
			else
				ReleaseNode(p,Pin.Node);

			if (!RdOnly)
			{
				Disconnect(From,FromDef->No,Pin.Node,Pin.No);
				ReleaseRef(p,Pin.Node);
			}
		}

		if (RdOnly)
			return 0;

		if (From->Get(From,FromDef->No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE)
		{
			array List;
			const int *i;

			if (Format.Type == PACKET_VIDEO)
			{
				if (!p->VOutputId)
				{
					if (FoundFirst)
						*FoundFirst = 1;
					return 0;
				}

				if (TryPin(p,From,FromDef->No,p->VOutputId,FoundFirst,Failed))
					return 1;
			}

			if (Format.Type == PACKET_AUDIO && !EqMissing)
			{
				if (!p->AOutputId)
				{
					if (FoundFirst)
						*FoundFirst = 1;
					return 0;
				}

				if (TryPin(p,From,FromDef->No,p->AOutputId,FoundFirst,Failed))
					return 1;
			}

			PacketFormatEnumClass(&List,&Format);
			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
				if (!IsCodecSkip(p,*i) && TryPin(p,From,FromDef->No,*i,FoundFirst,Failed))
				{
					ArrayClear(&List);
					return 1;
				}

			if (Format.Type == PACKET_AUDIO && EqMissing)
			{
				if (!p->AOutputId)
				{
					if (FoundFirst)
						*FoundFirst = 1;
					return 0;
				}

				if (TryPin(p,From,FromDef->No,p->AOutputId,FoundFirst,Failed))
					return 1;
			}

			ArrayClear(&List);
		}
		break;
	}
	return 0;
}

static node* AddRef(player_base* p,node* Node)
{
	if (Node)
	{
		bool_t Found;
		ref Ref;
		int No;

		Ref.RefCount = 1;
		if (Node->Get(Node,NODE_PARTOF,&Ref.Node,sizeof(node*)) != ERR_NONE)
			Ref.Node = Node;

		No = ArrayFind(&p->Ref,ARRAYCOUNT(p->Ref,ref),sizeof(ref),&Ref,(arraycmp)CmpRef,&Found);
		if (!Found)
			ArrayAdd(&p->Ref,ARRAYCOUNT(p->Ref,ref),sizeof(ref),&Ref,(arraycmp)CmpRef,64);
		else
			ARRAYBEGIN(p->Ref,ref)[No].RefCount++;
	}
	return Node;
}

static void ReleaseRef(player_base* p,node* Node)
{
	if (Node)
	{
		int No;
		ref Ref;
		bool_t Found;

		if (Node->Get(Node,NODE_PARTOF,&Ref.Node,sizeof(node*)) != ERR_NONE)
			Ref.Node = Node;

		No = ArrayFind(&p->Ref,ARRAYCOUNT(p->Ref,ref),sizeof(ref),&Ref,(arraycmp)CmpRef,&Found);
		if (Found && --(ARRAYBEGIN(p->Ref,ref)[No].RefCount)==0)
		{
			ArrayRemove(&p->Ref,ARRAYCOUNT(p->Ref,ref),sizeof(ref),&Ref,(arraycmp)CmpRef);
			NodeDelete(Ref.Node);
		}
	}
}

static bool_t UpdateNode(player_base* p,node* Node)
{
	bool_t Buffered;

	assert(NodeIsClass(Node->Class,FLOW_CLASS));

	if (!NodeIsClass(Node->Class,OUT_CLASS))
	{
		// update output pins
		datadef DataDef;
		int No;

		for (No=0;NodeEnum(Node,No,&DataDef)==ERR_NONE;++No)
			if (DataDef.Flags & DF_OUTPUT)
				if (!UpdateOutput(p,Node,&DataDef,NULL))
				{
					ReleaseNode(p,Node);
					return 0;
				}
	}

	if (Node->Class == p->VOutputId)
	{
		p->VOutput = Node;
		Node->Set(Node,VOUT_PLAY,&p->TimerPlay,sizeof(bool_t));
		Node->Get(Node,VOUT_IDCT,&p->VIDCT,sizeof(p->VIDCT));
		if (!p->VIDCT)
			p->VideoAccel = 0;

		if (p->FullScreenAfterSync && !p->FullScreen)
			Notify(p,PLAYER_FULLSCREEN,1);
	}

	if (Node->Class == p->AOutputId)
	{
		node* Timer;
		p->AOutput = Node;
		if (Node->Get(Node,AOUT_TIMER,&Timer,sizeof(Timer))==ERR_NONE)
			SetTimerNode(p,Timer,1);
	}

	if (Node->Get(Node,FLOW_BUFFERED,&Buffered,sizeof(Buffered))==ERR_NONE && Buffered)
		p->FlowBuffer = Node;

	return 1;
}

static int ReleaseStreamsNotify(player_base* p,int Param,int Param2)
{
	datadef DataDef; 
	int No;

	for (No=0;NodeEnum((node*)p->Format,No,&DataDef)==ERR_NONE;++No)
	{
		if (DataDef.No >= FORMAT_COMMENT && DataDef.No < FORMAT_COMMENT+MAXSTREAM)
			NullPin((node*)p->Format,DataDef.No);

		if (DataDef.No >= FORMAT_STREAM && DataDef.No < FORMAT_STREAM+MAXSTREAM)
			ReleaseOutput(p,(node*)p->Format,&DataDef);
	}

	return ERR_NONE;
}

static int UpdateStreamsMask(player_base* p)
{
	int Result = UpdateStreams(p,1,0);
	if (Result == ERR_NONE && p->Format)
	{
		tick_t Time;
		p->Timer->Get(p->Timer,TIMER_TIME,&Time,sizeof(tick_t));
		p->Format->Set(p->Format,FORMAT_UPDATEMASK,&Time,sizeof(tick_t));
	}
	return Result;
}

static int UpdateStreamsNotify(player_base* p,int Param,int Param2)
{
	// called by process thread
	// we are inside the LockProcess so Lock/Unlock not needed
	// inputprocess doesn't bother us here
	UpdateStreams(p,-1,0);
	return ERR_NONE;
}

static void ReleaseFormat(player_base* p)
{
	NullPin((node*)p->Format,FORMAT_GLOBAL_COMMENT);
	p->Format->Set(p->Format,FORMAT_INPUT,NULL,0);
	NodeDelete((node*)p->Format);
	p->Format = NULL;
}

static void ErrorCodec(packetformat* Format)
{
	tchar_t Name[64];
	int Id = 0;
	switch (Format->Type)
	{
	case PACKET_VIDEO: 
		Id = PLAYER_VIDEO_NOT_FOUND; 
		break;
	case PACKET_AUDIO: 
#ifdef CONFIG_GPL
		if (Format->Format.Audio.Format == AUDIOFMT_AAC)
			Id = PLAYER_AUDIO_NOT_FOUND2;
		else
#endif
			Id = PLAYER_AUDIO_NOT_FOUND; 
		break;
	}
	if (Id && PacketFormatName(Format,Name,TSIZEOF(Name)))
		ShowError(PLAYER_ID,PLAYER_ID,Id,Name);
}

static int UpdateStreams(player_base* p,int ForceRefresh,bool_t CodecSkip)
{
	packetformat Format;
	datadef DataDef;
	int FoundCodec;
	int No;

	p->UpdateStreamsNeeded = 0;

	if (!p->Format)
		return ERR_INVALID_PARAM;

	// first release unused streams
	for (No=0;NodeEnum((node*)p->Format,No,&DataDef)==ERR_NONE;++No)
		if (DataDef.No >= FORMAT_STREAM && DataDef.No < FORMAT_STREAM+MAXSTREAM)
		{
			p->StreamNo = DataDef.No - FORMAT_STREAM;
			if (!p->StreamSkip[p->StreamNo] && 
				p->Format->Get(p->Format,DataDef.No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE && 
				Format.Type != PACKET_NONE && p->Selected[Format.Type]>=0 && p->Selected[Format.Type] != p->StreamNo)
				ReleaseOutput(p,(node*)p->Format,&DataDef);
		}

	for (No=0;NodeEnum((node*)p->Format,No,&DataDef)==ERR_NONE;++No)
	{
		if (DataDef.No >= FORMAT_COMMENT && DataDef.No < FORMAT_COMMENT+MAXSTREAM)
		{
			p->StreamNo = DataDef.No - FORMAT_COMMENT;
			TryComment(p,(node*)p->Format,&DataDef);
		}

		if (DataDef.No >= FORMAT_STREAM && DataDef.No < FORMAT_STREAM+MAXSTREAM)
		{
			p->StreamNo = DataDef.No - FORMAT_STREAM;
			if (!p->StreamSkip[p->StreamNo] && 
				p->Format->Get(p->Format,DataDef.No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE && 
				Format.Type != PACKET_NONE)
			{
				if (p->Selected[Format.Type]<0)
					p->Selected[Format.Type] = p->StreamNo;

				if (p->Selected[Format.Type] == p->StreamNo && 
					!UpdateOutput(p,(node*)p->Format,&DataDef,&FoundCodec) && FoundCodec!=1)
				{
					if (!FoundCodec && !CodecSkip)
						ErrorCodec(&Format);

					p->Selected[Format.Type] = -1;
					p->StreamSkip[p->StreamNo] = 1;
				}
			}
		}
	}

	UpdateVideo(p,ForceRefresh);
	UpdateTimeouts(p);
	return ERR_NONE;
}

static int UpdateFormat(player_base* p,bool_t BufferChanged)
{
	if (BufferChanged)
		p->BufferWarning = 0;
	p->CurrBufferSize2 = p->MicroDrive ? p->MDBufferSize2 : p->BufferSize2;
	p->BufferMax = p->CurrBufferSize2;
	if (p->Format)
		p->Format->Set(p->Format,FORMAT_BUFFERSIZE,&p->CurrBufferSize2,sizeof(int));
	return ERR_NONE;
}

static void RemoveFilter(player_base* p,node* Filter,int In,int Out)
{
	pin InPin;
	pin OutPin;
	if (Filter->Get(Filter,In,&InPin,sizeof(InPin))==ERR_NONE && InPin.Node &&
		Filter->Get(Filter,Out,&OutPin,sizeof(OutPin))==ERR_NONE && OutPin.Node)
	{
		if (ConnectionUpdate(InPin.Node,InPin.No,OutPin.Node,OutPin.No)==ERR_NONE)
		{
			ConnectionUpdate(NULL,0,Filter,Out);
			ConnectionUpdate(Filter,In,NULL,0);
			ReleaseRef(p,Filter);
		}
		else
		{
			ConnectionUpdate(InPin.Node,InPin.No,Filter,In);
			ConnectionUpdate(Filter,Out,OutPin.Node,OutPin.No);
		}
	}
}

static void InsertFilter(player_base* p,node* Filter,int In,int Out,node* Output,int OutputIn)
{
	pin Pin;
	if (Output->Get(Output,OutputIn,&Pin,sizeof(Pin))==ERR_NONE && Pin.Node)
	{
		if (ConnectionUpdate(Pin.Node,Pin.No,Filter,In)==ERR_NONE &&
			ConnectionUpdate(Filter,Out,Output,OutputIn)==ERR_NONE)
			GetNode(p,Filter->Class);
		else
		{
			ConnectionUpdate(Pin.Node,Pin.No,Output,OutputIn); //try restoring...
			ConnectionUpdate(NULL,0,Filter,Out);
			ConnectionUpdate(Filter,In,NULL,0);
		}
	}
}

static int UpdateEqualizer(player_base* p)
{
	if (p->Equalizer)
		p->Equalizer->Get(p->Equalizer,EQUALIZER_ENABLED,&p->Eq,sizeof(p->Eq));

	if (!p->Eq && Used(p,p->Equalizer))
		RemoveFilter(p,p->Equalizer,CODEC_INPUT,CODEC_OUTPUT);
	else
	if (p->Eq && !Used(p,p->Equalizer) && p->AOutput)
		InsertFilter(p,p->Equalizer,CODEC_INPUT,CODEC_OUTPUT,p->AOutput,OUT_INPUT);

	return ERR_NONE;
}

static void Unload(player_base* p,bool_t KeepPlay,bool_t KeepStreams,bool_t Refresh) 
{
	p->Sync = 0;
	p->Fill = 0;
	p->InSeek = 0;
	p->SeekAfterSync.Num = -1;

	if (!KeepPlay)
		SetPlay(p,1);

	if (!KeepStreams || p->Format)
	{
		int i;
		for (i=0;i<PACKET_MAX;++i)
			p->Selected[i] = -1;
	}

	if (p->Format)
		ReleaseFormat(p);

	if (p->Input)
	{
		NodeDelete((node*)p->Input);
		p->Input = NULL;
	}

	if (p->InputBase)
	{
		NodeDelete((node*)p->InputBase);
		p->InputBase = NULL;
	}

	p->Title[0] = 0;
	Notify(p,PLAYER_TITLE,1);

	assert(ARRAYEMPTY(p->Ref));
	ArrayClear(&p->Ref);
	ArrayClear(&p->Comment);

	p->Position = -1;
	p->NoMoreInput = 0;
	p->CurrentChanged = 1;
	p->BufferWarning = 0;
	p->CodecSkipCount = 0;

	if (!KeepPlay)
		UpdateTimeouts(p);
	UpdateRunInput(p);
	UpdateRunProcess(p);

	if (Refresh)
		Notify(p,PLAYER_PERCENT,1);
}

static void URLToTitle(tchar_t* Title, int TitleLen, const tchar_t* URL)
{
	tchar_t Ext[MAXPATH];
	tchar_t *i,*j;
	bool_t HasHost;
	i = (tchar_t*) GetMime(URL,NULL,0,&HasHost);
	if (i==URL || !HasHost || tcschr(i,'/') || tcschr(i,'\\'))
		SplitURL(URL,NULL,0,NULL,0,Title,TitleLen,Ext,TSIZEOF(Ext));
	else
		tcscpy_s(Title,TitleLen,URL);

	// replace %20 and '_' with space
	for (j=i=Title;*i;++i)
	{
		if (*i=='_')
			*j++ = ' ';
		else
		if (i[0]=='%' && i[1]=='2' && i[2]=='0')
		{
			*j++ = ' ';
			i += 2;
		}
		else
			*j++ = *i;
	}
	*j=0;
}

static bool_t LoadPlaylist(player* Player,const array* List,stream* Input,int* Index,const tchar_t* Path,const void* Probe,int ProbeSize)
{
	int *i;
	bool_t SeekHead = 0;

	for (i=ARRAYBEGIN(*List,int);i!=ARRAYEND(*List,int);++i)
	{
		playlist* Playlist = (playlist*)NodeCreate(*i);
		if (Playlist)
		{
			tchar_t Base[MAXPATH];
			tchar_t Abs[MAXPATH];
			tchar_t ItemPath[MAXPATH];
			tchar_t ItemTitle[256];
			tick_t ItemLength;
			int Pos = *Index;

			if (SeekHead)
			{
				Input->Seek(Input,ProbeSize,SEEK_SET);
				SeekHead = 0;
			}

			if (Probe)
				Playlist->Set(Playlist,PLAYLIST_DATAFEED,Probe,ProbeSize);

			if (Playlist->Set(Playlist,PLAYLIST_STREAM,&Input,sizeof(Input)) == ERR_NONE)
			{
				SplitURL(Path,Base,TSIZEOF(Base),Base,TSIZEOF(Base),NULL,0,NULL,0);

				while (Playlist->ReadList(Playlist,ItemPath,TSIZEOF(ItemPath),ItemTitle,TSIZEOF(ItemTitle),&ItemLength)==ERR_NONE)
				{
					AbsPath(Abs,TSIZEOF(Abs),ItemPath,Base);
					Player->Set(Player,PLAYER_LIST_URL+Pos,Abs,sizeof(Abs));
					Player->Set(Player,PLAYER_LIST_TITLE+Pos,ItemTitle,sizeof(ItemTitle));
					Player->Set(Player,PLAYER_LIST_LENGTH+Pos,&ItemLength,sizeof(ItemLength));
					++Pos;
				}

				SeekHead = 1;
			}

			Playlist->Set(Playlist,PLAYLIST_STREAM,NULL,0);
			NodeDelete((node*)Playlist);

			if (Pos != *Index)
			{
				*Index = Pos;
				return 1;
			}
		}
	}

	if (Probe) // restore file position
		Input->Seek(Input,ProbeSize,SEEK_SET);

	return 0;
}

static void ListSwap(player_base* p,int a,int b)
{
#ifdef MULTITHREAD
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
#endif

	LockEnter(p->Lock);
	if (a < p->PlayListCount && b < p->PlayListCount && a!=b)
	{
		item Tmp;

		Tmp = p->PlayList[a];
		p->PlayList[a] = p->PlayList[b];
		p->PlayList[b] = Tmp;

		if (p->Current == a)
			p->Current = b;
		else
		if (p->Current == b)
			p->Current = a;

		p->PlayList[a].Changed = 1;
		p->PlayList[b].Changed = 1;

		NotifyList(p);
	}
	LockLeave(p->Lock);
}

static int Load(player_base* p,bool_t Silent,bool_t OnlyLocal,bool_t Nested);

static bool_t ReDirect(player_base* p,const tchar_t* New,const tchar_t* Orig,bool_t Silent,bool_t OnlyLocal,bool_t Nested,int* Result)
{
	tchar_t Base[MAXPATH];
	tchar_t Target[MAXPATH];
	SplitURL(Orig,Base,TSIZEOF(Base),Base,TSIZEOF(Base),NULL,0,NULL,0);
	AbsPath(Target,TSIZEOF(Target),New,Base);
	if (tcsicmp(Target,Orig)==0)
		return 0;

	p->Player.Set(p,PLAYER_LIST_URL+p->Current,Target,sizeof(Target));
	*Result = Load(p,Silent,OnlyLocal,Nested);
	return 1;
}

static int Load(player_base* p,bool_t Silent,bool_t OnlyLocal,bool_t Nested)
{
	tchar_t ContentType[MAXPATH];
	array List;
	block Probe;
	tchar_t* URL;
	stream* Input;
	int ProbeSize;
	int Result;
	int No;
	bool_t PosPrevKey = p->SeekAfterSyncPrevKey;
	fraction Pos = p->SeekAfterSync;
	p->SeekAfterSync.Num = -1;
	p->InSeek = 0;
	p->TemporaryHidden = Pos.Num > 0;

	if (p->CurrentChanged || p->PlayList[p->Current].Changed || p->IOError)
	{
		tchar_t Mime[MAXMIME];
		int Length;
		pin Comment;
		bool_t Network;

		if (p->Format)
			Pos.Num = -1; // invalidate last SeekAfterSync

		Unload(p,1,OnlyLocal,0);

		// find input

		URL = p->PlayList[p->Current].URL;
		if (!URL[0])
		{
			if (!Silent)
				UpdateTimeouts(p);
			return ERR_NONE;
		}

		GetMime(URL,Mime,TSIZEOF(Mime),&p->HasHost);
		Network = p->HasHost || (URL[0]=='\\' && URL[1]=='\\');
		if (OnlyLocal && Network)
		{
			if (!Silent)
				UpdateTimeouts(p);
			return ERR_FILE_NOT_FOUND;
		}

		Input = GetStream(URL,Silent);
		if (!Input)
		{
			if (!Silent)
				UpdateTimeouts(p);
			return ERR_NOT_SUPPORTED;
		}

		if (p->PlayList[p->Current].Title[0])
			tcscpy_s(p->Title,TSIZEOF(p->Title),p->PlayList[p->Current].Title);
		else
			URLToTitle(p->Title,TSIZEOF(p->Title),URL);

		memset(p->StreamSkip,0,sizeof(p->StreamSkip));

		UpdateSpeed(p);
		SetTimerNode(p,NodeEnumObject(NULL,SYSTIMER_ID),0); // restore system timer
		p->Input = Input;

		Comment.No = PLAYER_COMMENT;
		Comment.Node = (node*)p;
		p->Input->Set(p->Input,STREAM_COMMENT,&Comment,sizeof(pin));
		p->Input->Set(p->Input,STREAM_SILENT,&Silent,sizeof(Silent));

		Result = p->Input->Set(p->Input,STREAM_URL,URL,TSIZEOF(p->PlayList[p->Current].URL));

		if (Result != ERR_NONE)
		{
			Unload(p,Silent,0,1);
			return Result;
		}

newstream:
		if (p->Input->Get(p->Input,STREAM_LENGTH,&Length,sizeof(int))!=ERR_NONE)
			Length = -1;

		// find format
		if (!AllocBlock(PROBE_SIZE,&Probe,0,HEAP_ANY))
		{
			Unload(p,Silent,0,1);
			return ERR_OUT_OF_MEMORY;
		}

		ProbeSize = p->Input->ReadBlock(p->Input,&Probe,0,PROBE_SIZE);

		// important: get content type after probe was read 
		if (p->Input->Get(p->Input,STREAM_CONTENTTYPE,ContentType,sizeof(ContentType))!=ERR_NONE)
			ContentType[0] = 0;

		if (ProbeSize > 0)
		{ 
			int *i;

			if (!p->InputBase)
			{
				node* Process = NULL;

				NodeEnumClassEx(&List,STREAMPROCESS_CLASS,ContentType,URL,Probe.Ptr,ProbeSize);
				for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
				{
					Process = NodeCreate(*i);
					if (Process)
					{
						Process->Set(Process,STREAM_COMMENT,&Comment,sizeof(pin));
						Process->Set(Process,STREAM_SILENT,&Silent,sizeof(Silent));

						if (Process->Set(Process,STREAMPROCESS_INPUT,&p->Input,sizeof(node*)) == ERR_NONE)
							break;

						NodeDelete(Process);
					}
				}
				
				ArrayClear(&List);

				if (Process)
				{
					p->InputBase = p->Input;
					p->Input = (stream*)Process;
					FreeBlock(&Probe);
					goto newstream;
				}
			}

			NodeEnumClassEx(&List,FORMAT_CLASS,ContentType,URL,Probe.Ptr,ProbeSize);
			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
			{
				p->Format = (format*)NodeCreate(*i);
				if (p->Format)
				{
					notify Notify;
					int Align = p->HasHost?1:ALIGN_SIZE;
					bool_t FindSubtitles = !p->HasHost;
					UpdateFormat(p,0); // set buffersize as soon as possible
					p->Format->Set(p->Format,FORMAT_AUTO_READSIZE,&Network,sizeof(bool_t));
					p->Format->Set(p->Format,FORMAT_FILEALIGN,&Align,sizeof(Align));
					p->Format->Set(p->Format,FORMAT_DATAFEED,Probe.Ptr,ProbeSize);
					p->Format->Set(p->Format,FORMAT_FIND_SUBTITLES,&FindSubtitles,sizeof(FindSubtitles));
					p->Format->Set(p->Format,FORMAT_GLOBAL_COMMENT,&Comment,sizeof(pin));

					Notify.This = p;
					Notify.Func = (notifyfunc)UpdateStreamsNotify;
					p->Format->Set(p->Format,FORMAT_UPDATESTREAMS,&Notify,sizeof(Notify));

					Notify.This = p;
					Notify.Func = (notifyfunc)ReleaseStreamsNotify;
					p->Format->Set(p->Format,FORMAT_RELEASESTREAMS,&Notify,sizeof(Notify));

					if (p->Format->Set(p->Format,FORMAT_INPUT,&p->Input,sizeof(stream*)) == ERR_NONE)
						break;

					ReleaseFormat(p);

					if (p->Input->Seek(p->Input,0,SEEK_CUR) > PROBE_SIZE) //maybe less if file smaller as PROBE_SIZE
						p->Input->Seek(p->Input,PROBE_SIZE,SEEK_SET);
				}
			}
			ArrayClear(&List);

			if (!p->Format && !Nested)
			{
				NodeEnumClassEx(&List,PLAYLIST_CLASS,ContentType,URL,Probe.Ptr,ProbeSize);
				if (!ARRAYEMPTY(List))
				{
					int Index = p->PlayListCount;
					if (LoadPlaylist(&p->Player,&List,p->Input,&Index,URL,Probe.Ptr,ProbeSize))
					{
						FreeBlock(&Probe);
						// remove this from playlist
						for (No=p->Current+1;No<p->PlayListCount;++No)
							ListSwap(p,No-1,No);

						SetPlayListCount(p,p->PlayListCount-1);
						return Load(p,Silent,OnlyLocal,1);
					}
				}
				ArrayClear(&List);
			}
		}

		FreeBlock(&Probe);

		if (!p->Format)
		{
			Unload(p,0,0,1);
			if (ProbeSize < 0)
				ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,URL);
			else
			if (!CheckExts(URL,T("exe:P")))
				ShowError(PLAYER_ID,PLAYER_ID,PLAYER_FORMAT_NOT_FOUND,URL);
			return ERR_NOT_SUPPORTED;
		}

		if (p->Player.CommentByName(p,-1,PlayerComment(COMMENT_REDIRECT),ContentType,TSIZEOF(ContentType)) &&
			ReDirect(p,ContentType,URL,Silent,OnlyLocal,Nested,&Result))
			return Result;

		// only after format initialized (may have reopened http connection with different pragma)
		p->Streaming = p->HasHost && 
			(p->Input->Get(p->Input,STREAM_LENGTH,&Length,sizeof(int))!=ERR_NONE ||	Length<0);
		p->Timing = 1;
		p->Format->Get(p->Format,FORMAT_TIMING,&p->Timing,sizeof(p->Timing));

		p->SkipAccelFrom = 0;
		p->PlayList[p->Current].Changed = 0;
		p->CurrentChanged = p->HasHost; // force reload when example connection stops
		p->NoMoreInput = 0;
		p->Position = 0;
		p->BufferMax = p->CurrBufferSize2;
		p->Sync = 1; // start with syncing
		p->PosNotify = -1;
		p->MinBuffer = 0;

		Notify(p,PLAYER_TITLE,1);

		Flush(p);

		if (Pos.Num > 0)
		{
			p->SeekAfterSync = Pos; // restore syncaftersync (now we are sure there will a synced event)
			p->SeekAfterSyncPrevKey = PosPrevKey;
		}

		StopLoadMode(p);
		UpdatePlay(p);
	}
	else
	{
		UpdateFormat(p,0);
		UpdateStreams(p,0,0);
		ClearStats(p);

		if (Pos.Num > 0)
			Result = Seek2(p,-1,&Pos,1);
		else
			Result = Seek2(p,0,NULL,0);

		if (Result != ERR_NONE)
			return Result;
	}

	UpdateRunInput(p);
	UpdateRunProcess(p);
	UpdateTimeouts(p);

#ifdef MULTITHREAD
	EventSet(p->EventProcess); // wakeup input threads (if needed)
#else
	p->WaitForProcess = 0;
#endif

	return ERR_NONE;
}

static int InputThread(player_base* p);
static int ProcessThread(player_base* p);

static void Unlock(player_base* p, bool_t Input)
{
#ifdef MULTITHREAD
	//DEBUG_MSG2(DEBUG_PLAYER,T("Unlocked %d %08x"),p->LockCount,GetCurrentThreadId());
	if (Input) 
	{
		LockLeave(p->LockInput);
		--p->LockInputCount;
		assert(p->LockInputCount>=0);
	}
	LockLeave(p->LockProcess);
	--p->LockProcessCount;
	assert(p->LockProcessCount>=0);
	LockLeave(p->Lock);
#endif
}

static void Lock(player_base* p, bool_t Input)
{
#ifdef MULTITHREAD
	//DEBUG_MSG2(DEBUG_PLAYER,T("Locked Start %d %08x"),p->LockCount,GetCurrentThreadId());
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
	LockEnter(p->Lock);
	p->LockProcessCount++;
	LockEnter(p->LockProcess);
	if (Input)
	{
		p->LockInputCount++;
		LockEnter(p->LockInput);
	}
	//DEBUG_MSG(DEBUG_PLAYER,T("Locked End"));
#endif
}

static int UpdateWnd(player_base* p,void* Wnd)
{
	if (Wnd != p->Wnd)
	{
		p->Wnd = Wnd;
		if (p->Wnd)
		{
#ifdef MULTITHREAD
			p->InputThread = ThreadCreate(InputThread,p,25);
			p->ProcessThread = ThreadCreate(ProcessThread,p,25);
			p->InputPriority = 0;
			p->ProcessPriority = 0;
#endif
			Lock(p,1);
			if (p->PlayListCount && !p->DiscardList)
				Load(p,1,1,0);
			else
			{
				SetPlayListCount(p,0);
				p->SeekAfterSync.Num = -1;
			}
			Unlock(p,1);
		}
		else
		{
			Lock(p,1);
			if (!p->KeepList)
				SetPlayListCount(p,0);

#ifndef REGISTRY_GLOBAL
			NodeRegSave((node*)p);
			NodeRegSave(p->Color);
			NodeRegSave(p->Equalizer);
#endif
			Unlock(p,1);

#ifdef MULTITHREAD
			p->RunProcess = 0;
			ThreadPriority(p->ProcessThread,0);
			EventSet(p->EventRunProcess);
			ThreadTerminate(p->ProcessThread);
			p->ProcessThread = NULL;

			p->RunInput = 0;
			ThreadPriority(p->InputThread,0);
			EventSet(p->EventProcess);
			EventSet(p->EventRunInput);
			ThreadTerminate(p->InputThread);
			p->InputThread = NULL;
#endif
			Unload(p,0,0,0);
		}
	}
	return ERR_NONE;
}

static int NextPrev(player_base* p,int Dir,bool_t StopIfEnd,bool_t NotTheSame)
{
	if (p->PlayListCount > 0)
	{
		int Index = FindCurrentIndex(p);
		int Tries;
		
		for (Tries=NotTheSame?1:0;Tries<p->PlayListCount;++Tries)
		{
			Index += Dir;
			if (Index >= p->PlayListCount || Index < 0)
			{
				if (StopIfEnd && p->ExitAtEnd)
					Notify(p,PLAYER_EXIT_AT_END,0);

				if (StopIfEnd && !p->Repeat)
				{
					SetPlay(p,1);
					Notify(p,PLAYER_PERCENT,0);
					return ERR_NONE;
				}
				RandomizePlayIndex(p,0);
				Index -= Dir*p->PlayListCount;
			}
			if (p->Current != p->PlayIndex[Index])
			{
				p->Current = p->PlayIndex[Index];
				p->CurrentChanged = 1;
			}
			if (Load(p,1,0,0) == ERR_NONE)
				return ERR_NONE;
			if (!Dir)
				break;
		}

		SetPlay(p,1);
		Notify(p,PLAYER_PERCENT,0);
	}
	else
		Unload(p,0,0,1);

	return ERR_NEED_MORE_DATA;
}

static int ProcessThread(player_base* p)
{
	int Result = ERR_NONE;

#ifdef MULTITHREAD
	SAFE_BEGIN

	while (p->Wnd)
	{
		LockEnter(p->LockProcess);
#ifndef NDEBUG
		p->ProcessLocked = ThreadId();
#endif
#endif

		if (p->RunProcess)
		{
			processstate State;
			State.Fill = p->Fill;

			p->Timer->Get(p->Timer,TIMER_TIME,&State.Time,sizeof(tick_t));

			//DEBUG_MSG1(DEBUG_PLAYER,T("Process Time:%d"),State.Time);

			Result = p->Format->Process(p->Format,&State);

			if (Result == ERR_SYNCED)
			{
				DEBUG_MSG3(DEBUG_PLAYER,T("Synced: %d (%d,%d)"),State.Time,p->SeekAfterSync.Num,p->SeekAfterSync.Den);

				p->Sync = 0;
				p->Position = State.Time;
				if (State.Time < 0)
					State.Time = 0;
				p->Timer->Set(p->Timer,TIMER_TIME,&State.Time,sizeof(tick_t));

				if (!p->Bench && p->SeekAfterSync.Num<0)
				{
					// enter fill mode (don't stop processing, even if playing if off)
					p->Fill = 1;
				}

				UpdateBuffer(p);
				UpdatePlay(p);
				UpdateRunProcess(p);
				UpdateTimeouts(p); // we are ready to check VOutput and update timeouts
			}
			else
			if (p->Fill && (Result == ERR_END_OF_FILE || Result == ERR_BUFFER_FULL
				|| (Result == ERR_NEED_MORE_DATA && (p->NoMoreInput || State.BufferUsedAfter >= p->CurrBufferSize2-2))))
			{
				DEBUG_MSG1(DEBUG_PLAYER,T("Filled: %d"),State.Time);
				
				if (Result == ERR_END_OF_FILE || Result == ERR_NEED_MORE_DATA)
					Result = ERR_BUFFER_FULL;

				p->Fill = 0;
				UpdatePlay(p);
				UpdateRunProcess(p);
			}

			if (Result == ERR_NEED_MORE_DATA)
			{
				p->BufferMax = p->CurrBufferSize2;

				if (p->NoMoreInput)
				{
					// IOError (Format doesn't know about NoMoreInput)
					Result = ERR_END_OF_FILE;
				}
#ifdef MULTITHREAD
				else
				if (!p->LoadMode && (p->Play || p->FFwd) && !p->Bench && !p->Sync && !p->Fill)
				{
					if (State.BufferUsedAfter >= (p->CurrBufferSize2>>1))
					{
						if (!p->BufferWarning && State.BufferUsedAfter >= p->CurrBufferSize2-2)
						{
							p->BufferWarning = 1;
							ShowError(PLAYER_ID,PLAYER_ID,PLAYER_BUFFER_WARNING);
						}
					}
					else
					{
						DEBUG_MSG(DEBUG_TEST,T("LoadMode Enter"));
						p->LoadMode = 1;
						p->Fill = 1;
						UpdatePlay(p); // UpdatetPriority(p) called too
						UpdateRunProcess(p);
						UpdateRunInput(p);
						Notify(p,PLAYER_LOADMODE,p->LoadMode);
					}
				}
#endif
			}

			if (!p->Sync && p->Position>=0)
				p->Position = State.Time;

#ifdef MULTITHREAD
			LockLeave(p->LockProcess);
#ifndef NDEBUG
			p->ProcessLocked = 0;
#endif
#endif

			if (p->UpdateStreamsNeeded)
			{
				Lock(p,1);
				UpdateStreams(p,-1,1);
				Seek2(p,p->Position>=0?p->Position:0,NULL,0);
				Unlock(p,1);
				Result = ERR_NONE;
			}

			switch (Result)
			{
			case ERR_SYNCED:

				Notify(p,PLAYER_TITLE,0);

				if (p->Current < p->PlayListCount && !p->PlayList[p->Current].Changed && 
					p->PlayList[p->Current].Length<0)
				{
					tick_t Duration;
					Lock(p,0);
					if (p->Format && p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))==ERR_NONE)
						UpdateListLength(p,Duration);
					Unlock(p,0);
				}

				if (p->TemporaryHidden)
				{
					Lock(p,0);
					p->TemporaryHidden = 0;
					UpdateVideo(p,p->SeekAfterSync.Num>=0?-1:0);
					Unlock(p,0);
				}

				if (!p->VOutput && p->FullScreen)
					Notify(p,PLAYER_FULLSCREEN,0);

				p->FullScreenAfterSync = 0;
				p->PosNotify = State.Time;

				if (p->SeekAfterSync.Num>=0)
				{
					Lock(p,1);
					if (p->SeekAfterSync.Num>=0) // may have changed
					{
						fraction Pos;
						Pos = p->SeekAfterSync;
						p->SeekAfterSync.Num = -1;
						Seek2(p,-1,&Pos,p->SeekAfterSyncPrevKey);
					}
					Unlock(p,1);
				}
				break;

			case ERR_END_OF_FILE:
				if ((p->Play || p->FFwd || p->Sync) && !p->InSeek)
				{
					Lock(p,1);
					if (p->Bench || p->IOError)
						SetPlay(p,1); // stop benchmarking and show results
					else
					{
						tick_t Duration;
						bool_t NotTheSame = p->Sync || 
							(State.Time < TICKSPERSEC && p->Format &&
							(p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))!=ERR_NONE || Duration > TICKSPERSEC*2));

						NextPrev(p,1,1,NotTheSame);
					}
					Unlock(p,1);
				}
				break;

			case ERR_NEED_MORE_DATA:
				//DEBUG_MSG(DEBUG_PLAYER,T("Process Needs More Data Sleep"));

#ifdef MULTITHREAD
				if (p->InputPriority>0)
				{
					p->InputPriority = 0;
					ThreadPriority(p->InputThread,p->InputPriority);
				}
				p->WaitForProcess = 0;
				EventSet(p->EventProcess); // signal InputThread to coninue
				ThreadSleep(p->Sync?4:25);
#else
				p->WaitForProcess = 0;
				InputThread(p);
				Result = ERR_NONE; // should not return ERR_BUFFER_FULL
#endif
				break;

			case ERR_BUFFER_FULL: // no packets processed, we should wait
				//DEBUG_MSG(DEBUG_PLAYER,T("Process Buffer Full Sleep"));

#ifdef MULTITHREAD
				ThreadSleep(4); // this should be more intelligent...
#else
				if (!p->WaitForProcess)
					Result = InputThread(p);
#endif
				break;

			default:
#ifdef MULTITHREAD
				if (p->WaitForProcess && State.BufferUsedAfter<State.BufferUsedBefore)
				{
					EventSet(p->EventProcess); // signal InputThread to coninue
					if (p->Sync)
						ThreadSleep(5); // invalid media could block everything (going through the file)
				}

				if (p->LockProcessCount > 0 || (!p->Sync && (State.BufferUsedAfter<p->UsedEnough2 || p->Clipping || !p->TimerPlay)))
				{
					DEBUG_MSG(DEBUG_PLAYER,T("Process Sleep"));
					ThreadSleep(0);
				}
#else
				if (p->WaitForProcess && State.BufferUsedAfter<State.BufferUsedBefore)
					p->WaitForProcess = 0;

				if (!p->WaitForProcess && Result != ERR_DROPPING && !p->Sync && !p->Fill)
					InputThread(p);
#endif

				break;
			}

			if (State.Time >= 0 && State.Time >= p->PosNotify && !p->Sync && !p->Bench && p->SeekAfterSync.Num<0)
			{
				// should be last, because notify handler could call any player function (load,unload,etc...)
				p->PosNotify = State.Time + p->PosNotifyStep;
				Notify(p,PLAYER_PERCENT,1);
			}
		}
#ifdef MULTITHREAD
		else
		{
			LockLeave(p->LockProcess);
#ifndef NDEBUG
			p->ProcessLocked = 0;
#endif
			EventWait(p->EventRunProcess,-1);
		}
	}

	SAFE_END
	return 0;
}
#else
		else
		{
			if (p->RunInput && !p->WaitForProcess)
				Result = InputThread(p); // no decoding process -> still read input in background
			else
				Result = ERR_STOPPED;
		}

	return Result;
}
#endif

static int InputThread(player_base* p)
{
	int Result = ERR_NONE;

#ifdef MULTITHREAD
	SAFE_BEGIN

	p->IOError = 0;
	while (p->Wnd)
	{
		LockEnter(p->LockInput);
#ifndef NDEBUG
		p->InputLocked = ThreadId();
#endif
#else

	if (!p->WaitForProcess)
#endif
		if (p->RunInput)
		{
			int Used;
			Result = p->Format->Read(p->Format,p->BufferMax,&Used);

			if (Result == ERR_DEVICE_ERROR)
			{
				if (++p->IOError >= 20)
					Result = ERR_END_OF_FILE;
			}
			else
				p->IOError = 0;

			if (Result == ERR_END_OF_FILE)
			{
				p->NoMoreInput = 1;
				UpdateRunInput(p);
			}

			//DEBUG_MSG3(-1,T("Input read %d (%d) max:%d"),Used,Result,p->BufferMax);

#ifdef MULTITHREAD
			LockLeave(p->LockInput);
#ifndef NDEBUG
			p->InputLocked = 0;
#endif
#endif

			switch (Result)
			{
#ifdef MULTITHREAD
			case ERR_DEVICE_ERROR:
				ThreadSleep(100);
				break;
#endif
			case ERR_NEED_MORE_DATA:
				ThreadSleep(2);
				break;

			case ERR_BUFFER_FULL:

				if (p->LoadMode)
				{
					Lock(p,0);
					StopLoadMode(p);
					Unlock(p,0);
				}

				if (p->MicroDrive && p->BufferMax==p->CurrBufferSize2)
				{
					// buffer fully loaded, wait until it's almost empty
					p->BufferMax = p->BurstStart;
					//DEBUG_MSG1(-1,T("MicroDrive Full: new max:%d"),p->BufferMax);
				}

				DEBUG_MSG(DEBUG_PLAYER,T("Input waiting for process"));

				p->WaitForProcess = 1;

#ifdef MULTITHREAD
				EventWait(p->EventProcess,p->RunProcess?2000:-1);
				p->WaitForProcess = 0;
				DEBUG_MSG(DEBUG_PLAYER,T("Input waiting over"));
#endif
				break;

			case ERR_END_OF_FILE:
				if (p->LoadMode)
				{
					Lock(p,0);
					StopLoadMode(p);
					Unlock(p,0);
				}
				Result = ERR_NONE;
			
				//no break;
			default:
				if (p->BufferMax != p->CurrBufferSize2)
				{
					// microdrive started loading, change back to full buffer
					//DEBUG_MSG(-1,T("MicroDrive needs data"));
					p->BufferMax = p->CurrBufferSize2;
				}

				if (p->LoadMode && !p->Fill && Used >= (p->VOutput ? (p->Streaming ? VIDEO_STREAMING_UNDERRUN:((p->CurrBufferSize2 * p->UnderRun)/PERCENT_ONE)) : ((p->AudioUnderRun*1024)/BLOCKSIZE)))
				{
					Lock(p,0);
					StopLoadMode(p);
					Unlock(p,0);
				}

#ifdef MULTITHREAD
				if (p->MicroDrive && Used >= (MINBUFFER/BLOCKSIZE))
					ThreadSleep(2); // try to give some time to ProcessThread during read bursts
				else
				if (p->LockInputCount > 0 || (Used>=p->UsedEnough2+p->MinBuffer*2))
					ThreadSleep(0);
#endif
				break;
			}
		}
#ifdef MULTITHREAD
		else
		{
			LockLeave(p->LockInput);
#ifndef NDEBUG
			p->InputLocked = 0;
#endif
			EventWait(p->EventRunInput,-1);
		}
	}

	SAFE_END
	return 0;
}
#else
	return Result;
}
#endif

static void AddComment(player_base* p,int Stream,const tchar_t* Comment)
{
	const tchar_t* s = tcschr(Comment,'=');
	if (s)
	{
		uint8_t *i;
		tchar_t Null = 0;
		size_t Name = s-Comment;
		++s;

		LockEnter(p->LockComment);

		for (i=ARRAYBEGIN(p->Comment,uint8_t);i!=ARRAYEND(p->Comment,uint8_t);i=(uint8_t*)CommentNext(i))
			if (CommentStream(i) == Stream && Name==(int)tcslen(CommentName(i)) &&
				tcsnicmp(CommentName(i),Comment,Name)==0)
			{
				const uint8_t* Next = CommentNext(i);
				if (Next == ARRAYEND(p->Comment,uint8_t))
				{
					ArrayAppend(&p->Comment,NULL,i-Next,512); //shrink
					break;
				}
				*(tchar_t*)i = (tchar_t)INVALID_COMMENT; //invalidate old comment
			}

		ArrayAppend(&p->Comment,&Stream,sizeof(tchar_t),512);
		ArrayAppend(&p->Comment,Comment,sizeof(tchar_t)*Name,512);
		ArrayAppend(&p->Comment,&Null,sizeof(tchar_t),512);
		ArrayAppend(&p->Comment,s,sizeof(tchar_t)*(tcslen(s)+1),512);
		LockLeave(p->LockComment);

		if (tcsnicmp(Comment,PlayerComment(COMMENT_TITLE),Name)==0 || 
			tcsnicmp(Comment,PlayerComment(COMMENT_ARTIST),Name)==0 ||
			tcsnicmp(Comment,PlayerComment(COMMENT_AUTHOR),Name)==0)
			UpdateListTitle(p);
	}
}

static tick_t GetPosition(player_base* p,tick_t* OutDuration)
{
	tick_t Duration = 1;
	tick_t Time = p->Timing?p->Position:-1;

	if (p->Format && p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))==ERR_NONE && Duration>=0 && Time>=0)
	{
		if (p->InSeek && p->InSeekPos.Num>=0)
			Time = Scale(Duration,p->InSeekPos.Num,p->InSeekPos.Den);
		else
		if (p->SeekAfterSync.Num>=0)
			Time = Scale(Duration,p->SeekAfterSync.Num,p->SeekAfterSync.Den);
	}

	if (OutDuration)
		*OutDuration = Duration;

	return Time;
}

static int GetTimerString(player_base* p,tchar_t* Out,int OutLen)
{
	tick_t Duration;
	tick_t Time = GetPosition(p,&Duration);

	if (Time<0)
		Out[0] = 0;
	else
	{
		if (p->TimerLeft)
		{
			Time = Time - Duration;
			if (Time >= 0)
				Time = -1;
		}

		TickToString(Out,OutLen,Time,0,0,0);
	}
	return ERR_NONE;
}

static int UpdateAllVideoQuality(player_base* p)
{
	const ref *i;
	for (i=ARRAYBEGIN(p->Ref,ref);i!=ARRAYEND(p->Ref,ref);++i)
		UpdateVideoQuality(p,i->Node);
	return ERR_NONE;
}

static int Get(player_base* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;

#ifdef MULTITHREAD
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
#endif

	LockEnter(p->Lock);

	if (No >= PLAYER_ARRAY)
	{
		if (No >= PLAYER_LIST_URL && No < PLAYER_LIST_URL+p->PlayListCount)
		{
			No -= PLAYER_LIST_URL;
			tcscpy_s((tchar_t*)Data,Size/sizeof(tchar_t),p->PlayList[No].URL);
			Result = ERR_NONE;
		}
		else
		if (No >= PLAYER_LIST_TITLE && No < PLAYER_LIST_TITLE+p->PlayListCount)
		{
			No -= PLAYER_LIST_TITLE;
			tcscpy_s((tchar_t*)Data,Size/sizeof(tchar_t),p->PlayList[No].Title);
			Result = ERR_NONE;
		}
		else
		if (No >= PLAYER_LIST_AUTOTITLE && No < PLAYER_LIST_AUTOTITLE+p->PlayListCount)
		{
			No -= PLAYER_LIST_AUTOTITLE;
			if (p->PlayList[No].Title[0])
				tcscpy_s((tchar_t*)Data,Size/sizeof(tchar_t),p->PlayList[No].Title);
			else
				URLToTitle((tchar_t*)Data,Size/sizeof(tchar_t),p->PlayList[No].URL);
			Result = ERR_NONE;
		}
		else
		if (No >= PLAYER_LIST_LENGTH && No < PLAYER_LIST_LENGTH+p->PlayListCount)
		{
			No -= PLAYER_LIST_LENGTH;
			*(tick_t*)Data = p->PlayList[No].Length;
			Result = ERR_NONE;
		}
	}
	else
	switch (No)
	{
	case PLAYER_VIDEO_OVERLAY: GETVALUE(p->Overlay,bool_t); break;
	case PLAYER_BUFFER_SIZE: GETVALUE((p->BufferSize2*BLOCKSIZE)/1024,int); break;
	case PLAYER_MD_BUFFER_SIZE: GETVALUE((p->MDBufferSize2*BLOCKSIZE)/1024,int); break;
	case PLAYER_BURSTSTART: GETVALUE((p->BurstStart*BLOCKSIZE)/1024,int); break;
	case PLAYER_MICRODRIVE: GETVALUE(p->MicroDrive,bool_t); break;
	case PLAYER_UNDERRUN: GETVALUE(p->UnderRun,int); break;
	case PLAYER_AUDIO_UNDERRUN: GETVALUE(p->AudioUnderRun,int); break;
	case PLAYER_REPEAT: GETVALUE(p->Repeat,bool_t); break;
	case PLAYER_SHUFFLE: GETVALUE(p->Shuffle,bool_t); break;
	case PLAYER_PLAYATOPEN: GETVALUE(p->PlayAtOpen,bool_t); break;
	case PLAYER_PLAYATOPEN_FULL: GETVALUE(p->PlayAtOpenFull,bool_t); break;
	case PLAYER_EXIT_AT_END: GETVALUE(p->ExitAtEnd,bool_t); break;
	case PLAYER_KEEPPLAY_VIDEO: GETVALUE(p->KeepPlayVideo,bool_t); break;
	case PLAYER_SHOWINBACKGROUND: GETVALUE(p->ShowInBackground,bool_t); break;
	case PLAYER_SINGLECLICKFULLSCREEN: GETVALUE(p->SingleClickFullScreen,bool_t); break;
	case PLAYER_KEEPPLAY_AUDIO: GETVALUE(p->KeepPlayAudio,bool_t); break;
	case PLAYER_KEEPLIST: GETVALUE(p->KeepList,bool_t); break;
	case PLAYER_MOVEBACK_STEP: GETVALUE(p->MoveBack,tick_t); break;
	case PLAYER_MOVEFFWD_STEP: GETVALUE(p->MoveFFwd,tick_t); break;
	case PLAYER_PLAY: GETVALUE(p->Play,bool_t); break;
	case PLAYER_FFWD: GETVALUE(p->FFwd,bool_t); break;
	case PLAYER_PLAY_SPEED: GETVALUE(p->PlaySpeed,fraction); break;
	case PLAYER_FFWD_SPEED: GETVALUE(p->FFwdSpeed,fraction); break;
	case PLAYER_TITLE: GETSTRING(p->Title); break;
	case PLAYER_INPUT: GETVALUE(p->Input,stream*); break;
	case PLAYER_FORMAT: GETVALUE(p->Format,format*); break;
	case PLAYER_AOUTPUTID: GETVALUE(p->AOutputId,int); break;
	case PLAYER_VOUTPUTID: GETVALUE(p->VOutputId,int); break;
	case PLAYER_AOUTPUTID_MAX: GETVALUE(p->AOutputIdMax,int); break;
	case PLAYER_VOUTPUTID_MAX: GETVALUE(p->VOutputIdMax,int); break;
	case PLAYER_AOUTPUT: GETVALUE(p->AOutput,node*); break;
	case PLAYER_VOUTPUT: GETVALUE(p->VOutput,node*); break;
	case PLAYER_LIST_COUNT: GETVALUE(p->PlayListCount,int); break;
	case PLAYER_LIST_CURRENT: GETVALUE(p->Current,int); break;
	case PLAYER_LIST_CURRIDX: GETVALUE(FindCurrentIndex(p),int); break;
	case PLAYER_VIDEO_ACCEL: GETVALUE(p->VideoAccel,bool_t); break;
	case PLAYER_AUDIO_QUALITY: GETVALUE(p->AudioQuality,int); break;
	case PLAYER_FULLSCREEN: GETVALUE(p->FullScreen,bool_t); break;
	case PLAYER_AUTOPREROTATE: GETVALUE(p->AutoPreRotate,bool_t); break;
	case PLAYER_SKIN_VIEWPORT: GETVALUE(p->SkinViewport,rect); break;
	case PLAYER_CLIPPING: GETVALUE(p->Clipping,bool_t); break;
	case PLAYER_ASPECT: GETVALUE(p->Aspect,fraction); break;
	case PLAYER_STEREO: GETVALUE(p->Stereo,int); break;
	case PLAYER_FULL_ZOOM: GETVALUE(p->FullZoom,fraction); break;
	case PLAYER_SKIN_ZOOM: GETVALUE(p->SkinZoom,fraction); break;
	case PLAYER_SMOOTH50: GETVALUE(p->SmoothZoom50,bool_t); break;
	case PLAYER_SMOOTHALWAYS: GETVALUE(p->SmoothZoomAlways,bool_t); break;
	case PLAYER_FULL_DIR: GETVALUE(p->FullDir,int); break;
	case PLAYER_SKIN_DIR: GETVALUE(p->SkinDir,int); break;
	case PLAYER_REL_DIR: GETVALUE(p->RelDir,int); break;
	case PLAYER_BENCHMARK_SRC: GETVALUE(p->BenchSrc,point); break;
	case PLAYER_BENCHMARK_DST: GETVALUE(p->BenchDst,point); break;
	case PLAYER_BENCHMARK: GETVALUECOND(p->BenchTime,tick_t,!p->Bench); break;
	case PLAYER_POSITION: GETVALUECOND(GetPosition(p,NULL),tick_t,p->Position>=0); break;
	case PLAYER_LOADMODE: GETVALUE(p->LoadMode,bool_t); break;
	case PLAYER_NOTIFY: GETVALUE(p->Notify,notify); break;
	case PLAYER_LIST_NOTIFY: GETVALUE(p->ListNotify,notify); break;
	case PLAYER_VSTREAM: GETVALUE(p->Selected[PACKET_VIDEO],int); break;
	case PLAYER_ASTREAM: GETVALUE(p->Selected[PACKET_AUDIO],int); break;
	case PLAYER_SUBSTREAM: GETVALUE(p->Selected[PACKET_SUBTITLE],int); break;
	case PLAYER_SYNCING: GETVALUE(p->Sync || !p->Format,bool_t); break;
	case PLAYER_BACKGROUND: GETVALUE(p->Background,bool_t); break;
	case PLAYER_FOREGROUND: GETVALUE(p->Foreground,bool_t); break;
	case PLAYER_STREAMING: GETVALUE(p->Streaming,bool_t) break;
	case PLAYER_INSEEK: GETVALUE(p->InSeek,bool_t); break;
	case PLAYER_DISCARDLIST: GETVALUE(p->DiscardList,bool_t); break;
	case PLAYER_VIDEO_QUALITY: GETVALUE(p->VideoQuality,int); if (!SupportVQ(p)) Result = ERR_NOT_SUPPORTED; break;

	case PLAYER_PERCENT:
		if (p->Format && p->Input)
		{
			fraction* Percent = (fraction*)Data;
			tick_t Duration;
			int FilePos;
			int FileSize;
			tick_t Position = p->Position;

			if (p->InSeek && p->InSeekPos.Num>=0)
			{
				Percent->Num = p->InSeekPos.Num;
				Percent->Den = p->InSeekPos.Den;
				Result = ERR_NONE;
			}
			else
			if (p->SeekAfterSync.Num>=0)
			{
				Percent->Num = p->SeekAfterSync.Num;
				Percent->Den = p->SeekAfterSync.Den;
				Result = ERR_NONE;
			}
			else
			if (Position >= 0 && p->Format->Get(p->Format,FORMAT_DURATION,&Duration,sizeof(Duration))==ERR_NONE)
			{
				UpdateListLength(p,Duration);
				Percent->Num = Position;
				Percent->Den = Duration;
				Result = ERR_NONE;
			}
			else
			if (Position==0 && !p->Streaming && p->Timing)
			{
				Percent->Num = 0;
				Percent->Den = 0;
				Result = ERR_NONE;
			}
			else
			if (p->Input->Get(p->Input,STREAM_LENGTH,&FileSize,sizeof(FileSize))==ERR_NONE &&
				p->Format->Get(p->Format,FORMAT_FILEPOS,&FilePos,sizeof(FilePos))==ERR_NONE)
			{
				Percent->Num = FilePos;
				Percent->Den = FileSize;
				Result = ERR_NONE;
			}
		}
		break;

	case PLAYER_CURRENTDIR: GETSTRING(p->CurrentDir); break;
	case PLAYER_MUTE: GETVALUE(RefreshAudio(p)->Mute,bool_t); break;
	case PLAYER_VOLUME: GETVALUE(RefreshAudio(p)->Volume,int); break;
	case PLAYER_PAN: GETVALUE(RefreshAudio(p)->Pan,int); break;
	case PLAYER_PREAMP: GETVALUE(p->PreAmp,int); break;
	case PLAYER_TIMER_LEFT: GETVALUE(p->TimerLeft,bool_t); break;
	case PLAYER_TIMER: Result = GetTimerString(p,Data,Size/sizeof(tchar_t)); break;

	case PLAYER_DURATION:
		if (p->Format)
		{
			Result = p->Format->Get(p->Format,FORMAT_DURATION,Data,Size);
			if (Result == ERR_NONE) 
				UpdateListLength(p,*(tick_t*)Data);
		}
		break;
	}

	LockLeave(p->Lock);
	return Result;
}

static int UpdateInSeek(player_base* p)
{
	if (p->Format && p->Input && p->Timer)
	{
		fraction Pos = p->SeekAfterSync;
		p->InSeekPos.Num = -1;
		p->SeekAfterSync.Num = -1;
		if (!p->InSeek)
		{
			if (Pos.Num>=0)
			{
				Lock(p,1);
				Seek2(p,-1,&Pos,p->SeekAfterSyncPrevKey);
				Unlock(p,1);
			}
			else
				Notify(p,PLAYER_PERCENT,0);
		}
		UpdatePlay(p);
	}
	return ERR_NONE;
}

static int AddCodecSkip(player_base* p,const pin* Pin)
{
	packetformat Format;

	if (Pin->Node)
	{
		if (p->CodecSkipCount < MAXSKIP && 
			Pin->Node->Get(Pin->Node,Pin->No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE)
		{
			array List;
			const int *i;

			PacketFormatEnumClass(&List,&Format);
			for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
				if (!IsCodecSkip(p,*i) && *i != Pin->Node->Class)
				{
					// found an alternative
					ArrayClear(&List);

					p->CodecSkip[p->CodecSkipCount++] = Pin->Node->Class;
					p->UpdateStreamsNeeded = 1;
					return ERR_NONE;
				}

			ArrayClear(&List);
		}
		else
		{
			datadef DataDef;
			if (NodeDataDef(Pin->Node,Pin->No,&DataDef) && DataDef.Type==TYPE_NODE && DataDef.Format1==IDCT_CLASS &&
				p->VideoAccel && p->SkipAccelFrom != Pin->Node->Class)
			{
				p->SkipAccelFrom = Pin->Node->Class;
				p->UpdateStreamsNeeded = 1;
				return ERR_NONE;
			}
		}

	}

	return ERR_NOT_SUPPORTED;
}

static int SetCurrent(player_base* p,int i,bool_t Index)
{
	if (i>=p->PlayListCount)
		return ERR_INVALID_PARAM;

	if (i<0)
		Unload(p,0,0,1);
	else
	{
		if (p->Current != i)
		{
			p->Current = i;
			p->CurrentChanged = 1;
			if (Index)
				RandomizePlayIndex(p,1);
		}
		if (p->Wnd)
		{
			int Result = Load(p,0,0,0);
			if (Result == ERR_FILE_NOT_FOUND)
				Result = NextPrev(p,1,1,1);
			
			if (Result==ERR_NONE && p->Format && (p->Streaming || p->PlayAtOpen || p->PlayAtOpenFull))
			{
				p->FullScreenAfterSync = p->PlayAtOpenFull;
				if (p->FullScreenAfterSync && IsVideo(p) && !p->FullScreen)
					Notify(p,PLAYER_FULLSCREEN,1);
				p->Play = 1;
				p->FFwd = 0;
				SetPlay(p,0);
			}
		}
	}
	return ERR_NONE;
}

static NOINLINE bool_t CloseTo(tick_t Prev,tick_t Chapter,tick_t Pos)
{
	tick_t Limit;
	if (Prev<0) return 0;
	Limit = abs(Chapter-Prev)/32;
	if (Limit>10*TICKSPERSEC)
		Limit=10*TICKSPERSEC;
	return abs(Pos-Chapter)<Limit;
}

static NOINLINE tick_t FindChapter(player_base* p,int Dir)
{
	int No;
	if (p->Position>=0)
	{
		if (Dir<0)
			for (No=MAXCHAPTER-1;No>=0;--No)
			{
				tick_t Prev = PlayerGetChapter(&p->Player,No-1,NULL,0);
				tick_t Chapter = PlayerGetChapter(&p->Player,No,NULL,0);
				if (Chapter >= 0 && Chapter < p->Position && !CloseTo(Prev,Chapter,p->Position))
					return Chapter;
			}
		else
		{
			tick_t Prev = -1;
			for (No=1;No<MAXCHAPTER;++No)
			{
				tick_t Chapter = PlayerGetChapter(&p->Player,No,NULL,0);
				if (Chapter >= 0 && Chapter > p->Position && !CloseTo(Prev,Chapter,p->Position))
					return Chapter;
				Prev = Chapter;
			}
		}
	}
	return -1;
}

static int UpdateShowInBackground(player_base* p)
{
	UpdateBackground(p);
	return UpdateVideo(p,0);
}

static void UpdateSoftVolume(player_base* p)
{
	bool_t SoftVolume = QueryAdvanced(ADVANCED_SYSTEMVOLUME);
	if (p->SoftVolume != SoftVolume)
	{
		p->SoftVolume = SoftVolume;
		Lock(p,0);
		UpdateAudio(p);
		Unlock(p,0);
	}
}

static int Set(player_base* p, int No, const void* Data, int Size)
{
	tick_t t;
	int New,Old;
	int Result = ERR_INVALID_PARAM;
	
	if (No >= PLAYER_COMMENT && No < PLAYER_COMMENT+MAXSTREAM)
	{
		AddComment(p,No-PLAYER_COMMENT,(const tchar_t*)Data);
		return ERR_NONE;
	}

	switch (No)
	{
	case NODE_HIBERNATE:
		return Result;

	case NODE_CRASH:
#ifdef MULTITHREAD
		p->RunProcess = 0;
		p->RunInput = 0;
		p->Lock = NULL;
		p->LockInput = NULL;
		p->LockProcess = NULL;
		p->EventProcess = NULL;
		p->EventRunInput = NULL;
		p->EventRunProcess = NULL;
#ifndef NDEBUG
		p->ProcessLocked = 0;
		p->InputLocked = 0;
#endif
#endif
		return ERR_NONE;

	case PLAYER_NOT_SUPPORTED_DATA:
		// called by process thread
		assert(Size == sizeof(pin));
		return AddCodecSkip(p,(const pin*)Data);

	case NODE_SETTINGSCHANGED:
		UpdateWnd(p,Context()->Wnd);
		UpdatePriority(p);
		UpdateEqualizer(p);
		UpdateSoftVolume(p);
		return ERR_NONE;

	case PLAYER_PERCENT:
		assert(Size == sizeof(fraction));
		return InSeek(p,-1,(const fraction*)Data,1);									

	case PLAYER_POSITION:
		assert(Size == sizeof(tick_t));
		t = *(const tick_t*)Data;
		if (t<0) t=0;
		return InSeek(p,t,NULL,1);

	case PLAYER_BACKGROUND: SETVALUECMP(p->Background,bool_t,UpdateBackground(p),EqBool); return Result;
	case PLAYER_POWEROFF: SETVALUECMP(p->PowerOff,bool_t,UpdatePowerOff(p),EqBool); return Result;
	}

	Lock(p,0);
	if (No >= PLAYER_LIST_URL && No <= PLAYER_LIST_URL+p->PlayListCount)
	{
		No -= PLAYER_LIST_URL;
		if (No == p->PlayListCount)
			SetPlayListCount(p,No+1);
		if (No < p->PlayListCount)
		{
			if (!Size) Data = T("");
			p->PlayList[No].Changed = 1;
			tcscpy_s(p->PlayList[No].URL,TSIZEOF(p->PlayList[No].URL),(tchar_t*)Data);
			NotifyList(p);
			Result = ERR_NONE;
		}
	}
	else
	if (No >= PLAYER_LIST_TITLE && No <= PLAYER_LIST_TITLE+p->PlayListCount)
	{
		No -= PLAYER_LIST_TITLE;
		if (No == p->PlayListCount)
			SetPlayListCount(p,No+1);
		if (No < p->PlayListCount)
		{
			if (!Size) Data = T("");
			tcscpy_s(p->PlayList[No].Title,TSIZEOF(p->PlayList[No].Title),(tchar_t*)Data);
			NotifyList(p);
			Result = ERR_NONE;
		}
	}
	else
	if (No >= PLAYER_LIST_LENGTH && No <= PLAYER_LIST_LENGTH+p->PlayListCount)
	{
		No -= PLAYER_LIST_LENGTH;
		if (No == p->PlayListCount)
			SetPlayListCount(p,No+1);
		if (No < p->PlayListCount)
		{
			p->PlayList[No].Length = *(tick_t*)Data;
			NotifyList(p);
			Result = ERR_NONE;
		}
	}
	else
	switch (No)
	{
	case PLAYER_TIMER_LEFT: SETVALUE(p->TimerLeft,bool_t,ERR_NONE); break;
	case PLAYER_PLAY_SPEED: SETVALUE(p->PlaySpeed,fraction,UpdateSpeed(p)); break;
	case PLAYER_FFWD_SPEED: SETVALUE(p->FFwdSpeed,fraction,UpdateSpeed(p)); break;
	case PLAYER_REPEAT: SETVALUE(p->Repeat,bool_t,ERR_NONE); break;
	case PLAYER_PLAYATOPEN: SETVALUE(p->PlayAtOpen,bool_t,ERR_NONE); break;
	case PLAYER_PLAYATOPEN_FULL: SETVALUE(p->PlayAtOpenFull,bool_t,ERR_NONE); break;
	case PLAYER_EXIT_AT_END: SETVALUE(p->ExitAtEnd,bool_t,ERR_NONE); break;
	case PLAYER_KEEPPLAY_AUDIO: SETVALUE(p->KeepPlayAudio,bool_t,ERR_NONE); break;
	case PLAYER_KEEPPLAY_VIDEO: SETVALUE(p->KeepPlayVideo,bool_t,ERR_NONE); break;
	case PLAYER_SHOWINBACKGROUND: SETVALUE(p->ShowInBackground,bool_t,UpdateShowInBackground(p)); break;
	case PLAYER_SINGLECLICKFULLSCREEN: SETVALUE(p->SingleClickFullScreen,bool_t,ERR_NONE); break;
	case PLAYER_KEEPLIST: SETVALUE(p->KeepList,bool_t,ERR_NONE); break;
	case PLAYER_MOVEBACK_STEP: SETVALUE(p->MoveBack,tick_t,ERR_NONE); break;
	case PLAYER_MOVEFFWD_STEP: SETVALUE(p->MoveFFwd,tick_t,ERR_NONE); break;
	case PLAYER_SHUFFLE: SETVALUE(p->Shuffle,bool_t,RandomizePlayIndex(p,1)); break;
	case PLAYER_AUDIO_QUALITY: SETVALUECMP(p->AudioQuality,int,UpdateAudio(p),EqInt); break;
	case PLAYER_VIDEO_QUALITY: SETVALUE(p->VideoQuality,int,UpdateAllVideoQuality(p)); break;
	case PLAYER_FULLSCREEN: SETVALUECMP(p->FullScreen,bool_t,ERR_NONE,EqBool); break;
	case PLAYER_AUTOPREROTATE: SETVALUECMP(p->AutoPreRotate,bool_t,UpdateVideo(p,0),EqBool); break;
	case PLAYER_SKIN_VIEWPORT: SETVALUECMP(p->SkinViewport,rect,ERR_NONE,EqRect); break;
	case PLAYER_CLIPPING: SETVALUECMP(p->Clipping,bool_t,UpdateVideo(p,0),EqBool); break;
	case PLAYER_ASPECT: SETVALUECMP(p->Aspect,fraction,UpdateVideo(p,0),EqFrac); break;
	case PLAYER_STEREO: SETVALUECMP(p->Stereo,int,UpdateAudio(p),EqInt); break;
	case PLAYER_FULL_ZOOM: SETVALUECMP(p->FullZoom,fraction,UpdateVideo(p,0),EqFrac); break;
	case PLAYER_SKIN_ZOOM: SETVALUECMP(p->SkinZoom,fraction,UpdateVideo(p,0),EqFrac); break;
	case PLAYER_SMOOTH50: SETVALUECMP(p->SmoothZoom50,bool_t,UpdateVideo(p,0),EqBool); break;
	case PLAYER_SMOOTHALWAYS: SETVALUECMP(p->SmoothZoomAlways,bool_t,UpdateVideo(p,0),EqBool); break;
	case PLAYER_FULL_DIR: SETVALUECMP(p->FullDir,int,UpdateVideo(p,0),EqInt); break;
	case PLAYER_SKIN_DIR: SETVALUECMP(p->SkinDir,int,UpdateVideo(p,0),EqInt); break;
	case PLAYER_VIDEO_ACCEL: SETVALUE(p->VideoAccel,bool_t,ERR_NONE); break;
	case PLAYER_VSTREAM: SETVALUECMP(p->Selected[PACKET_VIDEO],int,UpdateStreamsMask(p),EqInt); break;
	case PLAYER_ASTREAM: SETVALUECMP(p->Selected[PACKET_AUDIO],int,UpdateStreamsMask(p),EqInt); break;
	case PLAYER_SUBSTREAM: SETVALUECMP(p->Selected[PACKET_SUBTITLE],int,UpdateStreamsMask(p),EqInt); break;
	case PLAYER_NOTIFY: SETVALUENULL(p->Notify,notify,ERR_NONE,p->Notify.Func=NULL); break;
	case PLAYER_LIST_NOTIFY: SETVALUENULL(p->ListNotify,notify,ERR_NONE,p->ListNotify.Func=NULL); break;
	case PLAYER_FOREGROUND: SETVALUECMP(p->Foreground,bool_t,UpdateForeground(p),EqBool); break;
	case PLAYER_INSEEK: SETVALUECMP(p->InSeek,bool_t,UpdateInSeek(p),EqBool); break;
	case PLAYER_MUTE: SETVALUECMP(p->Mute,bool_t,UpdateAudio(p),EqBool); break;
	case PLAYER_CURRENTDIR: SETSTRING(p->CurrentDir); break;
	case PLAYER_DISCARDLIST: SETVALUE(p->DiscardList,bool_t,ERR_NONE); break;
	case PLAYER_PREAMP: SETVALUECMP(p->PreAmp,int,UpdateAudio(p),EqInt); break;
	case PLAYER_PAN: SETVALUECMP(p->Pan,int,UpdateAudio(p),EqInt); break;
	case PLAYER_VOLUME: SETVALUECMP(p->Volume,int,UpdateVolume(p),EqInt); break; // need process locking for software win32 waveout pausing

	case PLAYER_UPDATEVIDEO: 
		Result = UpdateVideo(p,0);
		break;

	case PLAYER_ROTATEEND: 
		p->Rotating = 0;
		Result = UpdateVideo(p,0); 
		break;

	case PLAYER_RESETVIDEO:
		if (p->VOutput)
			p->VOutput->Set(p->VOutput,VOUT_RESET,NULL,0);
		break;

	case PLAYER_ROTATEBEGIN:
		p->Rotating = 1;
		if (p->VOutput)
		{
			bool_t Visible = 0;
			p->VOutput->Set(p->VOutput,VOUT_VISIBLE,&Visible,sizeof(Visible));
		}
		break;

	case PLAYER_VOUTPUTID: 
		assert(Size==sizeof(int));
		Old = p->VOutputId;
		New = *(int*)Data;
		if (New && !NodeEnumClass(NULL,New)) // old driver removed
		{
			New = p->VOutputIdMax;
			p->VideoAccel = VOutIDCT(New);
		}
		p->VOutputId = New;
		p->SkipAccelFrom = 0;
		if (!VOutIDCT(New))
			p->VideoAccel = 0;
		UpdateStreams(p,Old==0,0);
		break;

	case PLAYER_AOUTPUTID: 
		assert(Size==sizeof(int));
		Old = p->AOutputId;
		New = *(int*)Data;
		if (New && !NodeEnumClass(NULL,New)) // old driver removed
			New = p->AOutputIdMax;
		p->AOutputId = New;
		UpdateStreams(p,Old==0,0);
		break;

	case PLAYER_UPDATEEQUALIZER:
		Result = UpdateEqualizer(p);
		break;

	case PLAYER_VOUTPUTID_MAX: 
		assert(Size==sizeof(int));
		if (p->VOutputIdMax != *(int*)Data && p->VOutputId && p->VOutputIdMax && p->VOutputId != p->VOutputIdMax)
		{
			// new highest priority video driver added. use it
			p->VOutputId = p->VOutputIdMax;
			p->VideoAccel = VOutIDCT(p->VOutputId);
			UpdateStreams(p,0,0);
		}
		Result = ERR_NONE;
		break;

	case PLAYER_AOUTPUTID_MAX: 
		assert(Size==sizeof(int));
		if (p->AOutputIdMax != *(int*)Data && p->AOutputId && p->AOutputIdMax && p->AOutputId != p->AOutputIdMax)
		{
			// new highest priority audio driver added. use it
			p->AOutputId = p->AOutputIdMax;
			UpdateStreams(p,0,0);
		}
		Result = ERR_NONE;
		break;
		
	case PLAYER_MOVEBACK:
	case PLAYER_MOVEFFWD:
		if (p->Format && p->Input)
		{
			int FilePos;
			int FileSize;
			tick_t Position = GetPosition(p,NULL);
			fraction Percent;

			if (Position >= 0)
			{
				if (No == PLAYER_MOVEBACK)
					Position -= p->MoveBack;
				else
					Position += p->MoveFFwd;

				if (Position<0)
					Position=0;

				Result = InSeek(p,Position,NULL,No == PLAYER_MOVEBACK);
			}
			else
			if (p->Input->Get(p->Input,STREAM_LENGTH,&FileSize,sizeof(FileSize))==ERR_NONE &&
				p->Format->Get(p->Format,FORMAT_FILEPOS,&FilePos,sizeof(FilePos))==ERR_NONE)
			{
				if (No == PLAYER_MOVEBACK)
					FilePos -= FileSize/256;
				else
					FilePos += FileSize/256;

				Percent.Num = FilePos;
				Percent.Den = FileSize;
				if (Percent.Num<0)
					Percent.Num=0;

				Result = InSeek(p,-1,&Percent,No == PLAYER_MOVEBACK);
			}
			else
				Result = p->Player.Set(&p->Player,No == PLAYER_MOVEBACK?PLAYER_PREV:PLAYER_NEXT,NULL,0);
		}
		break; 

	default:

		// inputthread locking needed for these
		Lock(p,1);
		switch (No)
		{
		case PLAYER_BUFFER_SIZE:
			assert(Size==sizeof(int));
			p->BufferSize2 = (*(int*)Data *1024)/BLOCKSIZE;
			if (p->BufferSize2 < MINBUFFER/BLOCKSIZE)
				p->BufferSize2 = MINBUFFER/BLOCKSIZE;
			Result = UpdateFormat(p,1); 
			break;
		case PLAYER_MD_BUFFER_SIZE: 
			assert(Size==sizeof(int));
			p->MDBufferSize2 = (*(int*)Data *1024)/BLOCKSIZE;
			if (p->MDBufferSize2 < MINBUFFER/BLOCKSIZE)
				p->MDBufferSize2 = MINBUFFER/BLOCKSIZE;
			Result = UpdateFormat(p,1); 
			break;
		case PLAYER_BURSTSTART:
			assert(Size==sizeof(int));
			p->BurstStart = (*(int*)Data *1024)/BLOCKSIZE;
			if (p->BurstStart < MINBUFFER/BLOCKSIZE)
				p->BurstStart = MINBUFFER/BLOCKSIZE;
			Result = ERR_NONE; 
			break;
		case PLAYER_BENCHMARK:
			if (p->Format && p->Input)
				StartBench(p);
			break;

		case PLAYER_PLAY:
			if (Size == sizeof(bool_t))
			{
				if (*(bool_t*)Data)
				{
					if (!p->Format && p->Wnd && p->PlayListCount)
					{
						if (Load(p,0,0,0) == ERR_FILE_NOT_FOUND)
							NextPrev(p,1,1,1);
					}

					if (!p->Format)
					{
						Result = ERR_INVALID_DATA;
						break;
					}

					p->Play = 1;
					p->FFwd = 0;
				}
				else
				{
					p->Play = 0;
					p->FFwd = 0;
					p->Fill = 1;
				}
				Result = SetPlay(p,0);
			}
			break;

		case PLAYER_FFWD:
			if (Size == sizeof(bool_t))
			{
				if (*(bool_t*)Data)
				{
					if (!p->Format && p->Wnd && p->PlayListCount)
						Load(p,0,0,0);

					if (!p->Format)
					{
						Result = ERR_INVALID_DATA;
						break;
					}

					p->FFwd = 1;
				}
				else
				{
					p->FFwd = 0;
					if (!p->Play && !p->Bench) 
						p->Fill = 1;
				}
				Result = SetPlay(p,0);
			}
			break;

		case PLAYER_LIST_COUNT:
			assert(Size == sizeof(int));
			Result = SetPlayListCount(p,*(int*)Data);
			break;

		case PLAYER_LIST_CURRIDX:
			assert(Size == sizeof(int));
			if (*(int*)Data<p->PlayListCount && *(int*)Data>=0)
				Result = SetCurrent(p,p->PlayIndex[*(int*)Data],0);
			break;

		case PLAYER_LIST_CURRENT:
			assert(Size == sizeof(int));
			Result = SetCurrent(p,*(int*)Data,1);
			break;

		case PLAYER_NEXT:
			t = FindChapter(p,1);
			if (t >= 0)
				Result = Seek2(p,t,NULL,1);
			else
				Result = NextPrev(p,1,0,0);
			break;

		case PLAYER_PREV:
			t = FindChapter(p,-1);
			if (t >= 0)
				Result = Seek2(p,t,NULL,1);
			else
			if (Size>0 && p->Timing && p->Position > TICKSPERSEC*2 && Seek2(p,0,NULL,0)==ERR_NONE)
			{
				ClearStats(p);
				Result = ERR_NONE;
			}
			else
				Result = NextPrev(p,-1,0,0);
			break;

		case PLAYER_STOP:
			SetPlay(p,1);
			if (!Size)
			{
				if (p->Streaming)
					Unload(p,0,0,1);
				else
					NextPrev(p,0,0,0);
			}
			break;

		case PLAYER_RESYNC:
			ReSync(p,1);
			Result = ERR_NONE;
			break;

		case PLAYER_MICRODRIVE: SETVALUE(p->MicroDrive,bool_t,UpdateFormat(p,1)); break;
		case PLAYER_UNDERRUN: SETVALUE(p->UnderRun,int,ERR_NONE); break;
		case PLAYER_AUDIO_UNDERRUN: SETVALUE(p->AudioUnderRun,int,ERR_NONE); break;
		}

		Unlock(p,1);
		break;
	}

	Unlock(p,0);
	return Result;
}

static bool_t CmdComment(player_base* p,int Stream,const tchar_t* Name,tchar_t* Value,int ValueLen)
{
	bool_t Result = 0;
	const uint8_t *i;
	
	LockEnter(p->LockComment);
	for (i=ARRAYBEGIN(p->Comment,uint8_t);i!=ARRAYEND(p->Comment,uint8_t);i=CommentNext(i))
		if (CommentStream(i)!=INVALID_COMMENT &&
			(Stream<0 || CommentStream(i) == Stream) && tcsicmp(CommentName(i),Name)==0)
		{
			if (Result)
			{
				Result = 0;
				break;
			}

			if (Value)
				tcscpy_s(Value,ValueLen,CommentValue(i));

			Result = 1;
			if (Stream>=0)
				break;
		}

	LockLeave(p->LockComment);
	return Result;
}

static void Paint(player_base* p,void* DC,int x0,int y0)
{
	rect re;
	rect r;
	rect* Exclude = NULL;
	rgbval_t ColorKey;

#ifdef MULTITHREAD
	assert(p->ProcessLocked != ThreadId() && p->InputLocked != ThreadId());
#endif

	LockEnter(p->Lock);
	if (p->Primary)
	{
		if (p->VOutput && p->VOutput->Get(p->VOutput,VOUT_OUTPUTRECT,&re,sizeof(re))==ERR_NONE && 
			re.Width>0 && re.Height>0 && !p->TemporaryHidden)
		{
			re.x -= x0;
			re.y -= y0;
			if (p->VOutput->Get(p->VOutput,VOUT_COLORKEY,&ColorKey,sizeof(ColorKey)) == ERR_NONE)
			{
				if (ColorKey != p->ColorKey)
				{
					BrushDelete(p->ColorKeyBrush);
					p->ColorKey = ColorKey;
					p->ColorKeyBrush = BrushCreate(ColorKey);			
				}
				WinFill(DC,&re,NULL,p->ColorKeyBrush);
				Exclude = &re;
			}
			else
			if (!p->Clipping && !p->Overlay)
				Exclude = &re;
		}

		r = p->Viewport;
	}
	else
		r = p->SkinViewport;
	LockLeave(p->Lock);

	r.x -= x0;
	r.y -= y0;
	WinFill(DC,&r,Exclude,p->BlackBrush);
}

static int Create(player_base* p)
{
	int i;
	int Model;
	video Desktop;
	QueryDesktop(&Desktop);
	Model = QueryPlatform(PLATFORM_MODEL);

	// default values
	if (QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE)
	{
		p->UsedEnough2 = (256*1024)/BLOCKSIZE;
		p->BufferSize2 = (600*1024)/BLOCKSIZE;
	}
	else
	{
		p->UsedEnough2 = (512*1024)/BLOCKSIZE;
		p->BufferSize2 = (2400*1024)/BLOCKSIZE;
#ifdef TARGET_WINCE
		if (QueryPlatform(PLATFORM_VER) < 421)
			p->BufferSize2 = (1200*1024)/BLOCKSIZE;;
#endif

#ifdef TARGET_PALMOS
		p->UsedEnough2 = (256*1024)/BLOCKSIZE;
		p->BufferSize2 = (2000*1024)/BLOCKSIZE;
#endif
	}

	p->RndSeed = (uint32_t)(GetTimeTick()+GetTimeDate());
	p->Color = NodeEnumObject(NULL,COLOR_ID);
	p->Equalizer = NodeEnumObject(NULL,EQUALIZER_ID);

	p->Player.Enum = (nodeenum)Enum;
	p->Player.Get = (nodeget)Get;
	p->Player.Set = (nodeset)Set;
	p->Player.Paint = (playerpaint)Paint;
	p->Player.CommentByName = (playercomment)CmdComment;
	p->Player.ListSwap = (playerswap)ListSwap;
	p->Player.Process = (playerprocess)ProcessThread;

	p->MDBufferSize2 = (16000*1024)/BLOCKSIZE;
	p->BurstStart = (1500*1024)/BLOCKSIZE;
	p->UnderRun = PERCENT_ONE*7/10;
	p->AudioUnderRun = 64;
	p->MoveFFwd = p->MoveBack = TICKSPERSEC*10;
	p->PlayAtOpen = 0;
	p->PlayAtOpenFull = 0;
	p->KeepList = 1;
	p->KeepPlayAudio = 1;
#if defined(TARGET_WIN32)
	p->SingleClickFullScreen = 0;
	p->KeepPlayVideo = 1;
	p->ShowInBackground = 1;
	p->AutoPreRotate = 0;
#else
	p->SingleClickFullScreen = 1;
	p->KeepPlayVideo = 0;
	p->ShowInBackground = 0;
	p->AutoPreRotate = Desktop.Width != Desktop.Height;
#endif
	p->FullZoom.Den = 1;
	p->FullZoom.Num = 0;
	p->SkinZoom.Den = 1;
	p->SkinZoom.Num = 0;
	p->VideoQuality = 2;
#if !defined(SH3) && !defined(MIPS)
	p->AudioQuality = 2;
	p->SmoothZoom50 = 1;
	p->SmoothZoomAlways = 0;
#else
	p->AudioQuality = 1;
	p->SmoothZoom50 = 0;
	p->SmoothZoomAlways = 0;
#if defined(SH3)
	p->FullZoom.Num = p->SkinZoom.Num = 1;
#endif
#endif
#ifdef TARGET_SYMBIAN
	p->Volume = 50;
#else
	p->Volume = 90;
#endif
	p->Repeat = 1;
#ifdef TARGET_WIN32
	p->Aspect.Num = 0;
#else
	p->Aspect.Num = 1;
#ifdef ARM
	// use aspect with hardware scaler overlay drivers...
	if (NodeEnumClass(NULL,FOURCC('A','T','I','4')) ||
		NodeEnumClass(NULL,FOURCC('G','E','2','D')) ||
		NodeEnumClass(NULL,FOURCC('A','H','I','_')) ||
		NodeEnumClass(NULL,FOURCC('2','7','0','G')))
		p->Aspect.Num = 0;
#endif
#endif
	p->Aspect.Den = 1;
	p->PlaySpeed.Num = 1;
	p->PlaySpeed.Den = 1;
	p->FFwdSpeed.Num = 2;
	p->FFwdSpeed.Den = 1;
	p->SeekAfterSync.Num = -1;
	p->Primary = 1;
	p->SkinDir = -1;
	p->Foreground = 1;

	if (Model==MODEL_TUNGSTEN_T3)
		p->FullDir = -1; // special case because it can be slided into square format
	else
	{
		p->FullDir = GetOrientation();
		if (Desktop.Width < Desktop.Height)
		{
			if (GetHandedness())
				p->FullDir = CombineDir(p->FullDir,DIR_SWAPXY | DIR_MIRRORLEFTRIGHT,0);
			else
				p->FullDir = CombineDir(p->FullDir,DIR_SWAPXY | DIR_MIRRORUPDOWN,0);
		}
	}

	for (i=0;i<PACKET_MAX;++i)
		p->Selected[i] = -1;
	p->CurrentChanged = 1;
	p->Position = -1;

	p->AOutputIdMax = p->AOutputId = NodeEnumClass(NULL,AOUT_CLASS);
	p->VOutputIdMax = p->VOutputId = NodeEnumClass(NULL,VOUT_CLASS);
	p->VideoAccel = VOutIDCT(p->VOutputId);

#ifdef MULTITHREAD
	p->LockProcessCount = 0;
	p->LockInputCount = 0;
	p->LockComment = LockCreate();
	p->Lock = LockCreate();
	p->LockInput = LockCreate();
	p->LockProcess = LockCreate();
	p->EventProcess = EventCreate(0,0);
	p->EventRunProcess = EventCreate(1,0);
	p->EventRunInput = EventCreate(1,0);
#endif

	p->ColorKey = RGB_NULL;
	p->ColorKeyBrush = NULL;
	p->BlackBrush = BrushCreate(0);

	UpdateFormat(p,0);
	UpdateRunProcess(p);
	UpdateRunInput(p);
	return ERR_NONE;
}

static void Delete(player_base* p)
{
	UpdateWnd(p,NULL);

#ifdef MULTITHREAD
	EventClose(p->EventRunInput);
	EventClose(p->EventRunProcess);
	EventClose(p->EventProcess);

	LockDelete(p->Lock);
	LockDelete(p->LockInput);
	LockDelete(p->LockProcess);
	LockDelete(p->LockComment);
#endif

	BrushDelete(p->ColorKeyBrush);
	BrushDelete(p->BlackBrush);

	free(p->PlayList);
	free(p->PlayIndex);
}

void PlayerSaveList(player* Player,const tchar_t* Path,int Class)
{
	playlist* Playlist = (playlist*)NodeCreate(Class);
	if (Playlist)
	{
		if (Playlist->WriteList)
		{
			stream* Input = GetStream(Path,0);
			if (Input)
			{
				bool_t b = 1;
				if (Input->Set(Input,STREAM_CREATE,&b,sizeof(b)) == ERR_NONE &&
					Input->Set(Input,STREAM_URL,Path,sizeof(tchar_t)*(tcslen(Path)+1)) == ERR_NONE &&
					Playlist->Set(Playlist,PLAYLIST_STREAM,&Input,sizeof(Input)) == ERR_NONE)
				{
					tchar_t Base[MAXPATH];
					tchar_t Rel[MAXPATH];
					tchar_t URL[MAXPATH];
					tchar_t Title[256];
					tick_t Length;
					int No,Count;
				
					SplitURL(Path,Base,TSIZEOF(Base),Base,TSIZEOF(Base),NULL,0,NULL,0);
					Player->Get(Player,PLAYER_LIST_COUNT,&Count,sizeof(Count));
					for (No=0;No<Count;++No)
					{
						if (Player->Get(Player,PLAYER_LIST_URL+No,URL,sizeof(URL))==ERR_NONE)
						{
							RelPath(Rel,TSIZEOF(Rel),URL,Base);
							Title[0] = 0;
							Length = -1;

							Player->Get(Player,PLAYER_LIST_TITLE+No,Title,sizeof(Title));
							Player->Get(Player,PLAYER_LIST_LENGTH+No,&Length,sizeof(Length));

							Playlist->WriteList(Playlist,Rel,Title,Length);
						}
					}

					Playlist->WriteList(Playlist,NULL,NULL,0);
					Playlist->Set(Playlist,PLAYLIST_STREAM,NULL,0);
				}

				Input->Set(Input,STREAM_URL,NULL,0);
				NodeDelete((node*)Input);
			}
		}

		Playlist->Set(Playlist,PLAYLIST_STREAM,NULL,0);
		NodeDelete((node*)Playlist);
	}
}

int PlayerAddDir(player* Player,int Index, const tchar_t* Path, const tchar_t* Exts, bool_t ExtFilter, int Deep)
{
	streamdir DirItem;
	stream* Stream;
	
	if (Deep < MAXDIRDEEP) // fail safe
	{
		Stream = GetStream(Path,1);
		if (Stream)
		{
			tchar_t Base[MAXPATH];

			int Result = Stream->EnumDir(Stream,Path,Exts,ExtFilter,&DirItem);

			if (Result == ERR_NONE)
			{
				tcscpy_s(Base,TSIZEOF(Base),Path);
				Stream->Get(Stream,STREAM_BASE,Base,sizeof(Base));
			}

			while (Result == ERR_NONE)
			{
				tchar_t PathItem[MAXPATH];
				AbsPath(PathItem,TSIZEOF(PathItem),DirItem.FileName,Base);

				if (tcsncmp(PathItem,Base,tcslen(Base))==0) // don't go to other sites with http
				{
					if (DirItem.Size < 0)
						Index = PlayerAddDir(Player,Index,PathItem,Exts,ExtFilter,Deep+1);
					else
						Index = PlayerAdd(Player,Index,PathItem,DirItem.DisplayName);
				}

				Result = Stream->EnumDir(Stream,NULL,NULL,0,&DirItem);
			}

			NodeDelete((node*)Stream);
		}
	}
	return Index;
}

int PlayerAdd(player* Player,int Index, const tchar_t* Path, const tchar_t* Title)
{
	stream* Input;
	array List;

	// process playlist files
	NodeEnumClassEx(&List,PLAYLIST_CLASS,NULL,Path,NULL,0);
	if (!ARRAYEMPTY(List) && (Input = GetStream(Path,0))!=NULL)
	{
		if (Input->Set(Input,STREAM_URL,Path,sizeof(tchar_t)*(tcslen(Path)+1)) == ERR_NONE &&
			LoadPlaylist(Player,&List,Input,&Index,Path,NULL,0))
			Path = NULL;
		Input->Set(Input,STREAM_URL,NULL,0);
		NodeDelete((node*)Input);
	}
	ArrayClear(&List);

	if (Path)
	{
		Player->Set(Player,PLAYER_LIST_URL+Index,Path,sizeof(tchar_t)*(tcslen(Path)+1));
		Player->Set(Player,PLAYER_LIST_TITLE+Index,Title,Title?sizeof(tchar_t)*(tcslen(Title)+1):0);
		++Index;
	}
	return Index;
}

const tchar_t* PlayerComment(int Code)
{
	switch (Code)
	{
	case COMMENT_TITLE: return T("TITLE");
	case COMMENT_ARTIST: return T("ARTIST");
	case COMMENT_ALBUM: return T("ALBUM");
	case COMMENT_GENRE: return T("GENRE");
	case COMMENT_LANGUAGE: return T("LANGUAGE");
	case COMMENT_AUTHOR: return T("AUTHOR");
	case COMMENT_COPYRIGHT: return T("COPYRIGHT");
	case COMMENT_PRIORITY: return T("PRIORITY");
	case COMMENT_YEAR: return T("YEAR");
	case COMMENT_TRACK: return T("TRACK");
	case COMMENT_COMMENT: return T("COMMENT");
	case COMMENT_COVER: return T("COVER");
	case COMMENT_REDIRECT: return T("REDIRECT");
	}
	return NULL;
}

tick_t PlayerGetChapter(player* p,int No,tchar_t* OutName,int OutNameLen)
{
	tchar_t Chapter[64];
	tchar_t s[64];
	int Hour,Min,Sec,MSec=0;

	if (No>0)
	{
		stprintf_s(Chapter,TSIZEOF(Chapter),T("CHAPTER%02d"),No);
		if (p->CommentByName(p,-1,Chapter,s,TSIZEOF(s)) && 
			stscanf(s,T("%d:%d:%d.%d"),&Hour,&Min,&Sec,&MSec)>=3)
		{
			if (OutName)
			{
				stprintf_s(Chapter,TSIZEOF(Chapter),T("CHAPTER%02dNAME"),No);
				if (!p->CommentByName(p,-1,Chapter,OutName,OutNameLen))
					stprintf_s(OutName,OutNameLen,T("%d"),No);
			}
			return Scale(((Hour*60+Min)*60+Sec)*1000+MSec,TICKSPERSEC,1000);
		}
	}
	return -1;
}

bool_t PlayerGetStream(player* Player,int No,packetformat* OutFormat,tchar_t* OutName,int OutNameLen,int* OutPri)
{
	tchar_t s[256];
	node* Format;
	packetformat PacketFormat;
	int i;

	if (Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format))==ERR_NONE && Format)
	{
		int Count[PACKET_MAX];
		memset(Count,0,sizeof(Count));

		for (i=0;Format->Get(Format,(FORMAT_STREAM+i)|PIN_FORMAT,&PacketFormat,sizeof(PacketFormat))==ERR_NONE;++i)
		{
			++Count[PacketFormat.Type];
			if (i==No)
			{
				if (OutPri)
				{
					if (Player->CommentByName(Player,i,PlayerComment(COMMENT_PRIORITY),s,TSIZEOF(s)))
						*OutPri = StringToInt(s,0);
					else
						*OutPri = 0;
				}
				if (OutName)
				{
					if (Player->CommentByName(Player,i,PlayerComment(COMMENT_LANGUAGE),s,TSIZEOF(s)) ||
						Player->CommentByName(Player,i,PlayerComment(COMMENT_TITLE),s,TSIZEOF(s)))
						tcscpy_s(OutName,OutNameLen,s);
					else
						stprintf_s(OutName,OutNameLen,T("%s %d"),LangStr(PLAYER_ID,STREAM_NAME+PacketFormat.Type),Count[PacketFormat.Type]);
				}
				if (OutFormat)
					*OutFormat = PacketFormat;
				return 1;
			}
		}
	}
	return 0;
}

static const nodedef Player =
{
	sizeof(player_base)|CF_GLOBAL|CF_SETTINGS,
	PLAYER_ID,
	NODE_CLASS,
	PRI_MAXIMUM+600,
	(nodecreate)Create,
	(nodedelete)Delete,
};

typedef struct playerbuffer
{
	node Node;
	node* Player;

} playerbuffer;

static int BufferEnum(playerbuffer* p, int* No, datadef* Param)
{
	int Result = NodeEnumTable(No,Param,BufferParams);
	if (Result == ERR_NONE)
		Param->Name = LangStr(PLAYER_ID,Param->No);
	return Result;
}

static int BufferGet(playerbuffer* p, int No, void* Data, int Size)
{
	return p->Player->Get(p->Player,No,Data,Size);
}

static int BufferSet(playerbuffer* p, int No, const void* Data, int Size)
{
	return p->Player->Set(p->Player,No,Data,Size);
}

static int BufferCreate(playerbuffer* p)
{
	p->Player = Context()->Player;
	p->Node.Enum = (nodeenum)BufferEnum;
	p->Node.Get = (nodeget)BufferGet;
	p->Node.Set = (nodeset)BufferSet;
	return ERR_NONE;
}

static const nodedef Buffer =
{
	sizeof(playerbuffer)|CF_GLOBAL|CF_SETTINGS,
	PLAYER_BUFFER_ID,
	NODE_CLASS,
	PRI_MAXIMUM+60,
	(nodecreate)BufferCreate
};

void Player_Init()
{
	NodeRegisterClass(&Player);
	Context()->Player = NodeEnumObject(NULL,PLAYER_ID);
	NodeRegisterClass(&Buffer);
}

void Player_Done()
{
	Context()->Player = NULL;
	NodeUnRegisterClass(PLAYER_ID);
	NodeUnRegisterClass(PLAYER_BUFFER_ID);
}

