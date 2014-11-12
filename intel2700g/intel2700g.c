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
 * $Id: intel2700g.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "stdafx.h"
#include "gxvadd.h"

#if defined(ARM)

#define VSYNC_WORKAROUND

typedef struct pvr
{
	overlay p;
	idct IdctVMT;

	// idct
	int mx,my;
	int SubPos;
	int ImageWrite[3];
	int MCompType;
	int FetchA[2][2];
	int Width16;
	int Height16;

	// vsync
#ifdef VSYNC_WORKAROUND
	bool_t SetupVSync;
	bool_t DisableVSync;
	int* VSyncPtr;
	int VSyncTotal; // total units
	int VSyncBottom;   // start of vsync
	int VSyncLimit; // units per 1msec
	tick_t VSyncRefresh;
#endif

	GXVAError (*M24VA_FrameDimensions)(gx_uint32 ui32Height, gx_uint32 ui32Width);
	GXVAError (*M24VA_FrameAllocate)(SMSurface *phFrames, gx_uint32 ui32FrameCnt);
	GXVAError (*M24VA_FrameFree)(SMSurface *phFrames, gx_uint32 ui32FrameCnt);
	GXVAError (*M24VA_BeginFrame)(SMSurface hOutputSurface, gx_bool bEnableIDCT);
	GXVAError (*M24VA_EndFrame)(gx_bool bSyncHardware);
	GXVAError (*M24VA_SetReferenceFrameAddress)(gx_uint32 ui32FrameNo, SMSurface hSurface);
	GXVAError (*M24VA_FrameBeginAccess)(SMSurface hSurface, gx_uint8 **ppuint8FrameAddress, gx_uint32 *pui32FrameStride, gx_bool bSyncHardware);
	GXVAError (*M24VA_FrameEndAccess)(SMSurface hSurface);
	GXVAError (*M24VA_WriteIZZBlock)(gx_int32 *pi32Data, gx_uint32 ui32Count);
	GXVAError (*M24VA_WriteIZZData)(gx_int32 i32Coeff, gx_uint32 ui32Index, gx_bool bEOB);
	GXVAError (*M24VA_SetIZZMode)(GXVA_IZZMODE eMode);
	GXVAError (*M24VA_WriteResidualDifferenceData)(gx_int16 *pi16Data, gx_uint32 ui32Count); 
	GXVAError (*M24VA_WriteMCCmdData)(gx_uint32 ui32Data);
	GXVAError (*M24VA_FCFrameAllocate)(SMSurface *phFrame, GXVA_SURF_FORMAT eFormat);
	GXVAError (*M24VA_FrameConvert)(SMSurface hDestSurface, SMSurface hSourSurface);                                        
	GXVAError (*M24VA_Initialise)(void*);
	GXVAError (*M24VA_Deinitialise)();
	GXVAError (*M24VA_Reset)();

	VDISPError (*VDISP_Initialise)(void*);
	VDISPError (*VDISP_Deinitialise)();
	VDISPError (*VDISP_OverlaySetAttributes)(PVDISP_OVERLAYATTRIBS psOverlayAttributes, SMSurface hSurface);
	VDISPError (*VDISP_OverlayFlipSurface)(SMSurface hSurfaceFlipTo, VDISP_DEINTERLACE eDeinterlace);
	VDISPError (*VDISP_OverlayContrast)(gx_uint32 ui32Contrast);
	VDISPError (*VDISP_OverlayGamma)(gx_uint32 ui32Gamma);
	VDISPError (*VDISP_OverlayBrightness)(gx_uint32 ui32Brightness);
	VDISPError (*VDISP_OverlaySetColorspaceConversion)(PVDISP_CSCCoeffs psCoeffs);

	int16_t IZZ[2*(64+8)];

	VDISP_OVERLAYATTRIBS Attribs;
	uint8_t* RegBase;
	bool_t CloseWMP;
	bool_t SetupIDCTRotate;
	bool_t M24VA;
	bool_t VDISP;
	bool_t IDCT;
	bool_t YUV422;
	bool_t Empty;
	bool_t IDCTInit;
	bool_t Initing;
	bool_t FirstUpdate;
	int FirstUpdateTime;
	int IDCTMemInit;
	int IDCTWidth;
	int IDCTHeight;
	int AllCount;
	int BufCount;
	int TempCount;
	int ShowNext;
	int ShowCurr;
	int ShowLast;
	int FlipLastTime;
	int NativeDirection;
	video Overlay;
	rect OverlayRect;
	blitfx OvlFX;
	blitfx SoftFX;
	pin IDCTOutput;
	bool_t IDCTRounding;
	int Brightness;
	int Contrast;
	int Saturation;
	int RGBAdjust[3];

	SMSurface Buf[MAXIDCTBUF]; 
	planes BufMem[MAXIDCTBUF];
	int BufFrameNo[MAXIDCTBUF];

} pvr;

#define PVR(p) ((pvr*)((char*)(p)-(int)&(((pvr*)0)->IdctVMT)))

//------------------------------------------------------------

#define MID			0x3FE0FF0
#define DVT01		0x3FE2164
#define DVT02		0x3FE2168
#define DVT03		0x3FE216C
#define DVLNUM		0x3FE2308

static void GetMode(pvr* p)
{
	RawFrameBufferInfo Info;
	HDC DC= GetDC(NULL);
	memset(&Info,0,sizeof(Info));
	ExtEscape(DC, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char*)&Info);
	ReleaseDC(NULL,DC);

	p->NativeDirection = 0;

#ifdef VSYNC_WORKAROUND
	p->VSyncPtr = NULL;
#endif

	if (Info.pFramePointer)
	{
		if (abs(Info.cyStride) < abs(Info.cxPixels * Info.cxStride))
			SwapInt(&Info.cxPixels,&Info.cyPixels); // correct possible orientation error

		if (abs(Info.cyStride) < abs(Info.cxStride))
		{
			p->NativeDirection |= DIR_SWAPXY;
			if (Info.cxStride<0) 
				p->NativeDirection |= DIR_MIRRORUPDOWN;
			if (Info.cyStride<0) 
				p->NativeDirection |= DIR_MIRRORLEFTRIGHT;
		}
		else
		{
			if (Info.cxStride<0) 
				p->NativeDirection |= DIR_MIRRORLEFTRIGHT;
			if (Info.cyStride<0) 
				p->NativeDirection |= DIR_MIRRORUPDOWN;
		}

		p->NativeDirection = CombineDir(0,p->NativeDirection,GetOrientation());
		p->RegBase = (uint8_t*)((uint32_t)Info.pFramePointer & ~(1024*1024-1)); // align to 1MB

#ifdef VSYNC_WORKAROUND
		{
			int VSyncTop = 0;

			p->VSyncTotal = -1;
			p->VSyncBottom = 0;

			TRY_BEGIN
			if ((*(uint32_t*)(p->RegBase+MID) & 0xFFFFFF) == 0x727189) // Intel 2700G ?
			{
				p->VSyncRefresh = TICKSPERSEC / 60; // todo: find a way to get lcd refresh rate
				p->VSyncTotal = *(uint32_t*)(p->RegBase+DVT01) & 4095;
				VSyncTop = *(uint32_t*)(p->RegBase+DVT02) & 4095;
				p->VSyncBottom = *(uint32_t*)(p->RegBase+DVT03) & 4095;
				p->VSyncLimit = Scale(p->VSyncTotal,TICKSPERSEC,p->VSyncRefresh*1000);
			}
			TRY_END

			if (p->VSyncTotal >= p->VSyncBottom && (p->VSyncBottom  - VSyncTop) >= Info.cyPixels)
			{
				p->VSyncBottom -= 16; // adjust a little
				p->VSyncPtr = (int*)(p->RegBase+DVLNUM);
			}
		}
#endif		
	}
	QueryDesktop(&p->p.Output.Format.Video);
}

static void BufferFormat(pvr* p,video* Format)
{
	*Format = p->Overlay;
	DefaultPitch(Format);
}

static void ClearIMC4(uint8_t* Ptr,const video* Format)
{
	FillColor(Ptr,
		Format->Pitch,0,0,Format->Width,Format->Height,8,0);
	FillColor(Ptr+Format->Pitch*Format->Height,
		Format->Pitch,0,0,Format->Width>>1,Format->Height>>1,8,128);
	FillColor(Ptr+Format->Pitch*Format->Height+(Format->Pitch >> 1),
		Format->Pitch,0,0,Format->Width>>1,Format->Height>>1,8,128);
}

