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
 * $Id: noplugins.c 593 2006-01-17 22:25:08Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../common/softidct/softidct.h"
#include "../common/playlist/m3u.h"
#include "../common/playlist/pls.h"
#include "../common/playlist/asx.h"
#include "../camera/law.h"
#include "../camera/adpcm.h"
#include "../camera/mjpeg.h"
#include "../camera/tiff.h"
#include "../camera/png.h"
#include "../splitter/avi.h"
#include "../splitter/asf.h"
#include "../splitter/wav.h"
#include "../splitter/mov.h"
#include "../splitter/mpg.h"
#include "../splitter/nsv.h"
#include "../matroska/matroska.h"
#include "../aac/faad.h"
#include "../libmad/libmad.h"
#include "../mpeg1/mpeg1.h"
#include "../vorbislq/vorbis.h"
#include "../mpc/mpc.h"
#include "../ac3/ac3.h"
#include "../amr/amrnb.h"
#include "../sonyhhe/ge2d.h"
#include "../zodiac/ati4200.h"
#include "../interface/win.h"
#include "../interface/about.h"
#include "../interface/benchresult.h"
#include "../interface/mediainfo.h"
#include "../interface/settings.h"
#include "../ati3200/ati3200.h"
#include "../intel2700g/intel2700g.h"
#include "../network/http.h"
#include "../ffmpeg/ffmpeg.h"

LIBGCC
LIBGCCFLOAT

void Static_Init()
{
	//splitter
	ASF_Init();
	AVI_Init();
	WAV_Init();
	MP4_Init();
	MPG_Init();
	NSV_Init();

	//camera
	MJPEG_Init();
	TIFF_Init();
	PNG_Init();
#if !defined(TARGET_PALMOS)
	Law_Init();
	ADPCM_Init();
#endif
	//matroska
	Matroska_Init();
	//mp3
	LibMad_Init();
	//mpeg1
	MPEG1_Init();
	MVES_Init();
	//vorbis
	OGG_Init();
	OGGEmbedded_Init();
	OGGPacket_Init();
	Vorbis_Init();
	//mpeg4aac
	FAAD_Init();
	//mpc
	MPC_Init();
	//ffmpeg
	FFMPEG_Init();

#if !defined(TARGET_PALMOS) || defined(_M_IX86)
	//amr
	AMRNB_Init();
#endif
	//ac3
	AC3_Init();

#if defined(TARGET_PALMOS)
	//sonyhhe
	GE2D_Init();
	ATI4200_Init();
#endif

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)
	HTTP_Init();
	ATI3200_Init();
	Intel2700G_Init();

	Win_Init();
	About_Init();
	BenchResult_Init();
	MediaInfo_Init();
	Settings_Init();
#endif
}

void Static_Done()
{

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)
	About_Done();
	BenchResult_Done();
	MediaInfo_Done();
	Settings_Done();
	Win_Done();

	HTTP_Done();
	ATI3200_Done();
	Intel2700G_Done();
#endif

#if !defined(TARGET_PALMOS) || defined(_M_IX86)
	//amr
	AMRNB_Done();
#endif
	//ac3
	AC3_Done();

#if defined(TARGET_PALMOS)
	//sonyhhe
	GE2D_Done();
	ATI4200_Done();
#endif

	//ffmpeg
	FFMPEG_Done();
	//mpc
	MPC_Done();
	//mpeg4aac
	FAAD_Done();
	//vorbis
	OGG_Done();
	OGGEmbedded_Done();
	OGGPacket_Done();
	Vorbis_Done();
	//mpeg1
	MPEG1_Done();
	MVES_Done();
	//mp3
	LibMad_Done();
	//matroska
	Matroska_Done();
	//camera
	MJPEG_Done();
	TIFF_Done();
	PNG_Done();
#if !defined(TARGET_PALMOS)
	Law_Done();
	ADPCM_Done();
#endif
	//splitter
	MP4_Done();
	ASF_Done();
	AVI_Done();
	WAV_Done();
	MPG_Done();
	NSV_Done();
}

void DLLTest() {}
