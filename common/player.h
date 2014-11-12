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
 * $Id: player.h 551 2006-01-09 11:55:09Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PLAYER_H
#define __PLAYER_H

#define PLAYER_ID			FOURCC('P','L','A','Y')
#define PLAYER_BUFFER_ID	FOURCC('P','L','A','B')

// buffer size in KB (int)
#define PLAYER_BUFFER_SIZE	0x20
// microdrive buffer size in KB (int)
#define PLAYER_MD_BUFFER_SIZE 0x80
// microdrive mode (bool_t)
#define PLAYER_MICRODRIVE	0x21
// repeat (bool_t)
#define PLAYER_REPEAT		0x22
// shuffle (bool_t)
#define PLAYER_SHUFFLE		0x23
// start play after open (bool_t)
#define PLAYER_PLAYATOPEN	0x24
// start play after open in fullscreen (bool_t)
#define PLAYER_PLAYATOPEN_FULL	0x7B
// exit at the end (bool_t)
#define PLAYER_EXIT_AT_END	0xB9
// keep playlist after WND set null (bool_t)
#define PLAYER_KEEPLIST		0x48
// play or pause (bool_t) 
#define PLAYER_PLAY			0x32
// play speed (fraction)
#define PLAYER_PLAY_SPEED	0x3F
// fastforward (bool_t) 
#define PLAYER_FFWD			0x49
// fastforward speed (fraction) 
#define PLAYER_FFWD_SPEED	0x4A
// position in fraction (fraction)
#define PLAYER_PERCENT		0x25
// position in time (tick_t) 
#define PLAYER_POSITION		0x28
// duration (tick_t) 
#define PLAYER_DURATION		0x46
// timer shown on screen (tchar_t[]) 
#define PLAYER_TIMER		0x7E
// show left time (bool_t) 
#define PLAYER_TIMER_LEFT	0x7F
// benchmark duration (tick_t) 
#define PLAYER_BENCHMARK	0x55
// benchmark video source size (point) 
#define PLAYER_BENCHMARK_SRC 0xB6
// benchmark video dest size (point) 
#define PLAYER_BENCHMARK_DST 0xB7
// display name (tchar_t[])
#define PLAYER_TITLE		0x29
// current format (format*)
#define PLAYER_FORMAT		0x2B
// current stream (stream*)
#define PLAYER_INPUT		0x2A
// current audio stream (int)
#define PLAYER_VSTREAM		0x76
// current video stream (int)
#define PLAYER_ASTREAM		0x77
// current subtitle stream (int)
#define PLAYER_SUBSTREAM	0x82
// current audio output class (int)
#define PLAYER_AOUTPUTID	0x83
// current video output class (int)
#define PLAYER_VOUTPUTID	0x84
// highest priorty audio output class (readonly int)
#define PLAYER_AOUTPUTID_MAX 0xA0
// highest priorty video output class (readonly int)
#define PLAYER_VOUTPUTID_MAX 0xA1
// current audio output (node*)
#define PLAYER_AOUTPUT		0x2C
// current video output (node*)
#define PLAYER_VOUTPUT		0x2D
// number of files in playlist (int)
#define PLAYER_LIST_COUNT	0x2E
// current file in playlist (int) 
#define PLAYER_LIST_CURRENT	0x2F
// current file index (suffled) in playlist (int) 
#define PLAYER_LIST_CURRIDX	0xA2
// fullscreen zoom factor (fraction_t)
#define PLAYER_FULL_ZOOM	0x35
// skin mode zoom factor (fraction_t)
#define PLAYER_SKIN_ZOOM	0x36
// bilinear zoom (bool_t)
#define PLAYER_SMOOTH50		0x47
// bilinear zoom (bool_t)
#define PLAYER_SMOOTHALWAYS	0x7A
// fullscreen direction flags (int)
#define PLAYER_FULL_DIR		0x39
// non fullscreen direction flags (int)
#define PLAYER_SKIN_DIR		0x45
// current relative dir (readonly) (int)
#define PLAYER_REL_DIR		0x7D
// overlay is on top or clipping needed (bool_t)
#define PLAYER_CLIPPING		0x3B
// skin viewport rectangle (rect)
#define PLAYER_SKIN_VIEWPORT 0x3C
// prerotate portrait movies
#define PLAYER_AUTOPREROTATE 0x3D
// fullscreen mode (bool_t)
#define PLAYER_FULLSCREEN	0x3E
// volume volume (int 0..100)
#define PLAYER_VOLUME		0x40
// volume mute (bool_t)
#define PLAYER_MUTE			0x41
// panning (int -128..128)
#define PLAYER_PAN			0x9D
// preamp (int -128..128)
#define PLAYER_PREAMP		0x9E
// audio quality (int 0..2)
#define PLAYER_AUDIO_QUALITY 0x42
// auto video quality (int 0..2)
#define PLAYER_VIDEO_QUALITY 0xBC
// video idct acceleration (bool_t)
#define PLAYER_VIDEO_ACCEL	0x44
// keep audio playing in background
#define PLAYER_KEEPPLAY_AUDIO 0x63
// keep video playing in background
#define PLAYER_KEEPPLAY_VIDEO 0x72
#define PLAYER_SHOWINBACKGROUND 0xBF
#define PLAYER_SINGLECLICKFULLSCREEN 0xC0 
// microdrive start at in KB (int)
#define PLAYER_BURSTSTART	0xA3
// how much to load (%)
#define PLAYER_UNDERRUN		0x67
// how much to load for audio (int)
#define PLAYER_AUDIO_UNDERRUN 0xBB
// move back step time (tick_t)
#define PLAYER_MOVEBACK_STEP 0x68
// move ffwd step time (tick_t)
#define PLAYER_MOVEFFWD_STEP 0x7C
// stereo enum (int)
#define PLAYER_STEREO		0x74
// aspect ratio (frac)		
#define PLAYER_ASPECT		0xBE
// application sent to background (bool_t)		
#define PLAYER_BACKGROUND	0x98
// player window is foreground (bool_t)		
#define PLAYER_FOREGROUND	0xB4
// set before and after sleep (bool_t)		
#define PLAYER_POWEROFF		0xB5
// video caps
#define PLAYER_VIDEO_CAPS	0x64
// discard saved playlist (bool)
#define PLAYER_DISCARDLIST	0xBA
// video output is a real overlay, readonly (bool_t)
#define PLAYER_VIDEO_OVERLAY 0xBD

