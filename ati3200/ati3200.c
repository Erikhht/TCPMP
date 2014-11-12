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
 * $Id: ati3200.c 624 2006-02-01 17:48:42Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "stdafx.h"

#if defined(ARM)

#define CLIENTVERSION "1.29.031201.07700" 
#define MAXSCALE	2
#define SCALE		MAXIDCTBUF

#define DCFACTOR 8

static const char DevOpenName[0x50] = "TCPMP";

enum
{
	PLANE_Y			= 0x10000000,
	PLANE_U			= 0x20000000,
	PLANE_V			= 0x40000000,
	SUCCESS			= 0,
	FAIL			= 1,
	ENDENUM			= 5,

	FORMAT_PAL4		= 1,
	FORMAT_PAL8		= 2,
	FORMAT_RGB444	= 3,
	FORMAT_RGB555	= 4,
	FORMAT_RGB565	= 5,
	FORMAT_YUYV		= 6,
	FORMAT_YVYU		= 7,
	FORMAT_YUV422	= 8,
	FORMAT_YUV420	= 9,
	FORMAT_UYVY		= 10,
	FORMAT_VYUY		= 11,
	FORMAT_PAL2		= 12,

	MIRROR_UPDOWN	= 1,
	MIRROR_LEFTRIGHT= 2,

	MEM_EXTERNAL	= 0x20,
	MEM_INTERNAL	= 0x40,

	MEM_ANY_INT		= -1, // prefer internal
	MEM_ANY_EXT		= -2, // prefer external
	MEM_ANY_CHG		= -3, // even:external odd:internal
	MEM_PRIMARY		= -4, // reused primary surface

	PROP_SCANTYPE	= 0,
	PROP_PADDING	= 1,
	PROP_BIAS		= 2,
	PROP_ROUNDING	= 3,
	PROP_FRAMENO	= 4,
	PROP_CODING		= 5,
	PROP_DEBUG		= 6,
};

//---------------------------------------------------------------
// ati format
//
// input is idct_ati_t[length] array
// 8x8 idct matrix reconstruction:
//
// clear block
// for (pos=0;length>0;--length,++data)
//   repeat X=0..3
//     if (data->skipX==-1) break;
//     pos += data->skipX
//     block[scan[pos++]] = data->dataX

typedef struct idct_ati
{
	char skip3,skip2,skip1,skip0;
	short data1,data0,data3,data2;
} idct_ati;

typedef struct devinfo
{
	char Name[0x50];
	char Version[0x50];
	int Revision;
	int ChipId;
	int RevisionId;
	int TotalMemory;
	int BusInterfaceMode;
	int InternalMemSize;
	int ExternalMemSize;
	int Caps1;
	int Caps2;
	int Caps3;
	int Caps4;
	char Pad[128];
} devinfo;

typedef struct modeinfo
{
	int Width;
	int Height;
	int PixelFormat;
	int Freq;
	int Rotation;
	int Mirror;
} modeinfo;

typedef struct surfinfo
{
	int Width;
	int Height;
	void* Addr;
	int Pitch;
	int Size;
	int Class;
	int One;
} surfinfo;

typedef struct planeinfo
{
	int Width;
	int Height;
	void* Addr;
	int Pitch;
	int One;
} planeinfo;

typedef struct ahirect
{
	int Left;
	int Top;
	int Right;
	int Bottom;
} ahirect;

typedef struct ahipoint
{
	int x;
	int y;
} ahipoint;

typedef void* ahidevice;
typedef void* ahicontext;
typedef void* ahisurface;
typedef void* ahiidct;

typedef struct ahioverlay
{
	ahirect DstRect; //0x00
	ahirect SrcRect; //0x10
	int ColorKey;    //0x20
	int ColorMask;   //0x24
	int Rotate;      //0x28
	int Mirror;      //0x2C
	int Zero;        //0x30
} ahioverlay;

typedef struct ahi
{
	overlay p;
	idct IdctVMT;

	bool_t Shrink;
	bool_t Stretch;
	bool_t StretchOnlyInternal;
	bool_t BlitMode;
	bool_t BlitDownScale;
	rect OverlayRect;
	video Overlay;
	blitfx SoftFX;

	int OvlFormat;
	int OvlUVX2;
	int OvlUVY2;
	int OvlPlaneY;
	int OvlPlaneU;
	int OvlPlaneV;

	// should be in the first part for lower offset
	int (*AhiIDCTMComp8x8)(ahiidct* IDCT, const idct_ati* Data, int Len, int* Back, int* Fwd, int No, int ZigZagType);
	int (*AhiIDCTMComp)(ahiidct* IDCT, int* Back, int* Fwd, int No);
	int (*AhiIDCT8x8)(ahiidct* IDCT, const idct_ati* Data, int Len, int No, int ZigZagType);

	ahiidct* IDCT;
	int DCAdd;
	//int DCAdj;
	int MacroWidth;
	int MacroNo;
	int MacroSubNo;

	int* MVBack;
	int* MVFwd;
	int MVBackTab[6];
	int MVFwdTab[6];
	idct_ati Data[16+2]; //+2 as safe because convert can write -1 after last item

	int SaveCount;
	int SaveScanType;
	idct_ati SaveData[16];

	int IDCTWidth;
	int IDCTHeight;
	int OverlayRPitch;
	int ScaleWidth;
	int ScaleHeight;
	int ScaleCurr;
	devinfo DevInfo;
	modeinfo ModeInfo;
	ahidevice* Device;
	ahicontext* Context;
	int MaxCount;
	int AllCount; //BufCount + TempCount
	int BufCount;
	int TempCount;
	int ShowNext;
	int ShowCurr;
	int ShowLast;
	bool_t ShowNextIdle;
	ahisurface* Primary; // only for blit mode
	ahisurface* Buf[MAXIDCTBUF+MAXSCALE];  
	ahisurface* BufR[MAXIDCTBUF+MAXSCALE]; 
	int BufMode[MAXIDCTBUF+MAXSCALE]; 
	int BufFrameNo[MAXIDCTBUF+MAXSCALE];
	ahioverlay Param;
	tchar_t Version[128];

	//idct
	pin IDCTOutput;
	int FrameNo;
	int Zero;
	bool_t IDCTRounding;

	idct_block_t Block[8*8];

	int (*AhiInit)(int Version);
	int (*AhiTerm)();

	int (*AhiDevEnum)(ahidevice** Device, devinfo* Info, int No);
	int (*AhiDevOpen)(ahicontext** ContextOut, ahidevice* Device, const char* Name, int v);
	int (*AhiDevClose)(ahicontext* Context);
	int (*AhiDevClientVersion)(ahicontext* Context, const char* s);

	int (*AhiDispModeGet)(ahicontext* Context, modeinfo* Info);
	int (*AhiDispModeEnum)(ahicontext* Context, modeinfo* Info, int No);
	int (*AhiDispModeClip)(ahicontext* Context, const ahirect*); //not sure
	int (*AhiDispSurfGet)(ahicontext* Context, ahisurface** SurfaceOut);

	int (*AhiSurfGetLargestFreeBlockSize)(ahicontext* Context, int Format, int* a, int* b, int Mem);
	int (*AhiSurfAlloc)(ahicontext* Context, ahisurface** SurfaceOut, int* Size, int Format, int Mem);
	int (*AhiSurfReuse)(ahicontext* Context, ahisurface** SurfaceOut, ahisurface* SurfaceReuse, int* Size, int Format, int Zero);
	int (*AhiSurfFree)(ahicontext* Context, ahisurface* s);
	int (*AhiSurfLock)(ahicontext* Context, ahisurface* s, void** Ptr, int Plane);
	int (*AhiSurfUnlock)(ahicontext* Context, ahisurface* s, int Plane);
	int (*AhiSurfInfo)(ahicontext* Context, ahisurface* s, surfinfo* Info);
	int (*AhiSurfPlaneInfo)(ahicontext* Context, ahisurface* s, planeinfo* Info, int Plane);

	int (*AhiDrawRopSet)(ahicontext* Context, int ROP);
	int (*AhiDrawSurfSrcSet)(ahicontext* Context, ahisurface* Src, int Plane);
	int (*AhiDrawSurfDstSet)(ahicontext* Context, ahisurface* Dst, int Plane);
	int (*AhiDrawIdle)(ahicontext* Context, int v);
	int (*AhiDispWaitVBlank)(ahicontext* Context, int v);
	int (*AhiDrawBitBlt)(ahicontext* Context, const ahirect* DstRect, const ahipoint* SrcPos);
	int (*AhiDrawStretchBlt)(ahicontext* Context, const ahirect* DstRect, const ahirect* SrcRect, int Mode);
	int (*AhiDrawRotateBlt)(ahicontext* Context, const ahirect* DstRect, const ahipoint* SrcPos, int Rotate, int Mirror, int Zero);

	int (*AhiDispOverlayCaps)(ahicontext* Context, int Mode, int* Caps);
	int (*AhiDispOverlayState)(ahicontext* Context, int State, int Zero);
	int (*AhiDispOverlayPos)(ahicontext* Context, const ahirect* Rect, int Zero);
	int (*AhiDispOverlaySurf)(ahicontext* Context, ahisurface* Surface, ahioverlay* Info, int Mode);

	int (*AhiIDCTOpen)(ahicontext* Context, ahiidct** IDCTOut, int Zero);
	int (*AhiIDCTClose)(ahiidct* IDCT);
	int (*AhiIDCTPropGet)(ahiidct* IDCT, int No, int* Data, int Size);
	int (*AhiIDCTPropSet)(ahiidct* IDCT, int No, const int* Data, int Size);
	int (*AhiIDCTFrameStart)(ahiidct* IDCT, ahisurface* Dst, ahisurface* Back, ahisurface* Fwd, int Frame, int Zero);
	int (*AhiIDCTFrameEnd)(ahiidct* IDCT, int Frame);

	int (*AhiUnmap)();

	bool_t TmpInited;
	bool_t NoRelease;
	bool_t AllowStretchFlip;
	bool_t AllowPrimaryReUse;
	bool_t AllowLowBitdepth;
	bool_t FullScreenOverlay;	  // overlay covers fullscreen	
	bool_t PrimaryReUse;
	bool_t LowBitdepth;
	bool_t Internal;	  // IDCT is using internal memory (it's faster, but less memory)
	bool_t Landscape;	  // avoid external memory overlays in landscape mode
	bool_t IDCTBug;
	bool_t IDCTInit;
	bool_t ErrorMemory;

} ahi;

const datatable AHIParams[] = 
{
	{ AHI_DEVICEBITMAP,	TYPE_BOOL, DF_SETUP|DF_SOFTRESET|DF_NOSAVE|DF_CHECKLIST },
	{ AHI_IDCTBUG,		TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ AHI_PRIMARYREUSE,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ AHI_LOWBITDEPTH,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ AHI_STRETCHFLIP,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ AHI_NORELEASE,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ AHI_NAME,			TYPE_STRING, DF_SETUP | DF_RDONLY|DF_GAP },
	{ AHI_VERSION,		TYPE_STRING, DF_SETUP | DF_RDONLY },
	{ AHI_REVISION,		TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_CHIPID,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },
	{ AHI_REVISIONID,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_TOTALMEMORY,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_INTERNALTOTALMEM,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_INTERNALFREEMEM,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_EXTERNALTOTALMEM,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_EXTERNALFREEMEM,	TYPE_INT, DF_SETUP | DF_RDONLY },
	{ AHI_CAPS1,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },
	{ AHI_CAPS2,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },
	{ AHI_CAPS3,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },
	{ AHI_CAPS4,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },
	{ AHI_UNMAP,		TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX },

	DATATABLE_END(AHI_ID)
};

const tchar_t* ATIBase = T("System\\GDI\\Drivers\\ATI");
const tchar_t* DisableDevice = T("DisableDeviceBitmap");

static bool_t Ahi2PixelFormat(int Format, pixel* p)
{
	switch (Format)
	{
	case FORMAT_PAL2:
		p->Flags = PF_PALETTE;
		p->BitCount = 2;
		break;
	case FORMAT_PAL4:
		p->Flags = PF_PALETTE;
		p->BitCount = 2;
		break;
	case FORMAT_PAL8:
		p->Flags = PF_PALETTE;
		p->BitCount = 2;
		break;
	case FORMAT_RGB444:
		DefaultRGB(p,16,4,4,4,0,0,0);
		break;
	case FORMAT_RGB555:
		DefaultRGB(p,16,5,5,5,0,0,0);
		break;
	case FORMAT_RGB565:
		DefaultRGB(p,16,5,6,5,0,0,0);
		break;
	case FORMAT_YUYV:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_YUYV;
		break;
	case FORMAT_YVYU:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_YVYU;
		break;
	case FORMAT_YUV420:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_I420;
		break;
	case FORMAT_UYVY:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_UYVY;
		break;
	case FORMAT_VYUY:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_VYUY;
		break;
	case FORMAT_YUV422:
		p->Flags = PF_FOURCC;
		p->FourCC = FOURCC_YV16;
		break;
	default:
		return 0;
	}
	return 1;
}

static void ClearPrimary(ahi* p)
{
	ahisurface* Primary = NULL;
	p->AhiDispSurfGet(p->Context,&Primary);
	if (Primary)
	{
		void* Ptr;
		surfinfo Info;
		pixel Pixel;

		p->AhiSurfInfo(p->Context,Primary,&Info);
		if (p->AhiSurfLock(p->Context,Primary,&Ptr,0)==SUCCESS)
		{
			Ahi2PixelFormat(Info.Class,&Pixel);
			FillColor(Ptr,Info.Pitch,0,0,Info.Width,Info.Height,Pixel.BitCount,0);
			p->AhiSurfUnlock(p->Context,Primary,0);
		}
	}
}

