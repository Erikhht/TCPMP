#define DD_OK 0

typedef struct IDirectDraw	*LPDIRECTDRAW;
typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawClipper *LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawPalette *LPDIRECTDRAWPALETTE;

static const guid IID_IDirectDraw = { 0x9c59509a, 0x39bd, 0x11d1, { 0x8C,0x4A,0x00,0xC0,0x4F,0xD9,0x30,0xC5 }};

typedef struct DDSCAPS
{
    DWORD	dwCaps;

} DDSCAPS;

typedef DDSCAPS *LPDDSCAPS;

typedef struct DDCAPS
{
	DWORD	dwSize;
    DWORD	dwVidMemTotal;
    DWORD	dwVidMemFree;
    DWORD	dwVidMemStride;
    DDSCAPS ddsCaps;
    DWORD	dwNumFourCCCodes;
    DWORD	dwPalCaps;
    DWORD	dwBltCaps;
    DWORD	dwCKeyCaps;
    DWORD	dwAlphaCaps;
    DWORD	dwRops[8];
    DWORD   dwOverlayCaps;
    DWORD	dwMaxVisibleOverlays;
    DWORD	dwCurrVisibleOverlays;
    DWORD	dwAlignBoundarySrc;
    DWORD	dwAlignSizeSrc;
    DWORD	dwAlignBoundaryDest;
    DWORD	dwAlignSizeDest;
    DWORD	dwMinOverlayStretch;
    DWORD	dwMaxOverlayStretch;
    DWORD   dwMiscCaps;

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
    DWORD	dwAlphaConstBitDepth;
	DWORD	dwAlphaConst;
    DDCOLORKEY dckDestColorkey;
    DDCOLORKEY dckSrcColorkey;

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
    LONG    lXPitch;
	DWORD	dwBackBufferCount;
	DWORD	dwRefreshRate;
	LPVOID	lpSurface;
	DDCOLORKEY	ddckCKDestOverlay;
	DDCOLORKEY	ddckCKDestBlt;
	DDCOLORKEY	ddckCKSrcOverlay;
	DDCOLORKEY	ddckCKSrcBlt;
	DDPIXELFORMAT	ddpfPixelFormat;
	DDSCAPS	ddsCaps;
    DWORD	dwSurfaceSize;

} DDSURFACEDESC;

typedef DDSURFACEDESC *LPDDSURFACEDESC;

#define DDPF_FOURCC				0x00000004
#define DDPF_RGB				0x00000040
#define DDPF_PALETTEINDEXED		0x00000020

#define DDWAITVB_BLOCKBEGIN		0x00000001
#define DDWAITVB_BLOCKEND		0x00000004

#define DDSD_CAPS				0x00000001
#define DDSD_HEIGHT				0x00000002
#define DDSD_WIDTH				0x00000004
#define DDSD_PITCH				0x00000008
#define DDSD_XPITCH             0x00000010
#define DDSD_BACKBUFFERCOUNT	0x00000020
#define DDSD_PIXELFORMAT		0x00001000

#define DDSCAPS_FLIP			0x00000004
#define DDSCAPS_OVERLAY			0x00000010
#define DDSCAPS_PRIMARYSURFACE	0x00000040
#define DDSCAPS_VIDEOMEMORY		0x00000100

#define DDFXCAPS_BLTARITHSTRETCHY 0x00000020
#define DDFXCAPS_OVERLAYARITHSTRETCHY 0x00040000

#define DDBLT_WAITNOTBUSY       0x01000000
#define DDBLT_WAITVSYNC         0x00000001

#define DDLOCK_WAITNOTBUSY		0x00000008

#define DDBLTCAPS_FOURCCTORGB   0x00000004

#define DDOVERLAYCAPS_CKEYDEST        0x00000200
#define DDOVERLAYCAPS_MIRRORLEFTRIGHT 0x00000010
#define DDOVERLAYCAPS_MIRRORUPDOWN    0x00000020
#define DDOVERLAYCAPS_OVERLAYSUPPORT  0x80000000