// player status
#define PLAYER_SYNCING		0x81
#define PLAYER_LOADMODE		0x96
#define PLAYER_STREAMING	0xB8

// notify pin for interface
#define PLAYER_NOTIFY		0x97
// on list changed event
#define PLAYER_LIST_NOTIFY  0x78
// for open dialog
#define PLAYER_CURRENTDIR	0x6D

// commands:
// update video settings
#define PLAYER_UPDATEVIDEO	0x90
// update equalizer settings
#define PLAYER_UPDATEEQUALIZER 0xA5
// begin screen rotation (turn off video)
#define PLAYER_ROTATEBEGIN	0x91
// end screen rotation 
#define PLAYER_ROTATEEND	0x92
// reset video driver
#define PLAYER_RESETVIDEO	0x93
// codec notify about not supported data (pin)
#define PLAYER_NOT_SUPPORTED_DATA 0x9F
// move back
#define PLAYER_MOVEBACK		0x9A
// move forward
#define PLAYER_MOVEFFWD		0x9B
// position bar moving state (boolean)
#define PLAYER_INSEEK		0x9C
// next chapter/track
#define PLAYER_NEXT			0xB0
// prev chapter/track
#define PLAYER_PREV			0xB1
// stop
#define PLAYER_STOP			0xB2
// resync
#define PLAYER_RESYNC		0xB3

// array type params (just a hint)
#define PLAYER_ARRAY		0x1000
// url in playlist (0x1000,0x1001,0x1002...)
#define PLAYER_LIST_URL		0x1000
// title in playlist (0x2000,0x2001,0x2002...)
#define PLAYER_LIST_TITLE	0x2000
// title or filename in playlist
#define PLAYER_LIST_AUTOTITLE 0x5000
// length in playlist (0x4000,0x4001,0x4002...)
#define PLAYER_LIST_LENGTH	0x4000
// input comments 
#define PLAYER_COMMENT		0x3000

typedef void (*playerpaint)(void* This,void* DC,int x0,int y0);
typedef int	(*playerprocess)(void* This);
typedef bool_t (*playercomment)(void* This,int Stream,const tchar_t* Name,tchar_t* Value,int ValueLen);
typedef void (*playerswap)(void* This,int a,int b);

typedef struct player_t
{
	VMT_NODE
	playerpaint		Paint;
	playercomment	CommentByName;
	playerswap		ListSwap;
	playerprocess	Process;

} player;

#define MAXCHAPTER	100
#define MAXDIRDEEP	10

void Player_Init();
void Player_Done();

DLL int PlayerAdd(player* Player,int Index, const tchar_t* Path, const tchar_t* Title);
DLL int PlayerAddDir(player* Player,int Index, const tchar_t* Path, const tchar_t* Exts, bool_t ExtFilter, int Deep);
DLL void PlayerSaveList(player* Player,const tchar_t* Path,int Class);
DLL const tchar_t* PlayerComment(int Code); // standard english comment tags
DLL tick_t PlayerGetChapter(player* Player,int No,tchar_t* OutName,int OutNameLen);
DLL bool_t PlayerGetStream(player* Player,int No,packetformat* OutFormat,tchar_t* OutName,int OutNameLen,int* OutPri);

// strings
#define PLAYER_MIME_NOT_FOUND	0x105
#define PLAYER_FORMAT_NOT_FOUND	0x106
#define PLAYER_BUFFER_WARNING	0x107
#define PLAYER_AUDIO_NOT_FOUND	0x108
#define PLAYER_VIDEO_NOT_FOUND	0x109
#define PLAYER_BUFFER_UNDERRUN	0x10A
#define PLAYER_AUDIO_NOT_FOUND2	0x10B
#define PLAYER_VIDEO_NOT_FOUND2	0x10C

#define PLAYERQUALITY_ENUM		FOURCC('P','L','A','Q')
#define PLAYERQUALITY_LOW		0x100
#define PLAYERQUALITY_MEDIUM	0x101
#define PLAYERQUALITY_HIGH		0x102

// translated comments
#define COMMENT_TITLE			0x110
#define COMMENT_ARTIST			0x111
#define COMMENT_ALBUM			0x112
#define COMMENT_LANGUAGE		0x113
#define COMMENT_GENRE			0x114
#define COMMENT_AUTHOR			0x115
#define COMMENT_COPYRIGHT		0x116
#define COMMENT_PRIORITY		0x117
#define COMMENT_COMMENT			0x118
#define COMMENT_TRACK			0x119
#define COMMENT_YEAR			0x11A
#define COMMENT_COVER			0x11B
#define COMMENT_REDIRECT		0x11C		

// default stream name prefixes
#define STREAM_NAME				0x120
#define STREAM_NAME_NONE		(STREAM_NAME + PACKET_NONE)
#define STREAM_NAME_VIDEO		(STREAM_NAME + PACKET_VIDEO)
#define STREAM_NAME_AUDIO		(STREAM_NAME + PACKET_AUDIO)
#define STREAM_NAME_SUBTITLE	(STREAM_NAME + PACKET_SUBTITLE)

#endif
