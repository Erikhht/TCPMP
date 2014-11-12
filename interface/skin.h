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
 * $Id: settings.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __SKIN_H
#define __SKIN_H

#define SKIN_VIEWPORT		0
#define SKIN_VOLUME			1
#define SKIN_VOLUME_THUMB	2
#define SKIN_SEEK			3
#define SKIN_SEEK_THUMB		4
#define SKIN_PLAY			5
#define SKIN_STOP			6
#define SKIN_NEXT			7
#define SKIN_PREV			8
#define SKIN_FULLSCREEN		9
#define SKIN_FFWD			10
#define SKIN_PLAYLIST		11
#define SKIN_OPEN			12
#define SKIN_MUTE			13
#define SKIN_REPEAT			14
#define SKIN_0				15
#define SKIN_1				16
#define SKIN_ROTATE			17
#define SKIN_TITLE			18
#define SKIN_PLAY_FULLSCREEN 19
#define SKIN_EXIT			20
#define SKIN_SHUFFLE		21

#define MAX_SKINITEM		22

typedef struct skinitem
{
	bool_t Pushed;
	rect Rect;
	point PushedPos;
	rgbval_t ColorFace;
	rgbval_t ColorText;
	tchar_t Text[8];

} skinitem;


typedef struct skinbitmap
{
	tchar_t Name[32];
	int Width;
	int Height;
	void* DC;
	void* Bitmap;

} skinbitmap;

typedef struct skin
{
	bool_t Valid;
	bool_t Hidden;
	bool_t VolumeBar;
	skinbitmap Bitmap[2];
	skinitem Item[MAX_SKINITEM];

} skin;

#define MAX_SKIN	3

#ifdef __cplusplus
extern "C" {
#endif

void* SkinLoad(skin* p,void* Wnd,const tchar_t* Path);
void SkinFree(skin* p,void*);
void SkinDraw(const skin* p,void* DC,rect*);
void SkinDrawItem(skin* p,void* Wnd,int n,rect*);
void SkinUpdate(skin* p,player* Player,void* Wnd,rect*);
int SkinMouse(skin* p,int no,int x,int y,player* Player,int* cmd,void* Wnd,rect*);

#ifdef __cplusplus
}
#endif

#endif