static int UpdateAlloc(pvr* p,int BufCount,int TempCount,int MemCount)
{
	int Result = ERR_NONE;
	int AllCount;
	
	if (BufCount && BufCount+TempCount<2)
		++TempCount;
	
	AllCount = BufCount + TempCount;
	if (AllCount > MAXIDCTBUF)
		return ERR_INVALID_PARAM;

	if (p->AllCount > AllCount)
	{
		p->M24VA_FrameFree(&p->Buf[AllCount],p->AllCount-AllCount);

		p->AllCount = AllCount;
		p->BufCount = BufCount;
		p->TempCount = TempCount;
	}
	else
	{
		p->BufCount = min(p->AllCount,BufCount);
		p->TempCount = p->AllCount - p->BufCount;

		while (p->AllCount < AllCount)
		{
			gx_uint8* Ptr;
			bool_t Clear = !p->IDCT || MemCount <= p->AllCount;

			if (p->IDCT && MemCount <= p->AllCount)
			{
				video Buffer;
				BufferFormat(p,&Buffer);
				if (SurfaceAlloc(p->BufMem[p->AllCount],&Buffer) != ERR_NONE)
				{
					Result = ERR_OUT_OF_MEMORY;
					break;
				}
				++MemCount;
			}
	
			if (p->M24VA_FrameAllocate(&p->Buf[p->AllCount],1) != GXVAError_OK)
			{
				Result = ERR_OUT_OF_MEMORY;
				break;
			}

			if (Clear && p->M24VA_FrameBeginAccess(p->Buf[p->AllCount],&Ptr,(gx_uint32*)&p->Overlay.Pitch,1) == GXVAError_OK)
			{
				ClearIMC4(Ptr,&p->Overlay);
				p->M24VA_FrameEndAccess(p->Buf[p->AllCount]);
			}

			p->BufFrameNo[p->AllCount] = -1;
			p->AllCount++;
			p->BufCount = min(p->AllCount,BufCount);
			p->TempCount = p->AllCount - p->BufCount;
		}
	}

	while (MemCount > p->AllCount)
		SurfaceFree(p->BufMem[--MemCount]);

	return Result;
}

static int16_t Sat11(int x,int Adjust)
{ 
	x = (x * (Adjust + 256)) >> 8;
	return (int16_t)(x>1023?1023:(x<-1024?-1024:x));
}

static int SatUV(pvr* p,int v)
{
	int m = p->Saturation;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m+256;
	v = (v*m) >> 8;
	return v;
}

static void UpdateColorSpace(pvr* p)
{
	VDISP_CSCCoeffs Color;
	p->VDISP_OverlayBrightness(p->Brightness+128);
	p->VDISP_OverlayContrast(p->Contrast+128);

	//R = 1.164(Y - 16) + 1.596(V - 128)
	//G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
	//B = 1.164(Y - 16)                  + 2.018(U - 128)

	Color.ui16RyCoeff = Sat11(298,p->RGBAdjust[0]);
	Color.ui16RuCoeff = Sat11(SatUV(p,0),p->RGBAdjust[0]);
	Color.ui16RvCoeff = Sat11(SatUV(p,408),p->RGBAdjust[0]);
	Color.ui16GyCoeff = Sat11(298,p->RGBAdjust[1]);
	Color.ui16GuCoeff = Sat11(SatUV(p,-100),p->RGBAdjust[1]);
	Color.ui16GvCoeff = Sat11(SatUV(p,-208),p->RGBAdjust[1]);
	Color.ui16ByCoeff = Sat11(298,p->RGBAdjust[2]);
	Color.ui16BuCoeff = Sat11(SatUV(p,517),p->RGBAdjust[2]);
	Color.ui16BvCoeff = Sat11(SatUV(p,0),p->RGBAdjust[2]);

	p->VDISP_OverlaySetColorspaceConversion(&Color);
}

static void ReInit(pvr* p);
static int Lock(pvr* p,int No,planes Planes,bool_t UseReInit);
static void Unlock(pvr* p,int No);

static bool_t UpdateDirection(pvr* p)
{
	int No;
	video New = p->Overlay;

	p->OvlFX = p->p.FX;

	memset(&p->SoftFX,0,sizeof(blitfx));
	p->SoftFX.Flags = p->p.FX.Flags & BLITFX_DITHER;
	p->SoftFX.ScaleX = SCALE_ONE;
	p->SoftFX.ScaleY = SCALE_ONE;

	if (p->IDCT && (!p->SetupIDCTRotate || p->YUV422))
	{
		p->SoftFX.Direction = 0;
		p->OvlFX.Direction = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->p.Output.Format.Video.Direction);
	}
	else
	{
		p->SoftFX.Direction = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->NativeDirection);
		p->OvlFX.Direction = CombineDir(p->NativeDirection,0,p->p.Output.Format.Video.Direction);
	}

	if (p->SoftFX.Direction & DIR_SWAPXY)
		SwapInt(&p->OvlFX.ScaleX,&p->OvlFX.ScaleY);

#ifdef VSYNC_WORKAROUND
	// hwrotation is used? no vsync needed
	p->DisableVSync = !p->SetupVSync || ((p->OvlFX.Direction ^ p->NativeDirection ^ p->p.Output.Format.Video.Direction) & DIR_SWAPXY) != 0;
#endif

	New.Width = p->Width16+16;
	New.Height = p->Height16+16;
	New.Direction = CombineDir(p->p.Input.Format.Video.Direction,p->SoftFX.Direction,0);
	if (New.Direction & DIR_SWAPXY)
		SwapInt(&New.Width,&New.Height);
	New.Pitch = New.Width;

//	DEBUG_MSG6(-1,T("UpdateDirection w:%d h:%d dir:%d native:%d softfx:%d ovlfx:%d"),
//		New.Width,New.Height,New.Direction,p->NativeDirection,p->SoftFX.Direction,p->OvlFX.Direction);

	if (New.Direction != p->Overlay.Direction)
	{
		planes Planes;
		int Rotate;
		planes BufMem = { NULL };
		int BufCount = p->BufCount;
		int TempCount = p->TempCount;
		video Buffer;
		BufferFormat(p,&Buffer);

		if (p->Attribs.ui16ValidFlags)
		{
			Sleep(50); // be sure last flipping finished
			p->Attribs.bOverlayOn = 0;
			p->VDISP_OverlaySetAttributes(&p->Attribs,p->Buf[0]);
			p->VDISP_OverlaySetAttributes(&p->Attribs,p->Buf[1]);
			Sleep(20); //wait for overlay turn off
		}

		// save old surfaces
		if (p->IDCT)
		{
			for (No=0;No<p->BufCount;++No)
				if (p->BufMem[No][0] && Lock(p,No,Planes,0)==ERR_NONE)
				{
					SurfaceCopy(&p->Overlay,&Buffer,Planes,p->BufMem[No],NULL);
					Unlock(p,No);
				}
		}
		else
		{
			// only ShowCurr
			if (Lock(p,p->ShowCurr,Planes,0)==ERR_NONE)
			{
				if (SurfaceAlloc(BufMem,&Buffer) == ERR_NONE)
					SurfaceCopy(&p->Overlay,&Buffer,Planes,BufMem,NULL);
				Unlock(p,p->ShowCurr);
			}
		}

		if (p->AllCount)
			p->M24VA_FrameFree(p->Buf,p->AllCount);
		p->AllCount = 0;
		p->TempCount = 0;
		p->BufCount = 0;
		p->Empty = 1;
		p->ShowLast = -1;
		if (p->ShowNext >= 0)
			p->ShowCurr = p->ShowNext;
		if (p->ShowCurr<0 || p->ShowCurr>=BufCount)
			p->ShowCurr = 0;

		if (p->M24VA_FrameDimensions(New.Height,New.Width) == GXVAError_HardwareNotAvailable && !p->Initing)
		{
			ReInit(p);
			return 1;
		}

		p->Overlay = New;
		UpdateAlloc(p,BufCount,TempCount,BufCount+TempCount);

		// restore old surfaces (rotated)
		Rotate = CombineDir(Buffer.Direction,0,New.Direction);
		if (p->IDCT)
		{
			for (No=0;No<p->BufCount;++No)
				if (p->BufMem[No][0] && Lock(p,No,Planes,1)==ERR_NONE)
				{
					SurfaceRotate(&Buffer,&p->Overlay,p->BufMem[No],Planes,Rotate);
					Unlock(p,No);
				}
		}
		else
		if (BufMem[0])
		{
			if (Lock(p,p->ShowCurr,Planes,1)==ERR_NONE)
			{
				SurfaceRotate(&Buffer,&p->Overlay,BufMem,Planes,Rotate);
				Unlock(p,p->ShowCurr);
			}
			SurfaceFree(BufMem);
		}

		return 1;
	}
	return 0;
}

