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
 * $Id: pcm_soft.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "pcm_soft.h"

static INLINE int PCMShift(const audio* Format)
{
	int Bit = 0;
	if (Format->Bits>8) Bit = 1;
	if (Format->Bits>16) Bit = 2;
	if (!(Format->Flags & PCM_PLANES) && Format->Channels==2)
		++Bit;
	return Bit;
}

void PCM_Init()
{
}

void PCM_Done()
{
	pcm_soft* p;
	while ((p = Context()->PCM)!=NULL)
	{
		Context()->PCM = p->Next;
		CodeDone(&p->Code);
		free(p);
	}
}

#if (!defined(ARM) && !defined(SH3) && !defined(MIPS)) || !defined(CONFIG_DYNCODE)

void PCM_S16_To_S16_Stereo(pcm_soft* This,const planes DstPtr,const constplanes SrcPtr,int DstLength,pcmstate* State,int Volume)
{
	const int32_t* Src = SrcPtr[0];
	int32_t* Dst = DstPtr[0];
	int32_t* DstEnd = Dst + (DstLength >> 2);

	if (State->Step == 256)
		memcpy(Dst,Src,DstLength);
	else
	{
		int Pos = State->Pos;
		int Step = State->Step;

		while (Dst != DstEnd)
		{
			*(Dst++) = Src[Pos >> 8];
			Pos += Step;
		}
	}
}

void PCM_P32_To_S16_Stereo(pcm_soft* This,const planes DstPtr,const constplanes SrcPtr,int DstLength,pcmstate* State,int Volume)
{
	const int32_t* SrcA = SrcPtr[0];
	const int32_t* SrcB = SrcPtr[1];
	int16_t* Dst = DstPtr[0];
	int16_t* DstEnd = Dst + (DstLength >> 1);

	int UpLimit = (1 << This->Src.FracBits) - 1;
	int DownLimit = -(1 << This->Src.FracBits);
	int Shift = This->Src.FracBits - This->Dst.FracBits;

	if (State->Step == 256)
	{
		while (Dst != DstEnd)
		{
			int Left = *(SrcA++);
			int Right = *(SrcB++);

			if (Left > UpLimit) Left = UpLimit;
			if (Left < DownLimit) Left = DownLimit;

			if (Right > UpLimit) Right = UpLimit;
			if (Right < DownLimit) Right = DownLimit;

			Dst[0] = (int16_t)(Left >> Shift);
			Dst[1] = (int16_t)(Right >> Shift);
			Dst += 2;
		}
	}
	else
	{
		int Pos = State->Pos;
		int Step = State->Step;

		while (Dst != DstEnd)
		{
			int Left = SrcA[Pos >> 8];
			int Right = SrcB[Pos >> 8];

			if (Left > UpLimit) Left = UpLimit;
			if (Left < DownLimit) Left = DownLimit;

			if (Right > UpLimit) Right = UpLimit;
			if (Right < DownLimit) Right = DownLimit;

			Dst[0] = (int16_t)(Left >> Shift);
			Dst[1] = (int16_t)(Right >> Shift);
			Dst += 2;
			Pos += Step;
		}
	}
}

