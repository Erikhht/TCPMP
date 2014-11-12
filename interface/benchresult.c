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
 * $Id: benchresult.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "win.h"
#include "benchresult.h"

typedef struct benchresult
{
	win Win;
	int64_t TimeDate;
	tchar_t Log[2048];

} benchresult;

static NOINLINE void AddLog(benchresult* p, int Class, int Id, const tchar_t* Buffer)
{
	stcatprintf_s(p->Log,TSIZEOF(p->Log),T("%-30s %s\n"),LangStrDef(Class,Id),Buffer);
}

static NOINLINE void AddItem(benchresult* p, winunit* y, int Id, const tchar_t* Buffer)
{
	WinPropLabel(&p->Win,y,LangStr(BENCHRESULT_ID,Id),Buffer);
	AddLog(p,BENCHRESULT_ID,Id,Buffer);
}

static int Command(benchresult* p,int Cmd)
{
	if (Cmd == BENCHRESULT_SAVE)
	{
		tchar_t URL[MAXPATH];
		tchar_t FileName[32];
		stprintf_s(FileName,TSIZEOF(FileName),T("bench_%06d_%06d"),(int)(p->TimeDate/1000000000),(int)((p->TimeDate/1000)%1000000));
		SaveDocument(FileName,p->Log,URL,TSIZEOF(URL));
		ShowMessage(LangStr(BENCHRESULT_ID,NODE_NAME),LangStr(BENCHRESULT_ID,BENCHRESULT_SAVED),URL);
		return ERR_NONE;
	}
	return ERR_INVALID_PARAM;
}
		
