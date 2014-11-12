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
 * $Id: audio.h 304 2005-10-20 11:02:59Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __AUDIO_H
#define __AUDIO_H

#define AUDIOFMT_PCM			0x01
#define AUDIOFMT_ADPCM_MS		0x02
#define AUDIOFMT_ALAW			0x06
#define AUDIOFMT_MULAW			0x07
#define AUDIOFMT_ADPCM_IMA		0x11
#define AUDIOFMT_ADPCM_IMA_QT	0x10011
#define AUDIOFMT_ADPCM_G726		0x45
#define AUDIOFMT_MP2			0x50
#define AUDIOFMT_MP3			0x55
#define AUDIOFMT_MSA			0x160
#define AUDIOFMT_WMA9			0x161
#define AUDIOFMT_WMA9PRO		0x162
#define AUDIOFMT_WMA9LL			0x163
#define AUDIOFMT_WMA9V			0x0A
#define AUDIOFMT_A52			0x2000
#define AUDIOFMT_AAC			0xAAC0
#define	AUDIOFMT_QDESIGN2		0x450
#define AUDIOFMT_TTA			0x77A1

// not sure about this...
#define AUDIOFMT_AMR_NB			0x57
#define AUDIOFMT_AMR_WB			0x58

#define AUDIOFMT_VORBIS_MODE1	0x674F
#define AUDIOFMT_VORBIS_MODE1P	0x676F
#define AUDIOFMT_VORBIS_MODE2	0x6750
#define AUDIOFMT_VORBIS_MODE2P	0x6770
#define AUDIOFMT_VORBIS_MODE3	0x6751
#define AUDIOFMT_VORBIS_MODE3P	0x6771
#define AUDIOFMT_VORBIS_PACKET	0x2674F
#define AUDIOFMT_VORBIS_INTERNAL_VIDEO 0x1674F
#define AUDIOFMT_VORBIS_INTERNAL_AUDIO 0x3674F

// not official! just internal usage
#define AUDIOFMT_QCELP			0xCC00
#define AUDIOFMT_SPEEX			0xCD00

#define SPEED_ONE			(1<<16)

// pcm flags
#define PCM_SWAPPEDSTEREO	0x0001
#define PCM_PLANES			0x0002
#define PCM_UNSIGNED		0x0004
#define PCM_ONLY_LEFT		0x0008
#define PCM_ONLY_RIGHT		0x0010
#define PCM_PACKET_BASED	0x0020
#define PCM_SWAPEDBYTES		0x0040
#define PCM_FLOAT			0x0080

#define PCM_LIFEDRIVE_FIX	0x1000

typedef struct audio
{
	int Format;
	int Channels;	
	int SampleRate;
	int BlockAlign;
	int Flags;
	int Bits;		
	int FracBits;	

} audio;

DLL void PCM_Init();
DLL void PCM_Done();

DLL struct pcm_soft* PCMCreate(const audio* DstFormat, const audio* SrcFormat, bool_t Dither, bool_t UseVolume);
DLL void PCMReset(struct pcm_soft* Handle);
DLL void PCMConvert(struct pcm_soft* Handle, const planes Dst, const constplanes Src, int* DstLength, int* SrcLength, int Speed, int Volume);
DLL int PCMDstLength(struct pcm_soft* Handle, int SrcLength);
DLL void PCMRelease(struct pcm_soft* Handle);

//---------------------------------------------------------------
// audio output (abstract)

#define STEREO_NORMAL		0
#define STEREO_SWAPPED		1
#define STEREO_JOINED		2
#define STEREO_LEFT			3
#define STEREO_RIGHT		4

#define AOUT_CLASS			FOURCC('A','O','U','T')
// output volume (int 0..100)
#define AOUT_VOLUME			0x51
// volume mute (bool_t)
#define AOUT_MUTE			0x52
// panning (int)
#define AOUT_PAN			0x5C
// preamp (int)
#define AOUT_PREAMP			0x5D
// quality (int enum string)
#define AOUT_QUALITY		0x53
// stereo mode (int enum string)
#define AOUT_STEREO			0x59
// video mode for buffer length (bool)
#define AOUT_MODE			0x5A
// audio device as timer
#define AOUT_TIMER			0x5B

void Audio_Init();
void Audio_Done();

DLL int AOutEnum(void* p, int* No, datadef* Param);

#endif
