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
 * $Id: waveout_win32.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include <math.h>

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#define BUFFER_MUSIC		1*TICKSPERSEC
#define BUFFER_VIDEO		2*TICKSPERSEC

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#define BENCH_SIZE		32

struct waveout;

typedef struct wavebuffer
{
	WAVEHDR Head;
	planes Planes;
	struct wavebuffer* Next; //next in chain
	struct wavebuffer* GlobalNext;
	tick_t RefTime;
	tick_t EstRefTime;
	int Bytes;
	block Block;

} wavebuffer;

typedef struct waveout
{
	node Node;
	node Timer;
	pin Pin;
	packetformat Input;
	packetformat Output;
	packetprocess Process;

	wavebuffer* Buffers; // global chain
	wavebuffer* FreeFirst;
	wavebuffer* FreeLast;

	int BufferLength; // one buffer length (waveout format)
	tick_t BufferScaledTime; // scaled time of one waveout buffer (BufferLength)
	int BufferScaledAdjust; // waveout bytes to scaled time convert (12bit fixed point)

	int Total; // source format
	int Dropped; // dropped packets

	int Bytes; // output format
	int FillPos;
	int Skip;
	wavebuffer* FillFirst;
	wavebuffer* FillLast;
	wavebuffer** Pausing;
	tick_t FillLastTime;

	HWAVEOUT Handle;
	CRITICAL_SECTION Section;

	void* PCM;
	int PCMSpeed;
	int BufferLimit;
	int BufferLimitFull;
	tick_t Tick;
	int TimeRef;
	LONG Waiting;	// number of waiting buffers in WaveOut
	LONG Used;		// number of used buffers (waiting or in fill chain)

	bool_t Play;
	fraction Speed;
	fraction SpeedTime;
	int AdjustedRate;

	bool_t Dither;
	bool_t BufferMode;
	int Stereo;
	int Quality;

	bool_t ForcePriority;
	bool_t SoftwareVolume;
	bool_t MonoVol;
	bool_t Mute; 
	int PreAmp;
	int VolumeDev;	// backup value when mute is turned on
	int VolumeSoft;
	int VolumeSoftLog;
	int VolumeRamp;

	WAVEFORMATEX Format;

	int BenchWait[BENCH_SIZE];
	int BenchSum[BENCH_SIZE];
	int BenchCurrSum;
	int BenchAvg;
	int BenchAdj;
	size_t BenchAvgLimit;
	int BenchSpeedAvg;
	int BenchWaitPos;
	
} waveout;

#define WAVEOUT(p) ((waveout*)((char*)(p)-OFS(waveout,Timer)))

static void Pause(waveout* p);
static void Write(waveout* p, tick_t CurrTime);

