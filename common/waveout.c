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
 * $Id: waveout.c 548 2006-01-08 22:41:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

void VolumeMul(int Vol,void* Dst,int DstLength,const audio* Format)
{
	uint8_t *u8,*u8e;
	int16_t *s16,*s16e;

	switch (Format->Bits)
	{
	case 8:
		u8=Dst;
		u8e=u8+DstLength;
		for (;u8!=u8e;++u8)
		{
			int i = (((*u8 - 0x80) * Vol) >> 8) + 0x80;
			*u8 = (uint8_t)(i>255?255:(i<0?0:i));
		}
		break;

	case 16:
		s16=Dst;
		s16e=s16+(DstLength>>1);
		for (;s16!=s16e;++s16)
		{
			int i = (*s16 * Vol) >> 8;
			*s16 = (int16_t)(i>32767?32767:(i<-32768?-32768:i));
		}
		break;
	}
}

int VolumeRamp(int Ramp,void* Dst,int DstLength,const audio* Format)
{
	uint8_t *u8,*u8e;
	int16_t *s16,*s16e;
	int Inc = (10*(1<<RAMPSIZE)) / (Format->Channels * Format->SampleRate);
	if (Inc<=0) Inc=1;

	switch (Format->Bits)
	{
	case 8:
		u8=Dst;
		u8e=u8+DstLength;
		for (;u8!=u8e && Ramp<RAMPLIMIT;++u8,Ramp+=Inc)
			*u8 = (uint8_t)((((*u8 - 0x80) * Ramp) >> RAMPSIZE) + 0x80);
		break;

	case 16:
		s16=Dst;
		s16e=s16+(DstLength>>1);
		for (;s16!=s16e && Ramp<RAMPLIMIT;++s16,Ramp+=Inc)
			*s16 = (int16_t)((*s16 * Ramp) >> RAMPSIZE);
		break;

	default:
		Ramp = RAMPLIMIT;
		break;
	}

	return Ramp;
}

static void AlignRate(int* Rate,int Target)
{
	if (*Rate > Target-16 && *Rate < Target+16)
		*Rate = Target;
}

static int UpdateInput(waveout_base* p)
{
	if (p->Done)
		p->Done(p);

	p->Total = 0;
	p->Dropped = 0;

	if (p->Input.Type == PACKET_AUDIO)
	{
		int Result = ERR_NONE;
		int Try;

		if (p->Input.Format.Audio.Format != AUDIOFMT_PCM) 
		{
			PacketFormatClear(&p->Input);
			return ERR_INVALID_DATA;
		}

		if (p->Input.Format.Audio.Channels == 0 ||
			p->Input.Format.Audio.SampleRate == 0)
			return ERR_NONE;

		AlignRate(&p->Input.Format.Audio.SampleRate,8000);
		AlignRate(&p->Input.Format.Audio.SampleRate,11025);
		AlignRate(&p->Input.Format.Audio.SampleRate,16000);
		AlignRate(&p->Input.Format.Audio.SampleRate,22050);
		AlignRate(&p->Input.Format.Audio.SampleRate,44100);

		PacketFormatClear(&p->Output);
		p->Output.Type = PACKET_AUDIO;
		p->Output.Format.Audio = p->Input.Format.Audio;
		p->Output.Format.Audio.Flags = 0;
		p->Dither = 0;

		if (p->Stereo==STEREO_SWAPPED)
			p->Output.Format.Audio.Flags |= PCM_SWAPPEDSTEREO;
		else
		if (p->Stereo!=STEREO_NORMAL)
		{
			p->Output.Format.Audio.Channels = 1;
			if (p->Stereo==STEREO_LEFT)
				p->Output.Format.Audio.Flags |= PCM_ONLY_LEFT;
			if (p->Stereo==STEREO_RIGHT)
				p->Output.Format.Audio.Flags |= PCM_ONLY_RIGHT;
		}

		switch (p->Quality)
		{
		case 0: // low quality for very poor devices
			p->Output.Format.Audio.Bits = 8;
			p->Output.Format.Audio.FracBits = 7;
			p->Output.Format.Audio.Channels = 1;
			p->Output.Format.Audio.SampleRate = 22050;
			break;

		case 1: // no dither and only standard samplerate
			if (p->Output.Format.Audio.Bits > 8)
			{
				p->Output.Format.Audio.Bits = 16;
				p->Output.Format.Audio.FracBits = 15;
			}
			else
			{
				p->Output.Format.Audio.Bits = 8;
				p->Output.Format.Audio.FracBits = 7;
			}
			if (p->Output.Format.Audio.SampleRate >= 44100)
				p->Output.Format.Audio.SampleRate = 44100;
			else
				p->Output.Format.Audio.SampleRate = 22050;
			break;

		default:
		case 2: // original samplerate 
			if (p->Output.Format.Audio.Bits > 8)
			{
				p->Output.Format.Audio.Bits = 16;
				p->Output.Format.Audio.FracBits = 15;
			}
			else
			{
				p->Output.Format.Audio.Bits = 8;
				p->Output.Format.Audio.FracBits = 7;
			}
			p->Dither = 1;
			break;
		}

		for (Try=0;;++Try)
		{
			if (p->Output.Format.Audio.Bits <= 8)
				p->Output.Format.Audio.Flags |= PCM_UNSIGNED;

			Result = p->Init(p);
			if (Result == ERR_NONE)
				break;

			if (p->Done)
				p->Done(p);

			if (Try==0)
			{
				if (p->Output.Format.Audio.SampleRate > 36000)
					p->Output.Format.Audio.SampleRate = 44100;
				else
					++Try;
			}
			if (Try==1)
			{
				if (p->Output.Format.Audio.SampleRate != 22050)
					p->Output.Format.Audio.SampleRate = 22050;
				else
					++Try;
			}
#ifdef TARGET_SYMBIAN
			if (Try==2)
			{
				if (p->Output.Format.Audio.SampleRate != 16000)
					p->Output.Format.Audio.SampleRate = 16000;
				else
					++Try;
			}
			if (Try==3)
			{
				if (p->Output.Format.Audio.SampleRate != 11025)
					p->Output.Format.Audio.SampleRate = 11025;
				else
					++Try;
			}
			if (Try==4)
			{
				if (p->Output.Format.Audio.SampleRate != 8000)
					p->Output.Format.Audio.SampleRate = 8000;
				else
					++Try;
			}
			if (Try==5)
				break;
#else
			if (Try==2)
			{
				if (p->Output.Format.Audio.Channels > 1)
					p->Output.Format.Audio.Channels = 1;
				else
					++Try;
			}
			if (Try==3)
			{
				if (p->Output.Format.Audio.Bits != 8)
				{
					p->Output.Format.Audio.Bits = 8;
					p->Output.Format.Audio.FracBits = 7;
				}
				else
					++Try;
			}
			if (Try==4)
				break;
#endif
		}

		if (Try==5)
		{
			PacketFormatClear(&p->Input);
			if (Result == ERR_DEVICE_ERROR)
				ShowError(p->Node.Class,ERR_ID,Result);
			return Result;
		}
	}
	else
	if (p->Input.Type != PACKET_NONE)
		return ERR_INVALID_DATA;

	return ERR_NONE;
}