static const datatable PVRParams[] = 
{
#ifdef VSYNC_WORKAROUND
	{ INTEL2700G_VSYNC,			TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
#endif
	{ INTEL2700G_IDCTROTATE,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ INTEL2700G_CLOSE_WMP,		TYPE_BOOL, DF_SETUP|DF_CHECKLIST },

	DATATABLE_END(INTEL2700G_ID)
};

static int Enum(pvr* p, int* No, datadef* Param)
{
	if (OverlayEnum(&p->p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,PVRParams);
}

static void Done(pvr* p);

static int Set(pvr* p,int No,const void* Data,int Size)
{
	int Result = OverlaySet(&p->p,No,Data,Size);
	switch (No)
	{
#ifdef VSYNC_WORKAROUND
	case INTEL2700G_VSYNC: SETVALUE(p->SetupVSync,bool_t,OverlayUpdateFX(&p->p,0)); break;
#endif
	case INTEL2700G_CLOSE_WMP: SETVALUE(p->CloseWMP,bool_t,ERR_NONE); break;
	case INTEL2700G_IDCTROTATE: SETVALUE(p->SetupIDCTRotate,bool_t,OverlayUpdateFX(&p->p,0)); break;

	case NODE_CRASH:
		memset(p->BufMem,0,sizeof(p->BufMem)); // heap could be corrupted
		Done(p);
		break;
	}

	return Result;
}

static int Get(pvr* p,int No,void* Data,int Size)
{
	int Result = OverlayGet(&p->p,No,Data,Size);
	switch (No)
	{
#ifdef VSYNC_WORKAROUND
	case INTEL2700G_VSYNC: GETVALUE(p->SetupVSync,bool_t); break;
#endif
	case INTEL2700G_CLOSE_WMP: GETVALUE(p->CloseWMP,bool_t); break;
	case INTEL2700G_IDCTROTATE: GETVALUE(p->SetupIDCTRotate,bool_t); break;
	}
	return Result;
}

static int Update(pvr* p);

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	if (!(GetWindowLong(hWnd,GWL_STYLE) & WS_POPUP))
	{
		tchar_t Name[64];
		if (GetWindowText(hWnd,Name,64)>0 && tcscmp(Name,T("Windows Media"))==0)
			*(HWND*)lParam = hWnd;
	}
	return TRUE;
}

static HWND FindWMP()
{
	HWND Result = NULL;
	EnumWindows(EnumWindowsProc,(LPARAM)&Result);
	return Result;
}

static bool_t InitM24VA(pvr* p)
{
	HWND WMP = NULL;
	int ReTry;

	for (;;)
	{
		for (ReTry=0;ReTry<4;++ReTry)
		{
			if (ReTry)
				ThreadSleep(50);
			if (p->M24VA_Initialise(NULL) == GXVAError_OK)
			{
				p->M24VA = 1;
				return 1;
			}
		}

		if (!p->CloseWMP || WMP || (WMP = FindWMP())==NULL)
			break;

		PostMessage(WMP,WM_CLOSE,0,0);
	}

	ShowError(INTEL2700G_ID,INTEL2700G_ID,INTEL2700G_LOCKED);
	return 0;
}

static void ReInit(pvr* p)
{
	int BufCount = p->BufCount;
	int TempCount = p->TempCount;

	if (p->VDISP)
	{
		p->VDISP_Deinitialise();
		p->VDISP = 0;
	}

	// keep p->BufMem[]
	if (p->AllCount)
		p->M24VA_FrameFree(p->Buf,p->AllCount);
	p->AllCount = 0;
	p->TempCount = 0;
	p->BufCount = 0;

	if (p->M24VA)
	{
		p->M24VA_Deinitialise();
		p->M24VA = 0;
	}

	p->Initing = 1;
	if (InitM24VA(p))
	{
		if (p->VDISP_Initialise(NULL) == VDISPError_OK)
		{
			p->VDISP = 1;
			memset(&p->Attribs,0,sizeof(p->Attribs)); // force new udpate
			p->Overlay.Direction = -1; // force update in UpdateDirection
			p->FirstUpdate = 1;
			p->FirstUpdateTime = GetTimeTick() - 10000;

			UpdateColorSpace(p);
			UpdateDirection(p);
			UpdateAlloc(p,BufCount,TempCount,BufCount+TempCount);
			Update(p);
		}
		else
		{
			p->M24VA_Deinitialise();
			p->M24VA = 0;
		}
	}
	p->Initing = 0;
}

static void Done(pvr* p)
{
	int No;

	if (p->VDISP)
	{
		p->VDISP_Deinitialise();
		p->VDISP = 0;
	}

	if (p->AllCount)
		p->M24VA_FrameFree(p->Buf,p->AllCount);

	for (No=0;No<p->AllCount;++No)
		SurfaceFree(p->BufMem[No]);

	p->AllCount = 0;
	p->TempCount = 0;
	p->BufCount = 0;

	if (p->M24VA)
	{
		p->M24VA_Deinitialise();
		p->M24VA = 0;
	}
}

static int Init(pvr* p)
{
	int Result = ERR_DEVICE_ERROR;

	p->BufCount = 0;
	p->AllCount = 0;
	p->TempCount = 0;
	p->YUV422 = 0;
#ifdef VSYNC_WORKAROUND
	p->DisableVSync = !p->SetupVSync;
#endif

	if (p->IDCTInit)
	{
		if (!PlanarYUV420(&p->p.Input.Format.Video.Pixel))
		{
			if (PlanarYUV422(&p->p.Input.Format.Video.Pixel))
				p->YUV422 = 1;
			else
				return ERR_NOT_SUPPORTED;
		}
	}

	p->Initing = 1;

	if (InitM24VA(p))
	{
		if (p->VDISP_Initialise(NULL) == VDISPError_OK)
		{
			p->VDISP = 1;
			p->IDCT = p->IDCTInit;

			p->FirstUpdate = 1;
			memset(&p->Attribs,0,sizeof(p->Attribs));

			p->Brightness = 0;
			p->Contrast = 0;
			p->Saturation = 0;
			p->RGBAdjust[0] = 0;
			p->RGBAdjust[1] = 0;
			p->RGBAdjust[2] = 0;

			p->FlipLastTime = GetTimeTick();
			p->RegBase = NULL;

			p->IDCTOutput.Node = (node*)p;
			p->IDCTOutput.No = OUT_INPUT;
			p->IDCTRounding = 0;
			p->Width16 = ALIGN16(p->IDCT ? p->IDCTWidth:p->p.Input.Format.Video.Width)-16;
			if (p->Width16<0) p->Width16=0;
			p->Height16 = ALIGN16(p->IDCT ? p->IDCTHeight:p->p.Input.Format.Video.Height)-16;
			if (p->Height16<0) p->Height16=0;

			if (p->IDCT)
			{
				if (p->p.Input.Format.Video.Direction & DIR_SWAPXY)
					SwapInt(&p->p.Input.Format.Video.Width,&p->p.Input.Format.Video.Height);
				p->p.Input.Format.Video.Direction = 0;
			}
			else
				if (p->p.Input.Format.Video.Direction & DIR_SWAPXY)
					SwapInt(&p->Width16,&p->Height16);

			p->ShowCurr = 0;
			p->ShowNext = -1;
			p->ShowLast = -1;

			GetMode(p);

			p->Overlay = p->p.Input.Format.Video;
			p->Overlay.Pitch = 0;
			p->Overlay.Pixel.Flags = PF_FOURCC;
			p->Overlay.Pixel.FourCC = FOURCC_IMC4;
			p->Overlay.Aspect = ASPECT_ONE;
			p->Overlay.Direction = -1; // force update in UpdateDirection
			p->Overlay.Width = 0;
			p->Overlay.Height = 0;

			UpdateColorSpace(p);
			UpdateDirection(p);

			Result = UpdateAlloc(p,max(3-p->IDCT,p->IDCTMemInit),0,p->IDCTMemInit);

		}
	}
	else
		Result = ERR_NOT_SUPPORTED;

	p->Initing = 0;

	if (Result != ERR_NONE)
		Done(p);

	return Result;
}

static int Update(pvr* p)
{	
	int NativeFX;
	rect FullScreen;
	VDISP_OVERLAYATTRIBS Attribs;
	bool_t Slept = UpdateDirection(p);

	if (p->Brightness != p->p.FX.Brightness ||
		p->Contrast != p->p.FX.Contrast ||
		p->Saturation != p->p.FX.Saturation ||
		p->RGBAdjust[0] != p->p.FX.RGBAdjust[0] ||
		p->RGBAdjust[1] != p->p.FX.RGBAdjust[1] ||
		p->RGBAdjust[2] != p->p.FX.RGBAdjust[2])
	{
		p->Brightness = p->p.FX.Brightness;
		p->Contrast = p->p.FX.Contrast;
		p->Saturation = p->p.FX.Saturation;
		p->RGBAdjust[0] = p->p.FX.RGBAdjust[0];
		p->RGBAdjust[1] = p->p.FX.RGBAdjust[1];
		p->RGBAdjust[2] = p->p.FX.RGBAdjust[2];
		UpdateColorSpace(p);
	}

	NativeFX = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->NativeDirection);
	p->p.PrefDirection = CombineDir(NativeFX,0,p->p.Input.Format.Video.Direction);

	PhyToVirt(NULL,&FullScreen,&p->p.Output.Format.Video);
	p->p.ColorKey = COLORKEY; 

	VirtToPhy(&p->p.Viewport,&p->p.DstAlignedRect,&p->p.Output.Format.Video);
	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);
	if (p->IDCT)
	{
		p->OverlayRect = p->p.SrcAlignedRect;
		if (p->SoftFX.Direction & DIR_SWAPXY)
			SwapInt(&p->OverlayRect.Width,&p->OverlayRect.Height);
		if (p->SoftFX.Direction & DIR_MIRRORLEFTRIGHT)
			p->OverlayRect.x += p->Overlay.Width - p->OverlayRect.Width;
		if (p->SoftFX.Direction & DIR_MIRRORUPDOWN)
			p->OverlayRect.y += p->Overlay.Height - p->OverlayRect.Height;
		p->p.Caps = 0; // no dither
	}
	else
	{
		VirtToPhy(NULL,&p->OverlayRect,&p->Overlay);
		BlitRelease(p->p.Soft);
		p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
		BlitAlign(p->p.Soft, &p->OverlayRect, &p->p.SrcAlignedRect);
	}

	p->p.Caps |= VC_BRIGHTNESS|VC_SATURATION|VC_CONTRAST|VC_RGBADJUST;
	AnyAlign(&p->p.DstAlignedRect, &p->OverlayRect, &p->OvlFX, 2, 2, SCALE_ONE/4, SCALE_ONE*8);
	PhyToVirt(&p->p.DstAlignedRect,&p->p.GUIAlignedRect,&p->p.Output.Format.Video);

	OverlayUpdateShow(&p->p,1);
	if (p->p.ColorKey != RGB_NULL)
		WinInvalidate(&p->p.Viewport,1);

	// setup overlay attributes
	if (p->AllCount>=2)
	{
		bool_t Show = p->p.Show && (!p->IDCT || (p->IDCTWidth>0 && p->IDCTHeight>0));
		memset(&Attribs,0,sizeof(Attribs));

		Attribs.bDisableRotation = 0;
		if (p->OvlFX.Direction & DIR_SWAPXY)
			Attribs.ui16RotateDegree = (gx_uint16)((p->OvlFX.Direction & DIR_MIRRORUPDOWN) ? 90:270);
		else
			Attribs.ui16RotateDegree = (gx_uint16)((p->OvlFX.Direction & DIR_MIRRORUPDOWN) ? 180:0);

		Attribs.bCKeyOn = 1;//p->p.ColorKey != RGB_NULL;
		Attribs.ui32CKeyValue = RGBToFormat(p->p.ColorKey,&p->p.Output.Format.Video.Pixel); 
		Attribs.bOverlayOn = Show;
		Attribs.i16Left = (gx_int16)p->p.DstAlignedRect.x;
		Attribs.i16Top = (gx_int16)p->p.DstAlignedRect.y;
		Attribs.i16Right = (gx_int16)(p->p.DstAlignedRect.Width + p->p.DstAlignedRect.x);
		Attribs.i16Bottom = (gx_int16)(p->p.DstAlignedRect.Height + p->p.DstAlignedRect.y);
		Attribs.ui16SrcX1 = (gx_uint16)p->OverlayRect.x;
		Attribs.ui16SrcY1 = (gx_uint16)p->OverlayRect.y;
		Attribs.ui16SrcX2 = (gx_uint16)(p->OverlayRect.Width + p->OverlayRect.x);
		Attribs.ui16SrcY2 = (gx_uint16)(p->OverlayRect.Height + p->OverlayRect.y);
		Attribs.ui16ValidFlags = 
			VDISP_OVERLAYATTRIB_VALID_CKEY | VDISP_OVERLAYATTRIB_VALID_VISIBILITY |
			VDISP_OVERLAYATTRIB_VALID_DSTPOSITION | VDISP_OVERLAYATTRIB_VALID_SRCPOSITION | 0x20 | 0x40;

		if (memcmp(&p->Attribs,&Attribs,sizeof(Attribs))!=0)
		{
			if (!Slept && !p->FirstUpdate)
				Sleep(50); // be sure last flipping finished

			if (Show)
			{
				if (!p->FirstUpdate) // already updated?
				{
					if (p->Attribs.bOverlayOn || p->Attribs.ui16RotateDegree != Attribs.ui16RotateDegree)
					{
						// we don't want to trash the screen when the new rotation attributes are set
						// disable overlay first and flip the current surface with the new settings

						// with X50v A03 ROM setting attribs when the overlay is on seem to make them move
						// let's turn overlay off before setting new src/dst rectangles 

						Attribs.bOverlayOn = 0;
						p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[0]);
						p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[1]);
						p->Empty = 1;
					}

					if (p->Empty)
					{
						p->VDISP_OverlayFlipSurface(p->Buf[p->ShowCurr],VDISP_NONE);
						Sleep(50); //wait flipping
					}

					Attribs.bOverlayOn = 1;
					p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[0]);
					p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[1]);
				}
				else
				{
					int Time = GetTimeTick();
					p->FirstUpdate = 0;

					if (p->FirstUpdateTime+500 < Time) // avoid this step if it was done recently
					{
						// first create a 100% (without this the overlay stais black after sleep mode)
						p->FirstUpdateTime = Time;

						p->Attribs = Attribs;
						p->Attribs.i16Left = 0;
						p->Attribs.i16Top = 0;
						p->Attribs.i16Right = 16;
						p->Attribs.i16Bottom = 16;
						p->Attribs.ui16SrcX1 = 0;
						p->Attribs.ui16SrcY1 = 0;
						p->Attribs.ui16SrcX2 = 16;
						p->Attribs.ui16SrcY2 = 16;
						p->Attribs.bOverlayOn = QueryPlatform(PLATFORM_VER) >= 500;

						p->VDISP_OverlaySetAttributes(&p->Attribs,p->Buf[0]);
						p->VDISP_OverlaySetAttributes(&p->Attribs,p->Buf[1]);
						p->VDISP_OverlayFlipSurface(p->Buf[p->ShowCurr],VDISP_NONE);
						Sleep(50);
					}

					p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[0]);
					p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[1]);
				}

				Sleep(20); //wait for overlay turn on
				p->Empty = 0;
			}
			else
			{
				p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[0]);
				p->VDISP_OverlaySetAttributes(&Attribs,p->Buf[1]);
				Sleep(20); //wait for overlay turn off
			}

			p->Attribs = Attribs;
		}
	}

	return ERR_NONE;
}

