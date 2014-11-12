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
 * $Id: dmo_win32.c 622 2006-01-31 19:02:53Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#define _WIN32_WINNT 0x0400
#define INITGUID
#include <objbase.h>

#define DMO_CLASS		FOURCC('A','D','M','O')

// string table
#define DMO_MODULE		0x200
#define DMO_MODULEPPC	0x201
#define DMO_CLASSID		0x202

// registry
#define DMO_FOUND		0x400

#define WMV_ID			FOURCC('W','M','V','_')
#define WMS_ID			FOURCC('W','M','S','_')
#define WMVA_ID			FOURCC('W','M','V','A')
#define WMA_ID			FOURCC('W','M','A','_')
#define WMAV_ID			FOURCC('W','M','A','V')

#define DMO_INPUT_STATUSF_ACCEPT_DATA				0x00000001

#define DMO_INPUT_DATA_BUFFERF_SYNCPOINT			0x00000001
#define DMO_INPUT_DATA_BUFFERF_TIME					0x00000002
#define DMO_INPUT_DATA_BUFFERF_TIMELENGTH			0x00000004

#define DMO_INPUT_STREAMF_WHOLE_SAMPLES				0x00000001
#define DMO_INPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER	0x00000002
#define DMO_INPUT_STREAMF_FIXED_SAMPLE_SIZE			0x00000004
#define DMO_INPUT_STREAMF_HOLDS_BUFFERS				0x00000008

#define DMO_SET_TYPEF_TEST_ONLY						0x00000001
#define DMO_SET_TYPEF_CLEAR							0x00000002

#define DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER	0x00000001

#define	DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT			0x00000001
#define	DMO_OUTPUT_DATA_BUFFERF_TIME				0x00000002
#define	DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH			0x00000004
#define	DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE			0x01000000

