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
 * $Id: waveout_palmos.c 623 2006-02-01 13:19:15Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#define DRVBUFFER		4096
#define BUFFER_MIN		4*DRVBUFFER
#define BUFFER_MUSIC	65536*8
#define BUFFER_VIDEO	65536
#define VOLLIMIT		(16384-64)

#include "pace.h"

typedef struct waveout_palm
{
	waveout_base Base;
	SndStreamRef Stream;

	sysregs SysRegs;
	void* PCMCopy;
	int CopySpeed;
	bool_t Mute;
	int Volume;
	int PreAmp;
	int Pan;
	bool_t Play;
	bool_t Started;
	fraction Speed;
	fraction SpeedTime;
	
	buffer Buffer;
	bool_t BufferMode;
	int VolumeRamp;
	int BytePerSample; // output bytes per sample
	int BytePerSec; // output bytes per seconds (before speed adjustment)
	int SpeedBytePerSec; // speed adjusted BytePerSec
	int Ratio; //input to output bytepersec (8.8 fixed point)
	int Skip;
	int DrvBufferSize;
	tick_t DrvBufferDelay;
	tick_t BufferTick;
	tick_t Tick;
	int TimeRef;
	int BenchSpeed;
	int BenchLeft;
	bool_t AlwaysClose;
	bool_t Force16Bits;
	int Vol;

	Boolean Native;
	m68kcallback CallBack;

} waveout_palm;

#define WAVEOUT(p) ((waveout_palm*)((char*)(p)-OFS(waveout_palm,Base.Timer)))

static void ManualSleep(int MSec)
{
	int t0 = GetTimeTick();
	int Wait = (GetTimeFreq()*MSec)/1024;
	if (Wait==0) Wait=1;
	while (GetTimeTick()-t0<Wait);
}

static int Stop(waveout_palm* p)
{
	if (p->Started)
	{
		p->Started = 0;
		SndStreamStop(p->Stream);
		ManualSleep(50); // try to make sure callback exited
		if (p->AlwaysClose)
		{
			SndStreamDelete(p->Stream);
			p->Stream = 0;
		}
		p->DrvBufferSize = 0;
		p->DrvBufferDelay = 0;
	}
	return ERR_NONE;
}

static int CreateStream(waveout_palm* p);

static void Start(waveout_palm* p,tick_t CurrTime)
{
	if (p->Play && !p->Started && p->Buffer.WritePos != p->Buffer.ReadPos &&
		(CurrTime<0 || p->BufferTick <= CurrTime+SHOWAHEAD) &&
		(p->Stream || CreateStream(p)==ERR_NONE))
	{
		p->Started = 1;
		SndStreamStart(p->Stream);
	}
}

static int UpdateBufferMode(waveout_palm* p)
{
	int Size;

	if (p->Started)
		return ERR_NONE;

	BufferPack(&p->Buffer,0);

	Size = p->BufferMode ? BUFFER_VIDEO : BUFFER_MUSIC;
	if (Context()->LowMemory)
		Size >>= 1;
	
	if (Size < BUFFER_MIN)
		Size = BUFFER_MIN;
	Size = ALIGN16(Size);

	DisableOutOfMemory();
	for (;Size >= BUFFER_MIN;Size >>= 1)
		if (BufferAlloc(&p->Buffer,Size,1))
		{
			EnableOutOfMemory();
			if (p->Buffer.WritePos >= p->Buffer.Allocated)
				p->Buffer.WritePos = 0;
			return ERR_NONE;
		}

	EnableOutOfMemory();
	ShowOutOfMemory();
	return ERR_OUT_OF_MEMORY;
}

