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
 * $Id: waveout.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __WAVEOUT_H
#define __WAVEOUT_H

#define WAVEOUT_ID			FOURCC('W','A','V','E')

typedef struct waveout_base
{
	node Node;

	nodefunc Done;
	nodefunc Init;
	packetprocess Process;

	node Timer;
	pin Pin;
	packetformat Input;
	packetformat Output;
	int Total;
	int Dropped;
	int Stereo;
	bool_t Dither;
	int Quality;
	tick_t BufferTime;
	void* PCM;

} waveout_base;

void WaveOut_Init();
void WaveOut_Done();

int WaveOutBaseGet(waveout_base* p, int No, void* Data, int Size);
int WaveOutBaseSet(waveout_base* p, int No, const void* Data, int Size);
int WaveOutPCM(waveout_base* p);

#define RAMPSIZE		15
#define RAMPLIMIT		(1<<RAMPSIZE)

int VolumeRamp(int Ramp,void* Data,int Length,const audio* Format);
void VolumeMul(int Vol,void* Data,int Length,const audio* Format);

#endif
