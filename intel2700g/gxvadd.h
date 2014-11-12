/******************************************************************************
** Copyright (c) 2004, Intel Corporation
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** - Redistributions of source code must retain the above copyright notice, this 
**   list of conditions and the following disclaimer. 
** - Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution. 
** - Neither the name of the Intel Corporation nor the names of its contributors
**   may be used to endorse or promote products derived from this software without
**   specific prior written permission. 
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

/******************************************************************************
**
**  FILENAME:   gxvadd.h
**
**  DESCRIPTION:   GX Video Decode Acceleration and display API
**
******************************************************************************/

#ifndef _GXVADD_H_
#define _GXVADD_H_

/****************************************************************************/
/*********** External Interface for the GX video acceleration API **********/
/****************************************************************************/

#ifndef GXAPI

#if defined(UNDER_CE)
        #define GXAPI __stdcall
#else
        #define GXAPI
#endif

#endif 
/************************************************************ Standard Types */

typedef enum tag_gx_bool
{
        GX_FALSE,
        GX_TRUE
} gx_bool, *gx_pbool;

typedef unsigned char           gx_uint8;
typedef unsigned short          gx_uint16;
typedef unsigned long           gx_uint32;

typedef signed short            gx_int16;
typedef signed long             gx_int32;

typedef void*                   gx_pvoid;


typedef gx_uint32               SMSurface;


/*****************************************************************************/
/***************************** GXVA Hardware commands ***********************/
/*****************************************************************************/

/*********************************************** Motion Compensation Commands */

#define GXVA_CMD_MC_OPP_TYPE            (0x00000000)
#define GXVA_CMD_FETCH_PRED_B           (0x08000000)
#define GXVA_CMD_IDCT_MODE              (0x10000000)
#define GXVA_CMD_IMAGE_WRITE            (0x18000000)
#define GXVA_CMD_FETCH_PRED_A           (0x80000000)

#define GXVA_CMD_MASK                   (0xf8000000)

/* Motion Comp Operation Type (GXVA_CMD_MC_OPP_TYPE) */
#define GXVA_MC_TYPE_COL_PLANE_MASK     (0x00000003)
#define GXVA_MC_TYPE_COL_PLANE_Y        (0x00000000)
#define GXVA_MC_TYPE_COL_PLANE_U        (0x00000001)
#define GXVA_MC_TYPE_COL_PLANE_V        (0x00000002)
#define GXVA_MC_TYPE_BLK_WIDTH_MASK     (0x0000000c)
#define GXVA_MC_TYPE_BLK_WIDTH_4        (0x00000000)
#define GXVA_MC_TYPE_BLK_WIDTH_8        (0x00000004)
#define GXVA_MC_TYPE_BLK_WIDTH_16       (0x00000008)
#define GXVA_MC_TYPE_BLK_HGHT_MASK      (0x00000030)
#define GXVA_MC_TYPE_BLK_HGHT_4         (0x00000000)
#define GXVA_MC_TYPE_BLK_HGHT_8         (0x00000010)
#define GXVA_MC_TYPE_BLK_HGHT_16        (0x00000020)
#define GXVA_MC_TYPE_INTR_RND_MODE      (0x00000040)
#define GXVA_MC_TYPE_AVR_RND_MODE       (0x00000080)
#define GXVA_MC_TYPE_QUARTR_PIX_ACC     (0x00000100)
#define GXVA_MC_TYPE_WMV_FILTER_MODE    (0x00000400) 

#define GXVA_MC_TYPE_QUARTR_PIX_MODE    (0x00000200)

/* Fetch Prediction A (GXVA_CMD_FETCH_PRED_A)*/
#define GXVA_FETCH_A_PREDX_MASK         (0x00003fff)
#define GXVA_FETCH_A_PREDX_SHIFT        (0)
#define GXVA_FETCH_A_PREDY_MASK         (0x0fffc000)
#define GXVA_FETCH_A_PREDY_SHIFT        (14)
#define GXVA_FETCH_A_SRC_SEL_MASK       (0x30000000)
#define GXVA_FETCH_A_SRC_SEL_REF1       (0x00000000)
#define GXVA_FETCH_A_SRC_SEL_REF2       (0x10000000)
#define GXVA_FETCH_A_SRC_SEL_OUT        (0x20000000)
#define GXVA_FETCH_A_SRC_SEL_SHIFT      (28)
#define GXVA_FETCH_A_FIRST_PRED         (0x40000000)

