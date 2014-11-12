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
 * $Id: overlay_ddraw.c 276 2005-08-23 14:47:32Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#if defined(TARGET_WINCE)
#define TWIN(a) L ## a
#else
#define TWIN(a) a
#endif

typedef struct palitem
{
	char ch;
	uint8_t col;
	uint8_t y[4];
	uint8_t u;
	uint8_t v;
	uint8_t rgb[4][3];

} palitem;

#define MAPCOUNT	29
//#define MAPCOUNT	14

typedef struct console
{
	overlay Overlay;
	planes Planes;
	planes Dither;
	HANDLE Handle;
	HANDLE Buffer;
	HMODULE Kernel;
	CHAR_INFO *Char;
	int16_t Width,Height;
	bool_t Own;
	HWND (WINAPI* GetConsoleWindow)();
	BOOL (WINAPI* SetConsoleOutputCP)(UINT);
	palitem** Map;
	palitem Pal[256*MAPCOUNT];

} console;

#define SAT(Value) (Value < 0 ? 0: (Value > 255 ? 255: Value))
#define SATY(Value) (Value < 16 ? 16: (Value > 255 ? 255: Value))

static const uint8_t Pal[16*4] = 
{
	0,0,0,0,
	0,0,128,0,
	0,128,0,0,
	0,128,128,0,
	128,0,0,0,
	128,0,128,0,
	128,128,0,0,
	192,192,192,0,
	128,128,128,0,
	0,0,255,0,
	0,255,0,0,
	0,255,255,0,
	255,0,0,0,
	255,0,255,0,
	255,255,0,0,
	255,255,255,0,
};

static const char Map[MAPCOUNT*5] = 
{
	' ',0,0,0,0,
	'.',0,0,3,3,
	',',0,0,4,4,
	':',3,3,3,3,
	';',3,3,4,4,
	'=',5,5,5,5,
	'~',5,5,0,0,
	'/',0,10,10,0,
	'\\',10,0,0,10,
	')',6,2,6,2,
	'(',2,6,2,6,
	'#',12,12,12,12,
	'\262',15,15,15,15,
	'\261',9,9,9,9,
	'\260',2,2,2,2,
	't',5,5,3,5,
	'f',5,3,5,5,
	'%',4,6,4,6,
	'$',8,8,8,8,
	'&',8,8,8,10,
	'K',11,9,11,9,
	'S',10,10,10,10,
	'Z',8,10,10,8,
	'W',8,8,10,10,
	'M',10,10,8,8,
	'X',11,11,11,11,
	'Y',11,11,6,6,
	'G',10,10,10,12,
	'8',11,11,11,11,
};

static INLINE void ToRGB(uint8_t* p,int y,int u,int v)
{
	int r,g,b;
	y -= 16;
	u -= 128;
	v -= 128;
	r = (y*0x2568             + 0x3343*v) /0x2000;
	g = (y*0x2568 - 0x0c92*u  - 0x1a1e*v) /0x2000;
	b = (y*0x2568 + 0x40cf*u)             /0x2000;
	p[0] = (uint8_t)(SAT(r));
	p[1] = (uint8_t)(SAT(g));
	p[2] = (uint8_t)(SAT(b));
}