static void SetLowBitdepth(ahi* p,bool_t Value)
{
	if (p->LowBitdepth != Value && p->AllowLowBitdepth && p->DevInfo.ChipId == 0x57441002)
	{
		volatile int* Regs = ((int**)p->Context)[7];
		int Ctrl;

		p->LowBitdepth = Value;

		if (Value && !p->PrimaryReUse)
			ClearPrimary(p);

		while ((Regs[0x153] & 0x400)==0); // wait until not in vblank 
		while ((Regs[0x153] & 0x400)!=0); // wait until in vblank
		Ctrl = Regs[0x105];
		Ctrl &= ~7;
		Ctrl |= Value ? 2:6;
		Regs[0x105] = Ctrl;
	}
}

static bool_t Hide(ahi* p)
{
	if (p->p.Show && p->AllCount)
	{
		SetLowBitdepth(p,0);
		p->AhiDispOverlayState(p->Context,0,0);
		p->AhiDispWaitVBlank(p->Context,0);
	}
	return 1;
}

static void FreeScale(ahi* p,bool_t* Hidden)
{
	int No;
	for (No=SCALE;No<SCALE+2;++No)
		if (p->Buf[No])
		{
			if (Hidden && !*Hidden)
				*Hidden = Hide(p);

			p->AhiSurfFree(p->Context,p->Buf[No]);
			p->Buf[No] = NULL;
			if (p->BufMode[No] == MEM_PRIMARY)
			{
				p->PrimaryReUse = 0;
				WaitDisable(0);
			}
			p->BufMode[No] = 0;
		}

	p->ScaleWidth = 0;
	p->ScaleHeight = 0;
	p->ScaleCurr = 0;
}

static void FreeBuffer(ahi* p,int No,bool_t* Hidden)
{
	if (Hidden && !*Hidden && p->ShowCurr == No)
		*Hidden = Hide(p);

	if (p->Buf[No])
		p->AhiSurfFree(p->Context,p->Buf[No]);
	p->Buf[No] = NULL;

	if (p->BufR[No])
		p->AhiSurfFree(p->Context,p->BufR[No]);
	p->BufR[No] = NULL;

	if (p->BufMode[No] == MEM_PRIMARY)
	{
		p->PrimaryReUse = 0;
		WaitDisable(0);
	}
	p->BufMode[No] = 0;
}

static int Lock(ahi* p,int No,planes Planes,bool_t Force)
{
	if (!Force && (No<0 || No>=p->BufCount))
	{
		Planes[0] = NULL;
		Planes[1] = NULL;
		Planes[2] = NULL;
		return ERR_INVALID_PARAM;
	}

	p->AhiDrawIdle(p->Context,0);

	if (p->AhiSurfLock(p->Context,p->Buf[No],&Planes[0],p->OvlPlaneY)==SUCCESS)
	{
		if (!p->OvlPlaneU || p->AhiSurfLock(p->Context,p->Buf[No],&Planes[1],p->OvlPlaneU)==SUCCESS)
		{
			if (!p->OvlPlaneV || p->AhiSurfLock(p->Context,p->Buf[No],&Planes[2],p->OvlPlaneV)==SUCCESS)
				return ERR_NONE;
			if (p->OvlPlaneU) p->AhiSurfUnlock(p->Context,p->Buf[No],p->OvlPlaneU);
		}
		p->AhiSurfUnlock(p->Context,p->Buf[No],p->OvlPlaneY);
	}
	return ERR_DEVICE_ERROR;
}

static void Unlock(ahi* p,int No)
{
	p->AhiSurfUnlock(p->Context,p->Buf[No],p->OvlPlaneY);
	if (p->OvlPlaneU) p->AhiSurfUnlock(p->Context,p->Buf[No],p->OvlPlaneU);
	if (p->OvlPlaneV) p->AhiSurfUnlock(p->Context,p->Buf[No],p->OvlPlaneV);
}

static bool_t CheckVersion(ahi* p,bool_t IDCT)
{
	int c = IDCT?27:24;
	int a,b;

	StrToTcs(p->Version,TSIZEOF(p->Version),p->DevInfo.Version);
	if (sscanf(p->DevInfo.Version,"%d.%d",&a,&b)!=2)
		a = 2; // force true 
	return a>1 || (a==1 && b>=c);
}

static void GetMode(ahi* p)
{
	// get current mode parameters
	p->AhiDispModeGet(p->Context,&p->ModeInfo);

	memset(&p->p.Output.Format.Video,0,sizeof(video));
	p->p.Output.Format.Video.Width = p->ModeInfo.Width;
	p->p.Output.Format.Video.Height = p->ModeInfo.Height;
	p->p.Output.Format.Video.Direction = 0;
	p->p.Output.Format.Video.Aspect = ASPECT_ONE;

	if (p->ModeInfo.Rotation!=0 && p->ModeInfo.Rotation!=2)
		p->p.Output.Format.Video.Direction ^= DIR_SWAPXY;

	switch (p->ModeInfo.Rotation)
	{
	case 1:
		p->p.Output.Format.Video.Direction ^= DIR_MIRRORLEFTRIGHT;
		break;
	case 2:
		p->p.Output.Format.Video.Direction ^= DIR_MIRRORLEFTRIGHT|DIR_MIRRORUPDOWN;
		break;
	case 3:
		p->p.Output.Format.Video.Direction ^= DIR_MIRRORUPDOWN;
		break;
	}

	if (p->ModeInfo.Mirror & MIRROR_LEFTRIGHT)
		p->p.Output.Format.Video.Direction ^= DIR_MIRRORLEFTRIGHT;

	if (p->ModeInfo.Mirror & MIRROR_UPDOWN)
		p->p.Output.Format.Video.Direction ^= DIR_MIRRORUPDOWN;

	Ahi2PixelFormat(p->ModeInfo.PixelFormat,&p->p.Output.Format.Video.Pixel);
	FillInfo(&p->p.Output.Format.Video.Pixel);

	AdjustOrientation(&p->p.Output.Format.Video,0);
}

static bool_t UpdateOverlay(ahi* p,int BufCount,int TempCount);
static int IDCTUpdateFunc(ahi* p);
static void Done(ahi* p);

static bool_t SafeAhiInit(ahi* p)
{
	if (p->TmpInited)
	{
		p->TmpInited = 0;
		return 1;
	}
	return p->AhiInit(0)==SUCCESS;
}

static void SafeAhiTerm(ahi* p)
{
	if (p->NoRelease)
		p->TmpInited = 1;
	else
	{
		p->AhiTerm();
		if (p->AhiUnmap)
			p->AhiUnmap();
	}
}

static int UpdateNoRelease(ahi* p)
{
	if (p->TmpInited && !p->NoRelease)
	{
		p->TmpInited = 0;
		SafeAhiTerm(p);
	}
	return ERR_NONE;
}

static int Enum(ahi* p, int* No, datadef* Param)
{
	if (OverlayEnum(&p->p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,AHIParams);
}

static int Set(ahi* p,int No,const void* Data,int Size)
{
	int Result = OverlaySet(&p->p,No,Data,Size);
	switch (No)
	{
	case NODE_CRASH:
		Done(p);
		break;

	case AHI_DEVICEBITMAP:
		if (Size==sizeof(bool_t))
		{
			DWORD Disp;
			DWORD Value;
			HKEY Key;
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, ATIBase, 0, NULL, 0, 0, NULL, &Key, &Disp) == ERROR_SUCCESS)
			{
				Value = *(bool_t*)Data;
				RegSetValueEx(Key, DisableDevice, 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
				RegCloseKey(Key);
				Result = ERR_NONE;
			}
		}
		break;

	case AHI_STRETCHFLIP: SETVALUE(p->AllowStretchFlip,bool_t,ERR_NONE); break;
	case AHI_LOWBITDEPTH: SETVALUE(p->AllowLowBitdepth,bool_t,ERR_NONE); break;
	case AHI_PRIMARYREUSE: SETVALUE(p->AllowPrimaryReUse,bool_t,ERR_NONE); break;
	case AHI_NORELEASE: SETVALUE(p->NoRelease,bool_t,UpdateNoRelease(p)); break; // no compare
	case AHI_IDCTBUG: SETVALUE(p->IDCTBug,bool_t,IDCTUpdateFunc(p)); break;
	}

	return Result;
}

static int GetFreeMemory(ahi* p,int Mem)
{
	int a=0;
	int b;
	if (p->AhiSurfGetLargestFreeBlockSize)
	{
		if (p->Context)
			p->AhiSurfGetLargestFreeBlockSize(p->Context,FORMAT_YUV420,&a,&b,Mem);
		else
		if (SafeAhiInit(p))
		{
			int No;
			for (No=0;;No++)
			{
				int Result = p->AhiDevEnum(&p->Device,&p->DevInfo,No);
				if (Result == SUCCESS && CheckVersion(p,0))
				{
					if (p->AhiDevOpen(&p->Context,p->Device,DevOpenName,2)==SUCCESS)
					{
						p->AhiDevClientVersion(p->Context,CLIENTVERSION);
						p->AhiSurfGetLargestFreeBlockSize(p->Context,FORMAT_YUV420,&a,&b,Mem);
						p->AhiDevClose(p->Context);
					}
					p->Context = NULL;
					break;
				}
				if (Result == ENDENUM)
					break;		
			}

			SafeAhiTerm(p);
		}

		a = (a * 3) >> 1; // 12bit/pixel
	}
	return a;
}

static int Get(ahi* p,int No,void* Data,int Size)
{
	int Result = OverlayGet(&p->p,No,Data,Size);
	switch (No)
	{
	case AHI_NAME: 
		StrToTcs((tchar_t*)Data,Size/sizeof(tchar_t),p->DevInfo.Name);
		Result = ERR_NONE;
		break;

	case AHI_VERSION: 
		StrToTcs((tchar_t*)Data,Size/sizeof(tchar_t),p->DevInfo.Version);
		Result = ERR_NONE;
		break;

	case AHI_DEVICEBITMAP:
		if (Size==sizeof(bool_t))
		{
			HKEY Key;

			Result = ERR_NONE;
			*(bool_t*)Data = 0;

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ATIBase, 0, 0, &Key) == ERROR_SUCCESS)
			{
				DWORD Value;
				DWORD RegSize = sizeof(Value);
				DWORD RegType;

				if (RegQueryValueEx(Key, DisableDevice, 0, &RegType, (LPBYTE)&Value, &RegSize) == ERROR_SUCCESS && 
					RegType == REG_DWORD && Value!=0)
					*(bool_t*)Data = 1;

				RegCloseKey(Key);
			}
		}
		break;

	case AHI_STRETCHFLIP: GETVALUE(p->AllowStretchFlip,bool_t); break;
	case AHI_LOWBITDEPTH: GETVALUE(p->AllowLowBitdepth,bool_t); break;
	case AHI_PRIMARYREUSE: GETVALUE(p->AllowPrimaryReUse,bool_t); break;
	case AHI_NORELEASE: GETVALUE(p->NoRelease,bool_t); break;
	case AHI_IDCTBUG: GETVALUE(p->IDCTBug,bool_t); break;
	case AHI_REVISION: GETVALUE(p->DevInfo.Revision,int); break;
	case AHI_CHIPID: GETVALUE(p->DevInfo.ChipId,int); break;
	case AHI_REVISIONID: GETVALUE(p->DevInfo.RevisionId,int); break;
	case AHI_TOTALMEMORY: GETVALUE(p->DevInfo.TotalMemory,int); break;
	case AHI_INTERNALTOTALMEM: GETVALUE(p->DevInfo.InternalMemSize,int); break;
	case AHI_EXTERNALTOTALMEM: GETVALUE(p->DevInfo.ExternalMemSize,int); break;
	case AHI_INTERNALFREEMEM: GETVALUE(GetFreeMemory(p,MEM_INTERNAL),int); break;
	case AHI_EXTERNALFREEMEM: GETVALUE(GetFreeMemory(p,MEM_EXTERNAL),int); break;
	case AHI_CAPS1: GETVALUE(p->DevInfo.Caps1,int); break;
	case AHI_CAPS2: GETVALUE(p->DevInfo.Caps2,int); break;
	case AHI_CAPS3: GETVALUE(p->DevInfo.Caps3,int); break;
	case AHI_CAPS4: GETVALUE(p->DevInfo.Caps4,int); break;
	case AHI_UNMAP: GETVALUE((int)p->AhiUnmap,int); break;
	}
	return Result;
}

static int GetPitch(ahi* p,ahisurface* Surf)
{
	planeinfo Info;
	surfinfo SInfo;

	if (p->OvlPlaneY)
	{
		p->AhiSurfPlaneInfo(p->Context,Surf,&Info,p->OvlPlaneY);
		return Info.Pitch;
	}
	else
	{
		p->AhiSurfInfo(p->Context,Surf,&SInfo);
		return SInfo.Pitch;
	}
}