static int Lock(pvr* p,int No,planes Planes,bool_t UseReInit)
{
	GXVAError Result;

	Planes[1] = NULL;
	Planes[2] = NULL;

	if (No<0 || No>=p->AllCount)
	{
		Planes[0] = NULL;
		return ERR_INVALID_PARAM;
	}

	Result = p->M24VA_FrameBeginAccess(p->Buf[No],(gx_uint8**) &Planes[0],(gx_uint32*)&p->Overlay.Pitch,1);

	if (Result == GXVAError_HardwareNotAvailable && UseReInit && !p->Initing)
	{
		ReInit(p);
		if (p->M24VA) 
			Result = p->M24VA_FrameBeginAccess(p->Buf[No],(gx_uint8**) &Planes[0],(gx_uint32*)&p->Overlay.Pitch,1);
	}
	
	return (Result == GXVAError_OK) ? ERR_NONE : ERR_DEVICE_ERROR;
}

static void Unlock(pvr* p,int No)
{
	p->M24VA_FrameEndAccess(p->Buf[No]);
}

static int Blit(pvr* p, const constplanes Data, const constplanes DataLast )
{
	planes Planes;
	int Result;

	if (!p->M24VA)
		return ERR_DEVICE_ERROR;

	p->ShowLast = p->ShowCurr;
	if (++p->ShowCurr == p->AllCount)
		p->ShowCurr = 0;

	Result = Lock(p,p->ShowCurr,Planes,1);
	if (Result == ERR_NONE)
	{
		BlitImage(p->p.Soft,Planes,Data,DataLast,p->Overlay.Pitch,-1);

		Unlock(p,p->ShowCurr);

		if (p->p.CurrTime<0 && p->p.LastTime>=0)
		{
			int t = GetTimeTick();
			if (t-p->FlipLastTime<16) // it would just stall unneccessary the benchmark
				return ERR_NONE;
			p->FlipLastTime = t;
		}

		if (p->Attribs.bOverlayOn)
			p->VDISP_OverlayFlipSurface(p->Buf[p->ShowCurr],VDISP_NONE);
	}
	return Result;
}