/* Fetch Prediction B (GXVA_CMD_FETCH_PRED_B)*/
#define GXVA_FETCH_B_IP_FIELD_MODE      (0x00000001)
#define GXVA_FETCH_B_ACC_FIELD_MODE     (0x00000002)
#define GXVA_FETCH_B_ACC_FIELD_SLCT     (0x00000004)
#define GXVA_FETCH_B_ACC_Y_POS_MASK     (0x00000018)
#define GXVA_FETCH_B_ACC_Y_POS_0        (0x00000000)
#define GXVA_FETCH_B_ACC_Y_POS_4        (0x00000008)
#define GXVA_FETCH_B_ACC_Y_POS_8        (0x00000010)
#define GXVA_FETCH_B_ACC_Y_POS_12       (0x00000018)
#define GXVA_FETCH_B_ACC_X_POS_MASK     (0x00000060)
#define GXVA_FETCH_B_ACC_X_POS_0        (0x00000000)
#define GXVA_FETCH_B_ACC_X_POS_4        (0x00000020)
#define GXVA_FETCH_B_ACC_X_POS_8        (0x00000040)
#define GXVA_FETCH_B_ACC_X_POS_12       (0x00000060)

/* IDCT Mode (GXVA_CMD_IDCT_MODE) */

#define GXVA_IDCT_MODE_FIELD            (0x00000001)
#define GXVA_IDCT_MODE_INTRA            (0x00000002)
#define GXVA_IDCT_MODE_BLK_SELX_MASK    (0x0000000c)
#define GXVA_IDCT_MODE_BLK_SELX_SHIFT   (2)
#define GXVA_IDCT_MODE_BLK_SELY_MASK    (0x00000030)
#define GXVA_IDCT_MODE_BLK_SELY_SHIFT   (4)

/* defines for simplicity in 8x8 mode */
#define GXVA_IDCT_MODE_BLK_TOP_LEFT     (0x00000000)
#define GXVA_IDCT_MODE_BLK_TOP_RGHT     (0x00000008)
#define GXVA_IDCT_MODE_BLK_BOT_LEFT     (0x00000020)
#define GXVA_IDCT_MODE_BLK_BOT_RGHT     (0x00000028)

#define GXVA_IDCT_MODE_BLK_BOT_FIELD_LEFT (0x00000010)
#define GXVA_IDCT_MODE_BLK_BOT_FIELD_RGHT (0x00000018)

#define GXVA_IDCT_MODE_DCOFFSET         (0x00000080)
#define GXVA_IDCT_BLK_SIZEX_4           (0x00000100)
#define GXVA_IDCT_BLK_SIZEX_8           (0x00000000)
#define GXVA_IDCT_BLK_SIZEY_4           (0x00000200)
#define GXVA_IDCT_BLK_SIZEY_8           (0x00000000)


/* Image Write (GXVA_CMD_IMAGE_WRITE) */
#define GXVA_IMAGE_WRITE_X_MASK         (0x000007ff)
#define GXVA_IMAGE_WRITE_X_SHIFT        (0)
#define GXVA_IMAGE_WRITE_Y_MASK         (0x003ff800)
#define GXVA_IMAGE_WRITE_Y_SHIFT        (11)
#define GXVA_IMAGE_WRITE_FIELD          (0x00400000)
#define GXVA_IMAGE_WRITE_WIDTH_MASK     (0x01800000)
#define GXVA_IMAGE_WRITE_WIDTH_4        (0x00000000)
#define GXVA_IMAGE_WRITE_WIDTH_8        (0x00800000)
#define GXVA_IMAGE_WRITE_WIDTH_16       (0x01000000)
#define GXVA_IMAGE_WRITE_HGHT_MASK      (0x06000000)
#define GXVA_IMAGE_WRITE_HGHT_4         (0x00000000)
#define GXVA_IMAGE_WRITE_HGHT_8         (0x02000000)
#define GXVA_IMAGE_WRITE_HGHT_16        (0x04000000)

/* End of Frame (GXVA_CMD_END_OF_FRAME) */


/*****************************************************************************/
/***************************** GXVA API Defines *****************************/
/*****************************************************************************/