static bool_t AllocBuffer(ahi* p,int No,int Width,int Height,int Mode)
{
	int FirstMode;
	int SecondMode;
	ahisurface* Surf = NULL;
	ahisurface* SurfR = NULL;
	int FinalMode = 0;

	int Size[2];
	Size[0] = Width;
	Size[1] = Height;

	if (Mode == MEM_ANY_CHG)
		Mode = (No & 1) ? MEM_ANY_INT : MEM_ANY_EXT;

	if (Mode == MEM_ANY_INT)
	{
		FirstMode = MEM_INTERNAL;
		SecondMode = MEM_EXTERNAL;
	}
	else
	if (Mode == MEM_ANY_EXT)
	{
		FirstMode = MEM_EXTERNAL;
		SecondMode = MEM_INTERNAL;
	}
	else
	{
		FirstMode = Mode;
		SecondMode = 0;
	}

	p->AhiSurfAlloc(p->Context,&Surf,Size,p->OvlFormat,FirstMode);
	if (Surf)
		FinalMode = FirstMode;

	if (!Surf && SecondMode)
	{
		p->AhiSurfAlloc(p->Context,&Surf,Size,p->OvlFormat,SecondMode);
		if (Surf)
			FinalMode = SecondMode;
	}

	if (!Surf && p->FullScreenOverlay && !p->Landscape && !p->PrimaryReUse && p->AllowPrimaryReUse)
	{
		ahisurface* Primary = NULL;
		p->AhiDispSurfGet(p->Context,&Primary);
		if (Primary)
		{
			p->AhiSurfReuse(p->Context,&Surf,Primary,Size,p->OvlFormat,0);
			if (Surf)
			{
				FinalMode = MEM_PRIMARY;
				p->PrimaryReUse = 1;
				WaitDisable(1);
			}
		}
	}

	DEBUG_MSG4(DEBUG_VIDEO,T("AHIALLOC %d: %d,%d %d"),No,Size[0],Size[1],FinalMode);

	if (Surf)
	{
		void* Ptr;

		if (No < SCALE)
		{
			p->Overlay.Pitch = GetPitch(p,Surf);
			Size[0] = Height;
			Size[1] = Width;

			p->AhiSurfReuse(p->Context,&SurfR,Surf,Size,p->OvlFormat,0);

			if (!SurfR)
			{
				// this shouldn't happen
				p->AhiSurfFree(p->Context,Surf);
				return 0;
			}

			p->OverlayRPitch = GetPitch(p,SurfR);
		}

		p->Buf[No] = Surf;
		p->BufR[No] = SurfR;
		p->BufMode[No] = FinalMode;
		p->BufFrameNo[No] = -1;

		if (p->OvlPlaneY)
		{
			planeinfo Info;
			p->AhiSurfPlaneInfo(p->Context,Surf,&Info,p->OvlPlaneY);
			if (p->AhiSurfLock(p->Context,Surf,&Ptr,p->OvlPlaneY)==SUCCESS)
			{
				FillColor(Ptr,Info.Pitch,0,0,Info.Width,Info.Height,8,0);
				p->AhiSurfUnlock(p->Context,Surf,p->OvlPlaneY);
			}
			p->AhiSurfPlaneInfo(p->Context,Surf,&Info,p->OvlPlaneU);
			if (p->AhiSurfLock(p->Context,Surf,&Ptr,p->OvlPlaneU)==SUCCESS)
			{
				FillColor(Ptr,Info.Pitch,0,0,Info.Width,Info.Height,8,128);
				p->AhiSurfUnlock(p->Context,Surf,p->OvlPlaneU);
			}
			p->AhiSurfPlaneInfo(p->Context,Surf,&Info,p->OvlPlaneV);
			if (p->AhiSurfLock(p->Context,Surf,&Ptr,p->OvlPlaneV)==SUCCESS)
			{
				FillColor(Ptr,Info.Pitch,0,0,Info.Width,Info.Height,8,128);
				p->AhiSurfUnlock(p->Context,Surf,p->OvlPlaneV);
			}
		}
		else
		{
			surfinfo SInfo;
			p->AhiSurfInfo(p->Context,Surf,&SInfo);
			if (p->AhiSurfLock(p->Context,Surf,&Ptr,0)==SUCCESS)
			{
				FillColor(Ptr,SInfo.Pitch,0,0,SInfo.Width,SInfo.Height,16,0);
				p->AhiSurfUnlock(p->Context,Surf,0);
			}
		}
		return 1;
	}
	return 0;
}

static void UpdateOverlaySurf(ahi* p,bool_t Idle)
{
	int Show = p->ShowCurr;
	if (p->Buf[SCALE])
	{
		Show = SCALE + p->ScaleCurr;
		Idle = 1; // is this needed?
	}
	if (p->Buf[Show])
	{
		if (Idle) p->AhiDrawIdle(p->Context,0);
		p->AhiDispOverlaySurf(p->Context,p->Buf[Show],&p->Param,0);
		//DEBUG_MSG1(DEBUG_VIDEO,T("ATI %08x"),p->Buf[Show]);
	}
}

static void Stretch(ahi* p,bool_t Update,bool_t Flip)
{
	ahisurface* Src;
	ahisurface*	Dst;
	ahirect DstRect;
	ahirect SrcRect;
	int ScaleMode = (p->p.FX.Flags & (BLITFX_ARITHSTRETCH50|BLITFX_ARITHSTRETCHALWAYS)) ? 5:1;

	if (p->Buf[SCALE+1] && Flip)
		p->ScaleCurr ^= 1;

	Src = p->Buf[p->ShowCurr];
	Dst = p->Buf[SCALE+p->ScaleCurr];

	DstRect.Left = 0;
	DstRect.Top = 0;
	DstRect.Right = p->ScaleWidth >> p->OvlUVX2;
	DstRect.Bottom = p->ScaleHeight >> p->OvlUVY2;

	SrcRect.Left = p->OverlayRect.x >> p->OvlUVX2;
	SrcRect.Top = p->OverlayRect.y >> p->OvlUVY2;
	SrcRect.Right = (p->OverlayRect.x + p->OverlayRect.Width) >> p->OvlUVX2;
	SrcRect.Bottom = (p->OverlayRect.y + p->OverlayRect.Height) >> p->OvlUVY2;

	if (p->OvlPlaneU)
	{
		if (p->Shrink) p->AhiDrawIdle(p->Context,0);
		p->AhiDrawSurfSrcSet(p->Context,Src,p->OvlPlaneU);
		p->AhiDrawSurfDstSet(p->Context,Dst,p->OvlPlaneU);
		if (p->Stretch)
			p->AhiDrawStretchBlt(p->Context,&DstRect,&SrcRect,ScaleMode);
		else
			p->AhiDrawBitBlt(p->Context,&DstRect,(ahipoint*)&SrcRect.Left);
	}

	if (p->OvlPlaneV)
	{
		if (p->Shrink) p->AhiDrawIdle(p->Context,0);
		p->AhiDrawSurfSrcSet(p->Context,Src,p->OvlPlaneV);
		p->AhiDrawSurfDstSet(p->Context,Dst,p->OvlPlaneV);
		if (p->Stretch)
			p->AhiDrawStretchBlt(p->Context,&DstRect,&SrcRect,ScaleMode);
		else
			p->AhiDrawBitBlt(p->Context,&DstRect,(ahipoint*)&SrcRect.Left);
	}

	DstRect.Left <<= p->OvlUVX2;
	DstRect.Top <<= p->OvlUVY2;
	DstRect.Right <<= p->OvlUVX2;
	DstRect.Bottom <<= p->OvlUVY2;

	SrcRect.Left <<= p->OvlUVX2;
	SrcRect.Top <<= p->OvlUVY2;
	SrcRect.Right <<= p->OvlUVX2;
	SrcRect.Bottom <<= p->OvlUVY2;

	if (p->Shrink) p->AhiDrawIdle(p->Context,0);
	p->AhiDrawSurfSrcSet(p->Context,Src,p->OvlPlaneY);
	p->AhiDrawSurfDstSet(p->Context,Dst,p->OvlPlaneY);
	if (p->Stretch)
		p->AhiDrawStretchBlt(p->Context,&DstRect,&SrcRect,ScaleMode);
	else
		p->AhiDrawBitBlt(p->Context,&DstRect,(ahipoint*)&SrcRect.Left);

	if (p->Buf[SCALE+1] && Update && Flip)
		UpdateOverlaySurf(p,1);
}	
	
static void SwapPtr(ahi* p,int a,int b,int SwapShow)
{
	ahisurface* t;
	
	t = p->Buf[a];
	p->Buf[a] = p->Buf[b];
	p->Buf[b] = t;

	t = p->BufR[a];
	p->BufR[a] = p->BufR[b];
	p->BufR[b] = t;

	SwapInt(&p->BufMode[a],&p->BufMode[b]);

	if (SwapShow)
	{
		SwapInt(&p->BufFrameNo[a],&p->BufFrameNo[b]);
	
		if (p->ShowNext == a)
			p->ShowNext = b;
		else
		if (p->ShowNext == b)
			p->ShowNext = a;

		if (p->ShowLast == a)
			p->ShowLast = b;
		else
		if (p->ShowLast == b)
			p->ShowLast = a;

		if (p->ShowCurr == a)
			p->ShowCurr = b;
		else
		if (p->ShowCurr == b)
			p->ShowCurr = a;
	}
}

static void MoveBuf(ahi* p,int Src,int Dst)
{
	ahirect DstRect;
	ahipoint SrcPos;

	DstRect.Left = DstRect.Top = 0;
	DstRect.Right = p->Overlay.Width;
	DstRect.Bottom = p->Overlay.Height;
	SrcPos.x = SrcPos.y = 0;

	p->AhiDrawSurfSrcSet(p->Context,p->Buf[Src],p->OvlPlaneY);
	p->AhiDrawSurfDstSet(p->Context,p->Buf[Dst],p->OvlPlaneY);
	p->AhiDrawBitBlt(p->Context,&DstRect,&SrcPos);

	DstRect.Right >>= p->OvlUVX2;
	DstRect.Bottom >>= p->OvlUVY2;

	if (p->OvlPlaneU)
	{
		p->AhiDrawSurfSrcSet(p->Context,p->Buf[Src],p->OvlPlaneU);
		p->AhiDrawSurfDstSet(p->Context,p->Buf[Dst],p->OvlPlaneU);
		p->AhiDrawBitBlt(p->Context,&DstRect,&SrcPos);
	}
	if (p->OvlPlaneV)
	{
		p->AhiDrawSurfSrcSet(p->Context,p->Buf[Src],p->OvlPlaneV);
		p->AhiDrawSurfDstSet(p->Context,p->Buf[Dst],p->OvlPlaneV);
		p->AhiDrawBitBlt(p->Context,&DstRect,&SrcPos);
	}
}

static void SwapBuf(ahi* p,int a,int b)
{
	planes s,Tmp;

	if (SurfaceAlloc(Tmp,&p->Overlay)==ERR_NONE)
	{
		if (Lock(p,a,s,0)==ERR_NONE)
		{
			SurfaceCopy(&p->Overlay,&p->Overlay,s,Tmp,NULL); // save from a
			Unlock(p,a);

			MoveBuf(p,b,a); // move b->a

			if (Lock(p,b,s,0)==ERR_NONE)
			{
				SurfaceCopy(&p->Overlay,&p->Overlay,Tmp,s,NULL); // restore to b
				Unlock(p,b);
			}

			SwapPtr(p,a,b,0); // swap pointers
		}
		SurfaceFree(Tmp);
	}
}

static void Optimize(ahi* p)
{
	if (p->Buf[SCALE] && p->BufCount==3 && 
		!(p->BufMode[0] == p->BufMode[1] && p->BufMode[1] == p->BufMode[2]))
	{
		// we have one buffer in different pool
		// b-frame motion compensation is faster if the two reference frames
		// are in different pool as the b-frame

		if (p->BufMode[1] == p->BufMode[2])
			SwapBuf(p,0,2);
		else
		if (p->BufMode[0] == p->BufMode[2])
			SwapBuf(p,1,2);
	}
}

static void UpdateBrightness(ahi* p,int Brightness)
{
	int Delta,No;
	int DCAdd = Brightness * DCFACTOR;

	if (p->DCAdd != DCAdd)
	{
		Delta = DCAdd - p->DCAdd;
		p->DCAdd = DCAdd;
		
		if (p->IDCT)
		{
			planes s,Tmp;
			int ValidTmp = 0;
			int SwapNo = p->TempCount ? p->BufCount : 0;
			int MacroNo;
			int MacroCount = (p->Overlay.Width >> 4)*(p->Overlay.Height >> 4);

			// we need to save one buffer and adjust manually (if there are no temp buffers)
			if (p->BufCount && !p->TempCount && Lock(p,SwapNo,s,0)==ERR_NONE)
			{
				if (SurfaceAlloc(Tmp,&p->Overlay)==ERR_NONE)
				{
					ValidTmp = 1;
					SurfaceCopy(&p->Overlay,&p->Overlay,s,Tmp,NULL);
				}
				Unlock(p,SwapNo);
			}

			*(int32_t*)&p->Data[0] = -1;
			p->Data[0].skip0 = 0;
			p->Data[0].data0 = (idct_block_t)Delta;

			for (No=p->TempCount ? 0:1;No<p->BufCount;++No)
			{
				// swap context
				p->AhiIDCTFrameStart(p->IDCT,p->Buf[SwapNo],p->Buf[No],NULL,p->FrameNo,0);
				for (MacroNo=0;MacroNo<MacroCount;++MacroNo)
				{
					p->AhiIDCTMComp8x8(p->IDCT,p->Data,1,&p->Zero,NULL,MacroNo,1);
					p->AhiIDCTMComp8x8(p->IDCT,p->Data,1,&p->Zero,NULL,MacroNo,1);
					p->AhiIDCTMComp8x8(p->IDCT,p->Data,1,&p->Zero,NULL,MacroNo,1);
					p->AhiIDCTMComp8x8(p->IDCT,p->Data,1,&p->Zero,NULL,MacroNo,1);
					p->AhiIDCTMComp8x8(p->IDCT,NULL,0,&p->Zero,NULL,MacroNo,0);
					p->AhiIDCTMComp8x8(p->IDCT,NULL,0,&p->Zero,NULL,MacroNo,0);
				}
				p->AhiIDCTFrameEnd(p->IDCT,p->FrameNo++);

				// swap pointer
				SwapPtr(p,No,SwapNo,0);
			}
			
			if (ValidTmp)
			{
				if (Lock(p,SwapNo,s,0)==ERR_NONE)
				{
					blitfx FX;
					memset(&FX,0,sizeof(FX));
					FX.ScaleX = SCALE_ONE;
					FX.ScaleY = SCALE_ONE;
					FX.Brightness = Delta / DCFACTOR;
					SurfaceCopy(&p->Overlay,&p->Overlay,Tmp,s,&FX);
					Unlock(p,SwapNo);
				}
				SurfaceFree(Tmp);
			}
		}
	}
}

