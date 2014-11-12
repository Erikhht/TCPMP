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
 * $Id: equalizer.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#include "fixed.h"
#include <math.h>

#define MAXFILTER		10
#define ACCFAST_ADJUST	1

typedef struct eqfilter
{
	fix_t alpha;
	fix_t beta;
	fix_t gamma;
	fix_t alpha0;

} eqfilter;

typedef struct equalizer
{
	codec Codec;
	bool_t Enabled;
	bool_t Attenuate;
	int Amplify;	//-20..20
	int Eq[10];		//-20..20
	buffer Buffer;
	int Channels;
	void* PCM;
	fix_t Scale;
	fix_t ScalePreamp;
	fix_t ScaleFinal;
	eqfilter Filter[MAXFILTER];
	fix_t State[MAXPLANES][3+MAXFILTER*2];
	planes Tmp;

} equalizer;

static const datatable Params[] = 
{
	{ EQUALIZER_ENABLED,	TYPE_BOOL,DF_SETUP|DF_CHECKLIST },
	{ EQUALIZER_ATTENUATE,	TYPE_BOOL,DF_SETUP|DF_CHECKLIST },
	{ EQUALIZER_PREAMP,		TYPE_INT, DF_SETUP|DF_MINMAX|DF_SHORTLABEL, -20, 20 },

	{ EQUALIZER_1,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL|DF_GAP, -20, 20 },
	{ EQUALIZER_2,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_3,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_4,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_5,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_6,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_7,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_8,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_9,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },
	{ EQUALIZER_10,			TYPE_INT, DF_SETUP|DF_MINMAX|DF_NOWRAP|DF_SHORTLABEL, -20, 20 },

	{ EQUALIZER_RESET,		TYPE_RESET,DF_SETUP|DF_NOSAVE|DF_GAP },

	DATATABLE_END(EQUALIZER_ID)
};

static int Enum( equalizer* p, int* No, datadef* Param)
{
	if (CodecEnum(&p->Codec,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,Params);
}

static int Get( equalizer* p, int No, void* Data, int Size )
{
	int Result = CodecGet(&p->Codec,No,Data,Size);
	switch (No)
	{
	case EQUALIZER_RESET: Result = ERR_NONE; break;
	case EQUALIZER_ENABLED: GETVALUE(p->Enabled,bool_t); break;
	case EQUALIZER_ATTENUATE: GETVALUE(p->Attenuate,bool_t); break;
	case EQUALIZER_PREAMP: GETVALUE(p->Amplify,int); break;
	case EQUALIZER_1:
	case EQUALIZER_2:
	case EQUALIZER_3:
	case EQUALIZER_4:
	case EQUALIZER_5:
	case EQUALIZER_6:
	case EQUALIZER_7:
	case EQUALIZER_8:
	case EQUALIZER_9:
	case EQUALIZER_10:GETVALUE(p->Eq[No-EQUALIZER_1],int); break;
	}
	return Result;
}

static NOINLINE void UpdateScale(equalizer* p)
{
	fix_t Scale;
	int n;

	if (!p->Attenuate)
		p->Scale=FIXC(1.);
	if (p->Scale<FIXC(1./65556))
		p->Scale=FIXC(1./65556);
	Scale = fix_mul(p->Scale,p->ScalePreamp);
	p->ScaleFinal = FIXFAST_BSHIFT(Scale);
	for (n=0;n<MAXFILTER;++n)
		p->Filter[n].alpha = ACCFAST_BSHIFT(fix_mul(p->Filter[n].alpha0,Scale));
}

static const eqfilter Band44100[MAXFILTER] =
{
	{ FIXC(0.003013),FIXC(-0.993973),FIXC( 1.993901) },
	{ FIXC(0.008490),FIXC(-0.983019),FIXC( 1.982437) },
	{ FIXC(0.015374),FIXC(-0.969252),FIXC( 1.967331) },
	{ FIXC(0.029328),FIXC(-0.941343),FIXC( 1.934254) },
	{ FIXC(0.047918),FIXC(-0.904163),FIXC( 1.884869) },
	{ FIXC(0.130408),FIXC(-0.739184),FIXC( 1.582718) },
	{ FIXC(0.226555),FIXC(-0.546889),FIXC( 1.015267) },
	{ FIXC(0.344937),FIXC(-0.310127),FIXC(-0.181410) },
	{ FIXC(0.366438),FIXC(-0.267123),FIXC(-0.521151) },
	{ FIXC(0.379009),FIXC(-0.241981),FIXC(-0.808451) },
};

static const fix_t PowTab[37] = //pow(10,(n-18)/20.f))
{
	FIXC(0.125892541),FIXC(0.141253754),FIXC(0.158489319),FIXC(0.177827941),
	FIXC(0.199526231),FIXC(0.223872114),FIXC(0.251188643),FIXC(0.281838293),
	FIXC(0.316227766),FIXC(0.354813389),FIXC(0.398107171),FIXC(0.446683592),
	FIXC(0.501187234),FIXC(0.562341325),FIXC(0.630957344),FIXC(0.707945784),
	FIXC(0.794328235),FIXC(0.891250938),FIXC(1.000000000),FIXC(1.122018454),
	FIXC(1.258925412),FIXC(1.412537545),FIXC(1.584893192),FIXC(1.778279410),
	FIXC(1.995262315),FIXC(2.238721139),FIXC(2.511886432),FIXC(2.818382931),
	FIXC(3.162277660),FIXC(3.548133892),FIXC(3.981071706),FIXC(4.466835922),
	FIXC(5.011872336),FIXC(5.623413252),FIXC(6.309573445),FIXC(7.079457844),
	FIXC(7.943282347),
};

static INLINE fix_t Pow(int n)
{
	if (n<-18) n=-18;
	if (n>18) n=18;
	return PowTab[n+18];
}

static NOINLINE int UpdateParam(equalizer* p)
{
	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		int n;
		const eqfilter *src;
		eqfilter *dst;

		src = Band44100;
		dst = p->Filter;
		for (n=0;n<MAXFILTER;++n,++src,++dst)
		{
			dst->alpha0 = fix_mul(src->alpha,Pow(p->Eq[n])-FIXC(1.));
			dst->beta = ACCFAST_BSHIFT(src->beta);
			dst->gamma = ACCFAST_BSHIFT(src->gamma);
		}

		p->ScalePreamp = Pow(p->Amplify);
		UpdateScale(p);
	}
	return ERR_NONE;
}