static Err Callback(waveout_palm* p,SndStreamRef Stream,void *Buffer,uint32_t Frames)
{
	// as a general rule we don't call any OS callbacks here, because
	// we only have one emulstate structure and if both threads
	// call an m68k syscall this could mean problems...

	constplanes SrcPlanes;
	planes DstPlanes;
	int SrcLength,DstLength;
	int WritePos,ReadPos,Length,Speed;

	LoadSysRegs(&p->SysRegs);

	Length = Frames * p->BytePerSample;
	DstPlanes[0] = Buffer;

	if (p->Started)
	{
		WritePos = p->Buffer.WritePos;
		ReadPos = p->Buffer.ReadPos;

		if (p->DrvBufferSize != Length)
		{
			//assuming double buffering
			p->DrvBufferSize = Length;
			p->DrvBufferDelay = Scale(2*Length,TICKSPERSEC,p->SpeedBytePerSec); 
		}

		Speed = p->CopySpeed;
		if (Speed <= 0) 
		{
/*			int Middle = p->Buffer.Allocated >> 1;
			int Left = ReadPos - WritePos;
			if (Left<0)
				Left += p->Buffer.Allocated;

			Speed = p->BenchSpeed;
			
			if (Left > Middle && p->BenchLeft < Left)
			{
				Speed -= Left - p->BenchLeft;
				if (Speed < SPEED_ONE/8)
					Speed = SPEED_ONE/8;
			}

			if (Left < Middle && p->BenchLeft > Left)
				Speed -= Left - p->BenchLeft;

			p->BenchSpeed = Speed;
			p->BenchLeft = Left;
*/
			Speed = SPEED_ONE;
		}

		while (Length>0)
		{
			if (WritePos < ReadPos)
				SrcLength = p->Buffer.Allocated - ReadPos;
			else
				SrcLength = WritePos - ReadPos;
			SrcPlanes[0] = p->Buffer.Data + ReadPos;
			DstLength = Length;

			PCMConvert(p->PCMCopy,DstPlanes,SrcPlanes,&DstLength,&SrcLength,Speed,256);

			DstPlanes[0] = (uint8_t*)DstPlanes[0] + DstLength;
			Length -= DstLength;

			ReadPos += SrcLength;
			if (ReadPos == p->Buffer.Allocated)
				ReadPos = 0;

			if (!DstLength)
				break;
		}

		p->Buffer.ReadPos = ReadPos;
	}

	if (Length)
		memset(DstPlanes[0],(p->Base.Output.Format.Audio.Flags & PCM_UNSIGNED)?0x80:0x00,Length);
	
	return errNone;
}

DLLEXPORT unsigned long WaveOutCallBack(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt32 Param[4];
	memcpy(Param,userData68KP,16);
	return Callback((waveout_palm*)SWAP32(Param[0]),(SndStreamRef)SWAP32(Param[1]),
					(void*)SWAP32(Param[2]),SWAP32(Param[3]));
}