static void FreePrimaryReUse(ahi* p);

static void UpdateDirection(ahi* p,int Dir)
{
	int No;
	if (p->Overlay.Direction != Dir)
	{
		planes s,Tmp;
		video TmpSurface;
		int ValidTmp = 0;
		int SwapNo;

		// hide overlay
		if (p->p.Show && p->AllCount)
		{
			if (p->PrimaryReUse)
				FreePrimaryReUse(p);
			p->AhiDispOverlayState(p->Context,0,0);
			p->AhiDispWaitVBlank(p->Context,0);
		}

		SwapNo = p->TempCount ? p->BufCount : 0;

		// we need to save one buffer and swap manually (if there are no temp buffers)
		if (p->BufCount && !p->TempCount && Lock(p,SwapNo,s,0)==ERR_NONE)
		{
			if (SurfaceAlloc(Tmp,&p->Overlay)==ERR_NONE)
			{
				ValidTmp = 1;
				TmpSurface = p->Overlay;
				SurfaceCopy(&p->Overlay,&p->Overlay,s,Tmp,NULL);
			}
			Unlock(p,SwapNo);
		}

		p->Overlay.Direction = Dir;
		SwapInt(&p->Overlay.Width,&p->Overlay.Height);
		SwapInt(&p->Overlay.Pitch,&p->OverlayRPitch);
		IDCTUpdateFunc(p);

		// swap buffer pointers

		for (No=0;No<p->AllCount;++No)
		{
			ahisurface* t = p->Buf[No];
			p->Buf[No] = p->BufR[No];
			p->BufR[No] = t;
		}

		// swapxy content of buffers

		for (No=p->TempCount ? 0:1;No<p->BufCount;++No)
		{
			ahirect DstRect;
			ahipoint SrcPos;

			DstRect.Left = DstRect.Top = 0;
			DstRect.Right = p->Overlay.Width;
			DstRect.Bottom = p->Overlay.Height;
			SrcPos.x = SrcPos.y = 0;

			p->AhiDrawSurfSrcSet(p->Context,p->BufR[No],p->OvlPlaneY);
			p->AhiDrawSurfDstSet(p->Context,p->Buf[SwapNo],p->OvlPlaneY);
			p->AhiDrawRotateBlt(p->Context,&DstRect,&SrcPos,1,1,0);

			DstRect.Right >>= p->OvlUVX2;
			DstRect.Bottom >>= p->OvlUVY2;

			if (p->OvlPlaneU)
			{
				p->AhiDrawSurfSrcSet(p->Context,p->BufR[No],p->OvlPlaneU);
				p->AhiDrawSurfDstSet(p->Context,p->Buf[SwapNo],p->OvlPlaneU);
				p->AhiDrawRotateBlt(p->Context,&DstRect,&SrcPos,1,1,0);
			}
			if (p->OvlPlaneV)
			{
				p->AhiDrawSurfSrcSet(p->Context,p->BufR[No],p->OvlPlaneV);
				p->AhiDrawSurfDstSet(p->Context,p->Buf[SwapNo],p->OvlPlaneV);
				p->AhiDrawRotateBlt(p->Context,&DstRect,&SrcPos,1,1,0);
			}

			SwapPtr(p,No,SwapNo,0);
		}
		
		if (ValidTmp)
		{
			if (Lock(p,SwapNo,s,0)==ERR_NONE)
			{
				SurfaceRotate(&TmpSurface,&p->Overlay,Tmp,s,DIR_SWAPXY);
				Unlock(p,SwapNo);
			}
			SurfaceFree(Tmp);
		}

		if (p->ShowNext >= 0)
			p->ShowCurr = p->ShowNext;
		if (p->ShowCurr<0 || p->ShowCurr>=p->BufCount)
			p->ShowCurr = 0;
	}
}

static void UpdateColorKey(ahi* p)
{
	int No;

	p->p.ColorKey = COLORKEY; // use colorkey by default
	if (p->p.FullScreenViewport)
		p->p.ColorKey = CRGB(0,0,0); // black colorkey because of lowbitdepth
	if (p->PrimaryReUse || p->OvlFormat != FORMAT_YUV420)
		p->p.ColorKey = RGB_NULL;

	if (p->Landscape)
	{
		// disable colorkey for external overlays in landscape mode
		if (p->Buf[SCALE])
		{
			if (p->BufMode[SCALE] == MEM_EXTERNAL)
				p->p.ColorKey = RGB_NULL;

			if (p->BufMode[SCALE+1] == MEM_EXTERNAL)
				p->p.ColorKey = RGB_NULL;
		}
		else
		{
			for (No=0;No<p->AllCount;++No)
				if (p->BufMode[No] == MEM_EXTERNAL)
				{
					p->p.ColorKey = RGB_NULL;
					break;
				}
		}
	}
}

static void FreePrimaryReUse(ahi* p)
{
	int No;
	if (p->BufMode[SCALE] == MEM_PRIMARY || p->BufMode[SCALE+1] == MEM_PRIMARY)
		FreeScale(p,NULL);

	for (No=0;No<p->AllCount;++No)
		if (p->BufMode[No] == MEM_PRIMARY)
		{
			if (No != p->AllCount-1)
			{
				MoveBuf(p,No,p->AllCount-1);
				SwapPtr(p,No,p->AllCount-1,0);
			}

			FreeBuffer(p,p->AllCount-1,NULL);
			p->AllCount--;
			p->TempCount = p->AllCount - p->BufCount;
			if (p->TempCount < 0)
			{
				p->BufCount = p->AllCount;
				p->TempCount = 0;
			}
		}

	ClearPrimary(p);
}

static void UpdateLowBitdepth(ahi* p)
{
	SetLowBitdepth(p,p->p.Show && p->p.FullScreenViewport && !p->BlitMode);
}

static void UpdateAlign(ahi* p,int ScaleMode)
{
	blitfx OvlFX = p->p.FX;
	bool_t FullScreenOverlay;
	rect FullScreen;
	rect OverlayRect;
	
	PhyToVirt(NULL,&FullScreen,&p->p.Output.Format.Video);

	VirtToPhy(&p->p.Viewport,&p->p.DstAlignedRect,&p->p.Output.Format.Video);
	VirtToPhy(NULL,&p->OverlayRect,&p->p.Input.Format.Video);

	if (OvlFX.Direction & DIR_SWAPXY)
	{
		SwapInt(&p->OverlayRect.Width,&p->OverlayRect.Height);
		SwapInt(&OvlFX.ScaleX,&OvlFX.ScaleY);
	}

	OvlFX.Direction &= ~DIR_SWAPXY;
	if (!ScaleMode ||
		(OvlFX.ScaleX == OvlFX.ScaleY &&
		 OvlFX.ScaleX >= (SCALE_ONE*98)/100 && OvlFX.ScaleX < (SCALE_ONE*103)/100))
		OvlFX.ScaleX = OvlFX.ScaleY = SCALE_ONE;

	p->Stretch = OvlFX.ScaleX != SCALE_ONE || OvlFX.ScaleY != SCALE_ONE;
	p->StretchOnlyInternal = (p->Stretch && ScaleMode==2);
	p->Shrink = OvlFX.ScaleX < SCALE_ONE || OvlFX.ScaleY < SCALE_ONE;

	if ((p->p.FX.Flags & (BLITFX_ARITHSTRETCH50|BLITFX_ARITHSTRETCHALWAYS)) && p->Stretch)
	{
		// avoid bilinear scale overrun (shrink source)
		if (p->OverlayRect.Width>2) 
		{
			OvlFX.ScaleX = Scale(OvlFX.ScaleX,p->OverlayRect.Width,p->OverlayRect.Width-2);
			p->OverlayRect.Width -= 2;
		}
		if (p->OverlayRect.Height>2) 
		{
			OvlFX.ScaleY = Scale(OvlFX.ScaleY,p->OverlayRect.Height,p->OverlayRect.Height-2);
			p->OverlayRect.Height -= 2;
		}
	}

	AnyAlign(&p->p.DstAlignedRect, &p->OverlayRect, &OvlFX, 2, 1, SCALE_ONE>>1,SCALE_ONE*16);

	PhyToVirt(&p->p.DstAlignedRect,&p->p.GUIAlignedRect,&p->p.Output.Format.Video);

	FullScreenOverlay = EqRect(&FullScreen,&p->p.GUIAlignedRect);

	if (FullScreenOverlay != p->FullScreenOverlay)
	{
		p->FullScreenOverlay = FullScreenOverlay;
		p->p.Overlay = !FullScreenOverlay;
		p->MaxCount = MAXIDCTBUF;
	}

	if (p->PrimaryReUse && !p->FullScreenOverlay)
		FreePrimaryReUse(p);

//	if (ScaleMode>0 && p->IDCT && !p->Internal && p->Stretch && OvlFX.ScaleX < (SCALE_ONE*13/10))
//	{
		// external idct buffer with external scale buffer would be very slow
		// not too much scale factor -> fall back to 100% if internal scale buffer not available
//		p->StretchOnlyInternal = 1;
//	}

	if (ScaleMode>0 && p->IDCT && p->Landscape && !p->Stretch && !p->FullScreenOverlay)
	{
		// using idct with external memory and 100% zoom in landscape mode
		// this means external overlay in landscape mode -> try internal scale buffer (100% scale)
		// (except when the overlay covers the full screen)
		p->Stretch = 1;
		p->StretchOnlyInternal = 1; // but don't use external memory (it would make no sense)
	}

	if (p->Stretch)
	{
		p->SoftFX.Flags &= ~(BLITFX_AVOIDTEARING|BLITFX_VMEMROTATED|BLITFX_VMEMUPDOWN);
	}
	else
	{
		// no BLITFX_VMEMROTATED needed because overlay is always in lcd space
		// (just the primary RGB surface is rotated)
		p->SoftFX.Flags |= BLITFX_AVOIDTEARING;
		if (p->p.FX.Direction & DIR_MIRRORUPDOWN)
			p->SoftFX.Flags |= BLITFX_VMEMUPDOWN;
	}

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);

	if (p->IDCT)
		p->p.Caps = VC_BRIGHTNESS;

	// because blit might use alignment enlarge the dest area
	OverlayRect = p->OverlayRect;
	OverlayRect.Width = (OverlayRect.Width + (OverlayRect.x & 15) + 15) & ~15;
	OverlayRect.Height = (OverlayRect.Height + (OverlayRect.y & 15) + 15) & ~15;
	OverlayRect.x &= ~15;
	OverlayRect.y &= ~15;

	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);
	BlitAlign(p->p.Soft, &OverlayRect, &p->p.SrcAlignedRect);
}

// build ahi overlay param
static void UpdateAHI(ahi* p)
{
	if (p->Buf[SCALE])
	{
		p->Param.SrcRect.Left = 0;
		p->Param.SrcRect.Top = 0;
	}
	else
	{
		p->Param.SrcRect.Left = p->OverlayRect.x;
		p->Param.SrcRect.Top = p->OverlayRect.y;
	}
	p->Param.SrcRect.Right = p->Param.SrcRect.Left + p->p.DstAlignedRect.Width;
	p->Param.SrcRect.Bottom = p->Param.SrcRect.Top + p->p.DstAlignedRect.Height;

	p->Param.DstRect.Left = p->p.DstAlignedRect.x;
	p->Param.DstRect.Top = p->p.DstAlignedRect.y;
	p->Param.DstRect.Right = p->p.DstAlignedRect.x + p->p.DstAlignedRect.Width;
	p->Param.DstRect.Bottom = p->p.DstAlignedRect.y + p->p.DstAlignedRect.Height;

	DEBUG_MSG4(DEBUG_VIDEO,T("UpdateAHI %d,%d,%d,%d"),
		p->Param.SrcRect.Left,
		p->Param.SrcRect.Top,
		p->Param.SrcRect.Right,
		p->Param.SrcRect.Bottom);

	p->Param.Mirror = 0;
	if (p->p.FX.Direction & DIR_MIRRORLEFTRIGHT)
		p->Param.Mirror |= MIRROR_LEFTRIGHT;
	if (p->p.FX.Direction & DIR_MIRRORUPDOWN)
		p->Param.Mirror |= MIRROR_UPDOWN;

	if (p->p.ColorKey == CRGB(0,0,0)) {
		// lowbitdepth
		p->Param.ColorMask = 0xF;
		p->Param.ColorKey = 0;
	}
	else
	if (p->p.ColorKey != RGB_NULL) {
		p->Param.ColorMask = 
			p->p.Output.Format.Video.Pixel.BitMask[0] | 
			p->p.Output.Format.Video.Pixel.BitMask[1] |
			p->p.Output.Format.Video.Pixel.BitMask[2];
		p->Param.ColorKey = RGBToFormat(p->p.ColorKey,&p->p.Output.Format.Video.Pixel);
	}
	else
	{
		p->Param.ColorMask = 0;
		p->Param.ColorKey = 0;
	}
}