static int SetVolumeSoft(waveout* p,int v,bool_t m)
{
	int OldVolumeSoftLog = p->VolumeSoftLog;
	bool_t OldSkip = p->Mute || p->VolumeSoft==0;

	p->VolumeSoft = v;
	p->Mute = m;

	if (p->SoftwareVolume || p->PreAmp)
	{
		if (p->SoftwareVolume && p->Handle && (p->Mute || p->VolumeSoft==0)!=OldSkip)
		{
			if (!OldSkip)
				Pause(p);
			else 
			if (p->Play)
			{
				// adjust tick so packets without RefTime won't mess up timing
				EnterCriticalSection(&p->Section);
				p->Tick += Scale(GetTimeTick()-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
				p->TimeRef = GetTimeTick();
				LeaveCriticalSection(&p->Section);
			}
		}

		v += p->PreAmp;
		if (v<-40) v=-40;

		p->VolumeSoftLog = (int)(pow(10,(50+v)/62.3));
		if (p->VolumeSoftLog < 3)
			p->VolumeSoftLog = 3;
	}
	else
		p->VolumeSoftLog = 256;

	if (p->Handle)
	{
		int Adjust = ScaleRound(p->VolumeSoftLog,256,OldVolumeSoftLog);
		if (Adjust != 256)
		{
			wavebuffer* List;
			EnterCriticalSection(&p->Section);
			for (List=p->Buffers;List;List=List->GlobalNext)
				VolumeMul(Adjust,(void*)List->Block.Ptr,p->BufferLength,&p->Output.Format.Audio);
			LeaveCriticalSection(&p->Section);
		}
	}

	return ERR_NONE;
}

static int GetVolume(waveout* p)
{
	if (!p->SoftwareVolume)
	{
		DWORD Value;
		if (waveOutGetVolume(NULL,&Value) == MMSYSERR_NOERROR)
		{
			if (p->MonoVol)
			{
				Value &= 0xFFFF;
				Value |= Value << 16;
			}
			if (Value)
				p->Mute = 0;
			if (!p->Mute)
				p->VolumeDev = ((LOWORD(Value)+HIWORD(Value)+600)*100) / (0xFFFF*2);
		}
		return p->VolumeDev;
	}
	return p->VolumeSoft;
}

static tick_t Time(waveout* p)
{
	if (p->Speed.Num==0)
		return TIME_BENCH;

	if (p->Play)
	{
		tick_t t;
		EnterCriticalSection(&p->Section);
		t = p->Tick + Scale(GetTimeTick()-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
		LeaveCriticalSection(&p->Section);
		return t;
	}	

	return p->Tick;
}

static int TimerGet(void* pt, int No, void* Data, int Size)
{
	waveout* p = WAVEOUT(pt);
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_PLAY: GETVALUE(p->Play,bool_t); break;
	case TIMER_SPEED: GETVALUE(p->Speed,fraction); break;
	case TIMER_TIME: GETVALUE(Time(p),tick_t); break;
	}
	return Result;
}

static int Get(waveout* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT: GETVALUE(p->Pin,pin); break;
	case OUT_INPUT|PIN_FORMAT: GETVALUE(p->Input,packetformat); break;
	case OUT_INPUT|PIN_PROCESS: GETVALUE(p->Process,packetprocess); break;
	case OUT_OUTPUT|PIN_FORMAT: GETVALUE(p->Output,packetformat); break;
	case OUT_TOTAL:GETVALUE(p->Total,int); break;
	case OUT_DROPPED:GETVALUE(p->Dropped,int); break;
	case AOUT_VOLUME: GETVALUE(GetVolume(p),int); break;
	case AOUT_MUTE: GETVALUE(p->Mute,bool_t); break;
	case AOUT_PREAMP: GETVALUE(p->PreAmp,int); break;
	case AOUT_STEREO: GETVALUE(p->Stereo,int); break; 
	case AOUT_MODE: GETVALUE(p->BufferMode,bool_t); break;
	case AOUT_QUALITY: GETVALUE(p->Quality,int); break;
	case AOUT_TIMER: GETVALUE(&p->Timer,node*); break;
	}
	return Result;
}

static void UpdateBenchAvg(waveout* p)
{
	int n;

	p->BenchAvg += 4;
	p->BenchAdj = (24*16) / p->BenchAvg;
	p->BenchSpeedAvg = SPEED_ONE/(p->BenchAvg*BENCH_SIZE);

	n = p->Input.Format.Audio.Bits >> 3;
	if (!(p->Input.Format.Audio.Flags & PCM_PLANES))
		n *= p->Input.Format.Audio.Channels;
	n = p->Output.Format.Audio.SampleRate;
	if (n>0)
		p->BenchAvgLimit = (n * p->BenchAvg) / 160;
	else
		p->BenchAvgLimit = MAX_INT;

	p->BenchCurrSum += 2 * BENCH_SIZE;
	for (n=0;n<BENCH_SIZE;++n)
	{
		p->BenchSum[n] += 2 * BENCH_SIZE;
		p->BenchWait[n] += 2;
	}
}

static void ReleaseBuffer(waveout* p,wavebuffer* Buffer,bool_t UpdateTick)
{
	EnterCriticalSection(&p->Section);

	if (UpdateTick)
	{
		tick_t Old;
		int Time = GetTimeTick();

		Old = p->Tick + Scale(Time-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);

		if (Buffer->RefTime >= 0)
			p->Tick = Buffer->RefTime + p->BufferScaledTime;
		else
			p->Tick += p->BufferScaledTime;
		p->TimeRef = Time;

		//DEBUG_MSG2(-1,T("WaveOutTime: %d %d"),Old,p->Tick);

		// if difference is little then just adjust (because GetTimeTick() is more linear)
		if (abs(Old - p->Tick) < TICKSPERSEC/2)
			p->Tick = Old + ((p->Tick - Old) >> 2);
	}

	Buffer->Next = NULL;
	if (p->FreeLast)
		p->FreeLast->Next = Buffer;
	else
		p->FreeFirst = Buffer;
	p->FreeLast = Buffer;
	p->Used--;
	LeaveCriticalSection(&p->Section);
}

