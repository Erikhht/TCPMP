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
 * $Id: mediainfo.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "win.h"
#include "mediainfo.h"

static int CommentNo[] = { COMMENT_TITLE, COMMENT_ARTIST, COMMENT_AUTHOR, COMMENT_TRACK, COMMENT_ALBUM, COMMENT_YEAR, 
						   COMMENT_GENRE, COMMENT_COPYRIGHT, COMMENT_COMMENT, 0, COMMENT_LANGUAGE, 0 };

static void Comment(win* Win,player* Player,winunit* y,int Stream)
{
	tchar_t s[256];
	int i;

	for (i=0;CommentNo[i];++i)
		if (Player->CommentByName(Player,Stream,PlayerComment(CommentNo[i]),s,TSIZEOF(s)))
		{
			if (Stream>=0 && Player->CommentByName(Player,-1,PlayerComment(CommentNo[i]),NULL,0))
				continue;

			WinPropLabel(Win,y,LangStr(PLAYER_ID,CommentNo[i]),s);
		}

	if (Stream>=0)
		for (++i;CommentNo[i];++i)
			if (Player->CommentByName(Player,Stream,PlayerComment(CommentNo[i]),s,TSIZEOF(s)))
				WinPropLabel(Win,y,LangStr(PLAYER_ID,CommentNo[i]),s);
}

static void Info(win* Win,node* Node,winunit* y,int Name)
{
	int No;
	datadef DataDef;

	if (Name)
		WinPropLabel(Win,y,LangStr(MEDIAINFO_ID,Name),LangStr(Node->Class,NODE_NAME));

	for (No=0;NodeEnum(Node,No,&DataDef)==ERR_NONE;++No)
		if (!(DataDef.Flags & (DF_OUTPUT|DF_HIDDEN)))
		{
			tchar_t s[256];
			tick_t Tick;
			int i;
			bool_t Ok = 0;

			switch (DataDef.Type)
			{
			case TYPE_TICK:
				Ok = Node->Get(Node,DataDef.No,&Tick,sizeof(Tick))==ERR_NONE;
				TickToString(s,TSIZEOF(s),Tick,0,1,0);
				break;

			case TYPE_INT:
				Ok = Node->Get(Node,DataDef.No,&i,sizeof(i))==ERR_NONE;
				IntToString(s,TSIZEOF(s),i,(DataDef.Flags & DF_HEX)!=0);
				if (DataDef.Flags & DF_KBYTE)
					tcscat_s(s,TSIZEOF(s),T(" KB"));
				break;

			case TYPE_STRING:
				Ok = Node->Get(Node,DataDef.No,s,sizeof(s))==ERR_NONE;
				break;					
			}

			if (Ok)
				WinPropLabel(Win,y,DataDef.Name,s);
		}
}