static NOINLINE int UpdateEqualizer()
{
	context* p = Context();
	if (!p->Player)
		return ERR_NONE;
	return p->Player->Set(p->Player,PLAYER_UPDATEEQUALIZER,NULL,0);
}

static int Set( equalizer* p, int No, const void* Data, int Size )
{
	int Result = CodecSet(&p->Codec,No,Data,Size);
	switch (No)
	{
	case EQUALIZER_ENABLED: SETVALUE(p->Enabled,bool_t,UpdateEqualizer()); break;
	case EQUALIZER_ATTENUATE: SETVALUE(p->Attenuate,int,UpdateParam(p)); break;
	case EQUALIZER_PREAMP: SETVALUE(p->Amplify,int,UpdateParam(p)); break;
	case EQUALIZER_1: 
	case EQUALIZER_2: 
	case EQUALIZER_3: 
	case EQUALIZER_4: 
	case EQUALIZER_5: 
	case EQUALIZER_6: 
	case EQUALIZER_7: 
	case EQUALIZER_8: 
	case EQUALIZER_9: 
	case EQUALIZER_10:SETVALUE(p->Eq[No-EQUALIZER_1],int,UpdateParam(p)); break;
	}
	return Result;
}

static int Flush(equalizer* p)
{
	memset(p->State,0,sizeof(p->State));
	return ERR_NONE;
}

static int UpdateInput(equalizer* p)
{
	PCMRelease(p->PCM);
	p->PCM = NULL;
	BufferClear(&p->Buffer);

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		if (!p->Enabled || p->Codec.In.Format.Format.Audio.Channels>MAXPLANES)
			return ERR_INVALID_PARAM;

		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,FIX_FRACBITS+1);
		p->Codec.Out.Format.Format.Audio.Bits = sizeof(fix_t)*8;
		p->Codec.Out.Format.Format.Audio.Flags = PCM_PLANES;
#ifndef FIXED_POINT
		p->Codec.Out.Format.Format.Audio.Flags |= PCM_FLOAT;