static int Process(waveout_palm* p,const packet* Packet,const flowstate* State)
{
	constplanes SrcPlanes;
	planes DstPlanes;
	int Length,SrcLength,DstLength,Left,ReadPos,WritePos;
	
	ReadPos = p->Buffer.ReadPos;
	WritePos = p->Buffer.WritePos;

	if (!Packet)
	{
		if (State->DropLevel)
			++p->Base.Dropped;
		return (!p->Started || !p->Speed.Num || WritePos==ReadPos) ? ERR_NONE : ERR_BUFFER_FULL;
	}

	DstLength = (Packet->Length * p->Ratio + 255) >> 8; // it may give little larger as needed length

	if (DstLength > p->Buffer.Allocated - DRVBUFFER*2)
	{
		++p->Base.Dropped;
		return ERR_NONE; // drop large packets
	}

	// avoid filling the buffer full (it would mean buffer is empty)
	ReadPos -= p->BytePerSample;
	if (ReadPos < 0)
		ReadPos += p->Buffer.Allocated;

	// check if there is enough free space in buffer
	Left = ReadPos - WritePos;
	if (Left<0)
		Left += p->Buffer.Allocated;

	if (DstLength > Left)
	{
		if (!p->Started)
			Start(p,State->CurrTime);

		if (p->Speed.Num)
			return ERR_BUFFER_FULL;

		// in benchmark mode we need the package to 
		// be processed to measure real performance

		WritePos = ReadPos - DstLength;
		WritePos &= ~3; // align to dword (DstLength is just an approx)
		if (WritePos<0)
			WritePos += p->Buffer.Allocated;

		Left = ReadPos - WritePos;
		if (Left<0)
			Left += p->Buffer.Allocated;
	}

	// process packet
	SrcPlanes[0] = Packet->Data[0];
	SrcPlanes[1] = Packet->Data[1];
	Length = Packet->Length;
	p->Base.Total += Length;

	if (Packet->RefTime >= 0)
	{
		int Diff = p->Buffer.Allocated - Left; // bytes already in the buffer (with 100% speed)
		p->BufferTick = Packet->RefTime - Scale(Diff,TICKSPERSEC,p->BytePerSec) - p->DrvBufferDelay;
		if (p->BufferTick < 0)
			p->BufferTick = 0;

		if (p->Started)
		{
			// adjust timer if it's already running
			int Time = GetTimeTick();
			tick_t Tick = Scale(Time-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
			tick_t Old = p->Tick + Tick;

			// if difference is little then just adjust (because GetTimeTick() is more linear)
			if (abs(Old - p->BufferTick) < TICKSPERSEC/2)
				p->BufferTick = Old + ((p->BufferTick - Old) >> 2);

			p->Tick = p->BufferTick;
			p->TimeRef = Time;
		}
	}

	if (p->Skip)
	{
		SrcLength = min(p->Skip,Length);
		SrcPlanes[0] = (uint8_t*)SrcPlanes[0] + SrcLength;
		SrcPlanes[1] = (uint8_t*)SrcPlanes[1] + SrcLength;
		Length -= SrcLength;
		p->Skip -= SrcLength;
	}

	while (Length>0)
	{
		if (ReadPos < WritePos)
			DstLength = p->Buffer.Allocated - WritePos;
		else
			DstLength = ReadPos - WritePos;
		DstPlanes[0] = p->Buffer.Data + WritePos;
		SrcLength = Length;

		PCMConvert(p->Base.PCM,DstPlanes,SrcPlanes,&DstLength,&SrcLength,SPEED_ONE,p->Vol/4); //volume needed only for lifedrive fix

		if (p->VolumeRamp < RAMPLIMIT)
			p->VolumeRamp = VolumeRamp(p->VolumeRamp,DstPlanes[0],DstLength,&p->Base.Output.Format.Audio);

		SrcPlanes[0] = (uint8_t*)SrcPlanes[0] + SrcLength;
		SrcPlanes[1] = (uint8_t*)SrcPlanes[1] + SrcLength;
		Length -= SrcLength;

		WritePos += DstLength;
		if (WritePos == p->Buffer.Allocated)
			WritePos = 0;

		if (!SrcLength) // unaligned input (not supported)
			break;
	}

	p->Buffer.WritePos = WritePos;

	if (Length>0 && p->Base.Input.Format.Audio.BlockAlign>0)
		p->Skip = p->Base.Input.Format.Audio.BlockAlign - Length % p->Base.Input.Format.Audio.BlockAlign;

	if (!p->Started)
		Start(p,State->CurrTime);

	return ERR_NONE;
}

static int Flush(waveout_palm* p)
{
	Stop(p);
	p->Skip = 0;
	p->VolumeRamp = 0;
	p->BufferTick = -1;
	p->Buffer.ReadPos = p->Buffer.WritePos = 0;
	p->BenchSpeed = SPEED_ONE;
	p->BenchLeft = 0;
	return ERR_NONE;
}

static int Done(waveout_palm* p)
{
	Flush(p);
	if (p->Stream)
	{
		// Tungsten T1 seems to still use one or two callbacks after stopping
		SndStreamSetVolume(p->Stream,0);
		ManualSleep(100);
		SndStreamDelete(p->Stream);
		p->Stream = 0;
	}
	BufferClear(&p->Buffer);
	PCMRelease(p->PCMCopy);
	p->PCMCopy = NULL;
	PCMRelease(p->Base.PCM);
	p->Base.PCM = NULL;
	return ERR_NONE;
}

static const int32_t VolDb[201] = { //pow(10,(50+N)/49.828)
10,10,11,11,12,12,13,13,14,15,
16,16,17,18,19,20,21,22,23,24,
25,26,27,29,30,32,33,35,36,38,
40,42,44,46,48,50,53,55,58,61,
64,67,70,73,76,80,84,88,92,97,
101,106,111,116,122,128,134,140,147,154,
161,168,176,185,194,203,212,222,233,244,
256,268,280,294,308,322,337,353,370,388,
406,425,445,466,488,512,536,561,588,616,
645,675,707,741,776,812,851,891,933,977,
1024,1072,1123,1176,1232,1290,1351,1415,1482,1552,
1625,1702,1783,1867,1955,2048,2145,2246,2352,2464,
2580,2702,2830,2964,3104,3251,3405,3566,3734,3911,
4096,4290,4493,4705,4928,5161,5405,5661,5928,6209,
6503,6810,7132,7470,7823,8193,8580,8986,9411,9856,
10323,10811,11322,11858,12418,13006,13621,14265,14940,15646,
16386,17161,17973,18823,19713,20646,21622,22645,23716,24838,
26012,27243,28531,29881,31294,32774,34324,35947,37647,39428,
41293,43246,45291,47433,49676,52026,54486,57063,59762,62589,
65549,68649,71896,75296,78857,82587,86493,90584,94868,99355,
104054};

static int UpdateVolume(waveout_palm* p)
{
	if (p->Stream)
	{
		if (p->Mute || p->Volume<0)
			p->Vol = 0;
		else
		{
			int PreAmp = p->PreAmp+100;
			if (PreAmp<0) PreAmp=0;
			if (PreAmp>200) PreAmp=200;
			p->Vol = Scale(p->Volume,VolDb[PreAmp],100);
			if (p->Vol > VOLLIMIT)
				p->Vol = VOLLIMIT;
		}

		SndStreamSetVolume(p->Stream,p->Vol);
	}
	return ERR_NONE;
}

static int UpdatePan(waveout_palm* p)
{
	if (p->Stream)
		SndStreamSetPan(p->Stream,(p->Pan*1024)/100);
	return ERR_NONE;
}

static NOINLINE int UpdateSpeed(waveout_palm* p)
{
	p->SpeedTime = p->Speed;
	p->SpeedTime.Num *= TICKSPERSEC;
	p->SpeedTime.Den *= GetTimeFreq();
	if (!p->Speed.Num)
		p->CopySpeed = 0; //benchmark mode
	else
		p->CopySpeed = Scale(SPEED_ONE,p->Speed.Num,p->Speed.Den);
	p->SpeedBytePerSec = Scale(p->BytePerSec,p->Speed.Den,p->Speed.Num);
	p->DrvBufferSize = 0;
	p->DrvBufferDelay = 0;
	return ERR_NONE;
}

static int CreateStream(waveout_palm* p)
{
	SndSampleType Type;
	SndStreamWidth Width;

#if defined(_M_IX86)
	// this mess is all about avoiding simulator error message (no clue why the orginal pointer is invalid)
	static uint8_t CallbackJmp[] = {0xB8,0,0,0,0,0xFF,0xE0};
	SndStreamBufferCallback CallbackPtr = (SndStreamBufferCallback)CallbackJmp;
	*(SndStreamBufferCallback*)(CallbackJmp+1) = (SndStreamBufferCallback)Callback;
	if (!p->Native)
		CallbackPtr = (SndStreamBufferCallback)m68kCallBack(p->CallBack,(NativeFuncType*)T("tcpmp.dll\0WaveOutCallBack"));
#else
	SndStreamBufferCallback CallbackPtr = (SndStreamBufferCallback)Callback;
	if (!p->Native)
		CallbackPtr = (SndStreamBufferCallback)m68kCallBack(p->CallBack,WaveOutCallBack);
#endif

	if (p->Base.Output.Format.Audio.Bits==16)
	{
		if (p->Base.Output.Format.Audio.Flags & PCM_SWAPEDBYTES)
			Type = sndInt16Big;
		else
			Type = sndInt16Little;
	}
	else
	if (p->Base.Output.Format.Audio.Bits==8)
	{
		if (p->Base.Output.Format.Audio.Flags & PCM_UNSIGNED)
			Type = sndUInt8;
		else
			Type = sndInt8;
	}
	else
		return ERR_NOT_SUPPORTED;

	switch (p->Base.Output.Format.Audio.Channels)
	{
	case 1: Width = sndMono; break;
	case 2: Width = sndStereo; break;
	default: 
		return ERR_NOT_SUPPORTED;
	}

	SaveSysRegs(&p->SysRegs);
	if (SndStreamCreate(&p->Stream,sndOutput,p->Base.Output.Format.Audio.SampleRate,Type,
		Width,CallbackPtr,p,DRVBUFFER,p->Native) != errNone)
		return ERR_DEVICE_ERROR;

	UpdateVolume(p);
	UpdatePan(p);
	return ERR_NONE;
}

static void Force16Bits(waveout_palm* p)
{
	if (p->Base.Output.Format.Audio.Bits<16)
	{
		p->Base.Output.Format.Audio.Flags &= ~PCM_UNSIGNED;
		p->Base.Output.Format.Audio.Bits = 16;
		p->Base.Output.Format.Audio.FracBits = 15;
	}
}

static int Init(waveout_palm* p)
{
	int Model = QueryPlatform(PLATFORM_MODEL);
	int InputRate;
	int Result;

	if (p->Force16Bits)
		Force16Bits(p); // NX80v with MCA2 doesn't like 8bit output?

	if (Model == MODEL_LIFEDRIVE)
		p->Base.Output.Format.Audio.Flags |= PCM_LIFEDRIVE_FIX;

	if (Model == MODEL_TUNGSTEN_T || Model == MODEL_TUNGSTEN_T2 || Model == MODEL_TUNGSTEN_T3)
	{
		if (Model != MODEL_TUNGSTEN_T3 && p->Base.Output.Format.Audio.SampleRate > 44100) //48000hz unstable on T|T and T2
			p->Base.Output.Format.Audio.SampleRate = 44100;
		// on my T3 (updated ROM) 24000hz crashes almost always
		if (p->Base.Output.Format.Audio.SampleRate > 22050 && p->Base.Output.Format.Audio.SampleRate < 32000)
			p->Base.Output.Format.Audio.SampleRate = 32000;
		if (p->Base.Output.Format.Audio.SampleRate > 16000 && p->Base.Output.Format.Audio.SampleRate < 22050)
			p->Base.Output.Format.Audio.SampleRate = 22050;
		if (p->Base.Output.Format.Audio.SampleRate > 11025 && p->Base.Output.Format.Audio.SampleRate < 16000)
			p->Base.Output.Format.Audio.SampleRate = 16000;
	}

	if (Model == MODEL_ZODIAC)
	{
		// it seems Zodiac doesn't support 48000Hz
		if (p->Base.Output.Format.Audio.SampleRate > 44100)
			p->Base.Output.Format.Audio.SampleRate = 44100;
	}

	// in general for digital camera movies:
	if (p->Base.Output.Format.Audio.SampleRate > 8000 && p->Base.Output.Format.Audio.SampleRate < 11025)
	{
		p->Base.Output.Format.Audio.SampleRate = 11025;
		Force16Bits(p);
	}
	if (p->Base.Output.Format.Audio.SampleRate <= 8000)
	{
		p->Base.Output.Format.Audio.SampleRate = 8000;
		Force16Bits(p);
	}

	InputRate = (p->Base.Input.Format.Audio.Bits/8)*p->Base.Input.Format.Audio.SampleRate;
	if (!(p->Base.Input.Format.Audio.Flags & PCM_PLANES))
		InputRate *= p->Base.Input.Format.Audio.Channels;
	if (!InputRate)
		return ERR_NOT_SUPPORTED;

	Result = CreateStream(p);
	if (Result != ERR_NONE)
		return Result;

	p->BufferMode = 1;
	p->TimeRef = GetTimeTick();
	p->BytePerSample = (p->Base.Output.Format.Audio.Bits * p->Base.Output.Format.Audio.Channels)/8;
	p->BytePerSec = p->Base.Output.Format.Audio.SampleRate * p->BytePerSample;
	p->Ratio = (256 * p->BytePerSec + InputRate - 1) / InputRate;

	WaveOutPCM(&p->Base);
	p->PCMCopy = PCMCreate(&p->Base.Output.Format.Audio,&p->Base.Output.Format.Audio,0,0);

	if (!p->PCMCopy || !p->Base.PCM)
		return ERR_OUT_OF_MEMORY;

	if (UpdateBufferMode(p) != ERR_NONE)
		return ERR_OUT_OF_MEMORY;

	UpdateSpeed(p);
	return ERR_NONE;
}

static int UpdatePlay(waveout_palm* p)
{
	if (p->Play)
	{
		p->TimeRef = GetTimeTick();
		Start(p,p->Tick);
	}
	else
	{
		//tick_t Adjust = p->DrvBufferDelay; can be too big jump into the future, which is disturbing at pause
		int Tick = GetTimeTick();
		Stop(p);
		p->Tick += Scale(Tick-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);// + Adjust;
	}
	return ERR_NONE;
}

static int TimerGet(void* pt, int No, void* Data, int Size)
{
	waveout_palm* p = WAVEOUT(pt);
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_PLAY: GETVALUE(p->Play,bool_t); break;
	case TIMER_SPEED: GETVALUE(p->Speed,fraction); break;
	case TIMER_TIME:
		assert(Size == sizeof(tick_t));

		if (p->Speed.Num==0)
			*(tick_t*)Data = TIME_BENCH;
		else
		if (p->Play)
			*(tick_t*)Data = p->Tick + Scale(GetTimeTick()-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
		else
			*(tick_t*)Data = p->Tick;

		Result = ERR_NONE;
		break;
	}
	return Result;
}

static int TimerSet(void* pt, int No, const void* Data, int Size)
{
	waveout_palm* p = WAVEOUT(pt);
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_PLAY: SETVALUECMP(p->Play,bool_t,UpdatePlay(p),EqBool); break;
	case TIMER_SPEED: SETVALUECMP(p->Speed,fraction,UpdateSpeed(p),EqFrac); break;
	case TIMER_TIME:
		assert(Size == sizeof(tick_t));
		p->Tick = *(tick_t*)Data;
		p->TimeRef = GetTimeTick();
		Result = ERR_NONE;
		break;
	}
	return Result;
}

