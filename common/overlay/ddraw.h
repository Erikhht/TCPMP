#define DD_OK 0

typedef struct IDirectDraw	*LPDIRECTDRAW;
typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawClipper *LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawPalette *LPDIRECTDRAWPALETTE;

static const guid IID_IDirectDraw = { 0x6C14DB80, 0xA733, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 }};

typedef struct DDSCAPS
{
    DWORD	dwCaps;

} DDSCAPS;

typedef DDSCAPS *LPDDSCAPS;

typedef struct DDCAPS
{
	DWORD	dwSize;
	DWORD	dwCaps;
	DWORD	dwCaps2;
	DWORD	dwCKeyCaps;
	DWORD	dwFXCaps;
	DWORD	dwFXAlphaCaps;
	DWORD	dwPalCaps;
	DWORD	dwSVCaps;
	DWORD	dwAlphaBltConstBitDepths;
	DWORD	dwAlphaBltPixelBitDepths;
	DWORD	dwAlphaBltSurfaceBitDepths;
	DWORD	dwAlphaOverlayConstBitDepths;
	DWORD	dwAlphaOverlayPixelBitDepths;
	DWORD	dwAlphaOverlaySurfaceBitDepths;
	DWORD	dwZBufferBitDepths;
	DWORD	dwVidMemTotal;
	DWORD	dwVidMemFree;
	DWORD	dwMaxVisibleOverlays;
	DWORD	dwCurrVisibleOverlays;
	DWORD	dwNumFourCCCodes;
	DWORD	dwAlignBoundarySrc;
	DWORD	dwAlignSizeSrc;
	DWORD	dwAlignBoundaryDest;
	DWORD	dwAlignSizeDest;
	DWORD	dwAlignStrideAlign;
	DWORD	dwRops[8];
	DDSCAPS	ddsCaps;
	DWORD	dwMinOverlayStretch;
	DWORD	dwMaxOverlayStretch;
	DWORD	dwMinLiveVideoStretch;
	DWORD	dwMaxLiveVideoStretch;
	DWORD	dwMinHwCodecStretch;
	DWORD	dwMaxHwCodecStretch;
	DWORD	dwReserved1;
	DWORD	dwReserved2;
	DWORD	dwReserved3;
	DWORD	dwSVBCaps;
	DWORD	dwSVBCKeyCaps;
	DWORD	dwSVBFXCaps;
	DWORD	dwSVBRops[8];
	DWORD	dwVSBCaps;
	DWORD	dwVSBCKeyCaps;
	DWORD	dwVSBFXCaps;
	DWORD	dwVSBRops[8];
	DWORD	dwSSBCaps;
	DWORD	dwSSBCKeyCaps;
	DWORD	dwSSBFXCaps;
	DWORD	dwSSBRops[8];
	DWORD	dwReserved4;
	DWORD	dwReserved5;
	DWORD	dwReserved6;

} DDCAPS;

typedef DDCAPS *LPDDCAPS;

typedef struct DDCOLORKEY
{
    DWORD	dwColorSpaceLowValue;
    DWORD	dwColorSpaceHighValue;

} DDCOLORKEY;

typedef DDCOLORKEY *LPDDCOLORKEY;

typedef struct DDOVERLAYFX
{
    DWORD	dwSize;
    DWORD	dwAlphaEdgeBlendBitDepth;
    DWORD	dwAlphaEdgeBlend;
    DWORD	dwReserved;
    DWORD	dwAlphaDestConstBitDepth;
	DWORD	dwAlphaDestConst;
    DWORD	dwAlphaSrcConstBitDepth;
	DWORD	dwAlphaSrcConst;
    DDCOLORKEY dckDestColorkey;
    DDCOLORKEY dckSrcColorkey;
    DWORD   dwDDFX;
    DWORD	dwFlags;

} DDOVERLAYFX;

typedef DDOVERLAYFX *LPDDOVERLAYFX;

typedef struct DDPIXELFORMAT
{
	DWORD	dwSize;
	DWORD	dwFlags;
	DWORD	dwFourCC;
	DWORD	dwRGBBitCount;
	DWORD	dwRBitMask;
	DWORD	dwGBitMask;
	DWORD	dwBBitMask;
	DWORD	dwRGBAlphaBitMask;

} DDPIXELFORMAT;

typedef DDPIXELFORMAT *LPDDPIXELFORMAT;

