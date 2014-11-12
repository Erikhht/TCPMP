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
 * $Id: sample_con.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../aac/faad.h"
#include "../ffmpeg/ffmpeg.h"
#include "../splitter/mov.h"
#include "../common/zlib/zlib.h"

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void GetModulePath(tchar_t* Path,const tchar_t* Module);

void SoftIDCT_Init() {}
void SoftIDCT_Done() {}
void OverlayXScale_Init() {}
void OverlayXScale_Done() {}
void OverlayGDI_Init() {}
void OverlayGDI_Done() {}
void OverlayFlyTV_Init() {}
void OverlayFlyTV_Done() {}
void OverlayDDraw_Init() {}
void OverlayDDraw_Done() {}
void ASX_Init() {}
void ASX_Done() {}
void PLS_Init() {}
void PLS_Done() {}
void M3U_Init() {}
void M3U_Done() {}
void* NodeLoadModule(const tchar_t* Path,int* Id,void** AnyFunc,void** Db) { return NULL; }
void NodeFreeModule(void* Module,void* Db) {}
bool_t NodeRegLoadValue(int Class, int Id, void* Data, int Size, int Type) { return 0; }
void NodeRegSaveValue(int Class, int Id, const void* Data, int Size, int Type) {}
void NodeRegLoad(node* p) {}
void NodeRegSave(node* p) {}
int uncompress OF((Bytef *dest,uLongf *destLen, const Bytef *source, uLong sourceLen)) { return -1; }

const uint8_t ff_h263_loop_filter_strength[32] = {0};
int ff_mpeg4_find_frame_end(void *pc, const uint8_t *buf, int buf_size) { return -100; }
int ff_mpeg4_decode_picture_header(void * s, void *gb) { return 0; }
void ff_wmv2_add_mb(void *s, void* i, uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr) {}
void ff_mspel_motion(void *s,  uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
					 uint8_t **ref_picture, void* f, int motion_x, int motion_y, int h) {}

void Static_Init() 
{
	MP4_Init();
	FAAD_Init();
	FFMPEG_Init();
}
void Static_Done() 
{
	MP4_Done();
	FAAD_Done();
	FFMPEG_Done();
}

void Main()
{
	int Int;
	bool_t Bool;
	node* Player = Context()->Player;
	node* Color = NodeEnumObject(NULL,COLOR_ID);
	tchar_t Path[MAXPATH];
		
	StringAdd(1,MP4_ID,NODE_PROBE,T("scan(4096,or(eq(le32(4),'moov'),eq(le32(4),'wide'),eq(le32(4),'free'),eq(le32(4),'mdat'),eq(le32(4),'pnot'),eq(le32(4),'prfl'),eq(le32(4),'udta')),!or(eq(le32(4),'ftyp'),eq(le32(4),'skip')),fwd(be32))"));
	StringAdd(1,FAAD_ID,NODE_CONTENTTYPE,T("acodec/0xAAC0,acodec/0x00FF"));
	StringAdd(1,FILE_ID,NODE_CONTENTTYPE,T("file"));

	GetModulePath(Path,NULL);
	tcscat_s(Path,MAXPATH,T("sample.mp4"));

	Int = CONSOLE_ID;
	Player->Set(Player,PLAYER_VOUTPUTID,&Int,sizeof(Int));

	Int = 8;
	Color->Set(Color,COLOR_BRIGHTNESS,&Int,sizeof(Int));
	Int = 32;
	Color->Set(Color,COLOR_CONTRAST,&Int,sizeof(Int));
	Int = 24;
	Color->Set(Color,COLOR_SATURATION,&Int,sizeof(Int));

	Context_Wnd((void*)1); //fake window handle

	Bool = 1;
	Player->Set(Player,PLAYER_FULLSCREEN,&Bool,sizeof(Bool));

	Player->Set(Player,PLAYER_LIST_URL+0,Path,sizeof(Path));

	((player*)Player)->Paint(Player,NULL,0,0);

	Bool = 1;
	Player->Set(Player,PLAYER_PLAY,&Bool,sizeof(Bool));

	while (!GetAsyncKeyState(27))
	{
#ifndef MULTITHREAD
		if (((player*)Player)->Process(Player) == ERR_BUFFER_FULL)
			ThreadSleep(GetTimeFreq()/250);
#else
		ThreadSleep(GetTimeFreq()/10);
#endif
	}

	Context_Wnd(NULL);
}

#include <windows.h>

int main(int paramn,const char** param)
{
	if (Context_Init("sample","sample",3,T(""),NULL))
	{
		Main();
		Context_Done();
	}
}

#undef malloc
#undef realloc
#undef free
#include "../ffmpeg/libavcodec/avcodec.h"

AVCodec wmv1_decoder = { NULL };
AVCodec wmv2_decoder = { NULL };
AVCodec msmpeg4v1_decoder = { NULL };
AVCodec msmpeg4v2_decoder = { NULL };
AVCodec msmpeg4v3_decoder = { NULL };
AVCodec mpeg4_decoder = { NULL };
AVCodec h263_decoder = { NULL };