static palitem* GetMap(console* p,int y0,int y1,int y2,int y3,int u,int v)
{
	palitem** q;
	q = &p->Map[(y0>>5)*8*8*8*16*16+
					     (y1>>5)*8*8*16*16+
					     (y2>>5)*8*16*16+
					     (y3>>5)*16*16+
					     (u>>4)*16+(v>>4)];

	if (*q==NULL)
	{
		//find best
		palitem* Best = NULL;
		int BestDiff = MAX_INT;
		palitem* Pal = p->Pal;
		uint8_t RGB[4][3];
		int i;

		y0 &= ~31;
		y1 &= ~31;
		y2 &= ~31;
		y3 &= ~31;
		u &= ~15;
		v &= ~15;

		ToRGB(RGB[0],y0,u,v);
		ToRGB(RGB[1],y1,u,v);
		ToRGB(RGB[2],y2,u,v);
		ToRGB(RGB[3],y3,u,v);

		for (i=0;i<MAPCOUNT*256;++i,++Pal)
		{
			int Diff;
#if defined(_MSC_VER) && defined(_M_IX86)
			uint16_t Diff2[2];
			__asm
			{
				mov eax,Pal
				pxor mm0,mm0

				movq mm2,[RGB]
				movd mm4,[RGB+8]
				movq mm3,mm2
				punpcklbw mm2,mm0		
				punpckhbw mm3,mm0		
				punpcklbw mm4,mm0		

				movq mm5,[eax+8]
				movd mm7,[eax+8+8]
				movq mm6,mm5
				punpcklbw mm5,mm0		
				punpckhbw mm6,mm0		
				punpcklbw mm7,mm0

				psubw mm2,mm5
				psubw mm3,mm6
				psubw mm4,mm7
				pxor mm5,mm5
				pxor mm6,mm6
				pxor mm7,mm7
				pcmpgtw	mm5,mm2
				pcmpgtw	mm6,mm3
				pcmpgtw	mm7,mm4
				pxor mm2,mm5
				pxor mm3,mm6
				pxor mm4,mm7
				paddw mm2,mm5
				paddw mm3,mm6
				paddw mm4,mm7

				paddw mm2,mm3
				paddw mm2,mm4
				movq mm0,mm2
				psrl mm0,32
				paddw mm0,mm2
				movd Diff2,mm0
			};
			Diff = Diff2[0] + Diff2[1];
#else			
			Diff = 
				abs(RGB[0][0] - Pal->rgb[0][0]) + 
				abs(RGB[0][1] - Pal->rgb[0][1]) + 
				abs(RGB[0][2] - Pal->rgb[0][2]);

			if (Diff >= BestDiff) continue;
			Diff +=
				abs(RGB[1][0] - Pal->rgb[1][0]) + 
				abs(RGB[1][1] - Pal->rgb[1][1]) + 
				abs(RGB[1][2] - Pal->rgb[1][2]);

			if (Diff >= BestDiff) continue;
			Diff +=
				abs(RGB[2][0] - Pal->rgb[2][0]) + 
				abs(RGB[2][1] - Pal->rgb[2][1]) + 
				abs(RGB[2][2] - Pal->rgb[2][2]);

			if (Diff >= BestDiff) continue;
			Diff +=
				abs(RGB[3][0] - Pal->rgb[3][0]) + 
				abs(RGB[3][1] - Pal->rgb[3][1]) + 
				abs(RGB[3][2] - Pal->rgb[3][1]);
#endif

			if (Diff < BestDiff)
			{
				BestDiff = Diff;
				Best = Pal;
			}
		}

#if defined(_MSC_VER) && defined(_M_IX86)
		__asm { emms };
#endif
		*q = Best;
	}
	return *q;
}

static int Init(console* p)
{
	int i,j,k,l;
	palitem* w;

	if (!p->Map)
	{
		p->Map = (palitem**) malloc(8*8*8*8*16*16*sizeof(palitem*));
		if (!p->Map)
			return ERR_OUT_OF_MEMORY;
		memset(p->Map,0,8*8*8*8*16*16*sizeof(palitem*));
	}

	if (SurfaceAlloc(p->Planes,&p->Overlay.Output.Format.Video)!=ERR_NONE)
		return ERR_OUT_OF_MEMORY;

	if (SurfaceAlloc(p->Dither,&p->Overlay.Output.Format.Video)!=ERR_NONE)
		return ERR_OUT_OF_MEMORY;

	memset(p->Planes[0],0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height);
	memset(p->Planes[1],128,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);
	memset(p->Planes[2],128,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);

	memset(p->Dither[0],0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height);
	memset(p->Dither[1],0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);
	memset(p->Dither[2],0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);

	p->Kernel = LoadLibrary(T("kernel32.dll"));
	*(FARPROC*)&p->GetConsoleWindow = GetProcAddress(p->Kernel,TWIN("GetConsoleWindow"));
	*(FARPROC*)&p->SetConsoleOutputCP = GetProcAddress(p->Kernel,TWIN("SetConsoleOutputCP"));

	p->Handle = INVALID_HANDLE_VALUE;
	p->Width = (int16_t)(p->Overlay.Output.Format.Video.Width/2);
	p->Height = (int16_t)(p->Overlay.Output.Format.Video.Height/2);
	p->Char = malloc(sizeof(CHAR_INFO)*p->Width*p->Height);
	if (!p->Char)
		return ERR_OUT_OF_MEMORY;

	w = p->Pal;
	for (i=0;i<16;++i)
		for (j=0;j<16;++j)
			for (k=0;k<MAPCOUNT;++k,++w)
			{
				int r,g,b,t;
				const char* ch = &Map[k*5];

				w->ch = ch[0];
				w->col = (uint8_t)(i*16+j);

				++ch;
				for (l=0;l<4;++l,++ch)
				{
					w->rgb[l][0] = (uint8_t)((Pal[i*4+0]*(32-ch[0])+Pal[j*4+0]*ch[0])>>5);
					w->rgb[l][1] = (uint8_t)((Pal[i*4+1]*(32-ch[0])+Pal[j*4+1]*ch[0])>>5);
					w->rgb[l][2] = (uint8_t)((Pal[i*4+2]*(32-ch[0])+Pal[j*4+2]*ch[0])>>5);
				}

				r = (w->rgb[0][0]+w->rgb[1][0]+w->rgb[2][0]+w->rgb[3][0])/4;
				g = (w->rgb[0][1]+w->rgb[1][1]+w->rgb[2][1]+w->rgb[3][1])/4;
				b = (w->rgb[0][2]+w->rgb[1][2]+w->rgb[2][2]+w->rgb[3][2])/4;

				t = (-(1212 * r) - (2384 * g) + (3596 * b))/0x2000 + 128;
				w->u = (uint8_t)(SAT(t));

				t = ((3596 * r) - (3015 * g) - (582 * b))/0x2000 + 128;
				w->v = (uint8_t)(SAT(t));

				t = ((2105 * w->rgb[0][0]) + (4128 * w->rgb[0][1]) + (802 * w->rgb[0][2]))/0x2000 + 16;
				w->y[0] = (uint8_t)(SATY(t));

				t = ((2105 * w->rgb[1][0]) + (4128 * w->rgb[1][1]) + (802 * w->rgb[1][2]))/0x2000 + 16;
				w->y[1] = (uint8_t)(SATY(t));

				t = ((2105 * w->rgb[2][0]) + (4128 * w->rgb[2][1]) + (802 * w->rgb[2][2]))/0x2000 + 16;
				w->y[2] = (uint8_t)(SATY(t));

				t = ((2105 * w->rgb[3][0]) + (4128 * w->rgb[3][1]) + (802 * w->rgb[3][2]))/0x2000 + 16;
				w->y[3] = (uint8_t)(SATY(t));
			}

	p->Overlay.ClearFX = BLITFX_ONLYDIFF;
	return ERR_NONE;
}