static void UpdateAlloc(ahi* p,int BufCount,int AllCount,int ScaleMode,bool_t* Hidden)
{
	int No;
	int Mode;

	if (ScaleMode==2)
		Mode = MEM_EXTERNAL; // only accept external
	else
	if (p->IDCT)
		Mode = MEM_ANY_CHG;
	else
		Mode = MEM_ANY_INT; // prefer internal memory transfer

	if (!p->IDCT && p->Landscape && p->AllCount)
	{
		// try to reallocate buffer if it's not in the right memory

		if ((Mode == MEM_EXTERNAL && p->BufMode[0] != MEM_EXTERNAL) ||
			(Mode == MEM_ANY_INT && p->BufMode[0] != MEM_INTERNAL))
		{
			// free surfaces

			FreeScale(p,Hidden);
			for (No=p->AllCount-1;No>=0;--No)
				FreeBuffer(p,No,Hidden);
			p->AllCount = 0;
		}
	}

	if (AllCount != p->AllCount)
	{
		FreeScale(p,Hidden);

		if (AllCount < p->AllCount && p->PrimaryReUse)
			FreePrimaryReUse(p);

		//try to keep same number of internal and external buffers
		if (Mode == MEM_ANY_CHG)
		{
			int Ext = 0;
			int Int = 0;
			for (No=0;No<AllCount;++No)
			{
				if (p->BufMode[No] == MEM_EXTERNAL)
					Ext++;
				if (p->BufMode[No] == MEM_INTERNAL)
					Int++;
			}

			for (No=AllCount;No<p->AllCount;++No)
			{
				int Search = 0;
				if (p->BufMode[No] == MEM_EXTERNAL && Ext<Int)
					Search = MEM_INTERNAL;
				if (p->BufMode[No] == MEM_INTERNAL && Int<Ext)
					Search = MEM_EXTERNAL;

				if (Search)
				{
					int No2;
					for (No2=0;No2<AllCount;++No2)
						if (p->BufMode[No2] == Search)
						{
							MoveBuf(p,No2,No);
							SwapPtr(p,No2,No,0);
							if (Search == MEM_EXTERNAL)
							{
								Ext--;
								Int++;
							}
							else
							{
								Ext++;
								Int--;
							}
							break;
						}
				}
			}
		}

		//free unneccessary buffers
		for (No=p->AllCount-1;No>=AllCount;--No)
			FreeBuffer(p,No,Hidden);

		if (p->AllCount > AllCount)
			p->AllCount = AllCount;

		if (p->IDCT && p->Internal)
		{
			//allocate neccessary buffers in internal memory (if possible)
			for (No=p->AllCount;No<AllCount;++No)
			{
				if (!AllocBuffer(p,No,p->Overlay.Width,p->Overlay.Height,MEM_INTERNAL))
				{
					// failed? need use to external? (temp buffers doesn't count)
					if (p->AllCount < BufCount && p->DevInfo.ExternalMemSize) 
					{
						p->Internal = 0;

						// free all buffers, so they can be realloced as external
						for (No=0;No<p->AllCount;++No)
							FreeBuffer(p,No,Hidden);
						p->AllCount = 0;
					}
					break;
				}
				p->AllCount = No+1;
			}
		}

		//allocate neccessary buffers
		for (No=p->AllCount;No<AllCount;++No)
		{
			if (!AllocBuffer(p,No,p->Overlay.Width,p->Overlay.Height,Mode))
			{
				p->MaxCount = max(1,p->AllCount);
				break;
			}
			p->AllCount = No+1;
		}

		if (p->ShowCurr >= p->AllCount)
			p->ShowCurr = 0;
		if (p->ShowNext >= p->AllCount)
			p->ShowNext = -1;
		if (p->ShowLast >= p->AllCount)
			p->ShowLast = -1;
		p->BufCount = min(p->AllCount,BufCount);
		p->TempCount = p->AllCount - p->BufCount;
	}
}

static bool_t UpdateOverlay(ahi* p,int BufCount,int TempCount)
{
	int ScaleMode;
	bool_t Hidden = 0;
	int AllCount = BufCount + TempCount;
	if (AllCount>p->MaxCount) return 0;

	p->SoftFX.Flags = 0;
	p->SoftFX.Brightness = p->p.FX.Brightness;
	p->SoftFX.Contrast = p->p.FX.Contrast;
	p->SoftFX.Saturation = p->p.FX.Saturation;
	p->SoftFX.Direction = p->p.FX.Direction & DIR_SWAPXY;
	p->SoftFX.RGBAdjust[0] = p->p.FX.RGBAdjust[0];
	p->SoftFX.RGBAdjust[1] = p->p.FX.RGBAdjust[1];
	p->SoftFX.RGBAdjust[2] = p->p.FX.RGBAdjust[2];

	ScaleMode = 1;
	if (!p->IDCT && p->Landscape && (p->p.FX.ScaleX != SCALE_ONE || p->p.FX.ScaleY != SCALE_ONE))
		ScaleMode = 2; // first try normal=external scale=internal

scaletry:

	// alloc buffers
	UpdateAlloc(p,BufCount,AllCount,ScaleMode,&Hidden);

	// align
	UpdateAlign(p,ScaleMode);

	// allocate/free scale surface
	if (p->AllCount && p->Stretch)
	{
		if (p->ScaleWidth != p->p.DstAlignedRect.Width ||
			p->ScaleHeight != p->p.DstAlignedRect.Height)
		{
			int Mode;

			FreeScale(p,&Hidden);

			if (p->StretchOnlyInternal)
				Mode = MEM_INTERNAL; // only accept internal
			else
			if (p->BufMode[0] == MEM_INTERNAL) // try opposite of normal buffer
				Mode = MEM_ANY_EXT;  
			else
				Mode = MEM_ANY_INT;

			AllocBuffer(p,SCALE,p->p.DstAlignedRect.Width,p->p.DstAlignedRect.Height,Mode);
			if (p->AllowStretchFlip)
				AllocBuffer(p,SCALE+1,p->p.DstAlignedRect.Width,p->p.DstAlignedRect.Height,Mode);

			if (p->Buf[SCALE])
			{
				p->ScaleCurr = 0;
				p->ScaleWidth = p->p.DstAlignedRect.Width;
				p->ScaleHeight = p->p.DstAlignedRect.Height;
			
				Optimize(p);
				Stretch(p,0,0);
			}
		}
	}
	else
		FreeScale(p,&Hidden);

	if (p->Stretch && !p->Buf[SCALE] && p->AllCount)
	{
		--ScaleMode;
		goto scaletry; // no memory for scale buffer -> try again
	}

	UpdateColorKey(p);
	OverlayUpdateShow(&p->p,1);

	if (p->p.ColorKey != RGB_NULL)
		WinInvalidate(&p->p.Viewport,1);

	UpdateAHI(p);
	UpdateOverlaySurf(p,1);

	if ((p->p.Show || p->PrimaryReUse) && p->BufCount)
	{
		p->AhiDispOverlayState(p->Context,p->p.Show || p->PrimaryReUse,0);
		UpdateLowBitdepth(p);
	}

	if (!TempCount && p->AllCount != AllCount && p->BufCount<2 && !p->ErrorMemory)
	{
		p->ErrorMemory = 1;
		ShowError(AHI_ID,AHI_ID,AHI_OUT_OF_MEMORY);
	}

	return p->AllCount == AllCount;
}

static int UpdateBlit(ahi* p)
{
	blitfx OvlFX = p->p.FX;
	video Overlay;
	int BufWidth,BufHeight;

	p->p.ColorKey = RGB_NULL;

ReTry:

	p->SoftFX = p->p.FX;
	p->SoftFX.ScaleX = SCALE_ONE; // scale handled by hardware
	p->SoftFX.ScaleY = SCALE_ONE;
	OvlFX.Direction = 0;

	if (p->BlitDownScale)
	{
		p->SoftFX.ScaleX >>= 1;
		p->SoftFX.ScaleY >>= 1;
		OvlFX.ScaleX <<= 1;
		OvlFX.ScaleY <<= 1;
	}

	//align to 100%
	if (OvlFX.ScaleX > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleX < SCALE_ONE+(SCALE_ONE/16) &&
		OvlFX.ScaleY > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleY < SCALE_ONE+(SCALE_ONE/16))
	{
		OvlFX.ScaleX = SCALE_ONE;
		OvlFX.ScaleY = SCALE_ONE;
	}

	if (p->SoftFX.Direction & DIR_SWAPXY)
		SwapInt(&OvlFX.ScaleX,&OvlFX.ScaleY);

	p->Overlay.Direction = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->p.Output.Format.Video.Direction);
	Overlay = p->Overlay;
	Overlay.Width = p->p.Input.Format.Video.Width;
	Overlay.Height = p->p.Input.Format.Video.Height;
	if (p->BlitDownScale)
	{
		Overlay.Width >>= 1;
		Overlay.Height >>= 1;
	}
	if (Overlay.Direction & DIR_SWAPXY)
		SwapInt(&Overlay.Width,&Overlay.Height);

	VirtToPhy(&p->p.Viewport,&p->p.DstAlignedRect,&p->p.Output.Format.Video);
	VirtToPhy(NULL,&p->OverlayRect,&Overlay);

	AnyAlign(&p->p.DstAlignedRect, &p->OverlayRect, &OvlFX, 2, 1, SCALE_ONE>>1,SCALE_ONE*16);

	p->Stretch = (OvlFX.ScaleX != SCALE_ONE || OvlFX.ScaleY != SCALE_ONE) && 
			(p->p.DstAlignedRect.Width != p->OverlayRect.Width && p->p.DstAlignedRect.Height != p->OverlayRect.Height);

	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
	BlitAlign(p->p.Soft,&p->OverlayRect,&p->p.SrcAlignedRect);

	// pitch has to 16byte aligned, so to be able to rotate width we must align 8 pixels (16bit RGB)
	BufWidth = (p->OverlayRect.Width+7) & ~7;
	BufHeight = (p->OverlayRect.Height+7) & ~7;

	if (!p->Buf[0] || p->Overlay.Width != BufWidth || p->Overlay.Height != BufHeight)
	{
		FreeBuffer(p,0,NULL);
		p->Overlay.Width = BufWidth;
		p->Overlay.Height = BufHeight;
		if (!AllocBuffer(p,0,BufWidth,BufHeight,MEM_ANY_INT))
		{
			if (!p->BlitDownScale)
			{
				p->BlitDownScale = 1;
				goto ReTry;
			}
			return ERR_OUT_OF_MEMORY;
		}
	}

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
	BlitAlign(p->p.Soft,&p->OverlayRect,&p->p.SrcAlignedRect);

	if (!p->Stretch) // no scaling -> adjust dst rectangle with soft yuv blitting alignement
	{
		p->p.DstAlignedRect.x += (p->p.DstAlignedRect.Width - p->OverlayRect.Width) >> 1;
		p->p.DstAlignedRect.y += (p->p.DstAlignedRect.Height - p->OverlayRect.Height) >> 1;
		p->p.DstAlignedRect.Width = p->OverlayRect.Width;
		p->p.DstAlignedRect.Height = p->OverlayRect.Height;
	}

	PhyToVirt(&p->p.DstAlignedRect,&p->p.GUIAlignedRect,&p->p.Output.Format.Video);
	return ERR_NONE;
}

static int Reset(ahi* p)
{
	if (p->Context)
		GetMode(p);
	return ERR_NONE;
}

static int Orientation(void* This)
{
	ahi* p = (ahi*)This;
	if (!p->Context)
	{
		p->p.Output.Format.Video.Direction = -1;

		if (SafeAhiInit(p))
		{
			int No;
			for (No=0;;No++)
			{
				int Result = p->AhiDevEnum(&p->Device,&p->DevInfo,No);
				if (Result == SUCCESS && CheckVersion(p,0))
				{
					if (p->AhiDevOpen(&p->Context,p->Device,DevOpenName,2)==SUCCESS)
					{
						p->AhiDevClientVersion(p->Context,CLIENTVERSION);
						GetMode(p);
						p->AhiDevClose(p->Context);
					}
					p->Context = NULL;
					break;
				}
				if (Result == ENDENUM)
					break;		
			}

			SafeAhiTerm(p);
		}
	}

	return p->p.Output.Format.Video.Direction;
}

static int Blit(ahi* p, const constplanes Data, const constplanes DataLast);
static int BlitOverlay(ahi* p, const constplanes Data, const constplanes DataLast);
static int Update(ahi* p);
static int UpdateShow(ahi* p);

/*
static void CheckMem(const tchar_t* s)
{	
	MEMORYSTATUS Status;
	Status.dwLength = sizeof(Status);
	GlobalMemoryStatus(&Status); 
	DebugMessage(T("%s %d %d"),s,Status.dwAvailPhys,Status.dwAvailVirtual);
}
*/

