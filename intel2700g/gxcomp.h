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


#define GXVA_FrameDimensions                M24VA_FrameDimensions
#define GXVA_FrameAllocate                     M24VA_FrameAllocate
#define GXVA_FrameFree                           M24VA_FrameFree
#define GXVA_BeginFrame                         M24VA_BeginFrame
#define GXVA_EndFrame                            M24VA_EndFrame
#define GXVA_SetReferenceFrameAddress	M24VA_SetReferenceFrameAddress
#define GXVA_FrameBeginAccess              M24VA_FrameBeginAccess
#define GXVA_FrameEndAccess                 M24VA_FrameEndAccess
#define GXVA_WriteIZZData                       M24VA_WriteIZZData	
#define GXVA_SetIZZMode                         M24VA_SetIZZMode	
#define GXVA_WriteResidualDifferenceData    M24VA_WriteResidualDifferenceData
#define GXVA_WriteMCCmdData                 M24VA_WriteMCCmdData
#define GXVA_FCFrameAllocate                 M24VA_FCFrameAllocate
#define GXVA_FrameConvert                     M24VA_FrameConvert
#define GXVA_Initialize()                           M24VA_Initialise(gx_pvoid)
#define GXVA_Deinitialize                          M24VA_Deinitialise
#define GXVA_Reset                                   M24VA_Reset
#define GXVA_SimScriptOpen                    M24VA_SimScriptOpen
#define GXVA_SimScriptClose                   M24VA_SimScriptClose
#define GXVA_SimScriptFlush                   M24VA_SimScriptFlush
#define GXVA_SimScriptLog                      M24VA_SimScriptLog
#define GXVA_SetScriptMode                    M24VA_SetScriptMode
#define GXVA_GetStats                             M24VA_GetStats
#define GXVA_GetSlavePortStats             M24VA_GetSlavePortStats

#define VDISP_Initialize()              VDISP_Initialise(gx_pvoid)
#define VDISP_Deinitialize              VDISP_Deinitialise