#define DDOVER_HIDE				0x00000020
#define DDOVER_KEYDEST			0x00000040
#define DDOVER_SHOW				0x00000400
#define DDOVER_MIRRORLEFTRIGHT	0x00001000
#define DDOVER_MIRRORUPDOWN		0x00002000

#define DDCKEY_DESTOVERLAY		0x00000004

#define DDSCL_NORMAL			0x00000000
#define DDSCL_FULLSCREEN		0x00000001

#define DDERR_SURFACELOST		0x887601C2

#define DDFLIP_WAITNOTBUSY		0x00000008

#define DDENUMRET_CANCEL		0
#define DDENUMRET_OK			1

typedef struct IDirectDrawVMT
{
    HRESULT (STDCALL* QueryInterface)(struct IDirectDraw*, const guid*, LPVOID*);
    ULONG (STDCALL* AddRef)(struct IDirectDraw*);
    ULONG (STDCALL* Release)(struct IDirectDraw*);

    HRESULT (STDCALL* CreateClipper)(struct IDirectDraw*, DWORD, LPDIRECTDRAWCLIPPER*, void* );
    HRESULT (STDCALL* CreatePalette)(struct IDirectDraw*, DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE*, void* );
    HRESULT (STDCALL* CreateSurface)(struct IDirectDraw*,  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, void*);
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
    HRESULT (STDCALL* RestoreDisplayMode)(struct IDirectDraw*);
    HRESULT (STDCALL* SetCooperativeLevel)(struct IDirectDraw*, HWND, DWORD);
    HRESULT (STDCALL* SetDisplayMode)(struct IDirectDraw*, DWORD, DWORD,DWORD);
    HRESULT (STDCALL* WaitForVerticalBlank)(struct IDirectDraw*, DWORD, HANDLE);
    HRESULT (STDCALL* GetAvailableVidMem)(struct IDirectDraw*, LPDDSCAPS, LPDWORD, LPDWORD);
    HRESULT (STDCALL* GetSurfaceFromDC)(struct IDirectDraw*, HDC, LPDIRECTDRAWSURFACE*);
    HRESULT (STDCALL* RestoreAllSurfaces)(struct IDirectDraw*);
    HRESULT (STDCALL* TestCooperativeLevel)(struct IDirectDraw*);
    HRESULT (STDCALL* GetDeviceIdentifier)(struct IDirectDraw*,void*, DWORD );

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
#define IDirectDraw_WaitForVerticalBlank(p,a,b)	(p)->VMT->WaitForVerticalBlank(p,a,b)

typedef HRESULT (FAR PASCAL *LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);

typedef struct IDirectDrawSurfaceVMT
{
    HRESULT (STDCALL* QueryInterface)(struct IDirectDrawSurface*, void*, LPVOID*);
    ULONG (STDCALL* AddRef)(struct IDirectDrawSurface*);
    ULONG (STDCALL* Release)(struct IDirectDrawSurface*);

    HRESULT (STDCALL* AddOverlayDirtyRect)(struct IDirectDrawSurface*, LPRECT);
    HRESULT (STDCALL* Blt)(struct IDirectDrawSurface*, LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, void*);
    HRESULT (STDCALL* EnumAttachedSurfaces)(struct IDirectDrawSurface*, LPVOID,LPDDENUMSURFACESCALLBACK);
    HRESULT (STDCALL* EnumOverlayZOrders)(struct IDirectDrawSurface*, DWORD,LPVOID,void*);
    HRESULT (STDCALL* Flip)(struct IDirectDrawSurface*, LPDIRECTDRAWSURFACE, DWORD);
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
    HRESULT (STDCALL* UpdateOverlayZOrder)(struct IDirectDrawSurface*, DWORD, LPDIRECTDRAWSURFACE);
    HRESULT (STDCALL* GetDDInterface)(struct IDirectDrawSurface*, LPDIRECTDRAW *);
    HRESULT (STDCALL* AlphaBlt)(struct IDirectDrawSurface*, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, void*);

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
#define IDirectDrawSurface_Flip(p,a,b)					(p)->VMT->Flip(p,a,b)
#define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)	(p)->VMT->EnumAttachedSurfaces(p,a,b)