static int Init(ahi* p)
{
	int No;
	int Result = ERR_DEVICE_ERROR;

	p->Device = NULL;
	p->Context = NULL;
	memset(&p->Param,0,sizeof(p->Param));

	for (No=0;No<MAXIDCTBUF+MAXSCALE;++No)
	{
		p->Buf[No] = NULL;
		p->BufR[No] = NULL;
		p->BufMode[No] = 0;
		p->BufFrameNo[No] = -1;
	}

	if (SafeAhiInit(p))
	{
		p->SoftFX = p->p.FX;
		p->SoftFX.ScaleX = SCALE_ONE;
		p->SoftFX.ScaleY = SCALE_ONE;
		p->SoftFX.Direction = 0;

		memset(&p->Overlay,0,sizeof(video));
		p->Overlay.Width = ALIGN16(p->IDCTInit ? p->IDCTWidth:p->p.Input.Format.Video.Width);
		p->Overlay.Height = ALIGN16(p->IDCTInit ? p->IDCTHeight:p->p.Input.Format.Video.Height);
		p->Overlay.Direction = p->IDCTInit ? 0:p->p.Input.Format.Video.Direction;
		p->Overlay.Aspect = ASPECT_ONE;
		p->BlitDownScale = 0;

		if (p->IDCTInit && !PlanarYUV420(&p->p.Input.Format.Video.Pixel))
		{
			SafeAhiTerm(p);
			return ERR_NOT_SUPPORTED;
		}

		p->ErrorMemory = p->Overlay.Width==0 || p->Overlay.Height==0;
		p->Landscape = 0;
		p->Internal = 1;
		p->ScaleWidth = 0;
		p->ScaleHeight = 0;
		p->ScaleCurr = 0;
		p->ShowCurr = 0;
		p->ShowNext = -1;
		p->ShowNextIdle = 0;
		p->ShowLast = -1;
		p->MaxCount = MAXIDCTBUF;
		p->AllCount = 0;
		p->BufCount = 0;
		p->TempCount = 0;
		p->Zero = 0;
		p->FrameNo = 0;
		p->LowBitdepth = 0;
		p->PrimaryReUse = 0;
		p->Primary = NULL;

		WaitDisable(0);

		for (No=0;;No++)
		{
			int AtiResult = p->AhiDevEnum(&p->Device,&p->DevInfo,No);
			if (AtiResult == SUCCESS)
			{
				if (!CheckVersion(p,p->IDCTInit))
				{
					ShowError(AHI_ID,AHI_ID,AHI_OLDVERSION,p->Version);
					p->Context = NULL;
					Result = ERR_NOT_SUPPORTED; // don't show device error message afterwards
					break;
				}

				AtiResult = FAIL;
				TRY_BEGIN
				AtiResult = p->AhiDevOpen(&p->Context,p->Device,DevOpenName,2);
				TRY_END;

				if (AtiResult != SUCCESS)
				{
					ShowError(AHI_ID,AHI_ID,AHI_OPEN_ERROR);
					p->Context = NULL;
					Result = ERR_NOT_SUPPORTED; // don't show device error message afterwards
					break;
				}

				p->AhiDevClientVersion(p->Context,CLIENTVERSION);

				p->AhiDrawRopSet(p->Context,0xCCCC);
				p->AhiDispOverlayState(p->Context,0,0);
				p->AhiDispSurfGet(p->Context,&p->Primary);

				GetMode(p);

				if (p->DevInfo.ExternalMemSize>0 &&
					PlanarYUV(&p->p.Input.Format.Video.Pixel,NULL,NULL,NULL))
				{
					// because of software rotation we can't use YUV422 
					// even if the source is in this format. so we are using
					// YUV420 with all YUV planar formats

					p->Overlay.Pixel.Flags = PF_YUV420;
					p->p.Overlay = 1;
					p->BlitMode = 0;
					p->OvlFormat = FORMAT_YUV420;
					p->OvlUVX2 = 1;
					p->OvlUVY2 = 1;
					p->OvlPlaneY = PLANE_Y;
					p->OvlPlaneU = PLANE_U;
					p->OvlPlaneV = PLANE_V;
					p->p.Blit = (ovlblit)BlitOverlay;
					p->p.Update = (ovlfunc)Update;
					p->p.UpdateShow = (ovlfunc)UpdateShow;
				}
				else
				{
					if (p->IDCTInit) // this shouldn't happen
					{
						p->AhiDevClose(p->Context);
						p->Context = NULL;
						break;
					}

					DefaultRGB(&p->Overlay.Pixel,16,5,6,5,0,0,0);
					p->p.Overlay = 0;
					p->BlitMode = 1;
					p->OvlFormat = FORMAT_RGB565;
					p->OvlUVX2 = 0;
					p->OvlUVY2 = 0;
					p->OvlPlaneY = 0;
					p->OvlPlaneU = 0;
					p->OvlPlaneV = 0;
					p->p.Blit = (ovlblit)Blit;
					p->p.Update = (ovlfunc)UpdateBlit;
					p->p.UpdateShow = (ovlfunc)NULL;
				}

				// landscape mode doesn't like external memory overlays, 
				// probably reading the external primary framebuffer conflicts with overlay readings

				if ((p->ModeInfo.Rotation!=0 && p->ModeInfo.Rotation!=2) && p->DevInfo.ExternalMemSize>0)
				{	
					p->Landscape = 1;
					p->Internal = 0;
				}

				if (p->IDCTInit)
				{
					p->AhiIDCTOpen(p->Context,&p->IDCT,0);

					if (!p->IDCT)
					{
						p->AhiDevClose(p->Context);
						p->Context = NULL;
						ShowError(AHI_ID,AHI_ID,AHI_OPEN_ERROR);
						Result = ERR_NOT_SUPPORTED; // don't show device error message afterwards
						break;
					}

					p->IDCTRounding = 1;
					IDCTUpdateFunc(p);
					p->AhiIDCTPropSet(p->IDCT,PROP_ROUNDING,&p->IDCTRounding,sizeof(int));
					p->AhiIDCTPropSet(p->IDCT,PROP_BIAS,&p->Zero,sizeof(int));

				}

				Result = ERR_NONE;
				break;
			}
			if (AtiResult == ENDENUM)
				break;
		}

		if (!p->Context)
			SafeAhiTerm(p);
	}

	return Result;
}

static void Done(ahi* p)
{
	int No;

	if (p->Context && p->IDCT)
	{
		p->AhiIDCTClose(p->IDCT);
		p->IDCT = NULL;
	}

	FreeScale(p,NULL);
	for (No=MAXIDCTBUF-1;No>=0;--No)
		FreeBuffer(p,No,NULL);
	p->TempCount = p->AllCount = p->BufCount = 0;

	if (p->Context)
	{
		SetLowBitdepth(p,0);
		p->AhiDispOverlayState(p->Context,0,0);
		p->AhiDevClose(p->Context);
		SafeAhiTerm(p);
		p->Context = NULL;
	}
}

static int Update(ahi* p)
{	
	UpdateDirection(p,(p->p.FX.Direction ^ p->p.Input.Format.Video.Direction) & DIR_SWAPXY);
	UpdateBrightness(p,p->p.FX.Brightness);
	if (!UpdateOverlay(p,p->IDCT?p->BufCount:1,0)) // temp buffers are freed!
		return ERR_OUT_OF_MEMORY;
	return ERR_NONE;
}

static int UpdateShow(ahi* p)
{
	UpdateLowBitdepth(p);
	p->AhiDispOverlayState(p->Context,p->p.Show,0);
	return ERR_NONE;
}

static int Blit(ahi* p, const constplanes Data, const constplanes DataLast)
{
	ahisurface* Src = p->Buf[0];
	ahisurface* Dst = p->Primary;
	ahirect DstRect;
	ahirect SrcRect;
	int ScaleMode = (p->p.FX.Flags & (BLITFX_ARITHSTRETCH50|BLITFX_ARITHSTRETCHALWAYS)) ? 5:1;
	planes Planes;

	p->AhiDrawIdle(p->Context,0);
	if (Src && Dst && p->AhiSurfLock(p->Context,Src,&Planes[0],0)==SUCCESS)
	{
		BlitImage(p->p.Soft,Planes,Data,DataLast,-1,-1);
		p->AhiSurfUnlock(p->Context,Src,0);

		DstRect.Left = p->p.DstAlignedRect.x;
		DstRect.Top = p->p.DstAlignedRect.y;
		DstRect.Right = p->p.DstAlignedRect.x + p->p.DstAlignedRect.Width;
		DstRect.Bottom = p->p.DstAlignedRect.y + p->p.DstAlignedRect.Height;

		SrcRect.Left = p->OverlayRect.x;
		SrcRect.Top = p->OverlayRect.y;
		SrcRect.Right = p->OverlayRect.x + p->OverlayRect.Width;
		SrcRect.Bottom = p->OverlayRect.y + p->OverlayRect.Height;

		p->AhiDrawSurfSrcSet(p->Context,Src,0);
		p->AhiDrawSurfDstSet(p->Context,Dst,0);
		if (p->Stretch)
			p->AhiDrawStretchBlt(p->Context,&DstRect,&SrcRect,ScaleMode);
		else
			p->AhiDrawBitBlt(p->Context,&DstRect,(ahipoint*)&SrcRect.Left);
	}
	return ERR_NONE;
}

static int BlitOverlay(ahi* p, const constplanes Data, const constplanes DataLast)
{
	planes Planes;
	int Result;
	
	Result = Lock(p,p->ShowCurr,Planes,0);
	if (Result == ERR_NONE)
	{
		BlitImage(p->p.Soft,Planes,Data,DataLast,-1,-1);

		Unlock(p,p->ShowCurr);

		if (p->Buf[SCALE])
			Stretch(p,1,0);
	}
	return Result;
}

//**********************
//* IDCT               *
//**********************

const uint8_t ScanTable[3][64] = {
{
	 0,  1,  8, 16,  9,  2,  3, 10, 
	17,	24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 
	27,	20, 13,  6,  7, 14, 21, 28, 
	35, 42,	49, 56, 57, 50, 43, 36, 
	29, 22,	15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61,	54, 47, 55, 62, 63
},
{
 	 0,  1,  2,  3,  8,  9, 16, 17, 
	10, 11,  4,  5,  6,  7, 15, 14,
	13, 12, 19, 18, 24, 25, 32, 33, 
	26, 27, 20, 21, 22, 23, 28, 29,
	30, 31, 34, 35, 40, 41, 48, 49, 
	42, 43, 36, 37, 38, 39, 44, 45,
	46, 47, 50, 51, 56, 57, 58, 59, 
	52, 53, 54, 55, 60, 61, 62, 63
},
{
	 0,  8, 16, 24,  1,  9,  2, 10, 
	17, 25, 32, 40, 48, 56, 57, 49,
	41, 33, 26, 18,  3, 11,  4, 12, 
	19, 27, 34, 42, 50, 58, 35, 43,
	51, 59, 20, 28,  5, 13,  6, 14, 
	21, 29, 36, 44, 52, 60, 37, 45,
	53, 61, 22, 30,  7, 15, 23, 31, 
	38, 46, 54, 62, 39, 47, 55, 63
}};
const uint8_t ZigZagSwap[64] = {
	 0,  8,  1,  2,  9, 16, 24, 17, 
	10,	 3,  4, 11, 18, 25, 32, 40,
	33, 26, 19, 12,  5,  6, 13, 20, 
	27,	34, 41, 48, 56, 49, 42, 35, 
	28, 21,	14,  7, 15, 22, 29, 36, 
	43, 50,	57, 58, 51, 44, 37, 30, 
	23, 31, 38, 45, 52, 59, 60, 53, 
	46, 39, 47,	54, 61, 62, 55, 63
};
const uint8_t ZigZagSwapLength[64] = {
	1,   3,  3,  6,  6,  6, 10,	10,	
	10, 10, 15, 15, 15, 15, 15,	21, 
	21, 21, 21, 21, 21, 28, 28, 28,	
	28, 28, 28, 28, 36, 36, 36, 36, 
	36,	36, 36, 36, 43, 43, 43, 43, 
	43,	43, 43, 49, 49, 49, 49, 49,
	49, 54, 54, 54, 54, 54, 58, 58, 
	58, 58,	61, 61, 61, 63, 63, 64
};

#define AHI(p) ((ahi*)((char*)(p)-(int)&(((ahi*)0)->IdctVMT)))

// input: Block, Length
#define AHIDCBLOCK(Offset)					\
{											\
	if (Length < 1)							\
	{										\
		Block[0] = (idct_block_t)(Offset);	\
		Length = 1;							\
	}										\
	else									\
		Block[0] = (idct_block_t)(Block[0] + (Offset));\
}

#define AHIMVINC							\
	if (AHI(p)->MVBack) AHI(p)->MVBack++;	\
	if (AHI(p)->MVFwd) AHI(p)->MVFwd++;		
	
// ADJUST: don't care about MVFwd (only in b-frames which are not accumulated)
#define AHIMVADJBLOCK										\
	if (AHI(p)->MVBack && (AHI(p)->MVBack[0] & 0x00020002)==0x00020002)	\
		AHIDCBLOCK(2);						

// Block, Length -> Dst, Count
#define AHIPACK(Dst)										\
	ScanEnd = Scan + Length;								\
	for (Data=Dst;;++Data)									\
	{														\
		int v;												\
		*(int32_t*)Data = -1;								\
															\
		Last = Scan;										\
		while (Scan != ScanEnd && !Block[*Scan]) ++Scan;	\
		if (Scan == ScanEnd) break;							\
		v = *Scan;											\
		Data->skip0 = (char)(Scan++ - Last);				\
		Data->data0 = Block[v];								\
															\
		Count++;											\
		Last = Scan;										\
		while (Scan != ScanEnd && !Block[*Scan]) ++Scan;	\
		if (Scan == ScanEnd) break;							\
		v = *Scan;											\
		Data->skip1 = (char)(Scan++ - Last);				\
		Data->data1 = Block[v];								\
															\
		Last = Scan;										\
		while (Scan != ScanEnd && !Block[*Scan]) ++Scan;	\
		if (Scan == ScanEnd) break;							\
		v = *Scan;											\
		Data->skip2 = (char)(Scan++ - Last);				\
		Data->data2 = Block[v];								\
															\
		Last = Scan;										\
		while (Scan != ScanEnd && !Block[*Scan]) ++Scan;	\
		if (Scan == ScanEnd) break;							\
		v = *Scan;											\
		Data->skip3 = (char)(Scan++ - Last);				\
		Data->data3 = Block[v];								\
	}														

#define AHIPACKNORMAL(ScanType)								\
{															\
	const unsigned char *Scan = ScanTable[ScanType];		\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	idct_ati *Data;											\
	AHIPACK(AHI(p)->Data)									\
}

#define AHIPACKSWAP(Dst,ScanType)							\
{															\
	const unsigned char *Scan;								\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	idct_ati *Data;											\
	if (ScanType == IDCTSCAN_ZIGZAG)						\
	{														\
		Scan = ZigZagSwap;									\
		Length = ZigZagSwapLength[Length-1];				\
	}														\
	else													\
	{														\
		Scan = ScanTable[ScanType];							\
		ScanType = 3 - ScanType;							\
	}														\
	AHIPACK(Dst)											\
}

#define AHIPACKSWAPZIGZAG(Dst)								\
{															\
	const unsigned char *Scan;								\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	idct_ati *Data;											\
	Scan = ZigZagSwap;										\
	Length = ZigZagSwapLength[Length-1];					\
	AHIPACK(Dst)											\
}