const guid ID_IMEDIABUFFER = {0x59eff8b9,0x938c,0x4a26,{0x82,0xf2,0x95,0xcb,0x84,0xcd,0xc8,0x37}};
const guid ID_IUNKNOWN = {0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const guid ID_ICLASSFACTORY = {00000001,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const guid ID_IDMOQUALITYCONTROL = {0x65abea96,0xcf36,0x453f,{0xaf,0x8a,0x70,0x5e,0x98,0xf1,0x62,0x60}};
const guid WMCMEDIATYPE_Video = {0x73646976, 0x0000, 0x0010,{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const guid WMCMEDIATYPE_Audio = {0x73647561, 0x0000, 0x0010,{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const guid WMCFORMAT_VideoInfo = {0x05589f80, 0xc356, 0x11ce,{0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const guid WMCFORMAT_WaveFormatEx = {0x05589f81, 0xc356, 0x11ce,{0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const guid WMCMEDIASUBTYPE = {0x00000000,0x0000, 0x0010,{0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
const guid WMCMEDIASUBTYPE_RGB565 = {0xe436eb7b, 0x524f, 0x11ce,{0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const guid IID_IMediaObject = { 0xd8ad0f58, 0x5494, 0x4102,{0x97, 0xc5, 0xec, 0x79, 0x8e, 0x59, 0xbc, 0xf4}};

typedef int64_t REFERENCE_TIME;

struct IMediaObject;
struct IMediaBuffer;
struct IDMOQualityControl;

typedef struct VIDEOINFOHEADER 
{
	RECT rcSource;
	RECT rcTarget;
	DWORD dwBitRate;
	DWORD dwBitErrorRate;
	REFERENCE_TIME AvgTimePerFrame;
	BITMAPINFOHEADER bmiHeader;

} VIDEOINFOHEADER;

typedef struct DMO_MEDIA_TYPE
{
    guid majortype;
    guid subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    guid formattype;
    IUnknown* pUnk;
    ULONG cbFormat;
    BYTE* pbFormat;

} DMO_MEDIA_TYPE;

typedef struct DMO_OUTPUT_DATA_BUFFER
{
    struct IMediaBuffer* pBuffer;
    DWORD dwStatus;
    REFERENCE_TIME rtTimestamp;
    REFERENCE_TIME rtTimelength;

} DMO_OUTPUT_DATA_BUFFER;

typedef struct IMediaBufferVMT
{
    HRESULT (STDCALL *QueryInterface)(void* This, const guid* riid, void **ppvObject);    
    ULONG (STDCALL *AddRef)(void* This);     
    ULONG (STDCALL *Release)(void* This);    
    HRESULT (STDCALL *SetLength)(void* This, DWORD cbLength);    
    HRESULT (STDCALL *GetMaxLength)(void* This, DWORD *pcbMaxLength);    
    HRESULT (STDCALL *GetBufferAndLength)(void* This, BYTE **ppBuffer, DWORD *pcbLength);
    
} IMediaBufferVMT;

typedef struct IMediaBuffer
{
    struct IMediaBufferVMT *VMT;

} IMediaBuffer;

typedef struct IDMOQualityControlVMT
{
    HRESULT (STDCALL *QueryInterface)(struct IDMOQualityControl* This, const guid* riid, void** ppvObject);
    ULONG (STDCALL *AddRef)(struct IDMOQualityControl* This);
    ULONG (STDCALL *Release)(struct IDMOQualityControl* This);
    HRESULT (STDCALL *SetNow)(struct IDMOQualityControl* This, REFERENCE_TIME rtNow);
    HRESULT (STDCALL *SetStatus)(struct IDMOQualityControl* This, DWORD dwFlags);
    HRESULT (STDCALL *GetStatus)(struct IDMOQualityControl* This, DWORD *pdwFlags);

} IDMOQualityControlVMT;

typedef struct IDMOQualityControl
{
	IDMOQualityControlVMT *VMT;

} IDMOQualityControl;
 
typedef struct IMediaObjectVMT
{
    HRESULT (STDCALL *QueryInterface)(struct IMediaObject * This, const guid* riid, void** ppvObject);
    ULONG (STDCALL *AddRef)(struct IMediaObject * This);
    ULONG (STDCALL *Release)(struct IMediaObject * This);
    HRESULT (STDCALL *GetStreamCount)(struct IMediaObject * This, DWORD *pcInputStreams, DWORD *pcOutputStreams);
    HRESULT (STDCALL *GetInputStreamInfo)(struct IMediaObject * This, DWORD dwInputStreamIndex, DWORD *pdwFlags);
    HRESULT (STDCALL *GetOutputStreamInfo)(struct IMediaObject * This, DWORD dwOutputStreamIndex, DWORD *pdwFlags);    
	HRESULT (STDCALL *GetInputType)(struct IMediaObject * This, DWORD dwInputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt);
    HRESULT (STDCALL *GetOutputType)(struct IMediaObject * This, DWORD dwOutputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt);
    HRESULT (STDCALL *SetInputType)(struct IMediaObject * This, DWORD dwInputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags);    
    HRESULT (STDCALL *SetOutputType)(struct IMediaObject * This, DWORD dwOutputStreamIndex,const DMO_MEDIA_TYPE *pmt, DWORD dwFlags);    
    HRESULT (STDCALL *GetInputCurrentType)(struct IMediaObject * This, DWORD dwInputStreamIndex, DMO_MEDIA_TYPE *pmt);    
    HRESULT (STDCALL *GetOutputCurrentType)(struct IMediaObject * This, DWORD dwOutputStreamIndex, DMO_MEDIA_TYPE *pmt);    
    HRESULT (STDCALL *GetInputSizeInfo)(struct IMediaObject * This, DWORD dwInputStreamIndex, DWORD *pcbSize, DWORD *pcbMaxLookahead, DWORD *pcbAlignment);
    HRESULT (STDCALL *GetOutputSizeInfo)(struct IMediaObject * This, DWORD dwOutputStreamIndex, DWORD *pcbSize, DWORD *pcbAlignment);     
    HRESULT (STDCALL *GetInputMaxLatency)(struct IMediaObject * This, DWORD dwInputStreamIndex, REFERENCE_TIME *prtMaxLatency);    
    HRESULT (STDCALL *SetInputMaxLatency)(struct IMediaObject * This, DWORD dwInputStreamIndex, REFERENCE_TIME rtMaxLatency);    
    HRESULT (STDCALL *Flush)(struct IMediaObject * This);
    HRESULT (STDCALL *Discontinuity)(struct IMediaObject * This, DWORD dwInputStreamIndex);
    HRESULT (STDCALL *AllocateStreamingResources)(struct IMediaObject * This);    
    HRESULT (STDCALL *FreeStreamingResources)(struct IMediaObject * This);    
    HRESULT (STDCALL *GetInputStatus)(struct IMediaObject * This,DWORD dwInputStreamIndex,DWORD *dwFlags);    
    HRESULT (STDCALL *ProcessInput)(struct IMediaObject * This, DWORD dwInputStreamIndex, IMediaBuffer *pBuffer, DWORD dwFlags, REFERENCE_TIME rtTimestamp, REFERENCE_TIME rtTimelength);    
    HRESULT (STDCALL *ProcessOutput)(struct IMediaObject * This, DWORD dwFlags, DWORD cOutputBufferCount, DMO_OUTPUT_DATA_BUFFER *pOutputBuffers, DWORD *pdwStatus);   
    HRESULT (STDCALL *Lock)(struct IMediaObject * This, LONG bLock);
    
} IMediaObjectVMT;

typedef struct IMediaObject
{
    IMediaObjectVMT *VMT;

} IMediaObject;

#define DMO_INPUT		0x100
#define DMO_OUTPUT		0x101

typedef struct IOwnMediaBuffer
{
    struct IMediaBufferVMT *VMT;

	int RefCount;
	int Length;
	int Allocated; // -1 for external allocated buffer
	void* Data;

	struct IOwnMediaBuffer** Chain;
	struct IOwnMediaBuffer* Next;

} IOwnMediaBuffer;

typedef struct dmo
{
	codec Codec;

	HMODULE Module;
	guid ClassId;
	int (STDCALL *CreateCodecDMO)(IMediaObject**);
	int (STDCALL *CreateCodecDMOEx)(int Code,IMediaObject**);
	int (STDCALL *QueryUnloadCodecDMO)();
	int (STDCALL *DllGetClassObject)(const guid*,const guid*,IClassFactory**);
	int (STDCALL *DllCanUnloadNow)();

	IMediaObject* Media;
	IOwnMediaBuffer* InputBuffers;
	DMO_OUTPUT_DATA_BUFFER OutputBuffer[2];
	int OutputNoMask;
	int OutputNext;
	int OutputPrev;

	DMO_MEDIA_TYPE InType;
	DMO_MEDIA_TYPE OutType;
	int ConstSize;
	BITMAPINFOHEADER* Const;

	int WMPVersion;
	bool_t First;
	bool_t UseDropping;

} dmo;

static HRESULT STDCALL MoInitMediaType(DMO_MEDIA_TYPE* p, DWORD Size)
{
	p->pUnk = NULL;
	p->pbFormat = NULL;
	p->cbFormat = Size;
	if (Size)
	{
		p->pbFormat = CoTaskMemAlloc(Size);
		if (!p->pbFormat)
		{
			p->cbFormat = 0;
			return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

static HRESULT STDCALL MoFreeMediaType(DMO_MEDIA_TYPE* p)
{
	if (p->pUnk)
	{
		p->pUnk->lpVtbl->Release(p->pUnk);
		p->pUnk = NULL;
	}
	if (p->pbFormat)
	{
		CoTaskMemFree(p->pbFormat);
		p->pbFormat = NULL;
	}
	p->cbFormat = 0;
	return S_OK;
}

static HRESULT STDCALL MBQueryInterface(IOwnMediaBuffer * p, const guid* riid, void **ppvObject)
{
	if (memcmp(riid,&ID_IMEDIABUFFER,sizeof(GUID))==0 ||
		memcmp(riid,&ID_IUNKNOWN,sizeof(GUID))==0)
	{
		if (ppvObject)
		{
			++p->RefCount;
			*ppvObject = p;
		}
		return 0;
	}
	return -1;
}

static ULONG STDCALL MBAddRef(IOwnMediaBuffer * p)
{
	p->RefCount++;
	return 0;
}

static HRESULT STDCALL MBSetMaxLength(IOwnMediaBuffer * p, DWORD cbLength)
{
	if (cbLength > 0)
	{
		if ((int)cbLength > p->Allocated)
		{
			void* Data = realloc(p->Data,cbLength);
			if (!Data && cbLength)
				return E_OUTOFMEMORY;
			p->Data = Data;
			p->Allocated = cbLength;
		}
	}
	else
	{
		free(p->Data);
		p->Data = NULL;
		p->Allocated = 0;
	}
	if (p->Length > p->Allocated)
		p->Length = p->Allocated;
	return 0;
}

static ULONG STDCALL MBRelease(IOwnMediaBuffer * p)
{
	if (--p->RefCount<=0)
	{
		p->Length = 0;
		if (p->Chain)
		{
			p->Next = *p->Chain;
			*p->Chain = p;
		}
		else
		{
			free(p->Data);
			free(p);
			return 0;
		}
	}
	return p->RefCount;
}

static HRESULT STDCALL MBSetLength(IOwnMediaBuffer * p, DWORD cbLength)
{
	DEBUG_MSG2(DEBUG_VIDEO,T("MediaBuffer_SetLength %d (%d)"),cbLength,p->Allocated);
	if ((int)cbLength > p->Allocated)
		return -1;
	p->Length = cbLength;
	return 0;
}

static HRESULT STDCALL MBGetMaxLength(IOwnMediaBuffer * p, DWORD *pcbMaxLength)
{
	DEBUG_MSG1(DEBUG_VIDEO,T("MediaBuffer_GetMaxLength %d"),p->Allocated);
	if (pcbMaxLength)
		*pcbMaxLength = p->Allocated;
	return 0;
}

static HRESULT STDCALL MBGetBufferAndLength(IOwnMediaBuffer * p, BYTE **ppBuffer, DWORD *pcbLength)
{
	DEBUG_MSG1(DEBUG_VIDEO,T("MediaBuffer_GetBufferAndLength %d"),p->Length);
	if (ppBuffer)
		*ppBuffer = (BYTE*)p->Data;
	if (pcbLength)
		*pcbLength = p->Length;
	return 0;
}

static IMediaBufferVMT MediaBufferVMT =
{
    MBQueryInterface,
    MBAddRef,
    MBRelease,
    MBSetLength,
    MBGetMaxLength,
    MBGetBufferAndLength,
};

static void FreeBuffers(IOwnMediaBuffer** Chain)
{
	while (*Chain)
	{
		IOwnMediaBuffer* p = (*Chain)->Next;
		MBSetMaxLength(*Chain,0);
		free(*Chain);
		*Chain = p;
	}
}

static INLINE void AppendBuffer(IOwnMediaBuffer* p, int Length, const void* Data)
{
	memcpy((char*)p->Data + p->Length, Data, Length);
	p->Length += Length;
}

static IOwnMediaBuffer* AllocBuffer(int MaxLength,IOwnMediaBuffer** Chain)
{
	IOwnMediaBuffer* p = Chain ? *Chain : NULL;

	if (MaxLength <= 0)
		return NULL;

	if (p)
		*Chain = p->Next;
	else
	{
		p = (IOwnMediaBuffer*)malloc(sizeof(IOwnMediaBuffer));
		if (!p)
			return NULL;
		memset(p,0,sizeof(IOwnMediaBuffer));
		p->VMT = &MediaBufferVMT;
	}

	p->Chain = Chain;
	p->Length = 0;

	MBSetMaxLength(p,MaxLength);

	p->VMT->AddRef(p);
	return p;
}

static bool_t BuildI420(dmo* p)
{
	// manual video format guessing (for wince4 non pocketpc)
	VIDEOINFOHEADER* VideoInfo;

	PacketFormatClear(&p->Codec.Out.Format);
	p->Codec.Out.Format.Type = PACKET_VIDEO;

	memcpy(&p->Codec.Out.Format.Format.Video,&p->Codec.In.Format.Format.Video,sizeof(video));
	p->Codec.Out.Format.Format.Video.Pixel.Flags = PF_FOURCC;
	p->Codec.Out.Format.Format.Video.Pixel.FourCC = FOURCC_I420;
	p->Codec.Out.Format.Format.Video.Pixel.BitCount = 16;
	p->Codec.Out.Format.Format.Video.Pitch = p->Codec.Out.Format.Format.Video.Width;

	if (MoInitMediaType(&p->OutType,sizeof(VIDEOINFOHEADER)) != S_OK)
		return 0;

	VideoInfo = (VIDEOINFOHEADER*)p->OutType.pbFormat;
	memset(VideoInfo,0,sizeof(VIDEOINFOHEADER));

	VideoInfo->bmiHeader.biSize = sizeof(VideoInfo->bmiHeader);
	VideoInfo->bmiHeader.biWidth = p->Codec.Out.Format.Format.Video.Width;
	VideoInfo->bmiHeader.biHeight = p->Codec.Out.Format.Format.Video.Height;
	VideoInfo->bmiHeader.biCompression = p->Codec.Out.Format.Format.Video.Pixel.FourCC;
	VideoInfo->bmiHeader.biBitCount = (WORD)p->Codec.Out.Format.Format.Video.Pixel.BitCount;
	VideoInfo->bmiHeader.biPlanes = 1;
	VideoInfo->bmiHeader.biSizeImage = (p->Codec.Out.Format.Format.Video.Width * 
		p->Codec.Out.Format.Format.Video.Height * p->Codec.Out.Format.Format.Video.Pixel.BitCount)/8;

	VideoInfo->rcSource.left   = 0;
	VideoInfo->rcSource.top    = 0;
	VideoInfo->rcSource.right  = p->Codec.Out.Format.Format.Video.Width;
	VideoInfo->rcSource.bottom = p->Codec.Out.Format.Format.Video.Height;
	VideoInfo->rcTarget        = VideoInfo->rcSource;

	p->OutType.majortype = WMCMEDIATYPE_Video;
	p->OutType.formattype = WMCFORMAT_VideoInfo;
	p->OutType.subtype = WMCMEDIASUBTYPE;
	p->OutType.subtype.v1 = FOURCC_YUY2; // tricky
	p->OutType.bTemporalCompression = 0;
	p->OutType.bFixedSizeSamples = 1;
	p->OutType.lSampleSize = 0;
	return 1;
}

static bool_t BuildRGB16(dmo* p)
{
	// wmp10 enumerates falsely the input extra info for rgb16 output (wmv9 advanced profile)
	// we have to build our own version
	VIDEOINFOHEADER* VideoInfo;

	PacketFormatClear(&p->Codec.Out.Format);
	p->Codec.Out.Format.Type = PACKET_VIDEO;

	memcpy(&p->Codec.Out.Format.Format.Video,&p->Codec.In.Format.Format.Video,sizeof(video));
	DefaultRGB(&p->Codec.Out.Format.Format.Video.Pixel,16,5,6,5,0,0,0);
	p->Codec.Out.Format.Format.Video.Pitch = p->Codec.Out.Format.Format.Video.Width*2;
	p->Codec.Out.Format.Format.Video.Direction = DIR_MIRRORUPDOWN;

	if (MoInitMediaType(&p->OutType,sizeof(VIDEOINFOHEADER)+3*4) != S_OK)
		return 0;

	VideoInfo = (VIDEOINFOHEADER*)p->OutType.pbFormat;
	memset(VideoInfo,0,sizeof(VIDEOINFOHEADER));

	VideoInfo->bmiHeader.biSize = sizeof(VideoInfo->bmiHeader)+3*4;
	VideoInfo->bmiHeader.biWidth = p->Codec.Out.Format.Format.Video.Width;
	VideoInfo->bmiHeader.biHeight = p->Codec.Out.Format.Format.Video.Height;
	VideoInfo->bmiHeader.biCompression = 3;
	VideoInfo->bmiHeader.biBitCount = (WORD)p->Codec.Out.Format.Format.Video.Pixel.BitCount;
	VideoInfo->bmiHeader.biPlanes = 1;
	VideoInfo->bmiHeader.biSizeImage = (p->Codec.Out.Format.Format.Video.Width * 
		p->Codec.Out.Format.Format.Video.Height * p->Codec.Out.Format.Format.Video.Pixel.BitCount)/8;
	((int32_t*)(VideoInfo+1))[0] = p->Codec.Out.Format.Format.Video.Pixel.BitMask[0];
	((int32_t*)(VideoInfo+1))[1] = p->Codec.Out.Format.Format.Video.Pixel.BitMask[1];
	((int32_t*)(VideoInfo+1))[2] = p->Codec.Out.Format.Format.Video.Pixel.BitMask[2];

	VideoInfo->rcSource.left   = 0;
	VideoInfo->rcSource.top    = 0;
	VideoInfo->rcSource.right  = p->Codec.Out.Format.Format.Video.Width;
	VideoInfo->rcSource.bottom = p->Codec.Out.Format.Format.Video.Height;
	VideoInfo->rcTarget        = VideoInfo->rcSource;

	p->OutType.majortype = WMCMEDIATYPE_Video;
	p->OutType.formattype = WMCFORMAT_VideoInfo;
	p->OutType.subtype = WMCMEDIASUBTYPE_RGB565;
	p->OutType.bTemporalCompression = 0;
	p->OutType.bFixedSizeSamples = 0;
	p->OutType.lSampleSize = 0;
	return 1;
}

// mode0 yuv420
// mode1 rgb16
// mode2 rgb32

static bool_t MatchOutVideo(dmo* p,int Mode) 
{
	int No;
    for (No=0;;++No)
	{
        if (p->Media->VMT->GetOutputType(p->Media, 0, No, &p->OutType) != S_OK)
			break;

        if (memcmp(&p->OutType.formattype,&WMCFORMAT_VideoInfo,sizeof(GUID))==0 &&
			p->OutType.cbFormat >= sizeof(VIDEOINFOHEADER) && 
			p->OutType.pbFormat)
		{
		    VIDEOINFOHEADER *Info = (VIDEOINFOHEADER*) p->OutType.pbFormat;

			PacketFormatClear(&p->Codec.Out.Format);
			p->Codec.Out.Format.Type = PACKET_VIDEO;
			p->Codec.Out.Format.Format.Video.Pixel.FourCC = Info->bmiHeader.biCompression;
			p->Codec.Out.Format.Format.Video.Pixel.BitCount = Info->bmiHeader.biBitCount;
			p->Codec.Out.Format.Format.Video.Width = Info->bmiHeader.biWidth;
			p->Codec.Out.Format.Format.Video.Height = Info->bmiHeader.biHeight;
			p->Codec.Out.Format.Format.Video.Aspect = ASPECT_ONE; //todo: fix
			if (Info->bmiHeader.biCompression == 3)
			{
				p->Codec.Out.Format.Format.Video.Pixel.BitMask[0] = 0xF800;
				p->Codec.Out.Format.Format.Video.Pixel.BitMask[1] = 0x07E0;
				p->Codec.Out.Format.Format.Video.Pixel.BitMask[2] = 0x001F;
			}
			PacketFormatDefault(&p->Codec.Out.Format);

			switch (Mode)
			{
			case 0:
				if (PlanarYUV420(&p->Codec.Out.Format.Format.Video.Pixel))
					return 1;
				break;
			case 1:
				if (p->Codec.Out.Format.Format.Video.Pixel.BitCount==16 && 
					Info->bmiHeader.biCompression==3)
					return 1;
				break;
			case 2:
				if (p->Codec.Out.Format.Format.Video.Pixel.BitCount==32 && 
					Info->bmiHeader.biCompression==0)
					return 1;
				break;
			}
		}
		
		MoFreeMediaType(&p->OutType);
		memset(&p->OutType,0,sizeof(p->OutType));
	}
	return 0;
}

static bool_t BuildPCM(dmo* p)
{
	// manual audio format guessing

	WAVEFORMATEX *WaveFormat;
	if (MoInitMediaType(&p->OutType,sizeof(WAVEFORMATEX)) != S_OK)
		return 0;

	PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,16);

	WaveFormat = (WAVEFORMATEX*) p->OutType.pbFormat;
	WaveFormat->wFormatTag = (WORD)p->Codec.Out.Format.Format.Audio.Format;
	WaveFormat->nChannels = (WORD)p->Codec.Out.Format.Format.Audio.Channels;
	WaveFormat->nSamplesPerSec = p->Codec.Out.Format.Format.Audio.SampleRate;
	WaveFormat->nAvgBytesPerSec = p->Codec.Out.Format.ByteRate;
	WaveFormat->nBlockAlign = (WORD)p->Codec.Out.Format.Format.Audio.BlockAlign;
	WaveFormat->wBitsPerSample = (WORD)p->Codec.Out.Format.Format.Audio.Bits;
	WaveFormat->cbSize = 0;

	p->OutType.majortype = WMCMEDIATYPE_Audio;
	p->OutType.formattype = WMCFORMAT_WaveFormatEx;
	p->OutType.subtype = WMCMEDIASUBTYPE;
	p->OutType.subtype.v1 = AUDIOFMT_PCM;
	p->OutType.bTemporalCompression = 0;
	p->OutType.bFixedSizeSamples = 1;
	p->OutType.lSampleSize = 0;

	return 1;
}

static int Resend(dmo* p)
{
	int Result = ERR_INVALID_DATA;
	if (p->OutputPrev >= 0 && p->OutputPrev != p->OutputNext)
	{
		IOwnMediaBuffer* Buffer = (IOwnMediaBuffer*) p->OutputBuffer[p->OutputPrev].pBuffer;
		packet Packet;
		flowstate State;

		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;

		memset(&Packet,0,sizeof(Packet));
		Packet.RefTime = TIME_UNKNOWN;

		if (p->ConstSize)
		{
			BITMAPINFOHEADER* Info = Buffer->Data;
			Packet.Length = Buffer->Length - Info->biSize;
			Packet.Data[0] = (char*)Buffer->Data + Info->biSize;
		}
		else
		{
			Packet.Length = Buffer->Length;
			Packet.Data[0] = Buffer->Data;
		}
		
		if (Packet.Length > 0)
			Result = p->Codec.Out.Process(p->Codec.Out.Pin.Node,&Packet,&State);
	}
	return Result;
}

static int UpdateInput(dmo* p)
{
	VIDEOINFOHEADER* VideoInfo;
	WAVEFORMATEX* WaveFormat;
	int Dummy;
	int BufferSize;
	int AutoBufferSize;
	int No,Extra;
	bool_t ForceRGB;
	bool_t Found;
	bool_t MinBufferSize = 0;

	for (No=0;No<=p->OutputNoMask;++No)
		if (p->OutputBuffer[No].pBuffer)
		{
			p->OutputBuffer[No].pBuffer->VMT->Release(p->OutputBuffer[No].pBuffer);
			p->OutputBuffer[No].pBuffer = NULL;
		}

	p->Codec.ReSend = NULL;
	p->ConstSize = 0;
	p->Const = NULL;
	p->WMPVersion = QueryPlatform(PLATFORM_WMPVERSION);

 	MoFreeMediaType(&p->InType);
	memset(&p->InType,0,sizeof(p->InType));

	MoFreeMediaType(&p->OutType);
	memset(&p->OutType,0,sizeof(p->OutType));

	if (p->Media)
	{
		p->Media->VMT->FreeStreamingResources(p->Media);
		p->Media->VMT->Release(p->Media);
		p->Media = NULL;
	}

	if (p->Codec.In.Format.Type != PACKET_NONE)
	{
		uint32_t FourCC;
		switch (p->Codec.In.Format.Type)
		{
		case PACKET_VIDEO:
			FourCC = p->Codec.In.Format.Format.Video.Pixel.FourCC;
			break;

		case PACKET_AUDIO:
			FourCC = p->Codec.In.Format.Format.Audio.Format;
			break;

		default:
			FourCC = 0;
			break;
		}

		if (FourCC && p->CreateCodecDMOEx)
			p->CreateCodecDMOEx(FourCC,&p->Media);
		else
		if (p->CreateCodecDMO)
			p->CreateCodecDMO(&p->Media);
		else
		if (p->DllGetClassObject)
		{
			IClassFactory* Class = NULL;
			if (p->DllGetClassObject(&p->ClassId,&ID_ICLASSFACTORY,&Class) == S_OK)
			{
				Class->lpVtbl->CreateInstance(Class,NULL,(REFIID)&IID_IMediaObject,&p->Media);
				Class->lpVtbl->Release(Class);
			}
		}
		if (!p->Media)
			return ERR_INVALID_PARAM;

		switch (p->Codec.In.Format.Type)
		{
		case PACKET_VIDEO:

			Extra = p->Codec.In.Format.ExtraLength;
			if ((p->Codec.In.Format.Format.Video.Pixel.FourCC == FOURCC('W','M','V','3') || 
				 p->Codec.In.Format.Format.Video.Pixel.FourCC == FOURCC('W','M','V','2')) && Extra>4)
				Extra = 4; //hope this is right...

			if (MoInitMediaType(&p->InType,sizeof(VIDEOINFOHEADER)+Extra) != S_OK)
				return ERR_OUT_OF_MEMORY;
			
			VideoInfo = (VIDEOINFOHEADER*)p->InType.pbFormat;
			memset(VideoInfo,0,sizeof(VIDEOINFOHEADER));
			VideoInfo->bmiHeader.biSize = sizeof(VideoInfo->bmiHeader)+Extra;
			VideoInfo->bmiHeader.biWidth = p->Codec.In.Format.Format.Video.Width;
			VideoInfo->bmiHeader.biHeight = p->Codec.In.Format.Format.Video.Height;
			VideoInfo->bmiHeader.biCompression = p->Codec.In.Format.Format.Video.Pixel.FourCC;
			VideoInfo->bmiHeader.biBitCount = (WORD)p->Codec.In.Format.Format.Video.Pixel.BitCount;
			VideoInfo->bmiHeader.biPlanes = 1;
			VideoInfo->bmiHeader.biSizeImage = (p->Codec.In.Format.Format.Video.Width * 
				p->Codec.In.Format.Format.Video.Height * p->Codec.In.Format.Format.Video.Pixel.BitCount)/8;
			if (p->WMPVersion > 9)
				VideoInfo->bmiHeader.biSizeImage = 0;
			if (Extra)
				memcpy(VideoInfo+1,p->Codec.In.Format.Extra,Extra);

			VideoInfo->rcSource.left   = 0;
			VideoInfo->rcSource.top    = 0;
			VideoInfo->rcSource.right  = p->Codec.In.Format.Format.Video.Width;
			VideoInfo->rcSource.bottom = p->Codec.In.Format.Format.Video.Height;
			VideoInfo->rcTarget        = VideoInfo->rcSource;

			if (p->Codec.In.Format.PacketRate.Num)
			{
				VideoInfo->AvgTimePerFrame = (REFERENCE_TIME)p->Codec.In.Format.PacketRate.Den * 10000000 / p->Codec.In.Format.PacketRate.Num;
				VideoInfo->dwBitRate = Scale(VideoInfo->bmiHeader.biSizeImage*8,p->Codec.In.Format.PacketRate.Num,p->Codec.In.Format.PacketRate.Den);
			}

			p->InType.majortype = WMCMEDIATYPE_Video;
			p->InType.formattype = WMCFORMAT_VideoInfo;
			p->InType.subtype = WMCMEDIASUBTYPE;
			p->InType.subtype.v1 = p->Codec.In.Format.Format.Video.Pixel.FourCC;

			if (p->Media->VMT->SetInputType(p->Media, 0, &p->InType, 0) != S_OK)
				return ERR_INVALID_DATA;

			// for some reason using planar yuv fails with Window Mobile WMP10 :(
			ForceRGB = p->CreateCodecDMO && p->WMPVersion==10;

			// some version of C550 ROM uses low level stuff which raises exception in non Kernel mode (default on smartphone)
			Found = 0;
			TRY_BEGIN
			// first try I420 (sometimes it's not enumarated as supported format, even if it is)
			if (!(ForceRGB?BuildRGB16(p):BuildI420(p)) || p->Media->VMT->SetOutputType(p->Media,0, &p->OutType, 0) != S_OK)
			{
				MoFreeMediaType(&p->OutType);
				memset(&p->OutType,0,sizeof(p->OutType));

				if (ForceRGB || !MatchOutVideo(p,0))
				{
					video Desktop;
					QueryDesktop(&Desktop);
					if (Desktop.Pixel.BitCount!=16 || !MatchOutVideo(p,1))
					{
						if (!MatchOutVideo(p,2))
							return ERR_INVALID_DATA;
					}
				}
    
				if (p->Media->VMT->SetOutputType(p->Media,0, &p->OutType, 0) != S_OK)
					return ERR_INVALID_DATA;
			}
			Found = 1;
			TRY_END
			if (!Found)
				return ERR_INVALID_DATA;

			if (p->WMPVersion==9 && PlanarYUV420(&p->Codec.Out.Format.Format.Video.Pixel))
			{
				// override default pitch, it has to be dword aligned
				p->Codec.Out.Format.Format.Video.Pitch = (p->Codec.Out.Format.Format.Video.Pitch+3) & ~3;
				MinBufferSize = 1;
			}

			AutoBufferSize = GetImageSize(&p->Codec.Out.Format.Format.Video);

			if (p->CreateCodecDMO)
			{
				AutoBufferSize += VideoInfo->bmiHeader.biSize;
				p->ConstSize = VideoInfo->bmiHeader.biSize;
				p->Const = &VideoInfo->bmiHeader;
			}

			p->Codec.ReSend = Resend;	
			p->OutputNoMask = 1;
			p->UseDropping = 1;
			break;

		case PACKET_AUDIO:

			if (MoInitMediaType(&p->InType,sizeof(WAVEFORMATEX)+p->Codec.In.Format.ExtraLength) != S_OK)
				return ERR_OUT_OF_MEMORY;

			WaveFormat = (WAVEFORMATEX*)p->InType.pbFormat;
			memset(WaveFormat,0,sizeof(WAVEFORMATEX));

			WaveFormat->wFormatTag = (WORD)p->Codec.In.Format.Format.Audio.Format;
			WaveFormat->nChannels = (WORD)p->Codec.In.Format.Format.Audio.Channels;
			WaveFormat->nSamplesPerSec = p->Codec.In.Format.Format.Audio.SampleRate;
			WaveFormat->nAvgBytesPerSec = p->Codec.In.Format.ByteRate;
			WaveFormat->nBlockAlign = (WORD)p->Codec.In.Format.Format.Audio.BlockAlign;
			WaveFormat->wBitsPerSample = (WORD)p->Codec.In.Format.Format.Audio.Bits;
			WaveFormat->cbSize = (WORD)p->Codec.In.Format.ExtraLength;
			if (p->Codec.In.Format.ExtraLength)
			{
				// PocketPC WMP9
				if (p->CreateCodecDMO && p->WMPVersion==9 && WaveFormat->wFormatTag == AUDIOFMT_WMA9 && p->Codec.In.Format.ExtraLength==10)
				{
					char* e = (char*)(WaveFormat+1);
					e[0] = e[1] = 0;
					e[8] = e[9] = 0;
					memcpy(e+2,p->Codec.In.Format.Extra,6);
				}
				else
					memcpy(WaveFormat+1,p->Codec.In.Format.Extra,p->Codec.In.Format.ExtraLength);
			}

			p->InType.majortype = WMCMEDIATYPE_Audio;
			p->InType.formattype = WMCFORMAT_WaveFormatEx;
			p->InType.subtype = WMCMEDIASUBTYPE;
			p->InType.subtype.v1 = p->Codec.In.Format.Format.Audio.Format;
			p->InType.bTemporalCompression = 0;
			p->InType.bFixedSizeSamples = 1;
			p->InType.lSampleSize = 0;

			if (p->Media->VMT->SetInputType(p->Media, 0, &p->InType, 0) != S_OK)
				return ERR_INVALID_DATA;

			if (!BuildPCM(p))
				return ERR_INVALID_DATA;

			Found = 0;
			TRY_BEGIN
			if (p->Media->VMT->SetOutputType(p->Media,0, &p->OutType, 0) != S_OK)
				return ERR_INVALID_DATA;
			Found = 1;
			TRY_END
			if (!Found)
				return ERR_INVALID_DATA;
			
			// it seems pocketpc can't split output data
			if (WaveFormat->nBlockAlign && WaveFormat->nAvgBytesPerSec)
			{
				AutoBufferSize = Scale(p->Codec.Out.Format.ByteRate,WaveFormat->nBlockAlign,WaveFormat->nAvgBytesPerSec);
				AutoBufferSize *= 4; // just to be safe
				if (AutoBufferSize < 128*1024)
					AutoBufferSize = 128*1024;
			}
			else
				AutoBufferSize = 128*1024;

			p->OutputNoMask = 0;
			p->UseDropping = 0;
			break;

		default:
			return ERR_INVALID_DATA;
		}

		BufferSize = 0;
		if (p->Media->VMT->GetOutputSizeInfo(p->Media, 0, (DWORD*)&BufferSize, (DWORD*)&Dummy) != S_OK)
			return ERR_INVALID_DATA;

		if (BufferSize<=0 || (MinBufferSize && BufferSize < AutoBufferSize))
			BufferSize = AutoBufferSize;
		else
		if (p->WMPVersion>9)
			BufferSize += 4096;

		for (No=0;No<=p->OutputNoMask;++No)
		{
			p->OutputBuffer[No].pBuffer = (IMediaBuffer*) AllocBuffer(BufferSize,NULL);
			if (!p->OutputBuffer[No].pBuffer)
				return ERR_OUT_OF_MEMORY;
		}

		p->First = p->UseDropping;
		p->OutputNext = 0;
		p->OutputPrev = -1;
		p->Media->VMT->AllocateStreamingResources(p->Media);
	}

	FreeBuffers(&p->InputBuffers);
	return ERR_NONE;
}

static int Process(dmo* p, const packet* Packet, const flowstate* State)
{
	REFERENCE_TIME TimeStamp;
	DWORD Status;
	IOwnMediaBuffer* Buffer;
	IOwnMediaBuffer* LastBuffer;

	if (Packet)
	{
		if (State->DropLevel > 1)
		{
			p->Media->VMT->Discontinuity(p->Media,0);
			p->First = p->UseDropping;
			return ERR_NONE;
		}

		if (p->Media->VMT->GetInputStatus(p->Media, 0, &Status)==0 &&
			!(Status & DMO_INPUT_STATUSF_ACCEPT_DATA)) // this shouldn't happen
			p->Media->VMT->Discontinuity(p->Media,0);

		Status = 0;
		if (Packet->Key)
			Status |= DMO_INPUT_DATA_BUFFERF_SYNCPOINT;
		else
		if (p->First)
			return ERR_NEED_MORE_DATA;

		TimeStamp = 0;
		if (Packet->RefTime >= 0)
		{
			Status |= DMO_INPUT_DATA_BUFFERF_TIME;
			TimeStamp = ((REFERENCE_TIME)Packet->RefTime * 10000000 + (TICKSPERSEC/2)) / TICKSPERSEC;
		}

		Buffer = AllocBuffer(Packet->Length+p->ConstSize,&p->InputBuffers);
		if (Buffer)
		{
			if (p->Const)
			{
				if (p->WMPVersion>9) p->Const->biSizeImage = Packet->Length;
				AppendBuffer((IOwnMediaBuffer*)Buffer,p->ConstSize,p->Const);
			}

			if (Packet->Data[0])
				AppendBuffer((IOwnMediaBuffer*)Buffer,Packet->Length,Packet->Data[0]);

			p->Media->VMT->ProcessInput(p->Media,0,(IMediaBuffer*)Buffer,Status,TimeStamp,0);
			Buffer->VMT->Release(Buffer);
		}
	}

	LastBuffer = NULL;
	if (p->OutputPrev >= 0 && p->OutputPrev != p->OutputNext)
		LastBuffer = (IOwnMediaBuffer*) p->OutputBuffer[p->OutputPrev].pBuffer;

	if (State && State->DropLevel > 0)
	{
		DMO_OUTPUT_DATA_BUFFER Discard;
		Discard.dwStatus = 0;
		Discard.pBuffer = NULL;

		p->Media->VMT->ProcessOutput(p->Media, DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER, 1, &Discard, &Status);
	}

	for (;;)
	{
		Buffer = (IOwnMediaBuffer*) p->OutputBuffer[p->OutputNext].pBuffer;
		Buffer->Length = 0;

		p->OutputBuffer[p->OutputNext].dwStatus = 0;
		
		if (p->Media->VMT->ProcessOutput(p->Media, 0, 1, &p->OutputBuffer[p->OutputNext], &Status) != S_OK || Buffer->Length==0)
			break;

		if (State && State->DropLevel > 0)
		{
			p->Codec.Out.Process(p->Codec.Out.Pin.Node,NULL,State);
			continue;
		}

		if (p->OutputBuffer[p->OutputNext].dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIME)
			p->Codec.Packet.RefTime = (tick_t)((p->OutputBuffer[p->OutputNext].rtTimestamp * TICKSPERSEC + 5000000) / 10000000);
		else
			p->Codec.Packet.RefTime = -1;

		if (p->ConstSize)
		{
			BITMAPINFOHEADER* Info = Buffer->Data;
			p->Codec.Packet.Length = Buffer->Length - Info->biSize;
			p->Codec.Packet.Data[0] = (char*)Buffer->Data + Info->biSize;
			p->Codec.Packet.LastData[0] = NULL;
			if (LastBuffer)
				p->Codec.Packet.LastData[0] = (char*)LastBuffer->Data + Info->biSize;
			if (p->Codec.Packet.Length <= 0)
				p->Codec.Packet.Data[0] = NULL;
		}
		else
		{
			p->Codec.Packet.Length = Buffer->Length;
			p->Codec.Packet.Data[0] = Buffer->Data;
			p->Codec.Packet.LastData[0] = NULL;
			if (LastBuffer)
				p->Codec.Packet.LastData[0] = LastBuffer->Data;
		}

		p->OutputPrev = p->OutputNext;
		p->OutputNext = (p->OutputNext+1) & p->OutputNoMask;
		p->First = 0;
		return ERR_NONE;
	}

	return ERR_NEED_MORE_DATA;
}

static int Flush(dmo* p)
{
	if (p->Media)
	{
		p->Media->VMT->Discontinuity(p->Media,0);
		p->Media->VMT->Flush(p->Media);
	}
	p->OutputPrev = -1;
	p->First = p->UseDropping;
	return ERR_NONE;
}

static int Create(dmo* p)
{
	p->Module = LoadLibrary(LangStr(p->Codec.Node.Class,DMO_MODULEPPC));
	GetProc(&p->Module,&p->CreateCodecDMO,T("CreateCodecDMO"),0);
	GetProc(&p->Module,&p->CreateCodecDMOEx,T("CreateCodecDMOEx"),1);
	GetProc(&p->Module,&p->QueryUnloadCodecDMO,T("QueryUnloadCodecDMO"),1);

	if (!p->Module && StringToGUID(LangStr(p->Codec.Node.Class,DMO_CLASSID),&p->ClassId))
	{
		p->Module = LoadLibrary(LangStr(p->Codec.Node.Class,DMO_MODULE));
		GetProc(&p->Module,&p->DllGetClassObject,T("DllGetClassObject"),0);
		GetProc(&p->Module,&p->DllCanUnloadNow,T("DllCanUnloadNow"),1);
	}

	if (!p->Module)
		return ERR_NOT_SUPPORTED;

	p->Codec.Flush = Flush;
	p->Codec.Process = Process;
	p->Codec.UpdateInput = UpdateInput;
	return ERR_NONE;
}

static void Delete(dmo* p)
{
	if ((p->QueryUnloadCodecDMO==NULL || p->QueryUnloadCodecDMO()) &&
		(p->DllCanUnloadNow==NULL || p->DllCanUnloadNow()==S_OK))
		FreeLibrary(p->Module);
}

static const nodedef DMO =
{
	sizeof(dmo)|CF_ABSTRACT,
	DMO_CLASS,
	CODEC_CLASS,
	PRI_DEFAULT-100,
	(nodecreate)Create,
	(nodedelete)Delete,
};

static const nodedef WMV =
{
	0, // parent size
	WMV_ID,
	DMO_CLASS,
	PRI_DEFAULT-100,
};

static const nodedef WMS =
{
	0, // parent size
	WMS_ID,
	DMO_CLASS,
	PRI_DEFAULT-100,
};

static const nodedef WMVA =
{
	0, // parent size
	WMVA_ID,
	DMO_CLASS,
	PRI_DEFAULT-95,
};

static const nodedef WMA =
{
	0, // parent size
	WMA_ID,
	DMO_CLASS,
	PRI_DEFAULT-100,
};

static const nodedef WMAV =
{
	0, // parent size
	WMAV_ID,
	DMO_CLASS,
	PRI_DEFAULT-100,
};

//todo: auto find supported content-types
static int Find(int Class)
{
	if (CheckModule(LangStr(Class,DMO_MODULEPPC)))
		return 2;
	if (CheckModule(LangStr(Class,DMO_MODULE)))
		return 1;
	return 0;
}

void DMO_Init()
{
	int WMPVersion = QueryPlatform(PLATFORM_WMPVERSION);

#ifndef TARGET_WIN32
	// shellexecute won't work under win32
	CoInitializeEx(NULL,COINIT_MULTITHREADED);
#endif

	NodeRegisterClass(&DMO);

	if (Find(WMV_ID))
	{
		NodeRegisterClass(&WMV);
		NodeRegisterClass(&WMVF);
	}

	if (Find(WMVA_ID))
	{
		NodeRegisterClass(&WMVA);
		NodeRegisterClass(&WMVF);
	}

	if (Find(WMS_ID))
	{
		NodeRegisterClass(&WMS);
		NodeRegisterClass(&WMVF);
	}

	if (Find(WMA_ID))
	{
		NodeRegisterClass(&WMA);
		NodeRegisterClass(&WMAF);
	}

	if (Find(WMAV_ID))
	{
		NodeRegisterClass(&WMAV);
		NodeRegisterClass(&WMAF);

		if (WMPVersion==9 && QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_XSCALE)
		{
			DWORD Disp=0;
			HKEY Key;
			tchar_t* Base = T("SOFTWARE\\Microsoft\\Windows Media Player\\Codecs\\WMAVoice");

			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, Base, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
			{
				DWORD Value = 1;
				RegSetValueEx(Key, T("CPUPerformance"), 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
				RegCloseKey(Key);
			}
		}
	}
}

void DMO_Done()
{
	NodeUnRegisterClass(DMO_CLASS);
	NodeUnRegisterClass(WMAF_ID);
	NodeUnRegisterClass(WMVF_ID);

#ifndef TARGET_WIN32
	CoUninitialize();
#endif
}

#endif