int WaveOutPCM(waveout_base* p)
{
	PCMRelease(p->PCM);
	p->PCM = PCMCreate(&p->Output.Format.Audio,&p->Input.Format.Audio,p->Dither,0);
	return p->PCM ? ERR_NONE:ERR_OUT_OF_MEMORY;
}

int WaveOutBaseGet(waveout_base* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT: GETVALUE(p->Pin,pin); break;
	case OUT_INPUT|PIN_FORMAT: GETVALUE(p->Input,packetformat); break;
	case OUT_INPUT|PIN_PROCESS: GETVALUE(p->Input.Format.Audio.Channels?p->Process:DummyProcess,packetprocess); break;
	case OUT_OUTPUT|PIN_FORMAT: GETVALUE(p->Output,packetformat); break;
	case OUT_TOTAL:GETVALUE(p->Total,int); break;
	case OUT_DROPPED:GETVALUE(p->Dropped,int); break;
	case AOUT_STEREO: GETVALUE(p->Stereo,int); break; 
	case AOUT_QUALITY: GETVALUE(p->Quality,int); break;
	case AOUT_TIMER: GETVALUE(&p->Timer,node*); break;
	}
	return Result;
}

int WaveOutBaseSet(waveout_base* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT|PIN_FORMAT: 
		if (PacketFormatSimilarAudio(&p->Input,(const packetformat*)Data))
		{
			PacketFormatCopy(&p->Input,(const packetformat*)Data);
			return WaveOutPCM(p);
		}
		else
			SETPACKETFORMATCMP(p->Input,packetformat,UpdateInput(p)); 
		break;
	case OUT_INPUT: SETVALUE(p->Pin,pin,ERR_NONE); break;
	case OUT_TOTAL: SETVALUE(p->Total,int,ERR_NONE); break;
	case OUT_DROPPED: SETVALUE(p->Dropped,int,ERR_NONE); break;
	case AOUT_STEREO: SETVALUECMP(p->Stereo,int,UpdateInput(p),EqInt); break; 
	case AOUT_QUALITY: SETVALUECMP(p->Quality,int,UpdateInput(p),EqInt); break;
	}
	return Result;
}

