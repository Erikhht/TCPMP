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
 * $Id: color.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

typedef struct colorsetting
{
	node Node;
	bool_t Dither;
	int Brightness;
	int Contrast;
	int Saturation;
	int RGBAdjust[3];
	int Caps;

} colorsetting;

static const datatable Params[] = 
{
	{ COLOR_BRIGHTNESS,		TYPE_INT, DF_SETUP|DF_MINMAX, -128,127 },
	{ COLOR_CONTRAST,		TYPE_INT, DF_SETUP|DF_MINMAX, -128,127 },
	{ COLOR_SATURATION,		TYPE_INT, DF_SETUP|DF_MINMAX, -128,127 },
	{ COLOR_R_ADJUST,		TYPE_INT, DF_SETUP|DF_MINMAX, -64,63 },
	{ COLOR_G_ADJUST,		TYPE_INT, DF_SETUP|DF_MINMAX, -64,63 },
	{ COLOR_B_ADJUST,		TYPE_INT, DF_SETUP|DF_MINMAX, -64,63 },
	{ COLOR_DITHER,			TYPE_BOOL, DF_SETUP|DF_HIDDEN },
	{ COLOR_RESET,			TYPE_RESET,DF_SETUP|DF_NOSAVE|DF_GAP },
	{ COLOR_CAPS,			TYPE_INT, DF_HIDDEN },

	DATATABLE_END(COLOR_ID)
};

static int Enum(colorsetting* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static int Get(colorsetting* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case COLOR_RESET: Result = ERR_NONE; break;
	case COLOR_BRIGHTNESS: GETVALUE(p->Brightness,int); if (!(p->Caps & VC_BRIGHTNESS)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_CONTRAST: GETVALUE(p->Contrast,int); if (!(p->Caps & VC_CONTRAST)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_SATURATION: GETVALUE(p->Saturation,int); if (!(p->Caps & VC_SATURATION)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_R_ADJUST: GETVALUE(p->RGBAdjust[0],int); if (!(p->Caps & VC_RGBADJUST)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_G_ADJUST: GETVALUE(p->RGBAdjust[1],int); if (!(p->Caps & VC_RGBADJUST)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_B_ADJUST: GETVALUE(p->RGBAdjust[2],int); if (!(p->Caps & VC_RGBADJUST)) Result = ERR_NOT_SUPPORTED; break;
	case COLOR_DITHER: GETVALUE(p->Dither,bool_t); if (!(p->Caps & VC_DITHER)) Result = ERR_NOT_SUPPORTED; break;
	}
	return Result;
}

static NOINLINE int UpdateVideo()
{
	context* p = Context();
	if (!p->Player)
		return ERR_NONE;
	return p->Player->Set(p->Player,PLAYER_UPDATEVIDEO,NULL,0);
}

static int Set(colorsetting* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case COLOR_BRIGHTNESS: SETVALUECMP(p->Brightness,int,UpdateVideo(),EqInt); break;
	case COLOR_CONTRAST: SETVALUECMP(p->Contrast,int,UpdateVideo(),EqInt); break;
	case COLOR_SATURATION: SETVALUECMP(p->Saturation,int,UpdateVideo(),EqInt); break;
	case COLOR_R_ADJUST: SETVALUECMP(p->RGBAdjust[0],int,UpdateVideo(),EqInt); break;
	case COLOR_G_ADJUST: SETVALUECMP(p->RGBAdjust[1],int,UpdateVideo(),EqInt); break;
	case COLOR_B_ADJUST: SETVALUECMP(p->RGBAdjust[2],int,UpdateVideo(),EqInt); break;
	case COLOR_DITHER: SETVALUECMP(p->Dither,bool_t,UpdateVideo(),EqBool); break;
	case COLOR_CAPS: SETVALUE(p->Caps,int,ERR_NONE); break;
	}
	return Result;
}

static int Create(colorsetting* p)
{
	video Desktop;
	QueryDesktop(&Desktop);

	p->Node.Enum = (nodeenum)Enum;
	p->Node.Set = (nodeset)Set;
	p->Node.Get = (nodeget)Get;

#if !defined(SH3) && !defined(MIPS)
#if defined(TARGET_PALMOS)
	p->Dither = Desktop.Width*Desktop.Height <= 160*160;
#else
	p->Dither = Desktop.Width*Desktop.Height < 480*320;
#endif
#else
	p->Dither = 1; // gray still needs dither flag
#endif

#if !defined(TARGET_WIN32)
	// default for handheld LCD panels
	p->Brightness = 14;
	p->Saturation = 14;
#endif

#if defined(TARGET_PALMOS)
	// with SonyHHE using brightness and saturate is 10% slower
	if (NodeEnumClass(NULL,FOURCC('G','E','2','D')))
	{
		p->Brightness = 0;
		p->Saturation = 0;
	}
#endif

	p->RGBAdjust[0] = p->RGBAdjust[1] = p->RGBAdjust[2] = 0;
	return ERR_NONE;
}

static const nodedef Color =
{
	sizeof(colorsetting)|CF_GLOBAL|CF_SETTINGS,
	COLOR_ID,
	NODE_CLASS,
	PRI_MAXIMUM+575,
	(nodecreate)Create,
};

void Color_Init()
{
	NodeRegisterClass(&Color);
}

void Color_Done()
{
	NodeUnRegisterClass(COLOR_ID);
}
