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
 * $Id: pcm_soft.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PCM_SOFT_H
#define __PCM_SOFT_H

typedef struct pcmstate
{
	int Speed;
	int Step;
	int Pos;
	int Dither[MAXPLANES];

} pcmstate;

typedef void (*pcmsoftentry)(struct pcm_soft* This,const planes DstPtr,const constplanes SrcPtr,
							 int DstLength,pcmstate* State,int Volume);

typedef struct pcm_soft
{
	pcmsoftentry Entry;
	pcmstate State;

	dyncode Code;
	audio Src;
	audio Dst;
	bool_t Dither;

	int SrcType;
	int DstType;
	int SrcShift;
	int DstShift;
	int SrcUnsigned;
	int DstUnsigned;
	bool_t Stereo;
	bool_t Join;
	bool_t ActualDither;
	bool_t UseLeft;
	bool_t UseRight;
	bool_t UseVolume;
	bool_t LifeDriveFix;
	bool_t SrcSwap;
	bool_t DstSwap;
	int Clip;
	int Shift;
	int Limit2;
	int MaxLimit;
	int MinLimit;
	int SrcChannels;

	dyninst* InstSrcUnsigned;
	dyninst* InstDstUnsigned;

	struct pcm_soft* Next;

} pcm_soft;

void PCMCompile(pcm_soft*);

#endif
	