#define AHIBLOCKSWAP(ScanType)								\
{															\
	if (SubNo <= 4)											\
		AHIDCBLOCK(AHI(p)->DCAdd)							\
	Count = 0;												\
	if (SubNo == 2)											\
	{														\
		if (Length>0 && Length<=64)							\
			AHIPACKSWAP(AHI(p)->SaveData,ScanType)			\
		AHI(p)->SaveCount = Count;							\
		AHI(p)->SaveScanType = ScanType;					\
	}														\
	else													\
	{														\
		if (Length>0 && Length<=64)							\
			AHIPACKSWAP(AHI(p)->Data,ScanType)				\
		AHI(p)->AhiIDCT8x8(AHI(p)->IDCT,AHI(p)->Data,Count,AHI(p)->MacroNo,1 << ScanType); \
		AHIMVINC;											\
		if (SubNo == 3)										\
		{													\
			AHI(p)->AhiIDCT8x8(AHI(p)->IDCT,AHI(p)->SaveData,AHI(p)->SaveCount,AHI(p)->MacroNo,1 << AHI(p)->SaveScanType); \
			AHIMVINC;										\
		}													\
	}														\
}

#define AHIMCOMPBLOCKSWAP									\
{															\
	Count = 0;												\
	if (SubNo == 2)											\
	{														\
		if (Length>0 && Length<=64)							\
			AHIPACKSWAPZIGZAG(AHI(p)->SaveData)				\
		AHI(p)->SaveCount = Count;							\
	}														\
	else													\
	{														\
		if (Length>0 && Length<=64)							\
			AHIPACKSWAPZIGZAG(AHI(p)->Data)					\
		AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,AHI(p)->Data,Count,AHI(p)->MVBack,AHI(p)->MVFwd,AHI(p)->MacroNo,1 << IDCTSCAN_ZIGZAG);\
		AHIMVINC;											\
		if (SubNo == 3)										\
		{													\
			AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,AHI(p)->SaveData,AHI(p)->SaveCount,AHI(p)->MVBack,AHI(p)->MVFwd,AHI(p)->MacroNo,1 << IDCTSCAN_ZIGZAG);\
			AHIMVINC;										\
		}													\
	}														\
}

#define AHIQPEL(MVPtr)			\
		MV = *(MVPtr++);		\
		MV <<= 1;				\
		MV &= ~0x00010000;		

#define AHIQPELSWAP(MVPtr)		\
		MV = *(MVPtr++);		\
		MV = (MV << 17) |		\
		 ((uint32_t)MV >> 15);	\
		MV &= ~0x00010001;

static void IDCTMComp(void* p,const int* MVBack,const int* MVFwd)
{
	if (MVBack)
	{
		int MV;
		AHI(p)->MVBack = AHI(p)->MVBackTab;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[0] = MV;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[1] = MV;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[2] = MV;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[3] = MV;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[4] = MV;
		AHIQPEL(MVBack); AHI(p)->MVBackTab[5] = MV;
	}
	else
		AHI(p)->MVBack = NULL;

	if (MVFwd)
	{
		int MV;
		AHI(p)->MVFwd = AHI(p)->MVFwdTab;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[0] = MV;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[1] = MV;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[2] = MV;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[3] = MV;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[4] = MV;
		AHIQPEL(MVFwd); AHI(p)->MVFwdTab[5] = MV;
	}
	else
		AHI(p)->MVFwd = NULL;
}

static void IDCTMCompSwap(void* p,const int* MVBack,const int* MVFwd)
{
	if (MVBack)
	{
		int MV;
		AHI(p)->MVBack = AHI(p)->MVBackTab;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[0] = MV;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[2] = MV;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[1] = MV;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[3] = MV;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[4] = MV;
		AHIQPELSWAP(MVBack); AHI(p)->MVBackTab[5] = MV;
	}
	else
		AHI(p)->MVBack = NULL;

	if (MVFwd)
	{
		int MV;
		AHI(p)->MVFwd = AHI(p)->MVFwdTab;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[0] = MV;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[2] = MV;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[1] = MV;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[3] = MV;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[4] = MV;
		AHIQPELSWAP(MVFwd); AHI(p)->MVFwdTab[5] = MV;
	}
	else
		AHI(p)->MVFwd = NULL;
}

static void IDCTProcess(void* p,int x,int y)
{
	AHI(p)->MacroSubNo = 0;
	AHI(p)->MacroNo = x + y*AHI(p)->MacroWidth;
}

static void IDCTProcessSwap(void* p,int x,int y)
{
	AHI(p)->MacroSubNo = 0;
	AHI(p)->MacroNo = y + x*AHI(p)->MacroWidth;
}

static void IDCTIntra(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int Count = 0;
	int SubNo = ++AHI(p)->MacroSubNo;
	if (SubNo <= 4)
		AHIDCBLOCK(AHI(p)->DCAdd)
	
	if (Length>0 && Length<=64)
		AHIPACKNORMAL(ScanType)

	AHI(p)->AhiIDCT8x8(AHI(p)->IDCT,AHI(p)->Data,Count,AHI(p)->MacroNo,1 << ScanType);
}

static void IDCTInter(void* p,idct_block_t *Block,int Length)
{
	int Count = 0;

	if (Length>0 && Length<=64)
		AHIPACKNORMAL(IDCTSCAN_ZIGZAG)

	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,AHI(p)->Data,Count,AHI(p)->MVBack,AHI(p)->MVFwd,AHI(p)->MacroNo,1 << IDCTSCAN_ZIGZAG);
	AHIMVINC
}

static void IDCTInterAdj(void* p,idct_block_t *Block,int Length)
{
	int Count = 0;
	AHIMVADJBLOCK

	if (Length>0 && Length<=64)
		AHIPACKNORMAL(IDCTSCAN_ZIGZAG)

	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,AHI(p)->Data,Count,AHI(p)->MVBack,AHI(p)->MVFwd,AHI(p)->MacroNo,1 << IDCTSCAN_ZIGZAG);
	AHIMVINC
}

static void IDCTIntraSwap(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int Count;
	int SubNo = ++AHI(p)->MacroSubNo;

	AHIBLOCKSWAP(ScanType);
}

static void IDCTInterSwap(void* p,idct_block_t *Block,int Length)
{
	int Count;
	int SubNo = ++AHI(p)->MacroSubNo;

	AHIMCOMPBLOCKSWAP;
}

static void IDCTInterSwapAdj(void* p,idct_block_t *Block,int Length)
{
	int Count;
	int SubNo = ++AHI(p)->MacroSubNo;

	AHIMVADJBLOCK
	AHIMCOMPBLOCKSWAP
}

static void IDCTCopy16x16(void* p,int x,int y,int Forward)
{
	int MacroNo = x + y*AHI(p)->MacroWidth;
	int* Back = NULL;
	int* Fwd = NULL;

	if (Forward)
		Fwd = &AHI(p)->Zero;
	else
		Back = &AHI(p)->Zero;
	
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
}

static void IDCTCopy16x16Swap(void* p,int x,int y,int Forward)
{
	int MacroNo = y + x*AHI(p)->MacroWidth;
	int* Back = NULL;
	int* Fwd = NULL;

	if (Forward)
		Fwd = &AHI(p)->Zero;
	else
		Back = &AHI(p)->Zero;
	
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
	AHI(p)->AhiIDCTMComp8x8(AHI(p)->IDCT,NULL,0,Back,Fwd,MacroNo,0);
}

static int IDCTUpdateFunc(ahi* p)
{
	if (p->Overlay.Direction)
	{
		p->IdctVMT.Copy16x16 = IDCTCopy16x16Swap;
		p->IdctVMT.Process = IDCTProcessSwap;
		p->IdctVMT.MComp8x8 = IDCTMCompSwap;
		p->IdctVMT.MComp16x16 = IDCTMCompSwap;
		p->IdctVMT.Intra8x8 = (idctintra)IDCTIntraSwap;
		p->IdctVMT.Inter8x8 = (idctinter)(p->IDCTBug ? IDCTInterSwapAdj : IDCTInterSwap);
	}
	else 
	{
		p->IdctVMT.Copy16x16 = IDCTCopy16x16;
		p->IdctVMT.Process = IDCTProcess;
		p->IdctVMT.MComp8x8 = IDCTMComp;
		p->IdctVMT.MComp16x16 = IDCTMComp;
		p->IdctVMT.Intra8x8 = (idctintra)IDCTIntra;
		p->IdctVMT.Inter8x8 = (idctinter)(p->IDCTBug ? IDCTInterAdj : IDCTInter);
	}
	return ERR_NONE;
}

static int IDCTOutputFormat(ahi* p, const void* Data, int Size)
{
	packetformat Format;
	int Result;
	p->IDCTInit = 1;

	if (Size == sizeof(video))
	{
		memset(&Format,0,sizeof(Format));
		Format.Type = PACKET_VIDEO;
		Format.Format.Video = *(const video*)Data;
		Data = &Format;
		Size = sizeof(packetformat);

		//IDCTWidth or IDCTHeight could have changes so invalidate current input format
		memset(&p->p.Input.Format.Video,0,sizeof(video));
	}

	Result = p->p.Node.Set((node*)p,OUT_INPUT|PIN_FORMAT,Data,Size);

	if (Size && Result != ERR_NONE)
		p->p.Node.Set((node*)p,OUT_INPUT|PIN_FORMAT,NULL,0);

	p->IDCTInit = 0;
	return Result;
}

static int IDCTSend(void* p,tick_t RefTime,const flowstate* State);

static int IDCTSet(void* p, int No, const void* Data, int Size)
{
	flowstate State;
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case IDCT_MODE: if (*(int*)Data==0) Result = ERR_NONE; break;
	case IDCT_BUFFERWIDTH: SETVALUE(AHI(p)->IDCTWidth,int,ERR_NONE); break;
	case IDCT_BUFFERHEIGHT: SETVALUE(AHI(p)->IDCTHeight,int,ERR_NONE); break;
	case IDCT_FORMAT:
		assert(Size == sizeof(video) || !Data);
		Result = IDCTOutputFormat(AHI(p),Data,Size);
		break;

	case IDCT_BACKUP: 
		if (Size == sizeof(idctbackup))
			Result = IDCTRestore(p,(idctbackup*)Data);
		break;

	case IDCT_ROUNDING:
		if (Size == sizeof(bool_t) && AHI(p)->IDCT)
		{
			AHI(p)->IDCTRounding = !*(const int*)Data;
			AHI(p)->AhiIDCTPropSet(AHI(p)->IDCT,PROP_ROUNDING,&AHI(p)->IDCTRounding,sizeof(int));
			Result = ERR_NONE;
		}
		break;

	case IDCT_SHOW: 
		SETVALUE(AHI(p)->ShowNext,int,ERR_NONE); 
		if (AHI(p)->ShowNext >= AHI(p)->BufCount) AHI(p)->ShowNext = -1;
		break;

	case IDCT_BUFFERCOUNT:
		assert(Size == sizeof(int));
		Result = ERR_NONE;
		if (AHI(p)->BufCount != *(const int*)Data &&
			!UpdateOverlay(AHI(p),*(const int*)Data,0))
			Result = ERR_OUT_OF_MEMORY;
		break;

	case FLOW_FLUSH:
		AHI(p)->ShowNext = -1;
		Result = ERR_NONE;
		break;

	case FLOW_RESEND:
		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;
		Result = IDCTSend(p,-1,&State);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+AHI(p)->AllCount)
		SETVALUE(AHI(p)->BufFrameNo[No-IDCT_FRAMENO],int,ERR_NONE);
	return Result;
}

static int IDCTEnumAHI(void* p, int* No, datadef* Param)
{
	int Result = IDCTEnum(p,No,Param);
	if (Result == ERR_NONE && Param->No == IDCT_OUTPUT)
		Param->Flags |= DF_RDONLY;
	return Result;
}

static int IDCTGet(void* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case FLOW_BUFFERED: GETVALUE(1,bool_t); break;
	case NODE_PARTOF: GETVALUE(AHI(p),ahi*); break;
	case IDCT_OUTPUT|PIN_FORMAT: GETVALUECOND(AHI(p)->p.Input,packetformat,AHI(p)->IDCT); break;
	case IDCT_OUTPUT|PIN_PROCESS: GETVALUE(NULL,packetprocess); break;
	case IDCT_OUTPUT: GETVALUE(AHI(p)->IDCTOutput,pin); break;
	case IDCT_FORMAT: 
		assert(Size==sizeof(video));
		if (AHI(p)->IDCT)
		{
			video* i = (video*)Data;
			*i = AHI(p)->p.Input.Format.Video;
			if ((AHI(p)->Overlay.Direction ^ i->Direction) & DIR_SWAPXY)
			{
				i->Direction ^= DIR_SWAPXY;
				SwapInt(&i->Width,&i->Height);
			}
			Result = ERR_NONE;
		}
		break;
	case IDCT_MODE: GETVALUE(0,int); break;
	case IDCT_ROUNDING: GETVALUE(!AHI(p)->IDCTRounding,bool_t); break;
	case IDCT_BUFFERCOUNT: GETVALUE(AHI(p)->IDCT?AHI(p)->BufCount:0,int); break;
	case IDCT_BUFFERWIDTH: GETVALUE(AHI(p)->IDCTWidth,int); break;
	case IDCT_BUFFERHEIGHT: GETVALUE(AHI(p)->IDCTHeight,int); break;
	case IDCT_SHOW: GETVALUE(AHI(p)->ShowNext,int); break;
	case IDCT_BACKUP: 
		assert(Size == sizeof(idctbackup));
		Result = IDCTBackup(p,(idctbackup*)Data);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+AHI(p)->AllCount)
		GETVALUE(AHI(p)->BufFrameNo[No-IDCT_FRAMENO],int);
	return Result;
}

static int IDCTLock(void* p,int No,planes Planes,int* Brightness,video* Format)
{
	if (Brightness)
		*Brightness = AHI(p)->p.FX.Brightness;
	if (Format)
		*Format = AHI(p)->Overlay;
	return Lock(AHI(p),No,Planes,0);
}

static void IDCTUnlock(void* p,int No)
{
	Unlock(AHI(p),No);
}