typedef enum {

        GXVAError_OK = 0,
        GXVAError_GenericError,
        GXVAError_InvalidParameter,
        GXVAError_IncorrectSurfaceFormat,
        GXVAError_OutOfMemory,
        GXVAError_HardwareNotAvailable,
        GXVAError_NotInitialized,
        GXVAError_WriteTimedOut,
        GXVAError_SurfaceUnavailable,

} GXVAError;

/* IZZ mode */
typedef enum _GXVA_IZZMODE_
{
        GXVA_ZigZagScanOrder  = 0,
        GXVA_AltVertScanOrder = 1,
        GXVA_AltHorzScanOrder = 2,
        GXVA_RasterScanOrder  = 3,

} GXVA_IZZMODE;

/* Destination formations of the colour space convertor */
typedef enum 
{
        GXVA_SURF_FORMAT_420,
        GXVA_SURF_FORMAT_422,
        GXVA_SURF_FORMAT_RGB24,

} GXVA_SURF_FORMAT;


/*****************************************************************************/
/********************** GXVA API Function Prototypes ************************/
/*****************************************************************************/
#include "gxcomp.h"

//Frame Management (Stream level)
GXVAError GXAPI GXVA_FrameDimensions(gx_uint32 ui32Height, gx_uint32 ui32Width);

GXVAError GXAPI GXVA_FrameAllocate(SMSurface *phFrames, gx_uint32 ui32FrameCnt);
GXVAError GXAPI GXVA_FrameFree(SMSurface *phFrames, gx_uint32 ui32FrameCnt);

//Frame Control (Per Frame)
GXVAError GXAPI GXVA_BeginFrame(SMSurface hOutputSurface, gx_bool bEnableIDCT);
GXVAError GXAPI GXVA_EndFrame(gx_bool bSyncHardware);
GXVAError GXAPI GXVA_SetReferenceFrameAddress(gx_uint32 ui32FrameNo, SMSurface hSurface);

//Frame Access
GXVAError GXAPI GXVA_FrameBeginAccess(SMSurface hSurface, 
         gx_uint8 **ppuint8FrameAddress,
         gx_uint32 *pui32FrameStride,
         gx_bool bSyncHardware);

GXVAError GXAPI GXVA_FrameEndAccess(SMSurface hSurface);

//Frame decode
GXVAError GXAPI GXVA_WriteIZZBlock(gx_int32 *pi32Data, gx_uint32 ui32Count);
GXVAError GXAPI GXVA_WriteIZZData(gx_int32 i32Coeff, gx_uint32 ui32Index, gx_bool bEOB);
GXVAError GXAPI GXVA_SetIZZMode(GXVA_IZZMODE eMode);

GXVAError GXAPI GXVA_WriteResidualDifferenceData(gx_int16 *pi16Data, gx_uint32 ui32Count); 
GXVAError GXAPI GXVA_WriteMCCmdData(gx_uint32 ui32Data);

//Frame format conversion*/
GXVAError GXAPI GXVA_FCFrameAllocate(SMSurface *phFrame, GXVA_SURF_FORMAT eFormat);
GXVAError GXAPI GXVA_FrameConvert(SMSurface hDestSurface, SMSurface hSourSurface);                                        

// General, defined in all API's
GXVAError GXAPI GXVA_Initialize();
GXVAError GXAPI GXVA_Deinitialize();

GXVAError GXAPI GXVA_Reset();

/******************************************** Internal debug only functions */

gx_bool GXAPI GXVA_SimScriptOpen(char *pszFileName, int Wipe);

void GXAPI GXVA_SimScriptClose();
void GXAPI GXVA_SimScriptFlush();
void GXAPI GXVA_SimScriptLog(char *str,...);

void GXAPI GXVA_SetScriptMode(gx_uint32 ui32ScriptMode);

void GXAPI GXVA_GetStats(gx_uint32 *pui32IDCTAccessCount,
                         gx_uint32 *pui32RegAccessCount,
                         gx_uint32 *pui32CmdCount);

void GXAPI GXVA_GetSlavePortStats(gx_uint32 ui32SlavePortType,
                                  gx_uint32 *pui32CmdCount,
                                  gx_uint32 *pui32CmdPolls,
                                  gx_uint32 *pui32CmdTrys,
                                  gx_uint32 *pui32CmdOverflows);

/*****************************************************************************/
/******************************* PDP Overlay Defines *************************/
/*****************************************************************************/