typedef struct DDSURFACEDESC
{
	DWORD	dwSize;
	DWORD	dwFlags;
	DWORD	dwHeight;
	DWORD	dwWidth;
	LONG	lPitch;
	DWORD	dwBackBufferCount;
	DWORD	dwRefreshRate;
	DWORD	dwAlphaBitDepth;
	DWORD	dwReserved;
	LPVOID	lpSurface;
	DDCOLORKEY	ddckCKDestOverlay;
	DDCOLORKEY	ddckCKDestBlt;
	DDCOLORKEY	ddckCKSrcOverlay;
	DDCOLORKEY	ddckCKSrcBlt;
	DDPIXELFORMAT	ddpfPixelFormat;
	DDSCAPS	ddsCaps;

} DDSURFACEDESC;

typedef DDSURFACEDESC *LPDDSURFACEDESC;

#define DDPF_FOURCC				0x00000004
#define DDPF_RGB				0x00000040
#define DDPF_PALETTEINDEXED4	0x00000008
#define DDPF_PALETTEINDEXED8	0x00000020
#define DDPF_PALETTEINDEXED1	0x00000800
#define DDPF_PALETTEINDEXED2	0x00001000

#define DDSD_CAPS				0x00000001
#define DDSD_HEIGHT				0x00000002
#define DDSD_WIDTH				0x00000004
#define DDSD_PITCH				0x00000008
#define DDSD_PIXELFORMAT		0x00001000
#define DDSD_BACKBUFFERCOUNT	0x00000020

#define DDCAPS_ALIGNBOUNDARYDEST 0x00000002
#define DDCAPS_ALIGNSIZEDEST	0x00000004
#define DDCAPS_BLTSTRETCH		0x00000200
#define DDCAPS_OVERLAY			0x00000800
#define DDCAPS_OVERLAYSTRETCH	0x00004000
#define DDCAPS_COLORKEY			0x00400000

#define DDSCAPS_BACKBUFFER		0x00000004
#define DDSCAPS_COMPLEX			0x00000008
#define DDSCAPS_FLIP			0x00000010
#define DDSCAPS_OVERLAY			0x00000080
#define DDSCAPS_PRIMARYSURFACE	0x00000200
#define DDSCAPS_VIDEOMEMORY		0x00004000

#define DDFXCAPS_BLTARITHSTRETCHY 0x00000020
#define DDFXCAPS_OVERLAYARITHSTRETCHY 0x00040000

#define DDOVERFX_ARITHSTRETCHY	0x00000001

#define DDLOCK_WAIT				0x00000001

#define DDBLT_ASYNC				0x00000200

#define DDOVER_HIDE				0x00000200
#define DDOVER_KEYDEST			0x00000400
#define DDOVER_KEYDESTOVERRIDE  0x00000800
#define DDOVER_KEYSRC           0x00001000
#define DDOVER_KEYSRCOVERRIDE   0x00002000
#define DDOVER_SHOW				0x00004000
#define DDOVER_DDFX             0x00080000

#define DDCKEY_DESTOVERLAY		0x00000004

#define DDSCL_NORMAL			0x00000008
#define DDSCL_FULLSCREEN		0x00000001
#define DDSCL_EXCLUSIVE         0x00000010

#define DDFLIP_WAIT				0x00000001
#define DDFLIP_NOVSYNC			0x00000008

#define DDERR_SURFACELOST		0x887601C2

typedef struct IDirectDrawVMT
{
    HRESULT (STDCALL* QueryInterface)(struct IDirectDraw*, const guid*, LPVOID*);
    ULONG (STDCALL* AddRef)(struct IDirectDraw*);
    ULONG (STDCALL* Release)(struct IDirectDraw*);

    HRESULT (STDCALL* Compact)(struct IDirectDraw*);
    HRESULT (STDCALL* CreateClipper)(struct IDirectDraw*, DWORD, LPDIRECTDRAWCLIPPER*, void* );
    HRESULT (STDCALL* CreatePalette)(struct IDirectDraw*, DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE*, void* );
    HRESULT (STDCALL* CreateSurface)(struct IDirectDraw*,  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, void*);
    HRESULT (STDCALL* DuplicateSurface)( struct IDirectDraw*, LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE* );
    HRESULT (STDCALL* EnumDisplayModes)( struct IDirectDraw*, DWORD, LPDDSURFACEDESC, LPVOID, void* );
    HRESULT (STDCALL* EnumSurfaces)(struct IDirectDraw*, DWORD, LPDDSURFACEDESC, LPVOID, void* );
    HRESULT (STDCALL* FlipToGDISurface)(struct IDirectDraw*);
    HRESULT (STDCALL* GetCaps)( struct IDirectDraw*, LPDDCAPS, LPDDCAPS);
    HRESULT (STDCALL* GetDisplayMode)( struct IDirectDraw*, LPDDSURFACEDESC);
    HRESULT (STDCALL* GetFourCCCodes)(struct IDirectDraw*,  LPDWORD, LPDWORD );
    HRESULT (STDCALL* GetGDISurface)(struct IDirectDraw*, LPDIRECTDRAWSURFACE*);
    HRESULT (STDCALL* GetMonitorFrequency)(struct IDirectDraw*, LPDWORD);
    HRESULT (STDCALL* GetScanLine)(struct IDirectDraw*, LPDWORD);
    HRESULT (STDCALL* GetVerticalBlankStatus)(struct IDirectDraw*, LPBOOL );
    HRESULT (STDCALL* Initialize)(struct IDirectDraw*, GUID*);
    HRESULT (STDCALL* RestoreDisplayMode)(struct IDirectDraw*);
    HRESULT (STDCALL* SetCooperativeLevel)(struct IDirectDraw*, HWND, DWORD);
    HRESULT (STDCALL* SetDisplayMode)(struct IDirectDraw*, DWORD, DWORD,DWORD);
    HRESULT (STDCALL* WaitForVerticalBlank)(struct IDirectDraw*, DWORD, HANDLE );

} IDirectDrawVMT;