static int UpdateShow(pvr* p)
{
	Update(p);
	return ERR_NONE;
}

static int Reset(pvr* p)
{
	GetMode(p);
	return ERR_NONE;
}

//------------------------------------------------------------

static const uint8_t ScanTable[3][64] = {
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

static const uint8_t ScanTable8x4[3][64*2] = {
{
	// source
	 0,  1,  8, 16,  9,  2,  3, 10, 
	17,	24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 
	27,	20, 13,  6,  7, 14, 21, 28, 
	35, 42,	49, 56, 57, 50, 43, 36, 
	29, 22,	15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61,	54, 47, 55, 62, 63,

	// dest
	 0,  1, 16, 32, 17,  2,  3, 18, 
	33,	48, 64, 49, 34, 19,  4,  5,
	20, 35, 50, 64, 64, 64, 64, 64, 
	51,	36, 21,  6,  7, 22, 37, 52, 
	64, 64,	64, 64, 64, 64, 64, 64, 
	53, 38,	23, 39, 54, 64, 64, 64, 
	64, 64, 64, 64, 64, 55, 64, 64, 
	64, 64, 64,	64, 64, 64, 64, 64

}};

//------------------------------------------------------------

#define FUNC(x)			x##_0
#define FA_XMASK		GXVA_FETCH_A_PREDX_MASK
#define FA_YMASK		GXVA_FETCH_A_PREDY_MASK
#define FA_XSHIFT		GXVA_FETCH_A_PREDX_SHIFT
#define FA_YSHIFT		GXVA_FETCH_A_PREDY_SHIFT
#define IW_XSHIFT		GXVA_IMAGE_WRITE_X_SHIFT
#define IW_YSHIFT		GXVA_IMAGE_WRITE_Y_SHIFT
#define MIRRORXY		
#define IZZDATA	 		v <<= 1;
#define MVDIRX			+
#define MVDIRY			+
#define MVPOS_00		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_00		0
#define MVPOS_01		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_01		2
#define MVPOS_10		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_10		4
#define MVPOS_11		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_11		6
#define	IDCT_00			GXVA_IDCT_MODE_BLK_TOP_LEFT
#define	IDCT_01			GXVA_IDCT_MODE_BLK_TOP_RGHT
#define	IDCT_10			GXVA_IDCT_MODE_BLK_BOT_LEFT
#define	IDCT_11			GXVA_IDCT_MODE_BLK_BOT_RGHT

#include "intel2700g_idct420.h"

#define FUNC(x)			x##_90
#define FA_XMASK		GXVA_FETCH_A_PREDY_MASK
#define FA_YMASK		GXVA_FETCH_A_PREDX_MASK
#define FA_XSHIFT		GXVA_FETCH_A_PREDY_SHIFT
#define FA_YSHIFT		GXVA_FETCH_A_PREDX_SHIFT
#define IW_XSHIFT		GXVA_IMAGE_WRITE_Y_SHIFT
#define IW_YSHIFT		GXVA_IMAGE_WRITE_X_SHIFT
#define MIRRORXY		x=PVR(p)->Width16-x;
#define IZZDATA	 		if (v & 1) { d=-d; } v = ((v & 7) << 4)|((v & 56) >> 2);
#define MVDIRX			-
#define MVDIRY			+
#define MVPOS_00		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_00		2
#define MVPOS_01		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_01		0
#define MVPOS_10		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_10		6
#define MVPOS_11		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_11		4
#define	IDCT_00			GXVA_IDCT_MODE_BLK_BOT_LEFT
#define	IDCT_01			GXVA_IDCT_MODE_BLK_TOP_LEFT
#define	IDCT_10			GXVA_IDCT_MODE_BLK_BOT_RGHT
#define	IDCT_11			GXVA_IDCT_MODE_BLK_TOP_RGHT

#include "intel2700g_idct420.h"

#define FUNC(x)			x##_180
#define FA_XMASK		GXVA_FETCH_A_PREDX_MASK
#define FA_YMASK		GXVA_FETCH_A_PREDY_MASK
#define FA_XSHIFT		GXVA_FETCH_A_PREDX_SHIFT
#define FA_YSHIFT		GXVA_FETCH_A_PREDY_SHIFT
#define IW_XSHIFT		GXVA_IMAGE_WRITE_X_SHIFT
#define IW_YSHIFT		GXVA_IMAGE_WRITE_Y_SHIFT
#define MIRRORXY		x=PVR(p)->Width16-x; y=PVR(p)->Height16-y;
#define IZZDATA	 		if ((v ^ (v >> 3)) & 1) { d=-d; } v <<= 1;
#define MVDIRX			-
#define MVDIRY			-
#define MVPOS_00		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_00		6
#define MVPOS_01		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_01		4
#define MVPOS_10		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_10		2
#define MVPOS_11		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_11		0
#define	IDCT_00			GXVA_IDCT_MODE_BLK_BOT_RGHT
#define	IDCT_01			GXVA_IDCT_MODE_BLK_BOT_LEFT
#define	IDCT_10			GXVA_IDCT_MODE_BLK_TOP_RGHT
#define	IDCT_11			GXVA_IDCT_MODE_BLK_TOP_LEFT

#include "intel2700g_idct420.h"

#define FUNC(x)			x##_270
#define FA_XMASK		GXVA_FETCH_A_PREDY_MASK
#define FA_YMASK		GXVA_FETCH_A_PREDX_MASK
#define FA_XSHIFT		GXVA_FETCH_A_PREDY_SHIFT
#define FA_YSHIFT		GXVA_FETCH_A_PREDX_SHIFT
#define IW_XSHIFT		GXVA_IMAGE_WRITE_Y_SHIFT
#define IW_YSHIFT		GXVA_IMAGE_WRITE_X_SHIFT
#define MIRRORXY		y=PVR(p)->Height16-y;
#define IZZDATA	 		if (v & 8) { d=-d; } v = ((v & 7) << 4)|((v & 56) >> 2);
#define MVDIRX			+
#define MVDIRY			-
#define MVPOS_00		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_00		4
#define MVPOS_01		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_0
#define MVIDX_01		6
#define MVPOS_10		GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_10		0
#define MVPOS_11		GXVA_FETCH_B_ACC_Y_POS_8|GXVA_FETCH_B_ACC_X_POS_8
#define MVIDX_11		2
#define	IDCT_00			GXVA_IDCT_MODE_BLK_TOP_RGHT
#define	IDCT_01			GXVA_IDCT_MODE_BLK_BOT_RGHT
#define	IDCT_10			GXVA_IDCT_MODE_BLK_TOP_LEFT
#define	IDCT_11			GXVA_IDCT_MODE_BLK_BOT_LEFT

#include "intel2700g_idct420.h"

//------------------------------------------------------------

#include "intel2700g_idct422.h"

//------------------------------------------------------------

static idctmcomp const IDCTMComp16x16[2][4] =
{{
	(idctmcomp)IDCTMComp16x16Back_0,
	(idctmcomp)IDCTMComp16x16Back_90,
	(idctmcomp)IDCTMComp16x16Back_180,
	(idctmcomp)IDCTMComp16x16Back_270
},{
	(idctmcomp)IDCTMComp16x16BackFwd_0,
	(idctmcomp)IDCTMComp16x16BackFwd_90,
	(idctmcomp)IDCTMComp16x16BackFwd_180,
	(idctmcomp)IDCTMComp16x16BackFwd_270
}};

static idctmcomp const IDCTMComp8x8[2][4] =
{{
	(idctmcomp)IDCTMComp8x8Back_0,
	(idctmcomp)IDCTMComp8x8Back_90,
	(idctmcomp)IDCTMComp8x8Back_180,
	(idctmcomp)IDCTMComp8x8Back_270
},{
	(idctmcomp)IDCTMComp8x8BackFwd_0,
	(idctmcomp)IDCTMComp8x8BackFwd_90,
	(idctmcomp)IDCTMComp8x8BackFwd_180,
	(idctmcomp)IDCTMComp8x8BackFwd_270
}};

static idctinter const IDCTInter8x8[2][4] =
{{
	(idctinter) IDCTInter8x8Back_0,
	(idctinter) IDCTInter8x8Back_90,
	(idctinter) IDCTInter8x8Back_180,
	(idctinter) IDCTInter8x8Back_270
},{
	(idctinter) IDCTInter8x8BackFwd_0,
	(idctinter) IDCTInter8x8BackFwd_90,
	(idctinter) IDCTInter8x8BackFwd_180,
	(idctinter) IDCTInter8x8BackFwd_270
}};

static idctprocess const IDCTProcess[4] = 
{ 
	(idctprocess)IDCTProcess_0,
	(idctprocess)IDCTProcess_90,
	(idctprocess)IDCTProcess_180,
	(idctprocess)IDCTProcess_270 
};
static idctcopy const IDCTCopy16x16[4] = 
{ 
	(idctcopy)IDCTCopy16x16_0,
	(idctcopy)IDCTCopy16x16_90,
	(idctcopy)IDCTCopy16x16_180,
	(idctcopy)IDCTCopy16x16_270 
};
static idctintra const IDCTIntra8x8[4] = 
{ 
	(idctintra)IDCTIntra8x8_0,
	(idctintra)IDCTIntra8x8_90,
	(idctintra)IDCTIntra8x8_180,
	(idctintra)IDCTIntra8x8_270 
};

//------------------------------------------------------------

static int IDCTOutputFormat(pvr* p, const void* Data, int Size )
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

static int IDCTPVRRestore(pvr* p, idctbackup* Backup)
{
	int No;
	if (p && Backup->Format.Pixel.Flags)
	{
		planes Planes;
		blitfx FX;

		memset(&FX,0,sizeof(FX));
		FX.ScaleX = SCALE_ONE;
		FX.ScaleY = SCALE_ONE;

		p->IDCTWidth = Backup->Width;
		p->IDCTHeight = Backup->Height;

		for (No=0;No<Backup->Count;++No)
		{
			idctbufferbackup* Buffer = Backup->Buffer+No;
			if (Buffer->Buffer[0] && Buffer->Format.Pixel.FourCC==FOURCC_IMC4)
				p->BufMem[No][0] = Buffer->Buffer[0];
			else
			{
				video Format;
				memset(&Format,0,sizeof(Format));
				Format.Pixel.Flags = PF_FOURCC;
				Format.Pixel.FourCC = FOURCC_IMC4;
				Format.Aspect = ASPECT_ONE;
				Format.Width = p->IDCTWidth;
				Format.Height = p->IDCTHeight;
				DefaultPitch(&Format);
				SurfaceAlloc(p->BufMem[No],&Format);
			}
		}

		p->IDCTMemInit = Backup->Count;
		IDCTOutputFormat(p,&Backup->Format,sizeof(video));
		p->IDCTMemInit = 0;

		for (No=0;No<Backup->Count;++No)
		{
			idctbufferbackup* Buffer = Backup->Buffer+No;

			p->BufFrameNo[No] = Buffer->FrameNo;

			if (Buffer->Buffer[0] && Lock(p,No,Planes,1) == ERR_NONE)
			{
				FX.Direction = CombineDir(Buffer->Format.Direction, 0, p->Overlay.Direction);
				FX.Brightness = -Buffer->Brightness;
				SurfaceCopy(&Buffer->Format,&p->Overlay,Buffer->Buffer,Planes,&FX);
				Unlock(p,No);
			}

			if (Buffer->Buffer[0] == p->BufMem[No][0])
				memset(Buffer->Buffer,0,sizeof(planes));
		}

		p->FirstUpdate = 1;

		p->ShowNext = Backup->Show;
		if (p->ShowNext >= p->BufCount)
			p->ShowNext = -1;
	}

	for (No=0;No<Backup->Count;++No)
		SurfaceFree(Backup->Buffer[No].Buffer);

	memset(Backup,0,sizeof(idctbackup));
	return ERR_NONE;
}

static int IDCTPVRBackup(pvr* p, idctbackup* Backup)
{
	int No;
	planes Planes;
	memset(Backup,0,sizeof(idctbackup));

	if (!p->IDCT)
		return ERR_INVALID_DATA;

	Backup->Format = p->p.Input.Format.Video;
	Backup->Width = p->IDCTWidth;
	Backup->Height = p->IDCTHeight;
	Backup->Count = p->BufCount;
	Backup->Show = p->ShowNext;

	p->Initing = 1; // do not ReInit here

	for (No=0;No<Backup->Count;++No)
	{
		idctbufferbackup* Buffer = Backup->Buffer+No;

		Buffer->FrameNo = p->BufFrameNo[No];

		BufferFormat(p,&Buffer->Format);
		Buffer->Buffer[0] = p->BufMem[No][0];
		memset(p->BufMem[No],0,sizeof(planes));

		if (Buffer->Buffer[0])
			if (Lock(p,No,Planes,0)==ERR_NONE)
			{
				SurfaceCopy(&p->Overlay,&Buffer->Format,Planes,Buffer->Buffer,NULL);
				Unlock(p,No);
			}
			else
				ClearIMC4(Buffer->Buffer[0],&Buffer->Format);
	}

	p->Initing = 0;

	IDCTOutputFormat(p,NULL,0);
	return ERR_NONE;

}

static int IDCTSet(void* p, int No, const void* Data, int Size )
{
	flowstate State;
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case IDCT_BACKUP: 
		if (Size == sizeof(idctbackup))
			Result = IDCTPVRRestore(PVR(p),(idctbackup*)Data);
		break;

	case IDCT_MODE: if (*(int*)Data==0) Result = ERR_NONE; break;
	case IDCT_BUFFERWIDTH: SETVALUE(PVR(p)->IDCTWidth,int,ERR_NONE); break;
	case IDCT_BUFFERHEIGHT: SETVALUE(PVR(p)->IDCTHeight,int,ERR_NONE); break;
	case IDCT_FORMAT:
		assert(Size == sizeof(video) || !Data);
		Result = IDCTOutputFormat(PVR(p),Data,Size);
		break;

	case IDCT_ROUNDING: SETVALUE(PVR(p)->IDCTRounding,bool_t,ERR_NONE); break;

	case IDCT_SHOW: 
		SETVALUE(PVR(p)->ShowNext,int,ERR_NONE); 
		if (PVR(p)->ShowNext >= PVR(p)->BufCount) PVR(p)->ShowNext = -1;
		break;

	case IDCT_BUFFERCOUNT:
		assert(Size == sizeof(int));
		Result = ERR_NONE;
		if (PVR(p)->BufCount != *(const int*)Data)
			Result = UpdateAlloc(PVR(p),*(const int*)Data,0,PVR(p)->AllCount);
		break;

	case FLOW_FLUSH:
		PVR(p)->ShowNext = -1;
		Result = ERR_NONE;
		break;

	case FLOW_RESEND:
		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;
		Result = IDCTSend(p,-1,&State);
		Sleep(20);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+PVR(p)->AllCount)
		SETVALUE(PVR(p)->BufFrameNo[No-IDCT_FRAMENO],int,ERR_NONE);
	return Result;
}