#endif

		p->PCM = PCMCreate(&p->Codec.Out.Format.Format.Audio,&p->Codec.In.Format.Format.Audio,0,0);
		if (!p->PCM)
			return ERR_OUT_OF_MEMORY;
		p->Scale = FIXC(1);
		UpdateParam(p);
		Flush(p);
	}
	return ERR_NONE;
}

static NOINLINE void Rescale(planes data, fix_t v, int ramp, size_t length, int channels)
{
	fix_t dv;
	int n;

	if (length < 4*sizeof(fix_t)) return;
	while ((sizeof(fix_t)<<ramp) > length) --ramp;

#ifdef FIXED_POINT
	dv = (v-FIXC(1)) >> ramp;
#else
	dv = (fix_t)ldexp(v-FIXC(1),-ramp);
#endif

	for (n=0;n<channels;++n)
	{
		fix_t* i=data[n];
		fix_t* ie=i+(1<<ramp);

		v = FIXC(1);
		for (;i!=ie;++i,v+=dv)
			i[0] = fix_mul(i[0],v);

		ie = (fix_t*)((uint8_t*)ie+length)-(1<<ramp);
		for (;i!=ie;i+=4)
		{
			i[0] = fix_mul(i[0],v);
			i[1] = fix_mul(i[1],v);
			i[2] = fix_mul(i[2],v);
			i[3] = fix_mul(i[3],v);
		}
	}
}