typedef enum {

        VDISPError_OK = 0,
        VDISPError_GenericError,
        VDISPError_InvalidParameter,
        VDISPError_IncorrectSurfaceFormat,
        VDISPError_HardwareNotAvailable,
        VDISPError_NotInitialised,

} VDISPError;

typedef struct _VDISP_Tag_CSCCoeffs
{
        gx_uint16       ui16RyCoeff;
        gx_uint16       ui16RuCoeff;
        gx_uint16       ui16RvCoeff;
        gx_uint16       ui16GyCoeff;
        gx_uint16       ui16GuCoeff;
        gx_uint16       ui16GvCoeff;
        gx_uint16       ui16ByCoeff;
        gx_uint16       ui16BuCoeff;
        gx_uint16       ui16BvCoeff;

}VDISP_CSCCoeffs, *PVDISP_CSCCoeffs;


typedef enum _VDISP_DEINTERLACE_
{
        VDISP_NONE=0,
        VDISP_WEAVE=1,
        VDISP_BOB_ODD,
        VDISP_BOB_EVEN,
        VDISP_BOB_EVEN_NONINTERLEAVED

} VDISP_DEINTERLACE,*PVDISP_DEINTERLACE;

#define VDISP_OVERLAYATTRIB_VALID_DSTPOSITION   0x0001
#define VDISP_OVERLAYATTRIB_VALID_SRCPOSITION   0x0002
#define VDISP_OVERLAYATTRIB_VALID_CKEY          0x0004
#define VDISP_OVERLAYATTRIB_VALID_VISIBILITY    0x0010
#define VDISP_OVERLAYATTRIB_VALID_ROTATION      0x0020
#define VDISP_OVERLAYATTRIB_VALID_ROTATEDEGREE  0x0040
#define VDISP_OVERLAYATTRIB_VALID_VERIFY        0x8000   // Can we create an overlay?

typedef struct _VDISP_OVERLAYATTRIBS_
{
        gx_uint16       ui16ValidFlags;          // Flags to say which members of this structure are valid

        gx_int16        i16Top;                  // Signed position of top of overlay (overlay starts on this line)
        gx_int16        i16Left;                 // Signed position of left of overlay (overlay starts on this column)
        gx_int16        i16Bottom;               // Signed position of bottom of overlay (line above this one is last line of overlay)
        gx_int16        i16Right;                // Signed position of right overlay  (pixel before this one is last line of overlay)

        gx_uint16       ui16SrcX1;               // SrcX1 position of overlay data within buffer
        gx_uint16       ui16SrcY1;               // SrcY1 position of overlay data within buffer
        gx_uint16       ui16SrcX2;               // SrcX2 position of overlay data within buffer
        gx_uint16       ui16SrcY2;               // SrcY2 position of overlay data within buffer

        gx_bool         bCKeyOn;                 // Turn on/off colorkey;
        gx_uint32       ui32CKeyValue;           // ColorKey value

        gx_bool         bOverlayOn;              // Turn the overlay on/off
        
        gx_bool         bDisableRotation;        // force video to be displayed unrotated for extra performance
        gx_uint16       ui16RotateDegree;        // degree to rotate the surface 

}VDISP_OVERLAYATTRIBS,*PVDISP_OVERLAYATTRIBS;

/*****************************************************************************/
/**************** PDP Overlay API Function Prototypes ************************/
/*****************************************************************************/

VDISPError GXAPI VDISP_Initialize();
VDISPError GXAPI VDISP_Deinitialize();

VDISPError GXAPI VDISP_OverlaySetAttributes(PVDISP_OVERLAYATTRIBS psOverlayAttributes,
                                            SMSurface hSurface);

VDISPError GXAPI VDISP_OverlayFlipSurface(SMSurface hSurfaceFlipTo, VDISP_DEINTERLACE eDeinterlace);

VDISPError GXAPI VDISP_OverlayContrast(gx_uint32 ui32Contrast);
VDISPError GXAPI VDISP_OverlayGamma(gx_uint32 ui32Gamma);
VDISPError GXAPI VDISP_OverlayBrightness(gx_uint32 ui32Brightness);
VDISPError GXAPI VDISP_OverlaySetColorspaceConversion(PVDISP_CSCCoeffs psCoeffs);


#endif /* _GXVADD_H_ */