void PCMUniversal(pcm_soft* This,const planes DstPtr,const constplanes SrcPtr,int DstLength,pcmstate* State,int Volume)
{
	const uint8_t* Src[2];
	const uint8_t* DstEnd;
	uint8_t* Dst[2];
	bool_t SrcSwap = This->SrcSwap;
	bool_t DstSwap = This->DstSwap;
	int DstType = This->DstType;
	int SrcType = This->SrcType;
	int DstStep = 1 << This->DstShift;
	int Pos = State->Pos;
	int Step = State->Step;

	float SrcScale = 0;
	float DstScale = 0;

	int SrcUnsigned = This->SrcUnsigned;
	int DstUnsigned = This->DstUnsigned;

	int UpLimit = (1 << This->Src.FracBits) - 1;
	int DownLimit = -(1 << This->Src.FracBits);

	int SrcShift = 31 - This->Src.FracBits;
	int DstShift = 31 - This->Dst.FracBits;

	bool_t OnlyLeft = (This->Dst.Flags & PCM_ONLY_LEFT) != 0;
	bool_t OnlyRight = (This->Dst.Flags & PCM_ONLY_RIGHT) != 0;

	bool_t VolumeShift = This->Src.Bits >= 23;
	if (!This->UseVolume)
		Volume = 256;

	if (SrcType >= 12) SrcScale = (float)(1 << This->Src.FracBits);
	if (DstType >= 12) DstScale = 1.0f/(1 << This->Dst.FracBits);

	Dst[0] = (uint8_t*) DstPtr[0];
	Dst[1] = (uint8_t*) DstPtr[1];
	Src[0] = (uint8_t*) SrcPtr[0];
	Src[1] = (uint8_t*) SrcPtr[1];

	DstEnd = Dst[0] + DstLength;

	// mono,packed
	// mono,planar
	// stereo,packed
	// stereo,planar

	while (Dst[0] != DstEnd)
	{
		int Left;
		int Right;
	
		if (SrcUnsigned)
			switch (SrcType)
			{
			default:
			case 0:
			case 1:
				Left = Right = ((uint8_t*)Src[0])[Pos >> 8] - SrcUnsigned;
				break;
			case 2:
				Left =  ((uint8_t*)Src[0])[(Pos >> 8)*2] - SrcUnsigned;
				Right = ((uint8_t*)Src[0])[(Pos >> 8)*2+1] - SrcUnsigned;
				break;
			case 3:
				Left =  ((uint8_t*)Src[0])[(Pos >> 8)] - SrcUnsigned;
				Right = ((uint8_t*)Src[1])[(Pos >> 8)] - SrcUnsigned;
				break;

			case 4:
			case 5:
				Left = ((uint16_t*)Src[0])[Pos >> 8];
				if (SrcSwap)
					Left = SWAP16(Left);
				Left -= SrcUnsigned;
				Right = Left;
				break;
			case 6:
				Left =  ((uint16_t*)Src[0])[(Pos >> 8)*2];
				Right = ((uint16_t*)Src[0])[(Pos >> 8)*2+1];
				if (SrcSwap)
				{
					Left = SWAP16(Left);
					Right = SWAP16(Right);
				}
				Left -= SrcUnsigned;
				Right -= SrcUnsigned;
				break;
			case 7:
				Left =  ((uint16_t*)Src[0])[(Pos >> 8)];
				Right = ((uint16_t*)Src[1])[(Pos >> 8)];
				if (SrcSwap)
				{
					Left = SWAP16(Left);
					Right = SWAP16(Right);
				}
				Left -= SrcUnsigned;
				Right -= SrcUnsigned;
				break;

			case 8:
			case 9:
				Left = ((uint32_t*)Src[0])[Pos >> 8];
				if (SrcSwap)
					Left = SWAP32(Left);
				Left -= SrcUnsigned;
				Right = Left;
				break;
			case 10:
				Left =  ((uint32_t*)Src[0])[(Pos >> 8)*2];
				Right = ((uint32_t*)Src[0])[(Pos >> 8)*2+1];
				if (SrcSwap)
				{
					Left = SWAP32(Left);
					Right = SWAP32(Right);
				}
				Left -= SrcUnsigned;
				Right -= SrcUnsigned;
				break;
			case 11:
				Left =  ((uint32_t*)Src[0])[(Pos >> 8)];
				Right = ((uint32_t*)Src[1])[(Pos >> 8)];
				if (SrcSwap)
				{
					Left = SWAP32(Left);
					Right = SWAP32(Right);
				}
				Left -= SrcUnsigned;
				Right -= SrcUnsigned;
				break;
			}
		else
			switch (SrcType)
			{
			default:
			case 0:
			case 1:
				Left = Right = ((int8_t*)Src[0])[Pos >> 8];
				break;
			case 2:
				Left =  ((int8_t*)Src[0])[(Pos >> 8)*2];
				Right = ((int8_t*)Src[0])[(Pos >> 8)*2+1];
				break;
			case 3:
				Left =  ((int8_t*)Src[0])[(Pos >> 8)];
				Right = ((int8_t*)Src[1])[(Pos >> 8)];
				break;

			case 4:
			case 5:
				Left = ((int16_t*)Src[0])[Pos >> 8];
				if (SrcSwap)
					Left = (int16_t)SWAP16(Left);
				Right = Left;
				break;
			case 6:
				Left = ((int16_t*)Src[0])[(Pos >> 8)*2];
				Right = ((int16_t*)Src[0])[(Pos >> 8)*2+1];
				if (SrcSwap)
				{
					Left = (int16_t)SWAP16(Left);
					Right = (int16_t)SWAP16(Right);
				}
				break;
			case 7:
				Left =  ((int16_t*)Src[0])[(Pos >> 8)];
				Right = ((int16_t*)Src[1])[(Pos >> 8)];
				if (SrcSwap)
				{
					Left = (int16_t)SWAP16(Left);
					Right = (int16_t)SWAP16(Right);
				}
				break;

			case 8:
			case 9:
				Left = ((int32_t*)Src[0])[Pos >> 8];
				if (SrcSwap)
					Left = (int32_t)SWAP32(Left);
				Right = Left;
				break;
			case 10:
				Left =  ((int32_t*)Src[0])[(Pos >> 8)*2];
				Right = ((int32_t*)Src[0])[(Pos >> 8)*2+1];
				if (SrcSwap)
				{
					Left = (int32_t)SWAP32(Left);
					Right = (int32_t)SWAP32(Right);
				}
				break;
			case 11:
				Left =  ((int32_t*)Src[0])[(Pos >> 8)];
				Right = ((int32_t*)Src[1])[(Pos >> 8)];
				if (SrcSwap)
				{
					Left = (int32_t)SWAP32(Left);
					Right = (int32_t)SWAP32(Right);
				}
				break;
			case 12:
			case 13:
				Left = (int)(((float*)Src[0])[Pos >> 8]*SrcScale);
				Right = Left;
				break;
			case 14:
				Left =  (int)(((float*)Src[0])[(Pos >> 8)*2]*SrcScale);
				Right = (int)(((float*)Src[0])[(Pos >> 8)*2+1]*SrcScale);
				break;
			case 15:
				Left =  (int)(((float*)Src[0])[(Pos >> 8)]*SrcScale);
				Right = (int)(((float*)Src[1])[(Pos >> 8)]*SrcScale);
				break;
			}

		if (Volume != 256)
		{
			if (VolumeShift)
			{
				Left = (Left >> 8) * Volume;
				Right = (Right >> 8) * Volume;
			}
			else
			{
				Left = (Left * Volume) >> 8;
				Right = (Right * Volume) >> 8;
			}
		}

		if (Left > UpLimit) Left = UpLimit;
		if (Left < DownLimit) Left = DownLimit;

		if (Right > UpLimit) Right = UpLimit;
		if (Right < DownLimit) Right = DownLimit;

		Left <<= SrcShift;
		Right <<= SrcShift;

		Left >>= DstShift;
		Right >>= DstShift;

		if (OnlyLeft)
			Right = Left;
		if (OnlyRight)
			Left = Right;

		if ((This->Src.Flags ^ This->Dst.Flags) & PCM_SWAPPEDSTEREO)
			SwapInt(&Left,&Right);

		switch (DstType)
		{
		default:
		case 0:
		case 1:
			((int8_t*)Dst[0])[0] = (int8_t)(((Left + Right) >> 1) + DstUnsigned);
			break;
		case 2:
			((int8_t*)Dst[0])[0] = (int8_t)(Left + DstUnsigned);
			((int8_t*)Dst[0])[1] = (int8_t)(Right + DstUnsigned);
			break;
		case 3:
			((int8_t*)Dst[0])[0] = (int8_t)(Left + DstUnsigned);
			((int8_t*)Dst[1])[0] = (int8_t)(Right + DstUnsigned);
			break;

		case 4:
		case 5:
			Left = ((Left + Right) >> 1) + DstUnsigned;
			if (DstSwap)
				Left = SWAP16(Left);
			((int16_t*)Dst[0])[0] = (int16_t)Left;
			break;
		case 6:
			Left += DstUnsigned;
			Right += DstUnsigned;
			if (DstSwap)
			{
				Left = SWAP16(Left);
				Right = SWAP16(Right);
			}
			((int16_t*)Dst[0])[0] = (int16_t)Left;
			((int16_t*)Dst[0])[1] = (int16_t)Right;
			break;
		case 7:
			Left += DstUnsigned;
			Right += DstUnsigned;
			if (DstSwap)
			{
				Left = SWAP16(Left);
				Right = SWAP16(Right);
			}
			((int16_t*)Dst[0])[0] = (int16_t)Left;
			((int16_t*)Dst[1])[0] = (int16_t)Right;
			break;

		case 8:
		case 9:
			Left = ((Left + Right) >> 1) + DstUnsigned;
			if (DstSwap)
				Left = SWAP32(Left);
			((int32_t*)Dst[0])[0] = Left;
			break;
		case 10:
			Left += DstUnsigned;
			Right += DstUnsigned;
			if (DstSwap)
			{
				Left = SWAP32(Left);
				Right = SWAP32(Right);
			}
			((int32_t*)Dst[0])[0] = Left;
			((int32_t*)Dst[0])[1] = Right;
			break;
		case 11:
			Left += DstUnsigned;
			Right += DstUnsigned;
			if (DstSwap)
			{
				Left = SWAP32(Left);
				Right = SWAP32(Right);
			}
			((int32_t*)Dst[0])[0] = Left;
			((int32_t*)Dst[1])[0] = Right;
			break;
		case 12:
		case 13:
			Left = (Left + Right) >> 1;
			((float*)Dst[0])[0] = (float)Left*DstScale;
			break;
		case 14:
			((float*)Dst[0])[0] = (float)Left*DstScale;
			((float*)Dst[0])[1] = (float)Right*DstScale;
			break;
		case 15:
			((float*)Dst[0])[0] = (float)Left*DstScale;
			((float*)Dst[1])[0] = (float)Right*DstScale;
			break;
		}

		Pos += Step;
		Dst[0] += DstStep;
		Dst[1] += DstStep;
	}
}