static int Get(waveout_palm* p, int No, void* Data, int Size)
{
	int Result = WaveOutBaseGet(&p->Base,No,Data,Size);
	switch (No)
	{
	case AOUT_VOLUME: GETVALUE(p->Volume,int); break;
	case AOUT_MUTE: GETVALUE(p->Mute,bool_t); break;
	case AOUT_PREAMP: GETVALUE(p->PreAmp,int); break;
	case AOUT_PAN: GETVALUE(p->Pan,int); break;
	case AOUT_MODE: GETVALUE(p->BufferMode,bool_t); break;
	}
	return Result;
}

static int Set(waveout_palm* p, int No, const void* Data, int Size)
{
	int Result = WaveOutBaseSet(&p->Base,No,Data,Size);
	switch (No)
	{
	case AOUT_VOLUME: SETVALUECMP(p->Volume,int,UpdateVolume(p),EqInt); break;
	case AOUT_MUTE: SETVALUECMP(p->Mute,bool_t,UpdateVolume(p),EqBool); break; 
	case AOUT_PREAMP: SETVALUECMP(p->PreAmp,int,UpdateVolume(p),EqInt); break;
	case AOUT_PAN: SETVALUECMP(p->Pan,int,UpdatePan(p),EqInt); break;
	case AOUT_MODE: SETVALUECMP(p->BufferMode,bool_t,UpdateBufferMode(p),EqBool); break;
	case FLOW_FLUSH:
		Result = Flush(p);
		break;
	}
	return Result;
}