typedef struct IDirectDraw
{
	IDirectDrawVMT *VMT;

} IDirectDraw;

#define IDirectDraw_QueryInterface(p,a,b)		(p)->VMT->QueryInterface(p,a,b)
#define IDirectDraw_Release(p)					(p)->VMT->Release(p)
#define IDirectDraw_CreateSurface(p,a,b,c)		(p)->VMT->CreateSurface(p,a,b,c)
#define IDirectDraw_GetDisplayMode(p,a)         (p)->VMT->GetDisplayMode(p,a)
#define IDirectDraw_SetCooperativeLevel(p,a,b)  (p)->VMT->SetCooperativeLevel(p,a,b)
#define IDirectDraw_GetCaps(p,a,b)              (p)->VMT->GetCaps(p,a,b)
#define IDirectDraw_GetFourCCCodes(p,a,b)		(p)->VMT->GetFourCCCodes(p,a,b)
#define IDirectDraw_CreateClipper(p,a,b,c)		(p)->VMT->CreateClipper(p,a,b,c)

typedef struct IDirectDrawSurfaceVMT
{
    HRESULT (STDCALL* QueryInterface)(struct IDirectDrawSurface*, void*, LPVOID*);
    ULONG (STDCALL* AddRef)(struct IDirectDrawSurface*);
    ULONG (STDCALL* Release)(struct IDirectDrawSurface*);

    HRESULT (STDCALL* AddAttachedSurface)(struct IDirectDrawSurface*, LPDIRECTDRAWSURFACE);
    HRESULT (STDCALL* AddOverlayDirtyRect)(struct IDirectDrawSurface*, LPRECT);
    HRESULT (STDCALL* Blt)(struct IDirectDrawSurface*, LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, void*);
    HRESULT (STDCALL* BltBatch)(struct IDirectDrawSurface*, void*, DWORD, DWORD );
    HRESULT (STDCALL* BltFast)(struct IDirectDrawSurface*, DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD);
    HRESULT (STDCALL* DeleteAttachedSurface)(struct IDirectDrawSurface*, DWORD,LPDIRECTDRAWSURFACE);
    HRESULT (STDCALL* EnumAttachedSurfaces)(struct IDirectDrawSurface*, LPVOID,void*);
    HRESULT (STDCALL* EnumOverlayZOrders)(struct IDirectDrawSurface*, DWORD,LPVOID,void*);
    HRESULT (STDCALL* Flip)(struct IDirectDrawSurface*, LPDIRECTDRAWSURFACE, DWORD);
    HRESULT (STDCALL* GetAttachedSurface)(struct IDirectDrawSurface*, LPDDSCAPS, LPDIRECTDRAWSURFACE*);
    HRESULT (STDCALL* GetBltStatus)(struct IDirectDrawSurface*, DWORD);
    HRESULT (STDCALL* GetCaps)(struct IDirectDrawSurface*, LPDDSCAPS);
    HRESULT (STDCALL* GetClipper)(struct IDirectDrawSurface*, LPDIRECTDRAWCLIPPER*);
    HRESULT (STDCALL* GetColorKey)(struct IDirectDrawSurface*, DWORD, LPDDCOLORKEY);
    HRESULT (STDCALL* GetDC)(struct IDirectDrawSurface*, HDC*);
    HRESULT (STDCALL* GetFlipStatus)(struct IDirectDrawSurface*, DWORD);
    HRESULT (STDCALL* GetOverlayPosition)(struct IDirectDrawSurface*, LPLONG, LPLONG );
    HRESULT (STDCALL* GetPalette)(struct IDirectDrawSurface*, LPDIRECTDRAWPALETTE*);
    HRESULT (STDCALL* GetPixelFormat)(struct IDirectDrawSurface*, LPDDPIXELFORMAT);
    HRESULT (STDCALL* GetSurfaceDesc)(struct IDirectDrawSurface*, LPDDSURFACEDESC);
    HRESULT (STDCALL* Initialize)(struct IDirectDrawSurface*, LPDIRECTDRAW, LPDDSURFACEDESC);
    HRESULT (STDCALL* IsLost)(struct IDirectDrawSurface*);
    HRESULT (STDCALL* Lock)(struct IDirectDrawSurface*, LPRECT,LPDDSURFACEDESC,DWORD,HANDLE);
    HRESULT (STDCALL* ReleaseDC)(struct IDirectDrawSurface*, HDC);
    HRESULT (STDCALL* Restore)(struct IDirectDrawSurface*);
    HRESULT (STDCALL* SetClipper)(struct IDirectDrawSurface*, LPDIRECTDRAWCLIPPER);
    HRESULT (STDCALL* SetColorKey)(struct IDirectDrawSurface*, DWORD, LPDDCOLORKEY);
    HRESULT (STDCALL* SetOverlayPosition)(struct IDirectDrawSurface*, LONG, LONG );
    HRESULT (STDCALL* SetPalette)(struct IDirectDrawSurface*, LPDIRECTDRAWPALETTE);
    HRESULT (STDCALL* Unlock)(struct IDirectDrawSurface*, LPVOID);
    HRESULT (STDCALL* UpdateOverlay)(struct IDirectDrawSurface*, LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX);
    HRESULT (STDCALL* UpdateOverlayDisplay)(struct IDirectDrawSurface*, DWORD);
    HRESULT (STDCALL* UpdateOverlayZOrder)(struct IDirectDrawSurface*, DWORD, LPDIRECTDRAWSURFACE);

} IDirectDrawSurfaceVMT;