static int IDCTEnumPVR(void* p, int* No, datadef* Param)
{
	int Result = IDCTEnum(p,No,Param);
	if (Result == ERR_NONE && Param->No == IDCT_OUTPUT)
		Param->Flags |= DF_RDONLY;
	return Result;
}

static int IDCTGet(void* p, int No, void* Data, int Size )
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case FLOW_BUFFERED: GETVALUE(1,bool_t); break;
	case NODE_PARTOF: GETVALUE(PVR(p),pvr*); break;
	case IDCT_FORMAT: GETVALUECOND(PVR(p)->p.Input.Format.Video,video,PVR(p)->IDCT); break;
	case IDCT_OUTPUT|PIN_FORMAT: GETVALUECOND(PVR(p)->p.Input,packetformat,PVR(p)->IDCT); break;
	case IDCT_OUTPUT|PIN_PROCESS: GETVALUE(NULL,packetprocess); break;
	case IDCT_OUTPUT: GETVALUE(PVR(p)->IDCTOutput,pin); break;
	case IDCT_ROUNDING: GETVALUE(PVR(p)->IDCTRounding,bool_t); break;
	case IDCT_MODE: GETVALUE(0,int); break;
	case IDCT_BUFFERCOUNT: GETVALUE(PVR(p)->BufCount,int); break;
	case IDCT_BUFFERWIDTH: GETVALUE(PVR(p)->IDCTWidth,int); break;
	case IDCT_BUFFERHEIGHT: GETVALUE(PVR(p)->IDCTHeight,int); break;
	case IDCT_SHOW: GETVALUE(PVR(p)->ShowNext,int); break;
	case IDCT_BACKUP: 
		assert(Size == sizeof(idctbackup));
		Result = IDCTPVRBackup(PVR(p),(idctbackup*)Data);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+PVR(p)->AllCount)
		GETVALUE(PVR(p)->BufFrameNo[No-IDCT_FRAMENO],int);
	return Result;
}