static int UniversalType(const audio* Format)
{
	int Type = (Format->Channels - 1)*2;
	if (Format->Flags & PCM_PLANES) Type |= 1;
	if (Format->Flags & PCM_FLOAT) 
		Type += 12;
	else
	{
		if (Format->Bits>8) Type += 4;
		if (Format->Bits>16) Type += 4;
	}
	return Type;
}
	
#endif

pcm_soft* PCMCreate(const audio* DstFormat, const audio* SrcFormat, bool_t Dither, bool_t UseVolume)
{
	pcm_soft* p = Context()->PCM;
	if (p)
		Context()->PCM = p->Next;
	else
	{
		p = (pcm_soft*)malloc(sizeof(pcm_soft));
		if (!p)
			return NULL;

		memset(p,0,sizeof(pcm_soft));
		CodeInit(&p->Code);
	}

	if (!EqAudio(&p->Dst,DstFormat) ||
		!EqAudio(&p->Src,SrcFormat) ||
		p->UseVolume != UseVolume || 
		p->Dither != Dither)
	{
		p->Entry = NULL;
		p->Dst = *DstFormat;
		p->Src = *SrcFormat;
		p->Dither = Dither;
		p->UseVolume = UseVolume;

		// only PCM streams are supported
		if (p->Dst.Format==AUDIOFMT_PCM && p->Src.Format==AUDIOFMT_PCM) 
		{
			p->SrcSwap = (p->Src.Flags & PCM_SWAPEDBYTES)!=0 && (p->Src.Bits>8);
			p->DstSwap = (p->Dst.Flags & PCM_SWAPEDBYTES)!=0 && (p->Dst.Bits>8);
			p->SrcShift = PCMShift(&p->Src);
			p->DstShift = PCMShift(&p->Dst);

			p->SrcUnsigned = (p->Src.Flags & PCM_UNSIGNED) ? (1 << (p->Src.Bits-1)):0;
			p->DstUnsigned = (p->Dst.Flags & PCM_UNSIGNED) ? (1 << (p->Dst.Bits-1)):0;

			p->UseLeft = (p->Dst.Flags & PCM_ONLY_RIGHT)==0 || (p->Src.Channels <= 1);
			p->UseRight = (p->Dst.Flags & PCM_ONLY_LEFT)==0 && (p->Src.Channels > 1);
			p->SrcChannels = (p->UseLeft && p->UseRight) ? 2:1;

			p->Clip = (p->Src.Bits - p->Src.FracBits) - (p->Dst.Bits - p->Dst.FracBits);
			p->Stereo = p->SrcChannels > 1 && p->Dst.Channels > 1;
			p->Join = !p->Stereo && (p->SrcChannels > 1);
			
			p->Shift = p->Dst.FracBits - p->Src.FracBits - p->Join;
			p->LifeDriveFix = !p->UseVolume && !(p->Src.Flags & PCM_LIFEDRIVE_FIX) && (p->Dst.Flags & PCM_LIFEDRIVE_FIX) && p->Dst.Bits==16;
			p->ActualDither = !p->LifeDriveFix && !p->UseVolume && p->Dither && ((p->Clip>0) || (p->Shift<0));
			
			p->Limit2 = p->Src.Bits + p->Join - p->Clip - 1;

			if (p->UseVolume && p->Src.Bits < 23)
			{
				p->Shift -= 8;
				p->Limit2 += 8;
			}

			p->MaxLimit = (1 << p->Limit2) - 1;
			p->MinLimit = -(1 << p->Limit2);

			p->State.Speed = -1;
			p->State.Step = 256;

#if (defined(ARM) || defined(SH3) || defined(MIPS)) && defined(CONFIG_DYNCODE)
			CodeStart(&p->Code);
			PCMCompile(p);
			CodeBuild(&p->Code);
			if (p->Code.Size)
				p->Entry = (pcmsoftentry)p->Code.Code;
#else
			p->SrcType = UniversalType(&p->Src);
			p->DstType = UniversalType(&p->Dst);
			p->Entry = PCMUniversal;

			if (!p->UseVolume && !p->DstUnsigned && p->DstType==6 &&
				!((p->Src.Flags ^ p->Dst.Flags) & PCM_SWAPPEDSTEREO) &&
				!p->SrcSwap && !p->DstSwap && 
				!(p->Dst.Flags & (PCM_ONLY_LEFT|PCM_ONLY_RIGHT)))
			{
				if (p->Src.FracBits >= p->Dst.FracBits && !p->SrcUnsigned && p->SrcType==11 && !p->SrcSwap && !p->DstSwap)
					p->Entry = PCM_P32_To_S16_Stereo;
				else
				if (p->Src.FracBits == p->Dst.FracBits && !p->SrcUnsigned && p->SrcType==6)
					p->Entry = PCM_S16_To_S16_Stereo;
			}
#endif
		}
	}

	if (!p->Entry)
	{
		PCMRelease(p);
		p = NULL;
	}

	PCMReset(p);
	return p;
}