static void Reset(waveout* p)
{
	int n;

	// release buffers already sended to device
	if (p->Handle)
	{
		tick_t OldTick = p->Tick;
		waveOutReset(p->Handle);
		p->Tick = OldTick;
	}
	
	if (p->FillLast && p->FillLast->RefTime >= 0)
		p->FillLastTime = p->FillLast->RefTime;

	// release fill chain
	while (p->FillFirst)
	{
		wavebuffer* Buffer = p->FillFirst;
		p->FillFirst = Buffer->Next;
		ReleaseBuffer(p,Buffer,0);
	}

	p->FillLast = NULL;
	p->FillPos = p->BufferLength;
	p->Skip = 0;
	p->Bytes = 0;
	p->BufferLimit = p->BufferLimitFull;

	p->BenchAvg = 16-4;
	UpdateBenchAvg(p);

	p->BenchWaitPos = 0;
	p->BenchCurrSum = p->BenchAvg * BENCH_SIZE;
	for (n=0;n<BENCH_SIZE;++n)
	{
		p->BenchSum[n] = p->BenchCurrSum;
		p->BenchWait[n] = p->BenchAvg;
	}

	PCMReset(p->PCM); // init dither and subsample position
}

static wavebuffer* GetBuffer(waveout* p)
{
	wavebuffer* Buffer;

	// try to find a free buffer

	EnterCriticalSection(&p->Section);
	Buffer = p->FreeFirst;
	if (Buffer)
	{
		p->FreeFirst = Buffer->Next;
		if (Buffer == p->FreeLast)
			p->FreeLast = NULL;
	}
	if (!Buffer)
	{
		block Block;
		if (AllocBlock(p->BufferLength,&Block,p->Used>=20,HEAP_DYNAMIC))
		{
			Buffer = (wavebuffer*)malloc(sizeof(wavebuffer));
			if (!Buffer)
				FreeBlock(&Block);
		}

		if (Buffer)
		{
			Buffer->Block = Block;
			Buffer->Planes[0] = (uint8_t*)Block.Ptr;
			Buffer->Next = NULL;

			memset(&Buffer->Head,0,sizeof(WAVEHDR));
			Buffer->Head.lpData = (char*)Buffer->Block.Ptr;
			Buffer->Head.dwUser = (DWORD)Buffer;
			Buffer->Head.dwBufferLength = p->BufferLength;
			Buffer->Head.dwBytesRecorded = p->BufferLength;

			if (waveOutPrepareHeader(p->Handle, &Buffer->Head, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			{
				FreeBlock(&Buffer->Block);
				free(Buffer);
				Buffer = NULL;
			}
			else
			{
				Buffer->GlobalNext = p->Buffers;
				p->Buffers = Buffer;
			}
		}
		else
		{
			p->BufferLimitFull = p->Used;
			if (p->BufferLimit > p->Used)
				p->BufferLimit = p->Used;
			else
			if (p->BufferLimit > 4)
				p->BufferLimit--;
		}

	}

	if (Buffer)
	{
		p->Used++;
		Buffer->RefTime = -1;
		Buffer->Next =NULL;
	}

	LeaveCriticalSection(&p->Section);
	return Buffer;
}

static void Write(waveout* p, tick_t CurrTime)
{
	if (p->Play)
	{
		while (p->FillFirst != p->FillLast)
		{
			wavebuffer* Buffer = p->FillFirst;

			if (!p->Waiting && CurrTime >= 0 && Buffer->EstRefTime >= 0 && Buffer->EstRefTime > CurrTime + SHOWAHEAD)
				break;

			p->FillFirst = Buffer->Next;

			if (p->SoftwareVolume && (p->Mute || p->VolumeSoft==0))
				ReleaseBuffer(p,Buffer,0);
			else
			{
				if (waveOutWrite(p->Handle, &Buffer->Head, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
					ReleaseBuffer(p,Buffer,0);
				else
					InterlockedIncrement(&p->Waiting);
			}
		}
	}
}

static int Send(waveout* p, const constplanes Planes, int Length, tick_t RefTime, tick_t CurrTime, int Speed)
{
	wavebuffer* Buffer;
	int DstLength;
	int SrcLength;
	planes DstPlanes;
	constplanes SrcPlanes;

	p->Total += Length;

	SrcPlanes[0] = Planes[0];
	SrcPlanes[1] = Planes[1];

	if (p->Skip > 0)
	{
		SrcLength = min(p->Skip,Length);
		SrcPlanes[0] = (uint8_t*)SrcPlanes[0] + SrcLength;
		SrcPlanes[1] = (uint8_t*)SrcPlanes[1] + SrcLength;
		Length -= SrcLength;
		p->Skip -= SrcLength;
	}

	while (Length > 0)
	{
		if (p->FillPos >= p->BufferLength)
		{
			// allocate new buffer
			Buffer = GetBuffer(p);
			if (!Buffer)
				break;

			if (p->FillLast)
			{
				wavebuffer* Last = p->FillLast;
				if (Last->RefTime>=0)
					p->FillLastTime = Last->EstRefTime = Last->RefTime;
				else
				{
					if (p->FillLastTime>=0)
						p->FillLastTime += p->BufferScaledTime;
					Last->EstRefTime = p->FillLastTime;
				}
				Last->Next = Buffer;
			}
			else
				p->FillFirst = Buffer;
			p->FillLast = Buffer;
			p->FillPos = 0;
			Buffer->Bytes = p->Bytes;
		}
		else
			Buffer = p->FillLast;

		if (RefTime >= 0)
		{
			Buffer->RefTime = RefTime - ((p->FillPos * p->BufferScaledAdjust) >> 12);
			if (Buffer->RefTime < 0)
				Buffer->RefTime = 0;
			RefTime = TIME_UNKNOWN;
		}

		SrcLength = Length;
		DstLength = p->BufferLength - p->FillPos;
		DstPlanes[0] = (uint8_t*)Buffer->Block.Ptr + p->FillPos;

		PCMConvert(p->PCM,DstPlanes,SrcPlanes,&DstLength,&SrcLength,Speed,p->VolumeSoftLog);

		if (p->VolumeRamp < RAMPLIMIT)
			p->VolumeRamp = VolumeRamp(p->VolumeRamp,DstPlanes[0],DstLength,&p->Output.Format.Audio);

		p->Bytes += DstLength;
		p->FillPos += DstLength;

		SrcPlanes[0] = (uint8_t*)SrcPlanes[0] + SrcLength;
		SrcPlanes[1] = (uint8_t*)SrcPlanes[1] + SrcLength;
		Length -= SrcLength;

		if (!SrcLength)
			break;
	}

	if (Length && p->Input.Format.Audio.BlockAlign>0)
		p->Skip = p->Input.Format.Audio.BlockAlign - Length % p->Input.Format.Audio.BlockAlign;

	Write(p,CurrTime);
	return ERR_NONE;
}

static void CALLBACK WaveProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, 
                               DWORD dwParam1, DWORD dwParam2)
{
    if (uMsg == WOM_DONE)
    {
		wavebuffer* Buffer = (wavebuffer*)(((WAVEHDR*)dwParam1)->dwUser);
		waveout* p = (waveout*)dwInstance;
		if (p->ForcePriority)
		{
			void* Thread = GetCurrentThread();
			if (GetThreadPriority(Thread)!=THREAD_PRIORITY_HIGHEST)
				SetThreadPriority(Thread,THREAD_PRIORITY_HIGHEST);
		}
		if (p->Pausing)
		{
			// add buffer to fill chain
			Buffer->Next = *p->Pausing;
			*p->Pausing = Buffer;
			p->Pausing = &Buffer->Next;
		}
		else
			ReleaseBuffer(p,Buffer,1);
		InterlockedDecrement(&p->Waiting);
    }
}

static int UpdatePCM(waveout* p,const audio* InputFormat)
{
	p->SpeedTime = p->Speed;
	p->SpeedTime.Num *= TICKSPERSEC;
	p->SpeedTime.Den *= GetTimeFreq();

	p->BufferScaledTime = Scale(TICKSPERSEC,p->Speed.Num*p->BufferLength,p->Speed.Den*p->Format.nAvgBytesPerSec);
	if (p->BufferLength)
		p->BufferScaledAdjust = (p->BufferScaledTime*4096) / p->BufferLength;
	else
		p->BufferScaledAdjust = 0;
	if (!p->Speed.Num)
		p->PCMSpeed = SPEED_ONE;
	else
		p->PCMSpeed = Scale(SPEED_ONE,p->Speed.Num,p->Speed.Den);
	p->AdjustedRate = Scale(p->Output.Format.Audio.SampleRate,p->Speed.Den,p->Speed.Num);

	PCMRelease(p->PCM);
	p->PCM = PCMCreate(&p->Output.Format.Audio,InputFormat,p->Dither,p->SoftwareVolume || p->PreAmp);
	return ERR_NONE;
}

static int UpdateBufferTime(waveout* p)
{
	p->BufferLimit = Scale(p->BufferMode?BUFFER_VIDEO:BUFFER_MUSIC,p->Format.nAvgBytesPerSec,p->BufferLength*TICKSPERSEC);
	p->BufferLimitFull = p->BufferLimit;
	return ERR_NONE;
}

static int Process(waveout* p,const packet* Packet,const flowstate* State)
{
	if (!Packet)
	{
		if (State->DropLevel)
			++p->Dropped;
		else
			Write(p,State->CurrTime);
		return (p->Waiting<=0 || p->Speed.Num==0) ? ERR_NONE : ERR_BUFFER_FULL;
	}

	if (p->Speed.Num==0) // benchmark mode (auto adjust speed)
	{
		int Pos = p->BenchWaitPos;
		int OldSum;
		int Speed;

		if (p->Play)
		{
			while (Packet->Length > p->BenchAvgLimit)
				UpdateBenchAvg(p);

			p->BenchCurrSum -= p->BenchWait[Pos];
			p->BenchWait[Pos] = (p->Waiting * p->BufferLength) >> 12;
			p->BenchCurrSum += p->BenchWait[Pos];

			OldSum = p->BenchSum[Pos];
			p->BenchSum[Pos] = p->BenchCurrSum;

			if (++Pos == BENCH_SIZE)
				Pos = 0;

			p->BenchWaitPos = Pos;

			if (p->BenchCurrSum < 2*BENCH_SIZE*p->BenchAvg)
				Speed = (p->BenchCurrSum+1) * p->BenchSpeedAvg;
			else
				Speed = 2*SPEED_ONE+(p->BenchCurrSum-2*BENCH_SIZE*p->BenchAvg+1) * 4*p->BenchSpeedAvg;

			Speed -= p->BenchAdj*(p->BenchCurrSum - OldSum);

			if (p->Waiting < 3)
				Speed -= SPEED_ONE/5;
		}
		else
			Speed = SPEED_ONE;

		//DEBUG_MSG3(-1,T("Audio speed:%d length:%d (wait:%d)"),Speed,Packet->Length,p->Waiting);
		return Send(p,Packet->Data,Packet->Length,Packet->RefTime,State->CurrTime,Speed);
	}

	if (State->DropLevel)
		return ERR_NONE;

	if (p->Used >= p->BufferLimit)
	{
		Write(p,State->CurrTime);
		return ERR_BUFFER_FULL;
	}

	DEBUG_MSG3(DEBUG_AUDIO,T("Waveout reftime:%d used:%d waiting:%d"),Packet->RefTime,p->Used,p->Waiting);
	return Send(p,Packet->Data,Packet->Length,Packet->RefTime,State->CurrTime,p->PCMSpeed);
}

static bool_t FreeBuffers(waveout* p);

static int UpdateInput(waveout* p)
{
	Reset(p);
	FreeBuffers(p);

	if (p->Handle)
	{
		waveOutClose(p->Handle);
		p->Handle = NULL;
	}
	p->Total = 0;
	p->Dropped = 0;
	p->Process = DummyProcess;

	if (p->Input.Type == PACKET_AUDIO)
	{
		MMRESULT MMResult;
		int Try;

		if (p->Input.Format.Audio.Format != AUDIOFMT_PCM) 
		{
			PacketFormatClear(&p->Input);
			return ERR_INVALID_DATA;
		}

		if (p->Input.Format.Audio.Channels == 0 ||
			p->Input.Format.Audio.SampleRate == 0)
			return ERR_NONE; // probably initialized later

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
			p->Output.Format.Audio.Bits = 16;
			p->Output.Format.Audio.FracBits = 15;
			if (p->Output.Format.Audio.SampleRate >= 44100)
				p->Output.Format.Audio.SampleRate = 44100;
			else
				p->Output.Format.Audio.SampleRate = 22050;
			break;

		default:
		case 2: // original samplerate 
			p->Output.Format.Audio.Bits = 16;
			p->Output.Format.Audio.FracBits = 15;
			p->Dither = 1;
			break;
		}

		Try = 0;
		do
		{
			if (p->Output.Format.Audio.Bits <= 8)
				p->Output.Format.Audio.Flags |= PCM_UNSIGNED;

			p->Format.wFormatTag = WAVE_FORMAT_PCM;
			p->Format.nChannels = (WORD)p->Output.Format.Audio.Channels;
			p->Format.nSamplesPerSec = p->Output.Format.Audio.SampleRate;
			p->Format.wBitsPerSample = (WORD)p->Output.Format.Audio.Bits;
			p->Format.nBlockAlign = (WORD)((p->Format.nChannels * p->Format.wBitsPerSample) >> 3);
			p->Format.nAvgBytesPerSec = p->Format.nSamplesPerSec * p->Format.nBlockAlign;
			p->Format.cbSize = 0;

			MMResult = waveOutOpen(&p->Handle, WAVE_MAPPER, &p->Format,(DWORD)WaveProc,
									(DWORD)p,CALLBACK_FUNCTION);

			if (MMResult==WAVERR_BADFORMAT)
			{
				++Try;
				if (Try==1)
				{
					if (p->Output.Format.Audio.SampleRate > 35000)
						p->Output.Format.Audio.SampleRate = 44100;
					else
						++Try;
				}
				if (Try==2)
				{
					if (p->Output.Format.Audio.SampleRate != 22050)
						p->Output.Format.Audio.SampleRate = 22050;
					else
						++Try;
				}
				if (Try==3)
				{
					if (p->Output.Format.Audio.Channels > 1)
						p->Output.Format.Audio.Channels = 1;
					else
						++Try;
				}
				if (Try==4)
				{
					if (p->Output.Format.Audio.Bits != 8)
					{
						p->Output.Format.Audio.Bits = 8;
						p->Output.Format.Audio.FracBits = 7;
					}
					else
						++Try;
				}
				if (Try==5)
					break;
			}
		}
		while (MMResult==WAVERR_BADFORMAT);

		if (p->Handle)
		{
			p->Process = Process;
			p->TimeRef = GetTimeTick();
			p->BufferLength = 4096;
			if (p->Format.nAvgBytesPerSec > 65536)
				p->BufferLength *= 2;
			if (p->Format.nAvgBytesPerSec > 2*65536)
				p->BufferLength *= 2;
			UpdateBufferTime(p);
		
			p->FillPos = p->BufferLength;
			p->VolumeRamp = 0;

			UpdatePCM(p,&p->Input.Format.Audio);
			Reset(p);
		}
		else
		{
			PacketFormatClear(&p->Input);
			ShowError(p->Node.Class,ERR_ID,ERR_DEVICE_ERROR);
			return ERR_DEVICE_ERROR;
		}
	}
	else
	if (p->Input.Type != PACKET_NONE)
		return ERR_INVALID_DATA;
		
	return ERR_NONE;
}

static int Update(waveout* p)
{
	wavebuffer *OldFirst;
	wavebuffer *OldLast;
	audio OldFormat;
	wavebuffer* OldBuffers;
	wavebuffer* OldFill;
	int OldFillPos; 
	int OldUsed;
	bool_t OldVolume;
	int OldPreAmp;
	wavebuffer *Buffer,*Next;

	EnterCriticalSection(&p->Section);

	OldVolume = p->SoftwareVolume;
	OldPreAmp = p->PreAmp;
	OldUsed = p->Used;
	OldFirst = p->FreeFirst;
	OldLast = p->FreeLast;
	OldFormat = p->Output.Format.Audio;
	OldFormat.SampleRate = p->AdjustedRate;

	OldFill = p->FillFirst;
	OldFillPos = p->FillPos;

	p->FillFirst = NULL;
	p->FillLast = NULL;
	p->FillPos = p->BufferLength;
	p->FreeFirst = NULL;
	p->FreeLast = NULL;

	LeaveCriticalSection(&p->Section);

	Reset(p);

	OldBuffers = p->FreeFirst;
	if (p->FreeLast)
		p->FreeLast->Next = OldFill;
	else
		OldBuffers = OldFill;

	if (OldBuffers)
		p->FillLastTime = OldBuffers->EstRefTime - p->BufferScaledTime;

	p->PreAmp = 0;
	p->SoftwareVolume = 0;
	p->Used = OldUsed;
	p->FreeFirst = OldFirst;
	p->FreeLast = OldLast;

	// setup temporary format
	UpdatePCM(p,&OldFormat);

	for (Buffer=OldBuffers;Buffer;Buffer=Next)
	{
		Next = Buffer->Next;
		Send(p,Buffer->Planes,Next ? p->BufferLength:OldFillPos,Buffer->RefTime,-1,p->PCMSpeed);
		ReleaseBuffer(p,Buffer,0);
	}

	p->SoftwareVolume = OldVolume;
	p->PreAmp = OldPreAmp;
	UpdatePCM(p,&p->Input.Format.Audio);
	return ERR_NONE;
}

static bool_t FreeBuffers(waveout* p)
{
	wavebuffer** Ptr;
	wavebuffer* Buffer;
	bool_t Changed = 0;

	while (p->FreeFirst)
	{
		Buffer = p->FreeFirst;
		p->FreeFirst = Buffer->Next;

		// remove from global chain
		Ptr = &p->Buffers;
		while (*Ptr && *Ptr != Buffer)
			Ptr = &(*Ptr)->GlobalNext;
		if (*Ptr == Buffer)
			*Ptr = Buffer->Next;

		waveOutUnprepareHeader(p->Handle, &Buffer->Head, sizeof(WAVEHDR));
		FreeBlock(&Buffer->Block);
		free(Buffer);
		Changed = 1;
	}
	p->FreeLast = NULL;

	return Changed;
}

static int Hibernate(waveout* p,int Mode)
{
	bool_t Changed = 0;

	EnterCriticalSection(&p->Section);
	Changed = FreeBuffers(p);
	LeaveCriticalSection(&p->Section);

	return Changed ? ERR_NONE : ERR_OUT_OF_MEMORY;
}

static void Pause(waveout* p)
{
	p->Pausing = &p->FillFirst;
	waveOutReset(p->Handle);
	p->Pausing = NULL;
	p->FillLast = p->FillFirst;
	if (p->FillLast)
		while (p->FillLast->Next) p->FillLast = p->FillLast->Next;
}

static int UpdatePlay(waveout* p)
{
	if (p->Play)
	{
		p->TimeRef = GetTimeTick();
		if (p->Handle)
			Write(p,p->Tick);
	}
	else
	{
		if (!p->Waiting)
			p->Tick += Scale(GetTimeTick()-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
		else
		if (p->Handle)
			Pause(p);
	}
	return ERR_NONE;
}

static int SetVolumeDev(waveout* p,int v)
{
	p->VolumeDev = v;
	if (p->Mute)
		waveOutSetVolume(NULL,0);
	else
		waveOutSetVolume(NULL,0x10001 * ((0xFFFF * p->VolumeDev) / 100));
	return ERR_NONE;
}

static int TimerSet(void* pt, int No, const void* Data, int Size)
{
	waveout* p = WAVEOUT(pt);
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_PLAY: SETVALUECMP(p->Play,bool_t,UpdatePlay(p),EqBool); break;
	case TIMER_SPEED: SETVALUECMP(p->Speed,fraction,Update(p),EqFrac); break;
	case TIMER_TIME:
		assert(Size == sizeof(tick_t));
		EnterCriticalSection(&p->Section);
		p->Tick = *(tick_t*)Data;
		p->TimeRef = GetTimeTick();
		LeaveCriticalSection(&p->Section);
		Result = ERR_NONE;
		break;
	}
	return Result;
}

static void UpdateSoftwareVolume(waveout* p)
{
	bool_t SoftwareVolume = !QueryAdvanced(ADVANCED_SYSTEMVOLUME);
	if (SoftwareVolume != p->SoftwareVolume)
	{
		p->SoftwareVolume = SoftwareVolume;
		SetVolumeSoft(p,p->VolumeSoft,p->Mute);
		if (p->Handle)
			UpdatePCM(p,&p->Input.Format.Audio);
	}
}

static int UpdatePreAmp(waveout* p)
{
	SetVolumeSoft(p,p->VolumeSoft,p->Mute);
	if (p->Handle)
		UpdatePCM(p,&p->Input.Format.Audio);
	return ERR_NONE;
}

static int Set(waveout* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT|PIN_FORMAT: 
		if (PacketFormatSimilarAudio(&p->Input,(const packetformat*)Data))
		{
			PacketFormatCopy(&p->Input,(const packetformat*)Data);
			Result = UpdatePCM(p,&p->Input.Format.Audio);
		}
		else
			SETPACKETFORMATCMP(p->Input,packetformat,UpdateInput(p)); 
		break;
	case OUT_INPUT: SETVALUE(p->Pin,pin,ERR_NONE); break;
	case OUT_TOTAL: SETVALUE(p->Total,int,ERR_NONE); break;
	case OUT_DROPPED: SETVALUE(p->Dropped,int,ERR_NONE); break;

	case AOUT_VOLUME: 
		assert(Size==sizeof(int));
		UpdateSoftwareVolume(p);
		if (p->SoftwareVolume)
			Result = SetVolumeSoft(p,*(int*)Data,p->Mute);
		else
			Result = SetVolumeDev(p,*(int*)Data);
		break;

	case AOUT_PREAMP: SETVALUECMP(p->PreAmp,int,UpdatePreAmp(p),EqInt); break; 

	case AOUT_MUTE:
		assert(Size==sizeof(bool_t));
		UpdateSoftwareVolume(p);
		if (p->SoftwareVolume)
			Result = SetVolumeSoft(p,p->VolumeSoft,*(bool_t*)Data);
		else
		{
			if (!p->Mute) GetVolume(p); // save old volume to p->VolumeDev
			p->Mute = *(bool_t*)Data;
			Result = SetVolumeDev(p,p->VolumeDev);
		}
		break;

	case AOUT_STEREO: SETVALUECMP(p->Stereo,int,UpdateInput(p),EqInt); break; 
	case AOUT_QUALITY: SETVALUECMP(p->Quality,int,UpdateInput(p),EqInt); break; 
	case AOUT_MODE: SETVALUE(p->BufferMode,bool_t,UpdateBufferTime(p)); break;

	case FLOW_FLUSH:
		Reset(p);
		p->FillLastTime = TIME_UNKNOWN;
		p->VolumeRamp = 0;
		Result = ERR_NONE;
		break;

	case NODE_SETTINGSCHANGED:
		p->ForcePriority = QueryAdvanced(ADVANCED_WAVEOUTPRIORITY);
		break;

	case NODE_HIBERNATE:
		assert(Size == sizeof(int));
		Result = Hibernate(p,*(int*)Data);
		break;
	}
	return Result;
}

static int Create(waveout* p)
{
	WAVEOUTCAPS Caps;

	if (waveOutGetNumDevs()==0)
		return ERR_NOT_SUPPORTED;

	waveOutGetDevCaps(WAVE_MAPPER,&Caps,sizeof(Caps));

	p->Node.Get = (nodeget)Get;
	p->Node.Set = (nodeset)Set;

	p->Timer.Class = TIMER_CLASS;
	p->Timer.Enum = TimerEnum;
	p->Timer.Get = TimerGet;
	p->Timer.Set = TimerSet;

	p->VolumeDev = 70; //default when device is muted
	p->VolumeSoftLog = 256;
	p->Quality = 2;
	p->Speed.Den = p->Speed.Num = 1;
	p->SpeedTime.Num = TICKSPERSEC;
	p->SpeedTime.Den = GetTimeFreq();
	p->MonoVol = (Caps.dwSupport & WAVECAPS_LRVOLUME) == 0;

	InitializeCriticalSection(&p->Section);
	return ERR_NONE;
}

static void Delete(waveout* p)
{
	PacketFormatClear(&p->Input);
	PCMRelease(p->PCM);
	DeleteCriticalSection(&p->Section);
}

static const nodedef WaveOut =
{
	sizeof(waveout)|CF_GLOBAL,
	WAVEOUT_ID,
	AOUT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void WaveOut_Init()
{
	NodeRegisterClass(&WaveOut);
}

void WaveOut_Done()
{
	NodeUnRegisterClass(WAVEOUT_ID);
}

#endif