static int IDCTFrameStart(void* p,int FrameNo,int *OldFrameNo,int DstNo,int BackNo,int FwdNo,int ShowNo,bool_t Drop)
{
	ahisurface* Back;
	ahisurface* Fwd;
	ahisurface* Dst;

	if (!AHI(p)->IDCT)
		return ERR_DEVICE_ERROR;

	// we will try to avoid overwrite currently displayed buffers:
	//   ShowCurr currently displayed
	//   ShowLast may be still on screen (because flip occurs only during vblank)

	DEBUG_MSG6(DEBUG_VIDEO,T("ATI FrameStart %d,%d,%d (%d,%d,%d)"),DstNo,BackNo,FwdNo,ShowNo,AHI(p)->ShowCurr,AHI(p)->ShowLast);

	if (!AHI(p)->Buf[SCALE] && (AHI(p)->ShowLast == DstNo || AHI(p)->ShowCurr == DstNo))
	{
		// try to find a free buffer
		int SwapNo;
		for (SwapNo=0;SwapNo<AHI(p)->AllCount;++SwapNo)
			if (SwapNo != AHI(p)->ShowLast && SwapNo != AHI(p)->ShowCurr && 
				SwapNo != BackNo && SwapNo != FwdNo && SwapNo != ShowNo)
				break; 

		if (SwapNo < AHI(p)->AllCount || UpdateOverlay(AHI(p),AHI(p)->BufCount,AHI(p)->TempCount+1))
		{
			// use free buffer
			SwapPtr(AHI(p),SwapNo,DstNo,1);

			DEBUG_MSG2(DEBUG_VIDEO,T("ATI Swap %d,%d"),DstNo,SwapNo);
		}
		else
		if (AHI(p)->ShowLast >= 0 && AHI(p)->ShowCurr != AHI(p)->ShowLast &&
			AHI(p)->ShowLast != BackNo && AHI(p)->ShowLast != FwdNo) 
		{
			// no free buffer found and couldn't allocate new temp buffer
			// so we wait for vblank and use ShowLast
			if (!Drop)
				AHI(p)->AhiDispWaitVBlank(AHI(p)->Context,0);

			DEBUG_MSG2(DEBUG_VIDEO,T("ATI VBlank %d,%d"),DstNo,AHI(p)->ShowLast);

			if (DstNo != AHI(p)->ShowLast)
				SwapPtr(AHI(p),AHI(p)->ShowLast,DstNo,1);
		}
	}

	AHI(p)->ShowNext = ShowNo;
	AHI(p)->ShowNextIdle = ShowNo == DstNo;

	if (OldFrameNo)
		*OldFrameNo = AHI(p)->BufFrameNo[DstNo];
	AHI(p)->BufFrameNo[DstNo] = FrameNo;

	Back = NULL;
	Fwd = NULL;
	Dst = AHI(p)->Buf[DstNo];
	if (BackNo >= 0)
		Back = AHI(p)->Buf[BackNo];
	if (FwdNo >= 0)
		Fwd = AHI(p)->Buf[FwdNo];

	AHI(p)->MacroWidth = AHI(p)->Overlay.Width >> 4;
	AHI(p)->AhiIDCTFrameStart(AHI(p)->IDCT,Dst,Back,Fwd,AHI(p)->FrameNo,0);

	return ERR_NONE;
}

static void IDCTFrameEnd(void* p)
{
	AHI(p)->AhiIDCTFrameEnd(AHI(p)->IDCT,AHI(p)->FrameNo++);
}

static void IDCTDrop(void* p)
{
	int No;
	for (No=0;No<AHI(p)->AllCount;++No)
		AHI(p)->BufFrameNo[No] = -1;
}

static int IDCTNull(void* p,const flowstate* State,bool_t Empty)
{
	if (Empty)
		++AHI(p)->p.Total;
	if (!State || State->DropLevel)
		++AHI(p)->p.Dropped;
	return ERR_NONE;
}

static int IDCTSend(void* p,tick_t RefTime,const flowstate* State)
{
	bool_t Flip = 0;

	if (AHI(p)->ShowNext < 0)
		return ERR_NEED_MORE_DATA;

	if (State->CurrTime >= 0)
	{
		tick_t Diff;

		if (!AHI(p)->p.Play && State->CurrTime == AHI(p)->p.LastTime)
			return ERR_BUFFER_FULL;

		Diff = RefTime - (State->CurrTime + SHOWAHEAD);
		if (Diff >= 0)
		{
			if (!AHI(p)->Buf[SCALE+1] || Diff>(TICKSPERSEC*8/1000))
				return ERR_BUFFER_FULL;

			// we have time for scale flipping
			Flip = 1;
		}

		AHI(p)->p.LastTime = State->CurrTime;
	}
	else
		AHI(p)->p.LastTime = RefTime;

	if (State->CurrTime != TIME_RESEND)
		++AHI(p)->p.Total;

	AHI(p)->ShowLast = AHI(p)->ShowCurr;
	AHI(p)->ShowCurr = AHI(p)->ShowNext;

	if (AHI(p)->Buf[SCALE])
	{
		if (State->DropLevel)
			++AHI(p)->p.Dropped;
		else
			Stretch(AHI(p),1,Flip);
	}
	else
		UpdateOverlaySurf(AHI(p),AHI(p)->ShowNextIdle);

	return ERR_NONE;
}

static int Create(ahi* p);
static void Delete(ahi* p);

static const nodedef AHI =
{
	sizeof(ahi)|CF_GLOBAL|CF_SETTINGS,
	AHI_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+200,
	(nodecreate)Create,
	(nodedelete)Delete,
};

static const nodedef AHI_IDCT =
{
	CF_ABSTRACT,
	AHI_IDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT+50,
};

static void FindUnmap(ahi* p)
{
	uint8_t* Min;
	uint8_t* Max;
	uint32_t* q;
	bool_t Found = 0;
	int i;

	CodeFindPages((void*)(int)p->AhiInit,&Min,&Max,NULL);

	if (Min && Max>Min)
	{
		Max -= 8*4;
		for (q=(uint32_t*)Min;q<(uint32_t*)Max;++q)
			if (q[0] == 0xE3A06A06) //mov r6,#0x6000
			{
				for (i=1;i<8;++i)
					if (q[i] == 0xE3866002) // or r6,r6,#2
					{
						for (i=-1;i>-16;--i)
							if ((q[i] & 0xFFFF0000)==0xE92D0000)
							{
								if (Found)
									p->AhiUnmap = NULL;
								else
								{
									p->AhiUnmap = (int(*)())(q+i);
									Found = 1;
								}
								break;
							}
						break;
					}
			}
	}
}

static int Create(ahi* p) 
{ 
	p->Context = NULL;

	p->p.Node.Enum = (nodeenum)Enum;
	p->p.Node.Get = (nodeget)Get;
	p->p.Node.Set = (nodeset)Set;
	p->p.Init = (ovlfunc)Init;
	p->p.Done = (ovldone)Done;
	p->p.Reset = (ovlfunc)Reset;

	p->IdctVMT.Class = AHI_IDCT_ID;
	p->IdctVMT.Enum = IDCTEnumAHI;
	p->IdctVMT.Get = IDCTGet;
	p->IdctVMT.Set = IDCTSet;
	p->IdctVMT.Send = IDCTSend;
	p->IdctVMT.Null = IDCTNull;
	p->IdctVMT.Drop = IDCTDrop;
	p->IdctVMT.Lock = IDCTLock;
	p->IdctVMT.Unlock = IDCTUnlock;
	p->IdctVMT.FrameStart = IDCTFrameStart;
	p->IdctVMT.FrameEnd = IDCTFrameEnd;

	p->IDCTOutput.Node = (node*)p;
	p->IDCTOutput.No = OUT_INPUT;
	p->IDCTBug = 0;
	p->IDCTInit = 0;
	p->AllowStretchFlip = 1;
	p->AllowPrimaryReUse = 1;
	p->AllowLowBitdepth = 1;

	p->p.Module = LoadLibrary(T("ACE_DDI.DLL"));
	GetProc(&p->p.Module,&p->AhiInit,T("AhiInit"),0);
	GetProc(&p->p.Module,&p->AhiTerm,T("AhiTerm"),0);

	GetProc(&p->p.Module,&p->AhiDevEnum,T("AhiDevEnum"),0);
	GetProc(&p->p.Module,&p->AhiDevOpen,T("AhiDevOpen"),0);
	GetProc(&p->p.Module,&p->AhiDevClose,T("AhiDevClose"),0);
	GetProc(&p->p.Module,&p->AhiDevClientVersion,T("AhiDevClientVersion"),0);

	GetProc(&p->p.Module,&p->AhiDispModeGet,T("AhiDispModeGet"),0);
	GetProc(&p->p.Module,&p->AhiDispModeEnum,T("AhiDispModeEnum"),0);
	GetProc(&p->p.Module,&p->AhiDispSurfGet,T("AhiDispSurfGet"),0);

	GetProc(&p->p.Module,&p->AhiSurfGetLargestFreeBlockSize,T("AhiSurfGetLargestFreeBlockSize"),1);
	GetProc(&p->p.Module,&p->AhiSurfAlloc,T("AhiSurfAlloc"),0);
	GetProc(&p->p.Module,&p->AhiSurfReuse,T("AhiSurfReuse"),0);
	GetProc(&p->p.Module,&p->AhiSurfFree,T("AhiSurfFree"),0);
	GetProc(&p->p.Module,&p->AhiSurfLock,T("AhiSurfLock"),0);
	GetProc(&p->p.Module,&p->AhiSurfUnlock,T("AhiSurfUnlock"),0);
	GetProc(&p->p.Module,&p->AhiSurfInfo,T("AhiSurfInfo"),0);
	GetProc(&p->p.Module,&p->AhiSurfPlaneInfo,T("AhiSurfPlaneInfo"),0);

	GetProc(&p->p.Module,&p->AhiDrawRopSet,T("AhiDrawRopSet"),0);
	GetProc(&p->p.Module,&p->AhiDrawSurfSrcSet,T("AhiDrawSurfSrcSet"),0);
	GetProc(&p->p.Module,&p->AhiDrawSurfDstSet,T("AhiDrawSurfDstSet"),0);
	GetProc(&p->p.Module,&p->AhiDrawIdle,T("AhiDrawIdle"),0);
	GetProc(&p->p.Module,&p->AhiDispWaitVBlank,T("AhiDispWaitVBlank"),0);
	GetProc(&p->p.Module,&p->AhiDrawBitBlt,T("AhiDrawBitBlt"),0);
	GetProc(&p->p.Module,&p->AhiDrawStretchBlt,T("AhiDrawStretchBlt"),0);
	GetProc(&p->p.Module,&p->AhiDrawRotateBlt,T("AhiDrawRotateBlt"),0);

	GetProc(&p->p.Module,&p->AhiDispOverlayCaps,T("AhiDispOverlayCaps"),0);
	GetProc(&p->p.Module,&p->AhiDispOverlayState,T("AhiDispOverlayState"),0);
	GetProc(&p->p.Module,&p->AhiDispOverlayPos,T("AhiDispOverlayPos"),0);
	GetProc(&p->p.Module,&p->AhiDispOverlaySurf,T("AhiDispOverlaySurf"),0);

	GetProc(&p->p.Module,&p->AhiIDCTOpen,T("AhiIDCTOpen"),0);
	GetProc(&p->p.Module,&p->AhiIDCTClose,T("AhiIDCTClose"),0);
	GetProc(&p->p.Module,&p->AhiIDCTPropGet,T("AhiIDCTPropGet"),0);
	GetProc(&p->p.Module,&p->AhiIDCTPropSet,T("AhiIDCTPropSet"),0);
	GetProc(&p->p.Module,&p->AhiIDCTFrameStart,T("AhiIDCTFrameStart"),0);
	GetProc(&p->p.Module,&p->AhiIDCTFrameEnd,T("AhiIDCTFrameEnd"),0);
	GetProc(&p->p.Module,&p->AhiIDCTMComp8x8,T("AhiIDCTMComp8x8"),0);
	GetProc(&p->p.Module,&p->AhiIDCTMComp,T("AhiIDCTMComp"),0);
	GetProc(&p->p.Module,&p->AhiIDCT8x8,T("AhiIDCT8x8"),0);

	if (!p->p.Module)
		return ERR_NOT_SUPPORTED;

	FindUnmap(p);

	memset(&p->DevInfo,0,sizeof(p->DevInfo));

	if (SafeAhiInit(p))
	{
		int No;
		for (No=0;No<16;No++)
		{
			int Result = p->AhiDevEnum(&p->Device,&p->DevInfo,No);
			if (Result == SUCCESS)
			{
				if (p->DevInfo.ChipId == 0x57441002)
					p->IDCTBug = 1;
				if (p->DevInfo.ExternalMemSize>0 && CheckVersion(p,1))
				{
					p->p.AccelIDCT = &p->IdctVMT;
					NodeRegisterClass(&AHI_IDCT);
				}
				break;
			}
			if (Result == ENDENUM)
				break;
		}

		p->TmpInited = 1; // assume NoRelease, later loading settings will call AhiTerm if needed
		//SafeAhiTerm(p);
	}

	p->p.DoPowerOff = 1;

	Context()->HwOrientation = Orientation;
	Context()->HwOrientationContext = p;
	return ERR_NONE;
}

static void Delete(ahi* p)
{
	Context()->HwOrientation = NULL;
	Context()->HwOrientationContext = NULL;
	if (p->TmpInited)
	{
		p->TmpInited = 0;
		p->AhiTerm();
		// no unmap... maybe unmap casing problem too...
	}
}

void ATI3200_Init()
{
	NodeRegisterClass(&AHI);
}

void ATI3200_Done()
{
	NodeUnRegisterClass(AHI_ID);
	NodeUnRegisterClass(AHI_IDCT_ID);
}

#else
void ATI3200_Init() {}
void ATI3200_Done() {}
#endif