static int Process(equalizer* p, const packet* Packet, const flowstate* State)
{
	fix_t* s;
	int n,DstLength,SrcLength,Channels;
	if (!Packet)
		return ERR_NEED_MORE_DATA;

	Channels = p->Codec.In.Format.Format.Audio.Channels;
	SrcLength = Packet->Length;
	DstLength = PCMDstLength(p->PCM,SrcLength);
	if (DstLength*Channels*2 > p->Buffer.Allocated)
	{
		uint8_t* Data;
		if (!BufferAlloc(&p->Buffer,DstLength*Channels*2,1024))
			return ERR_OUT_OF_MEMORY;

		Data = p->Buffer.Data;
		for (n=0;n<Channels;++n)
		{
			p->Codec.Packet.Data[n] = Data; 
			Data += DstLength;
			p->Tmp[n] = Data; 
			Data += DstLength;
		}
	}

	PCMConvert(p->PCM,*(planes*)&p->Codec.Packet.Data,Packet->Data,&DstLength,&SrcLength,SPEED_ONE,256);
	p->Codec.Packet.RefTime = Packet->RefTime;
	p->Codec.Packet.Length = DstLength;

	s = p->State[0];
	for (n=0;n<Channels;++n)
	{
		eqfilter* f;
		fix_t* i=(fix_t*)p->Codec.Packet.Data[n];
		fix_t* ie=(fix_t*)((uint8_t*)i+DstLength);
		fix_t* j=(fix_t*)p->Tmp[n];
		fix_t a,b,c,d,e;
		e = p->ScaleFinal;
		a = s[2];
		b = s[1];
		c = s[0];
		for (ie-=3;i<ie;i+=4,j+=4)
		{
			d=i[0];
			i[0]=fixfast_mul(c,e);
			j[0]=ACCFAST_ASHIFT(c-a,ACCFAST_ADJUST);
			a=i[1];
			i[1]=fixfast_mul(d,e);
			j[1]=ACCFAST_ASHIFT(d-b,ACCFAST_ADJUST);
			b=i[2];
			i[2]=fixfast_mul(a,e);
			j[2]=ACCFAST_ASHIFT(a-c,ACCFAST_ADJUST);
			c=i[3];
			i[3]=fixfast_mul(b,e);
			j[3]=ACCFAST_ASHIFT(b-d,ACCFAST_ADJUST);
		}
		for (ie+=3;i!=ie;++i,++j)
		{
			j[0]=ACCFAST_ASHIFT(c-a,ACCFAST_ADJUST);
			a=b;
			b=c;
			c=i[0];
			i[0]=fixfast_mul(b,e);
		}
		s[2] = a;
		s[1] = b;
		s[0] = c;
		s += 3;

		for (f=p->Filter;f!=p->Filter+MAXFILTER;f+=2,s+=2*2)
			if (f[0].alpha!=0 || f[1].alpha!=0)
			{
				accfast_var(v0);
				accfast_var(v1);

				i=(fix_t*)p->Codec.Packet.Data[n];
				ie=(fix_t*)((uint8_t*)i+DstLength);
				j=(fix_t*)p->Tmp[n];
				a = s[0];
				c = s[1];
				b = s[2];
				d = s[3];
				for (--ie;i<ie;i+=2,j+=2)
				{
					accfast_mul(v0,a,f[0].beta);
					accfast_mul(v1,b,f[1].beta);
					accfast_mla(v0,j[0],f[0].alpha);
					accfast_mla(v1,j[0],f[1].alpha);
					accfast_mla(v0,c,f[0].gamma);
					accfast_mla(v1,d,f[1].gamma);
					a = accfast_get(v0,ACCFAST_ADJUST);
					b = accfast_get(v1,ACCFAST_ADJUST);
					i[0] += a+b;
					a = ACCFAST_ASHIFT(a,ACCFAST_ADJUST);
					b = ACCFAST_ASHIFT(b,ACCFAST_ADJUST);
					accfast_mul(v0,c,f[0].beta);
					accfast_mul(v1,d,f[1].beta);
					accfast_mla(v0,j[1],f[0].alpha);
					accfast_mla(v1,j[1],f[1].alpha);
					accfast_mla(v0,a,f[0].gamma);
					accfast_mla(v1,b,f[1].gamma);
					c = accfast_get(v0,ACCFAST_ADJUST);
					d = accfast_get(v1,ACCFAST_ADJUST);
					i[1] += c+d;
					c = ACCFAST_ASHIFT(c,ACCFAST_ADJUST);
					d = ACCFAST_ASHIFT(d,ACCFAST_ADJUST);
				}
				if (i==ie)
				{
					accfast_mul(v0,a,f[0].beta);
					accfast_mul(v1,b,f[1].beta);
					accfast_mla(v0,j[0],f[0].alpha);
					accfast_mla(v1,j[0],f[1].alpha);
					accfast_mla(v0,c,f[0].gamma);
					accfast_mla(v1,d,f[1].gamma);
					a = c; c = accfast_get(v0,ACCFAST_ADJUST);
					b = d; d = accfast_get(v1,ACCFAST_ADJUST);
					i[0] += c+d;
					c = ACCFAST_ASHIFT(c,ACCFAST_ADJUST);
					d = ACCFAST_ASHIFT(d,ACCFAST_ADJUST);
				}
				s[0] = a;
				s[1] = c;
				s[2] = b;
				s[3] = d;
			}
	}

	if (p->Attenuate)
	{
		fix_t Max = 0;
		for (n=0;n<Channels;++n)
		{
			fix_t* i=(fix_t*)p->Codec.Packet.Data[n];
			fix_t* ie=(fix_t*)((uint8_t*)i+DstLength);
			for (;i!=ie;++i)
			{
				fix_t v = *i;
				if (v<0) v=-v;
				if (v>Max) Max=v;
			}
		}

		if (Max > FIXC(1.))
		{
			Max = fix_mul(fix_1div(Max),FIXC(15./16));
			Rescale(*(planes*)&p->Codec.Packet.Data,Max,5,DstLength,Channels);
			
			p->Scale = fix_mul(p->Scale,Max);
			UpdateScale(p);
		}
		else
		if (Max < FIXC(6./8) && p->Scale < FIXC(1-1./256))
		{
			Max = FIXC(32./31);
			if (fix_mul(p->Scale,Max) > FIXC(1))
				Max = fix_1div(p->Scale);
			Rescale(*(planes*)&p->Codec.Packet.Data,Max,10,DstLength,Channels);
		
			p->Scale = fix_mul(p->Scale,Max);
			UpdateScale(p);
		}
	}
	else
	if (p->Scale != FIXC(1.))
		UpdateScale(p);

	return ERR_NONE;
}

static int Create(equalizer* p)
{
	p->Codec.Node.Enum = (nodeenum)Enum;
	p->Codec.Node.Set = (nodeset)Set;
	p->Codec.Node.Get = (nodeget)Get;
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef Equalizer =
{
	sizeof(equalizer)|CF_GLOBAL|CF_SETTINGS,
	EQUALIZER_ID,
	CODEC_CLASS,
	PRI_MAXIMUM+580,
	(nodecreate)Create,
};

void Equalizer_Init()
{
	NodeRegisterClass(&Equalizer);
}

void Equalizer_Done()
{
	NodeUnRegisterClass(EQUALIZER_ID);
}