static void Done(console* p)
{
	SurfaceFree(p->Planes); 
	SurfaceFree(p->Dither);
	FreeLibrary(p->Kernel);
	free(p->Char);
	p->Char = NULL;
}

static void Delete(console* p)
{
	free(p->Map);
	p->Map = NULL;
}

static int UpdateShow(console* p)
{
	CONSOLE_CURSOR_INFO Info;

	if (p->Overlay.Show)
	{
		COORD Size;
		p->Own = AllocConsole() != FALSE;

        p->Handle = CreateFile(T("CONOUT$"), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (p->Handle == INVALID_HANDLE_VALUE)
			return ERR_NOT_SUPPORTED;

		p->Buffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,0,NULL,CONSOLE_TEXTMODE_BUFFER,NULL);
		if (p->Buffer == INVALID_HANDLE_VALUE)
			return ERR_NOT_SUPPORTED;

		Size.X = p->Width;
		Size.Y = p->Height;
		SetConsoleScreenBufferSize(p->Buffer,Size);

		GetConsoleCursorInfo(p->Buffer, &Info);
		Info.bVisible = FALSE;
		SetConsoleCursorInfo(p->Buffer, &Info);

		SetConsoleMode(p->Buffer, 0);
		SetConsoleActiveScreenBuffer(p->Buffer);

		if (p->SetConsoleOutputCP)
			p->SetConsoleOutputCP(850);

		if (p->GetConsoleWindow)
			SetWindowPos(p->GetConsoleWindow(),HWND_TOP,0,0,1024,1024,SWP_NOMOVE|SWP_SHOWWINDOW);
	}
	else
	{
		if (p->Handle != INVALID_HANDLE_VALUE)
		{
			SetConsoleActiveScreenBuffer(p->Handle);

			CloseHandle(p->Buffer);
			CloseHandle(p->Handle);

			if (p->Own)
				FreeConsole();
		}
	}
	return ERR_NONE;
}

static int Lock(console* p, planes Planes, bool_t OnlyAligned )
{
	Planes[0] = p->Planes[0];
	return ERR_NONE;
}

static int Unlock(console* p )
{
	return ERR_NONE;
}