void PCMRelease(pcm_soft* p)
{
	if (p)
	{
		p->Next = Context()->PCM;
		Context()->PCM = p;
	}
}

int PCMDstLength(pcm_soft* p, int SrcLength)
{
	return (SrcLength >> p->SrcShift) << p->DstShift;
}

void PCMConvert(pcm_soft* p, const planes Dst, const constplanes Src, int* DstLength, int* SrcLength, int Speed, int Volume)
{
	int SrcSamples;
	int DstSamples;
	int AvailSamples;
	if (!p || !p->Entry) return;

	SrcSamples = *SrcLength >> p->SrcShift;
	DstSamples = *DstLength >> p->DstShift;

	if (p->State.Speed != Speed)
	{
		p->State.Speed = Speed;
		p->State.Step = Scale(Speed,p->Src.SampleRate,p->Dst.SampleRate*256);
		if (p->State.Step<=0)
			p->State.Step = 256;
	}

	if (p->State.Step != 256)
	{
		AvailSamples = ((SrcSamples << 8) - p->State.Pos + p->State.Step - 1) / p->State.Step;
		if (DstSamples > AvailSamples)
			DstSamples = AvailSamples;

		AvailSamples = (p->State.Pos + DstSamples * p->State.Step) >> 8;
		if (SrcSamples > AvailSamples)
			SrcSamples = AvailSamples;
	}
	else
	{
		if (DstSamples > SrcSamples)
			DstSamples = SrcSamples;
		SrcSamples = DstSamples;
	}
	
	*SrcLength = SrcSamples << p->SrcShift;
	*DstLength = DstSamples << p->DstShift;
	
	if (DstSamples)
		p->Entry(p,Dst,Src,*DstLength,&p->State,Volume);

	p->State.Pos += (DstSamples * p->State.Step) - (SrcSamples << 8);
}

void PCMReset(pcm_soft* p)
{
	if (p)
	{
		p->State.Pos = 0;
		p->State.Dither[0] = 0;
		p->State.Dither[1] = 0;
	}
}