typedef struct IDirectDrawSurface
{
	IDirectDrawSurfaceVMT *VMT;

} IDirectDrawSurface;

#define IDirectDrawSurface_Release(p)					(p)->VMT->Release(p)
#define IDirectDrawSurface_Blt(p,a,b,c,d,e)				(p)->VMT->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface_GetSurfaceDesc(p,a)			(p)->VMT->GetSurfaceDesc(p,a)
#define IDirectDrawSurface_Lock(p,a,b,c,d)				(p)->VMT->Lock(p,a,b,c,d)
#define IDirectDrawSurface_Unlock(p,b)					(p)->VMT->Unlock(p,b)
#define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)	(p)->VMT->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->VMT->SetColorKey(p,a,b)
#define IDirectDrawSurface_Restore(p)                   (p)->VMT->Restore(p)
#define IDirectDrawSurface_GetAttachedSurface(p,a,b)	(p)->VMT->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface_Flip(p,a,b)					(p)->VMT->Flip(p,a,b)
#define IDirectDrawSurface_SetClipper(p,a)				(p)->VMT->SetClipper(p,a)

typedef struct IDirectDrawClipperVMT
{
    HRESULT (STDCALL* QueryInterface)(struct IDirectDrawClipper*, void*, LPVOID*);
    ULONG (STDCALL* AddRef)(struct IDirectDrawClipper*);
    ULONG (STDCALL* Release)(struct IDirectDrawClipper*);

    HRESULT (STDCALL* GetClipList)(struct IDirectDrawClipper*, LPRECT, LPRGNDATA, LPDWORD);
    HRESULT (STDCALL* GetHWnd)(struct IDirectDrawClipper*, HWND*);
    HRESULT (STDCALL* Initialize)(struct IDirectDrawClipper*, LPDIRECTDRAW, DWORD);
    HRESULT (STDCALL* IsClipListChanged)(struct IDirectDrawClipper*, BOOL*);
    HRESULT (STDCALL* SetClipList)(struct IDirectDrawClipper*, LPRGNDATA,DWORD);
    HRESULT (STDCALL* SetHWnd)(struct IDirectDrawClipper*, DWORD, HWND);

} IDirectDrawClipperVMT;

typedef struct IDirectDrawClipper
{
	IDirectDrawClipperVMT *VMT;

} IDirectDrawClipper;

#define IDirectDrawClipper_Release(p)					(p)->VMT->Release(p)
#define IDirectDrawClipper_SetHWnd(p,a,b)				(p)->VMT->SetHWnd(p,a,b)