static int IDCTNull(void* p,const flowstate* State,bool_t Empty)
{
	if (Empty)
		++PVR(p)->p.Total;
	if (!State || State->DropLevel)
		++PVR(p)->p.Dropped;
	return ERR_NONE;
}

static int IDCTLock(void* p,int No,planes Planes,int* Brightness,video* Format)
{
	if (Brightness)
		*Brightness = 0;
	if (Format)
		*Format = PVR(p)->Overlay;
	return Lock(PVR(p),No,Planes,1);
}

static void IDCTUnlock(void* p,int No)
{
	Unlock(PVR(p),No);
}

static void SwapPtr(pvr* p,int a,int b,int SwapShow)
{
	SMSurface t;
	void* u;
	
	t = p->Buf[a];
	p->Buf[a] = p->Buf[b];
	p->Buf[b] = t;

	u = p->BufMem[a][0];
	p->BufMem[a][0] = p->BufMem[b][0];
	p->BufMem[b][0] = u;

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

static int IDCTFrameStart(void* p,int FrameNo,int* OldFrameNo,int DstNo,int BackNo,int FwdNo,int ShowNo,bool_t Drop)
{
	bool_t ReTry = 0;
	GXVAError Result;

retry:

	if (!PVR(p)->M24VA)
		return ERR_DEVICE_ERROR;

	// we will try to avoid overwrite currently displayed buffers:
	//   ShowCurr currently displayed
	//   ShowLast may be still on screen (because flip occurs only during vblank)

	if (PVR(p)->ShowLast == DstNo || PVR(p)->ShowCurr == DstNo)
	{
		// try to find a free buffer
		int SwapNo;
		for (SwapNo=0;SwapNo<PVR(p)->AllCount;++SwapNo)
			if (SwapNo != PVR(p)->ShowLast && SwapNo != PVR(p)->ShowCurr && 
				SwapNo != BackNo && SwapNo != FwdNo && SwapNo != ShowNo)
				break; 

		if (SwapNo < PVR(p)->AllCount || UpdateAlloc(PVR(p),PVR(p)->BufCount,PVR(p)->TempCount+1,PVR(p)->AllCount)==ERR_NONE)
		{
			// use free buffer
			SwapPtr(PVR(p),SwapNo,DstNo,1);
		}
		else
		if (PVR(p)->ShowLast >= 0 && PVR(p)->ShowCurr != PVR(p)->ShowLast &&
			PVR(p)->ShowLast != BackNo && PVR(p)->ShowLast != FwdNo) 
		{
			// no free buffer found and couldn't allocate new temp buffer
			// so we wait for vblank and use ShowLast
			if (DstNo != PVR(p)->ShowLast)
				SwapPtr(PVR(p),PVR(p)->ShowLast,DstNo,1);
		}
	}

	Result = PVR(p)->M24VA_BeginFrame(PVR(p)->Buf[DstNo],1);
	if (Result == GXVAError_OK)
	{
		if (BackNo >= 0)
			Result = PVR(p)->M24VA_SetReferenceFrameAddress(1,PVR(p)->Buf[BackNo]);

		if (FwdNo >= 0 && Result == GXVAError_OK)
			Result = PVR(p)->M24VA_SetReferenceFrameAddress(2,PVR(p)->Buf[FwdNo]);

		if (Result != GXVAError_OK)
			PVR(p)->M24VA_EndFrame(1);
	}

	if (Result == GXVAError_HardwareNotAvailable && !ReTry)
	{
		ReInit(PVR(p));
		ReTry = 1;
		goto retry;
	}
	else
	if (Result == GXVAError_OK)
	{
		if (PVR(p)->YUV422)
		{
			PVR(p)->IdctVMT.Process = IDCTProcess_422;
			PVR(p)->IdctVMT.Intra8x8 = (idctintra)IDCTIntra8x8_422;
			PVR(p)->IdctVMT.Copy16x16 = NULL;
			PVR(p)->IdctVMT.MComp16x16 = NULL;
			PVR(p)->IdctVMT.MComp8x8 = NULL;
			PVR(p)->IdctVMT.Inter8x8 = NULL;
		}
		else
		{
			int Degree;
			if (PVR(p)->SoftFX.Direction & DIR_SWAPXY)
				Degree = (PVR(p)->SoftFX.Direction & DIR_MIRRORUPDOWN) ? 1:3;
			else
				Degree = (PVR(p)->SoftFX.Direction & DIR_MIRRORUPDOWN) ? 2:0;

			PVR(p)->IdctVMT.Process = IDCTProcess[Degree];
			PVR(p)->IdctVMT.Copy16x16 = IDCTCopy16x16[Degree];
			PVR(p)->IdctVMT.MComp16x16 = (idctmcomp)IDCTMComp16x16[FwdNo>=0][Degree];
			PVR(p)->IdctVMT.MComp8x8 = (idctmcomp)IDCTMComp8x8[FwdNo>=0][Degree];
			PVR(p)->IdctVMT.Intra8x8 = IDCTIntra8x8[Degree];
			PVR(p)->IdctVMT.Inter8x8 = IDCTInter8x8[FwdNo>=0][Degree];
		}
	}
	else
		return ERR_DEVICE_ERROR;

	PVR(p)->ShowNext = ShowNo;

	if (OldFrameNo)
		*OldFrameNo = PVR(p)->BufFrameNo[DstNo];
	PVR(p)->BufFrameNo[DstNo] = FrameNo;

	PVR(p)->MCompType = GXVA_CMD_MC_OPP_TYPE;
	if (PVR(p)->IDCTRounding)
		PVR(p)->MCompType |= GXVA_MC_TYPE_INTR_RND_MODE;

	PVR(p)->GXVA_SetIZZMode(GXVA_RasterScanOrder);

	return ERR_NONE;
}

static void IDCTFrameEnd(void* p)
{
	PVR(p)->M24VA_EndFrame(1);
}

static void IDCTDrop(void* p)
{
	int No;
	for (No=0;No<PVR(p)->AllCount;++No)
		PVR(p)->BufFrameNo[No] = -1;
}

static bool_t IDCTTiming(pvr* p, tick_t CurrTime, tick_t RefTime)
{
	if (CurrTime >= 0)
	{
		tick_t Diff;

		if (!p->p.Play && CurrTime == p->p.LastTime)
			return 0;

		Diff = RefTime - (CurrTime + SHOWAHEAD);
		if (Diff >= 0)
		{
#ifdef VSYNC_WORKAROUND
			bool_t Tried;
			int Last;
			int Same;
			int Start;

			if (p->DisableVSync || !p->VSyncPtr || Diff>(TICKSPERSEC*20/1000))
				return 0;

			Start = *p->VSyncPtr & 4095;

			if (Scale(p->VSyncRefresh,p->VSyncBottom - Start,p->VSyncTotal) > (TICKSPERSEC*8/1000))
				return 0;

			// wait for vsync

			Tried = 0;
			Last = -1;
			Same = 0;

			for (;;)
			{
				int Pos = *p->VSyncPtr & 4095;

				// fail safe for stuck scanline counter
				if (Pos == Last)
				{
					if (++Same == 10000000)
						break;
				}
				else
				{
					Last = Pos;
					Same = 0;
				}

				// we got interrupted and missed the vsync. 
				if (Pos < Start) 
				{
					if (Tried) 
						break;

					Start = Pos;
					Tried = 1; // try again, but only one more time
				}

				Pos = p->VSyncBottom - Pos;
				if (Pos <= 0) 
					break;

				if (Pos >= 2*p->VSyncLimit) 
					Sleep(Pos/p->VSyncLimit-1); // sleep few msec
			}
#else
			return 0;
#endif
		}

		p->p.LastTime = CurrTime;
	}
	else
		p->p.LastTime = RefTime;

	p->p.CurrTime = CurrTime;
	return 1;
}

static int IDCTSend(void* p,tick_t RefTime,const flowstate* State)
{
	// no dropping check, because flipping costs no time

	if (PVR(p)->ShowNext < 0)
		return ERR_NEED_MORE_DATA;

	if (!IDCTTiming(PVR(p),State->CurrTime,RefTime))
		return ERR_BUFFER_FULL;

	if (State->CurrTime != TIME_RESEND)
		++PVR(p)->p.Total;

	if (State->CurrTime<0 && RefTime>=0)
	{
		// it would just stall unneccessary the benchmark and
		// can crash the 2700G driver with too frequent flipping

		int t = GetTimeTick();
		if (t-PVR(p)->FlipLastTime<16)  
			return ERR_NONE;
		PVR(p)->FlipLastTime = t;
	}

	PVR(p)->ShowLast = PVR(p)->ShowCurr;
	PVR(p)->ShowCurr = PVR(p)->ShowNext;

	if (PVR(p)->Attribs.bOverlayOn)
		PVR(p)->VDISP_OverlayFlipSurface(PVR(p)->Buf[PVR(p)->ShowCurr],VDISP_NONE);
	return ERR_NONE;
}

//------------------------------------------------------------

static int Create(pvr* p)
{
	p->p.Module = LoadLibrary(T("PVRVADD.DLL"));

	GetProc(&p->p.Module,&p->M24VA_FrameDimensions,T("M24VA_FrameDimensions"),0);
	GetProc(&p->p.Module,&p->M24VA_FrameAllocate,T("M24VA_FrameAllocate"),0);
	GetProc(&p->p.Module,&p->M24VA_FrameFree,T("M24VA_FrameFree"),0);
	GetProc(&p->p.Module,&p->M24VA_BeginFrame,T("M24VA_BeginFrame"),0);
	GetProc(&p->p.Module,&p->M24VA_EndFrame,T("M24VA_EndFrame"),0);
	GetProc(&p->p.Module,&p->M24VA_SetReferenceFrameAddress,T("M24VA_SetReferenceFrameAddress"),0);
	GetProc(&p->p.Module,&p->M24VA_FCFrameAllocate,T("M24VA_FCFrameAllocate"),0);
	GetProc(&p->p.Module,&p->M24VA_FrameConvert,T("M24VA_FrameConvert"),0);
	GetProc(&p->p.Module,&p->M24VA_FrameBeginAccess,T("M24VA_FrameBeginAccess"),0);
	GetProc(&p->p.Module,&p->M24VA_FrameEndAccess,T("M24VA_FrameEndAccess"),0);
	GetProc(&p->p.Module,&p->M24VA_WriteIZZBlock,T("M24VA_WriteIZZBlock"),0);
	GetProc(&p->p.Module,&p->M24VA_WriteIZZData,T("M24VA_WriteIZZData"),0);
	GetProc(&p->p.Module,&p->M24VA_SetIZZMode,T("M24VA_SetIZZMode"),0);
	GetProc(&p->p.Module,&p->M24VA_WriteResidualDifferenceData,T("M24VA_WriteResidualDifferenceData"),0);
	GetProc(&p->p.Module,&p->M24VA_WriteMCCmdData,T("M24VA_WriteMCCmdData"),0);
	GetProc(&p->p.Module,&p->M24VA_Initialise,T("M24VA_Initialise"),0);
	GetProc(&p->p.Module,&p->M24VA_Deinitialise,T("M24VA_Deinitialise"),0);
	GetProc(&p->p.Module,&p->M24VA_Reset,T("M24VA_Reset"),0);
	GetProc(&p->p.Module,&p->VDISP_Initialise,T("VDISP_Initialise"),0);
	GetProc(&p->p.Module,&p->VDISP_Deinitialise,T("VDISP_Deinitialise"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlaySetAttributes,T("VDISP_OverlaySetAttributes"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlayFlipSurface,T("VDISP_OverlayFlipSurface"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlayContrast,T("VDISP_OverlayContrast"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlayGamma,T("VDISP_OverlayGamma"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlayBrightness,T("VDISP_OverlayBrightness"),0);
	GetProc(&p->p.Module,&p->VDISP_OverlaySetColorspaceConversion,T("VDISP_OverlaySetColorspaceConversion"),0);

	if (!p->p.Module)
		return ERR_NOT_SUPPORTED;

#ifdef VSYNC_WORKAROUND
	{
		tchar_t Path[MAXPATH];
		GetModuleFileName(p->p.Module,Path,MAXPATH);
		if (FileDate(Path) < 20041205) // just guessing when vsync was corrected
			p->SetupVSync = 1;
	}
#endif

	p->p.Node.Enum = (nodeenum)Enum;
	p->p.Node.Get = (nodeget)Get;
	p->p.Node.Set = (nodeset)Set;
	p->p.Init = (ovlfunc)Init;
	p->p.Done = (ovldone)Done;
	p->p.Blit = (ovlblit)Blit;
	p->p.Update = (ovlfunc)Update;
	p->p.UpdateShow = (ovlfunc)UpdateShow;
	p->p.Reset = (ovlfunc)Reset;

	p->IdctVMT.Class = INTEL2700G_IDCT_ID;
	p->IdctVMT.Enum = IDCTEnumPVR;
	p->IdctVMT.Get = IDCTGet;
	p->IdctVMT.Set = IDCTSet;
	p->IdctVMT.Send = IDCTSend;
	p->IdctVMT.Null = IDCTNull;
	p->IdctVMT.Drop = IDCTDrop;
	p->IdctVMT.Lock = IDCTLock;
	p->IdctVMT.Unlock = IDCTUnlock;
	p->IdctVMT.FrameStart = IDCTFrameStart;
	p->IdctVMT.FrameEnd = IDCTFrameEnd;

	p->p.AccelIDCT = &p->IdctVMT;
	p->p.Overlay = 1;
	p->p.DoPowerOff = 1;
	p->FirstUpdateTime = GetTimeTick() - 10000;

	p->SetupIDCTRotate = 1;
	p->CloseWMP = 1;
	return ERR_NONE;
}

static const nodedef PVR =
{
	sizeof(pvr)|CF_GLOBAL|CF_SETTINGS,
	INTEL2700G_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+210,
	(nodecreate)Create,
};

static const nodedef PVRIDCT =
{
	CF_ABSTRACT,
	INTEL2700G_IDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT+50,
};

void Intel2700G_Init()
{
	NodeRegisterClass(&PVR);
	NodeRegisterClass(&PVRIDCT);
}

void Intel2700G_Done()
{
	NodeUnRegisterClass(INTEL2700G_ID);
	NodeUnRegisterClass(INTEL2700G_IDCT_ID);
}

#else
void Intel2700G_Init() {}
void Intel2700G_Done() {}
#endif