static int Create(waveout_palm* p)
{
	int Model = QueryPlatform(PLATFORM_MODEL);
	UInt32 CompanyID;
	UInt32 Version;

	if (Model!=MODEL_TUNGSTEN_T && (FtrGet(sysFileCSoundMgr, sndFtrIDVersion, &Version)!=errNone || Version<100))
		return ERR_NOT_SUPPORTED;

	p->Base.Init = (nodefunc)Init;
	p->Base.Done = (nodefunc)Done;
	p->Base.Process = (packetprocess)Process;
	p->Base.Node.Get = (nodeget)Get;
	p->Base.Node.Set = (nodeset)Set;
	p->Base.Timer.Class = TIMER_CLASS;
	p->Base.Timer.Enum = TimerEnum;
	p->Base.Timer.Get = TimerGet;
	p->Base.Timer.Set = TimerSet;
	p->Base.Quality = 2;
	p->Speed.Num = 1;
	p->Speed.Den = 1;

	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &CompanyID);
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &Version);

	// I think MCA2 doesn't like more stop and start sequences. Stream has to deleted and recreated
	p->AlwaysClose = CompanyID == 'sony' && sysGetROMVerMajor(Version)==5 && sysGetROMVerMinor(Version)<2;
	p->Force16Bits = sysGetROMVerMajor(Version)==5 && sysGetROMVerMinor(Version)<2;
	p->Native = 1;

	if (Model==MODEL_QDA700)
		p->Native = 0; // crashes with ARM callback(?)

	return ERR_NONE;
}

static const nodedef WaveOut =
{
	sizeof(waveout_palm)|CF_GLOBAL,
	WAVEOUT_ID,
	AOUT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
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