static int Init(win* p)
{
	tchar_t s[256];
	packetformat Format;
	int No;
	node* VOutput = NULL;
	node* Reader = NULL;
	node* Input = NULL;
	node* Player = Context()->Player;
	winunit y;

	Player->Get(Player,PLAYER_FORMAT,&Reader,sizeof(Reader));
	Player->Get(Player,PLAYER_INPUT,&Input,sizeof(Input));
	Player->Get(Player,PLAYER_VOUTPUT,&VOutput,sizeof(VOutput));

	y = 4;

	if (Input)
		Info(p,Input,&y,0);

	if (Reader)
	{
		pin Pin;

		Info(p,Reader,&y,MEDIAINFO_FORMAT);
		Comment(p,(player*)Player,&y,-1);

		for (No=0;Reader->Get(Reader,FORMAT_STREAM+No,&Pin,sizeof(Pin))==ERR_NONE;++No)
			if (PlayerGetStream((player*)Player,No,&Format,NULL,0,NULL))
			{
				y += 6;

				if (Format.Type != PACKET_NONE)
				{
					if (!PacketFormatName(&Format,s,TSIZEOF(s))) s[0] =0;
					WinPropLabel(p,&y,LangStr(PLAYER_ID,STREAM_NAME+Format.Type),s);
				}

				switch (Format.Type)
				{
				case PACKET_NONE:
					break;

				case PACKET_VIDEO:
					if (Pin.Node && Compressed(&Format.Format.Video.Pixel))
						Info(p,Pin.Node,&y,MEDIAINFO_CODEC);

					if (Format.Format.Video.Width && Format.Format.Video.Height)
					{
						stprintf_s(s,TSIZEOF(s),T("%d x %d"),Format.Format.Video.Width,Format.Format.Video.Height);
						WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_SIZE),s);
					}
					if (Format.PacketRate.Num)
					{
						FractionToString(s,TSIZEOF(s),&Format.PacketRate,0,3);
						WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_FPS),s);
					}
					if (Format.ByteRate)
					{
						IntToString(s,TSIZEOF(s),(Format.ByteRate+62)/125,0);
						tcscat_s(s,TSIZEOF(s),T(" kbit/s")); // 1000byte/sec
						WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_BITRATE),s);
					}

					break;

				case PACKET_AUDIO:

					if (Pin.Node && Format.Format.Audio.Format != AUDIOFMT_PCM)
						Info(p,Pin.Node,&y,MEDIAINFO_CODEC);

					s[0] = 0;
					if (Format.Format.Audio.SampleRate)
					{
						IntToString(s+tcslen(s),TSIZEOF(s)-tcslen(s),Format.Format.Audio.SampleRate,0);
						tcscat_s(s,TSIZEOF(s),T(" Hz "));
					}
					switch (Format.Format.Audio.Channels)
					{
					case 0: break;
					case 1: tcscat_s(s,TSIZEOF(s),LangStr(MEDIAINFO_ID,MEDIAINFO_AUDIO_MONO)); break;
					case 2: tcscat_s(s,TSIZEOF(s),LangStr(MEDIAINFO_ID,MEDIAINFO_AUDIO_STEREO)); break;
					default: stcatprintf_s(s,TSIZEOF(s),T("%d Ch"),Format.Format.Audio.Channels); break;
					}
					if (s[0])
						WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_FORMAT),s);

					if (Format.ByteRate)
					{
						IntToString(s,TSIZEOF(s),(Format.ByteRate+62)/125,0);
						tcscat_s(s,TSIZEOF(s),T(" kbit/s")); // 1000bit/sec
						WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_BITRATE),s);
					}

					break;

				case PACKET_SUBTITLE:
					if (Pin.Node)
						Info(p,Pin.Node,&y,MEDIAINFO_CODEC);
					break;
				}

				Comment(p,(player*)Player,&y,No);
			}

		if (VOutput)
		{
			int Total = 0;
			int Dropped = 0;

			VOutput->Get(VOutput,OUT_TOTAL,&Total,sizeof(int));
			VOutput->Get(VOutput,OUT_DROPPED,&Dropped,sizeof(int));

			Total += Dropped;
			if (Total)
			{
				y += 6;

				IntToString(s,TSIZEOF(s),Total,0);
				WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_VIDEO_TOTAL),s);

				IntToString(s,TSIZEOF(s),Dropped,0);
				WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_VIDEO_DROPPED),s);

				Player->Get(Player,PLAYER_VSTREAM,&No,sizeof(No));
				if (No>=0 && Reader->Get(Reader,(FORMAT_STREAM+No)|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE &&
					Format.Type == PACKET_VIDEO && Format.PacketRate.Num)
				{
					Simplify(&Format.PacketRate,(1<<30)/Total,(1<<30)/Total);
					Format.PacketRate.Num *= Total - Dropped;
					Format.PacketRate.Den *= Total;
					FractionToString(s,TSIZEOF(s),&Format.PacketRate,0,3);
					WinPropLabel(p,&y,LangStr(MEDIAINFO_ID,MEDIAINFO_AVG_FPS),s);
				}
			}
		}
	}

	return ERR_NONE;
}

static const menudef MenuDef[] =
{
	{ 0,PLATFORM_ID, PLATFORM_DONE },

	MENUDEF_END
};

WINCREATE(MediaInfo)

static int Create(win* p)
{
	MediaInfoCreate(p);
	p->LabelWidth = 64;
	p->WinWidth = 180;
	p->WinHeight = 240;
	p->Flags |= WIN_DIALOG|WIN_NOTABSTOP;
	p->MenuDef = MenuDef;
	p->Init = (nodefunc)Init;
	return ERR_NONE;
}

static const nodedef MediaInfo =
{
	sizeof(win),
	MEDIAINFO_ID,
	WIN_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create
};

void MediaInfo_Init()
{
	NodeRegisterClass(&MediaInfo);
}

void MediaInfo_Done()
{
	NodeUnRegisterClass(MEDIAINFO_ID);
}