static int Init(benchresult* p)
{
	node* Player = Context()->Player;
	node* Platform = Context()->Platform;
	node* Format;
	node* Input;
	node* VOutput;
	node* AOutput;
	winunit y;
	int i;
	int Frames;
	int Samples;
	int Bytes;
	tick_t Tick;
	tick_t OrigTick;
	point SizeSrc;
	point SizeDst;
	packetformat Video;
	packetformat Audio;
	tchar_t Buffer[256];
	fraction f;

	p->TimeDate = GetTimeDate();

	Player->Get(Player,PLAYER_FORMAT,&Format,sizeof(Format));
	Player->Get(Player,PLAYER_INPUT,&Input,sizeof(Input));

	VOutput = NULL;
	AOutput = NULL;
	OrigTick = 0;
	Frames = 0;
	Samples = 0;
	Bytes = 0;
	memset(&Video,0,sizeof(Video));
	memset(&Audio,0,sizeof(Audio));

	stprintf_s(p->Log,TSIZEOF(p->Log),LangStrDef(BENCHRESULT_ID,BENCHRESULT_LOG),Context()->ProgramName,Context()->ProgramVersion);
	tcscat_s(p->Log,TSIZEOF(p->Log),T("\n\n"));

	if (Format)
	{
		int No;
		pin Pin;
		packetformat PacketFormat;

		Format->Get(Format,FORMAT_FILEPOS,&Bytes,sizeof(Bytes));

		for (No=0;Format->Get(Format,FORMAT_STREAM+No,&Pin,sizeof(Pin))==ERR_NONE;++No)
			if (Pin.Node && Format->Get(Format,(FORMAT_STREAM+No)|PIN_FORMAT,&PacketFormat,sizeof(PacketFormat))==ERR_NONE)
			{
				if (PacketFormat.Type == PACKET_VIDEO) 
				{
					Video = PacketFormat;
					Player->Get(Player,PLAYER_VOUTPUT,&VOutput,sizeof(VOutput));
					if (VOutput)
						VOutput->Get(VOutput,OUT_TOTAL,&Frames,sizeof(Frames));
				}
				if (PacketFormat.Type == PACKET_AUDIO) 
				{
					Audio = PacketFormat;
					Player->Get(Player,PLAYER_AOUTPUT,&AOutput,sizeof(AOutput));
					if (AOutput)
					{
						packetformat Format;
						AOutput->Get(AOutput,OUT_TOTAL,&Samples,sizeof(Samples));
						if (AOutput->Get(AOutput,OUT_INPUT|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE &&
							Format.Type == PACKET_AUDIO)
						{
							if (Format.Format.Audio.Bits>=8)
								Samples /= Format.Format.Audio.Bits/8;
							if (!(Format.Format.Audio.Flags & PCM_PLANES) && Format.Format.Audio.Channels)
								Samples /= Format.Format.Audio.Channels;
						}
					}
				}
			}
	}

	y = 4;

	Player->Get(Player,PLAYER_BENCHMARK,&Tick,sizeof(Tick));

	if (Frames && Video.PacketRate.Num)
		OrigTick = Scale64(Frames,(int64_t)Video.PacketRate.Den*TICKSPERSEC,Video.PacketRate.Num);
	else
	if (Samples && Audio.Format.Audio.SampleRate)
		OrigTick = Scale(Samples,TICKSPERSEC,Audio.Format.Audio.SampleRate);

	if (Tick && OrigTick)
	{
		f.Num = OrigTick;
		f.Den = Tick;

		FractionToString(Buffer,TSIZEOF(Buffer),&f,1,2);
		AddItem(p,&y,BENCHRESULT_SPEED,Buffer);
	}

	if (Frames)
	{
		IntToString(Buffer,TSIZEOF(Buffer),Frames,0);
		AddItem(p,&y,BENCHRESULT_FRAMES,Buffer);
	}

	if (Samples)
	{
		IntToString(Buffer,TSIZEOF(Buffer),Samples,0);
		AddItem(p,&y,BENCHRESULT_SAMPLES,Buffer);
	}

	if (Bytes)
	{
		IntToString(Buffer,TSIZEOF(Buffer),Bytes/1024,0);
		tcscat_s(Buffer,TSIZEOF(Buffer),T(" KB"));
		AddItem(p,&y,BENCHRESULT_BYTES,Buffer);
	}

	y += 6;
	tcscat_s(p->Log,TSIZEOF(p->Log),T("\n"));

	TickToString(Buffer,TSIZEOF(Buffer),Tick,0,1,0);
	AddItem(p,&y,BENCHRESULT_TIME,Buffer);

	if (Frames && Tick)
	{
		f.Num = Frames;
		f.Den = Tick;
		Simplify(&f,MAX_INT/TICKSPERSEC,MAX_INT);
		f.Num *= TICKSPERSEC;
		FractionToString(Buffer,TSIZEOF(Buffer),&f,0,2);
		AddItem(p,&y,BENCHRESULT_FPS,Buffer);
	}

	if (Samples && Tick)
	{
		f.Num = Samples;
		f.Den = Tick;
		Simplify(&f,MAX_INT/TICKSPERSEC,MAX_INT);
		f.Num *= TICKSPERSEC;
		FractionToString(Buffer,TSIZEOF(Buffer),&f,0,0);
		AddItem(p,&y,BENCHRESULT_SRATE,Buffer);
	}

	if (Bytes && Tick)
	{
		f.Num = Scale(Bytes,8,1000);
		f.Den = Tick;
		Simplify(&f,MAX_INT/TICKSPERSEC,MAX_INT);
		f.Num *= TICKSPERSEC;
		if (f.Den && (f.Num/f.Den) > 1000)
		{
			Simplify(&f,MAX_INT,MAX_INT/1024);
			f.Den *= 1000;
			FractionToString(Buffer,TSIZEOF(Buffer),&f,0,1);
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" Mbit/s"));
		}
		else
		{
			FractionToString(Buffer,TSIZEOF(Buffer),&f,0,0);
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" kbit/s"));
		}
		AddItem(p,&y,BENCHRESULT_BANDWIDTH,Buffer);
	}

	y += 6;
	tcscat_s(p->Log,TSIZEOF(p->Log),T("\n"));

	if (OrigTick)
	{
		TickToString(Buffer,TSIZEOF(Buffer),OrigTick,0,1,0);
		AddItem(p,&y,BENCHRESULT_ORIG_TIME,Buffer);
	}

	if (Video.PacketRate.Num)
	{
		FractionToString(Buffer,TSIZEOF(Buffer),&Video.PacketRate,0,2);
		AddItem(p,&y,BENCHRESULT_ORIG_FPS,Buffer);
	}

	if (Audio.Format.Audio.SampleRate)
	{
		IntToString(Buffer,TSIZEOF(Buffer),Audio.Format.Audio.SampleRate,0);
		AddItem(p,&y,BENCHRESULT_ORIG_SRATE,Buffer);
	}

	if (Bytes && OrigTick)
	{
		f.Num = Scale(Bytes,8,1000);
		f.Den = OrigTick;
		Simplify(&f,MAX_INT/TICKSPERSEC,MAX_INT);
		f.Num *= TICKSPERSEC;
		if (f.Den && (f.Num/f.Den) > 1000)
		{
			f.Den *= 1000;
			FractionToString(Buffer,TSIZEOF(Buffer),&f,0,1);
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" Mbit/s"));
		}
		else
		{
			FractionToString(Buffer,TSIZEOF(Buffer),&f,0,0);
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" kbit/s"));
		}
		AddItem(p,&y,BENCHRESULT_ORIG_BANDWIDTH,Buffer);
	}

	if (Frames && Samples)
	{
		y += 9;
		WinLabel(&p->Win,&y,-1,-1,LangStr(BENCHRESULT_ID,BENCHRESULT_MSG),11,0,NULL);
	}

	tcscat_s(p->Log,TSIZEOF(p->Log),T("\n"));

	if (Input && Input->Get(Input,STREAM_URL,Buffer,sizeof(Buffer))==ERR_NONE)
		AddLog(p,BENCHRESULT_ID,BENCHRESULT_URL,Buffer);

	if (Input && Input->Get(Input,STREAM_LENGTH,&i,sizeof(i))==ERR_NONE)
	{
		IntToString(Buffer,TSIZEOF(Buffer),i,0);
		AddLog(p,BENCHRESULT_ID,BENCHRESULT_FILESIZE,Buffer);
	}

	if (Platform)
	{
		if (Platform->Get(Platform,PLATFORM_TYPE,Buffer,sizeof(Buffer))==ERR_NONE)
			AddLog(p,PLATFORM_ID,PLATFORM_TYPE,Buffer);

		if (Platform->Get(Platform,PLATFORM_VERSION,Buffer,sizeof(Buffer))==ERR_NONE)
			AddLog(p,PLATFORM_ID,PLATFORM_VERSION,Buffer);

		if (Platform->Get(Platform,PLATFORM_OEMINFO,Buffer,sizeof(Buffer))==ERR_NONE)
			AddLog(p,PLATFORM_ID,PLATFORM_OEMINFO,Buffer);

		ThreadSleep(GetTimeFreq()/10);

		if (Platform->Get(Platform,PLATFORM_CPUMHZ,&i,sizeof(i))==ERR_NONE)
		{
			IntToString(Buffer,TSIZEOF(Buffer),i,0);
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" Mhz"));
			AddLog(p,PLATFORM_ID,PLATFORM_CPUMHZ,Buffer);
		}
	}

	if (VOutput)
	{
		tcscpy_s(Buffer,TSIZEOF(Buffer),LangStrDef(VOutput->Class,NODE_NAME));
		if (VOutput->Get(VOutput,OUT_OUTPUT|PIN_FORMAT,&Video,sizeof(Video))==ERR_NONE)
		{
			if (Video.Format.Video.Direction & DIR_SWAPXY)
				SwapInt(&Video.Format.Video.Width,&Video.Format.Video.Height);
			stcatprintf_s(Buffer,TSIZEOF(Buffer),T(" %dx%d %dbits"),Video.Format.Video.Width,Video.Format.Video.Height,Video.Format.Video.Pixel.BitCount);
		}

		if (QueryAdvanced(ADVANCED_SLOW_VIDEO))
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" Slow"));
		if (QueryAdvanced(ADVANCED_COLOR_LOOKUP))
			tcscat_s(Buffer,TSIZEOF(Buffer),T(" Lookup"));

		AddLog(p,PLAYER_ID,PLAYER_VOUTPUT,Buffer);
	}

	if (Player->Get(Player,PLAYER_BENCHMARK_SRC,&SizeSrc,sizeof(point))==ERR_NONE &&
		Player->Get(Player,PLAYER_BENCHMARK_DST,&SizeDst,sizeof(point))==ERR_NONE && 
		SizeSrc.x>0 && SizeSrc.y>0)
		{
			stprintf_s(Buffer,TSIZEOF(Buffer),T("%dx%d -> %dx%d"),SizeSrc.x,SizeSrc.y,SizeDst.x,SizeDst.y);
			AddLog(p,BENCHRESULT_ID,BENCHRESULT_ZOOM,Buffer);
		}

	if (AOutput)
	{
		tcscpy_s(Buffer,TSIZEOF(Buffer),LangStrDef(AOutput->Class,NODE_NAME));
		if (AOutput->Get(AOutput,OUT_OUTPUT|PIN_FORMAT,&Audio,sizeof(Audio))==ERR_NONE)
			stcatprintf_s(Buffer,TSIZEOF(Buffer),T(" %dHz %dBits %dCh."),Audio.Format.Audio.SampleRate,Audio.Format.Audio.Bits,Audio.Format.Audio.Channels);
		AddLog(p,PLAYER_ID,PLAYER_AOUTPUT,Buffer);
	}

	return ERR_NONE;
}

static const menudef MenuDef[] =
{
	{ 0, PLATFORM_ID, PLATFORM_DONE, },
	{ 0, BENCHRESULT_ID, BENCHRESULT_SAVE, },

	MENUDEF_END
};

WINCREATE(BenchResult)

static int Create(benchresult* p)
{
	BenchResultCreate(&p->Win);
	p->Win.WinWidth = 180;
	p->Win.WinHeight = 240;
	p->Win.Flags |= WIN_DIALOG|WIN_NOTABSTOP;
	p->Win.MenuDef = MenuDef;
	p->Win.Init = (nodefunc)Init;
	p->Win.Command = (wincommand)Command;
	p->Win.LabelWidth = min(88,p->Win.ScreenWidth-45);
	return ERR_NONE;
}

static const nodedef BenchResult = 
{
	sizeof(benchresult),
	BENCHRESULT_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
};

void BenchResult_Init()
{
	NodeRegisterClass(&BenchResult);
}

void BenchResult_Done()
{
	NodeUnRegisterClass(BENCHRESULT_ID);
}