static int Blit(console* p, const constplanes Data, const constplanes DataLast )
{
	if (p->Handle != INVALID_HANDLE_VALUE)
	{
		COORD Pos,Size;
		SMALL_RECT Rect;
		int n,Row;
		CHAR_INFO *Char;
		uint8_t *y,*u,*v;
		int8_t *y2,*u2,*v2;
		int dy[4],du,dv,PalY;

		BlitImage(p->Overlay.Soft,p->Planes,Data,DataLast,-1,-1);

		y = p->Planes[0];
		u = p->Planes[1];
		v = p->Planes[2];

		y2 = p->Dither[0];
		u2 = p->Dither[1];
		v2 = p->Dither[2];

		if (p->Overlay.Dirty)
		{
			memset(y2,0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height);
			memset(u2,0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);
			memset(v2,0,p->Overlay.Output.Format.Video.Pitch*p->Overlay.Output.Format.Video.Height/4);
		}

		Char = p->Char;
		Row = 0;
		dy[0] = dy[1] = dy[2] = dy[3] = du = dv = 0;

		for (n = p->Width*p->Height;--n>=0;++Char)
		{
			int Pitch = p->Overlay.Output.Format.Video.Pitch;
			palitem* Pal;

			dy[0] += y[0] + y2[0];
			dy[1] += y[1] + y2[1];
			dy[2] += y[Pitch+0] + y2[Pitch+0];
			dy[3] += y[Pitch+1] + y2[Pitch+1];
			du += u[0] + u2[0];
			dv += v[0] + v2[0];

			dy[0] = SATY(dy[0]);
			dy[1] = SATY(dy[1]);
			dy[2] = SATY(dy[2]);
			dy[3] = SATY(dy[3]);
			du = SAT(du);
			dv = SAT(dv);

			Pal = GetMap(p,dy[0],dy[1],dy[2],dy[3],du,dv);

			y2[0] = (int8_t)((dy[0]-Pal->y[0])>>1);
			y2[1] = (int8_t)((dy[1]-Pal->y[1])>>1);
			y2[Pitch+0] = (int8_t)((dy[2]-Pal->y[2])>>1);
			y2[Pitch+1] = (int8_t)((dy[3]-Pal->y[3])>>1);

			PalY = Pal->y[0]+Pal->y[1]+Pal->y[2]+Pal->y[3];
			dy[0] = dy[1] = dy[2] = dy[3] = (dy[0]+dy[1]+dy[2]+dy[3]-PalY)/8;
			du = (du - Pal->u) >> 1;
			dv = (dv - Pal->v) >> 1;
			if (PalY < 48*4)
			{
				du /= 2;
				dv /= 2;

				if (PalY < 32*4)
				{
					du /= 4;
					dv /= 4;
				}
			}

			u2[0] = (int8_t)du;
			v2[0] = (int8_t)dv;

			Char->Attributes = Pal->col;
			Char->Char.AsciiChar = Pal->ch;

			y += 2;
			++u;
			++v;

			y2 += 2;
			++u2;
			++v2;

			if (++Row == p->Width)
			{
				Row = 0;
				y += p->Overlay.Output.Format.Video.Pitch;
				y2 += p->Overlay.Output.Format.Video.Pitch;
			}
		}

		Size.X = p->Width;
		Size.Y = p->Height;
		Pos.X = 0;
		Pos.Y = 0;
		Rect.Left = 0;
		Rect.Top = 0;
		Rect.Right = (int16_t)(p->Width-1);
		Rect.Bottom = (int16_t)(p->Height-1);
		WriteConsoleOutput(p->Buffer, p->Char, Size, Pos, &Rect);
	}
	return ERR_NONE;
}

static int Create(console* p)
{
	p->Overlay.Primary = 0;
	p->Overlay.Output.Format.Video.Width = 80*2;
	p->Overlay.Output.Format.Video.Height = 50*2;
	p->Overlay.Output.Format.Video.Direction = 0;
	p->Overlay.Output.Format.Video.Aspect = ASPECT_ONE;
	p->Overlay.Output.Format.Video.Pitch = p->Overlay.Output.Format.Video.Width;
	p->Overlay.Output.Format.Video.Pixel.Flags = PF_YUV420|PF_16ALIGNED;
	FillInfo(&p->Overlay.Output.Format.Video.Pixel);
	p->Overlay.Init = Init;
	p->Overlay.Done = Done;
	p->Overlay.Blit = Blit;
	p->Overlay.UpdateShow = UpdateShow;
	p->Overlay.Lock = Lock;
	p->Overlay.Unlock = Unlock;
	return ERR_NONE;
}

static const nodedef Console = 
{
	sizeof(console)|CF_GLOBAL,
	CONSOLE_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT-15,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void OverlayConsole_Init() 
{ 
	NodeRegisterClass(&Console);
}

void OverlayConsole_Done()
{
	NodeUnRegisterClass(CONSOLE_ID);
}

#endif
