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
 * $Id: adpcm.c 565 2006-01-12 14:11:44Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "adpcm.h"
#include "g726/g72x.h"

typedef struct state 
{
    int Predictor;
    int StepIndex;
    int Step;
    int Sample1;
    int Sample2;
    int CoEff1;
    int CoEff2;
    int IDelta;

} state;

typedef struct adpcm
{
	codec Codec;
	buffer Data;

	int Channel; //IMA_QT
	int16_t* Buffer;
	state State[2];
	g726_state G726[2];

} adpcm;

static const int IndexTable[16] = 
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int StepTable[89] = 
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// AdaptationTable[], AdaptCoeff1[], and AdaptCoeff2[] are from libsndfile
static const int AdaptationTable[] = 
{
	230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230
};
static const int AdaptCoeff1[] = 
{
	256, 512, 0, 192, 240, 460, 392
};
static const int AdaptCoeff2[] = 
{
	0, -256, 0, 64, 0, -208, -232
};

static _INLINE int IMA_Calc(state* s, int v)
{
    int StepIndex;
    int Predictor;
    int Diff,Step;

    Step = StepTable[s->StepIndex];
    StepIndex = s->StepIndex + IndexTable[v];

    if (StepIndex < 0) 
		StepIndex = 0;
    else if (StepIndex > 88) 
		StepIndex = 88;

    Diff = ((2 * (v & 7) + 1) * Step) >> 3;

    Predictor = s->Predictor;
    if (v & 8) 
		Predictor -= Diff;
    else 
		Predictor += Diff;

	if (Predictor > 32767) 
		Predictor = 32767;
	else if (Predictor < -32768) 
		Predictor = -32768;

    s->Predictor = Predictor;
    s->StepIndex = StepIndex;

    return Predictor;
}

static _INLINE int MS_Calc(state* s, int v)
{
    int Predictor;

    Predictor = ((s->Sample1 * s->CoEff1) + (s->Sample2 * s->CoEff2)) >> 8;
    Predictor += ((v & 0x08) ? v-0x10:v) * s->IDelta;

	if (Predictor > 32767) 
		Predictor = 32767;
	else if (Predictor < -32768) 
		Predictor = -32768;

    s->Sample2 = s->Sample1;
    s->Sample1 = Predictor;
    s->IDelta = (AdaptationTable[v] * s->IDelta) >> 8;
    if (s->IDelta < 16) 
		s->IDelta = 16;

    return Predictor;
}

static int Process(adpcm* p, const packet* Packet, const flowstate* State)
{
	int i;
	int Predictor;
	const uint8_t* In;
	const uint8_t* InEnd;
	int16_t* Out = p->Buffer;

	if (Packet)
	{
		if (Packet->RefTime >= 0)
			p->Codec.Packet.RefTime = Packet->RefTime;

		BufferPack(&p->Data,0);
		BufferWrite(&p->Data,Packet->Data[0],Packet->Length,1024);
	}
	else
		p->Codec.Packet.RefTime = TIME_UNKNOWN;

	if (!BufferRead(&p->Data,&In,p->Codec.In.Format.Format.Audio.BlockAlign))
		return ERR_NEED_MORE_DATA;

	InEnd = In + p->Codec.In.Format.Format.Audio.BlockAlign;

	switch (p->Codec.Node.Class)
	{
	case ADPCM_G726_ID:
	{
		g726_state *g1,*g2;
		g1 = g2 = &p->G726[0];
		if (p->Codec.In.Format.Format.Audio.Channels==2)
			++g2;

		switch (p->Codec.In.Format.Format.Audio.Bits)
		{
		case 2:
			for (;In<InEnd;++In,Out+=4)
			{
				Out[0] = (int16_t)g726_16_decoder(In[0] >> 6,g1);
				Out[1] = (int16_t)g726_16_decoder(In[0] >> 4,g2);
				Out[2] = (int16_t)g726_16_decoder(In[0] >> 2,g1);
				Out[3] = (int16_t)g726_16_decoder(In[0],g2);
			}
			break;
		case 3:
			InEnd -= 2;
			for (;In<InEnd;In+=3,Out+=8)
			{
				Out[0] = (int16_t)g726_24_decoder(In[0] >> 5,g1);
				Out[1] = (int16_t)g726_24_decoder(In[0] >> 2,g2);
				Out[2] = (int16_t)g726_24_decoder((In[0] << 1) | (In[1] >> 7),g1);
				Out[3] = (int16_t)g726_24_decoder(In[1] >> 4,g2);
				Out[4] = (int16_t)g726_24_decoder(In[1] >> 1,g1);
				Out[5] = (int16_t)g726_24_decoder((In[1] << 2) | (In[2] >> 6),g2);
				Out[6] = (int16_t)g726_24_decoder(In[2] >> 3,g1);
				Out[7] = (int16_t)g726_24_decoder(In[2] >> 0,g2);
			}
			break;
		case 4:
			for (;In<InEnd;++In,Out+=2)
			{
				Out[0] = (int16_t)g726_32_decoder(In[0] >> 4,g1);
				Out[1] = (int16_t)g726_32_decoder(In[0],g2);
			}
			break;
		case 5:
			InEnd -= 4;
			for (;In<InEnd;In+=5,Out+=8)
			{
				Out[0] = (int16_t)g726_40_decoder(In[0] >> 3,g1);
				Out[1] = (int16_t)g726_40_decoder((In[0] << 2) | (In[1] >> 6),g2);
				Out[2] = (int16_t)g726_40_decoder(In[1] >> 1,g1);
				Out[3] = (int16_t)g726_40_decoder((In[1] << 4) | (In[2] >> 4),g2);
				Out[4] = (int16_t)g726_40_decoder((In[2] << 1) | (In[3] >> 7),g1);
				Out[5] = (int16_t)g726_40_decoder(In[3] >> 2,g2);
				Out[6] = (int16_t)g726_40_decoder((In[3] << 3) | (In[4] >> 5),g1);
				Out[7] = (int16_t)g726_40_decoder(In[4] >> 0,g2);
			}
			break;
		}
		break;
	}
	case ADPCM_IMA_QT_ID:
	{
		int No,Ch;
		Ch = p->Codec.In.Format.Format.Audio.Channels;
		for (No=0;No<Ch;++No)
		{
			state *s;
			s = &p->State[0];
			s->Predictor = (int16_t)((In[1] & 0x80) | (In[0] << 8));

			s->StepIndex = In[1] & 0x7F;
			if (s->StepIndex > 88) 
				s->StepIndex = 88;

			In+=2;
			InEnd=In+32;
			Out = p->Buffer+No;

			for (;In<InEnd;++In)
			{
				*Out = (int16_t)IMA_Calc(s, In[0] & 0x0F);
				Out+=Ch;
				*Out = (int16_t)IMA_Calc(s, In[0] >> 4);
				Out+=Ch;
			}
		}

		Out = p->Buffer+Ch*64;
		break;
	}

	case ADPCM_IMA_ID:
	{
		state *s1,*s2;
		s1 = &p->State[0];
		s1->Predictor = (int16_t)(In[0] | (In[1] << 8));
		In+=2;

		s1->StepIndex = *In++;
		if (s1->StepIndex > 88) 
			s1->StepIndex = 88;
		++In;

		if (p->Codec.In.Format.Format.Audio.Channels == 2)
		{
			s2 = &p->State[1];
			s2->Predictor = (int16_t)(In[0] | (In[1] << 8));
			In+=2;

			s2->StepIndex = *In++;
			if (s2->StepIndex > 88) 
				s2->StepIndex = 88;
			++In;

			for (i=4;In<InEnd;++In,Out+=4)
			{
				Out[0] = (int16_t)IMA_Calc(s1, In[0] & 0x0F);
				Out[1] = (int16_t)IMA_Calc(s2, In[4] & 0x0F);
				Out[2] = (int16_t)IMA_Calc(s1, In[0] >> 4);
				Out[3] = (int16_t)IMA_Calc(s2, In[4] >> 4);

				if (--i==0)
				{
					i=4;
					In+=4;
				}
			}
		}
		else
		{
			for (;In<InEnd;++In,Out+=2)
			{
				Out[0] = (int16_t)IMA_Calc(s1, In[0] & 0x0F);
				Out[1] = (int16_t)IMA_Calc(s1, In[0] >> 4);
			}
		}
		break;
	}
	case ADPCM_MS_ID:
	{
		state *s1,*s2;
		s1 = &p->State[0];
		s2 = p->Codec.In.Format.Format.Audio.Channels==2 ? &p->State[1] : s1;

		Predictor = *In++;
		if (Predictor > 7)
			Predictor = 7;
		s1->CoEff1 = AdaptCoeff1[Predictor];
		s1->CoEff2 = AdaptCoeff2[Predictor];

		if (s2 != s1)
		{
			Predictor = *In++;
			if (Predictor > 7)
				Predictor = 7;
			s2->CoEff1 = AdaptCoeff1[Predictor];
			s2->CoEff2 = AdaptCoeff2[Predictor];
		}

		s1->IDelta = (int16_t)(In[0] | (In[1] << 8));
		In+=2;

		if (s2 != s1)
		{
			s2->IDelta = (int16_t)(In[0] | (In[1] << 8));
			In+=2;
		}

		s1->Sample1 = (int16_t)(In[0] | (In[1] << 8));
		In+=2;
		if (s2 != s1)
		{
			s2->Sample1 = (int16_t)(In[0] | (In[1] << 8));
			In+=2;
		}

		s1->Sample2 = (int16_t)(In[0] | (In[1] << 8));
		In+=2;
		if (s2 != s1)
		{
			s2->Sample2 = (int16_t)(In[0] | (In[1] << 8));
			In+=2;
		}

		*Out++ = (int16_t)s1->Sample1;
		if (s2 != s1) *Out++ = (int16_t)s2->Sample1;

		*Out++ = (int16_t)s1->Sample2;
		if (s2 != s1) *Out++ = (int16_t)s2->Sample2;

		for (;In<InEnd;++In,Out+=2)
		{
			Out[0] = (int16_t)MS_Calc(s1, In[0] >> 4);
			Out[1] = (int16_t)MS_Calc(s2, In[0] & 0x0F);
		}
		break;
	}
	}

	p->Codec.Packet.Length = (uint8_t*)Out - (uint8_t*)p->Buffer;
	return ERR_NONE;
}

static int UpdateInput(adpcm* p)
{
	BufferClear(&p->Data);
	free(p->Buffer);
	p->Buffer = NULL;

	if (p->Codec.In.Format.Type == PACKET_AUDIO)
	{
		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,16);

		if (!p->Codec.In.Format.Format.Audio.BlockAlign)
			p->Codec.In.Format.Format.Audio.BlockAlign = 1024;

		if (p->Codec.Node.Class == ADPCM_IMA_QT_ID)
			p->Codec.In.Format.Format.Audio.BlockAlign = (32+2)*p->Codec.In.Format.Format.Audio.Channels;

		if (p->Codec.Node.Class == ADPCM_G726_ID)
		{
			p->Codec.In.Format.Format.Audio.BlockAlign = 120;
			g726_init_state(&p->G726[0]);
			g726_init_state(&p->G726[1]);
		}

		p->Buffer = (int16_t*) malloc(sizeof(int16_t)*4*p->Codec.In.Format.Format.Audio.BlockAlign);
		if (!p->Buffer)
			return ERR_OUT_OF_MEMORY;

		p->Codec.Packet.Data[0] = p->Buffer;
	}

	return ERR_NONE;
}

static int Flush(adpcm* p)
{
	if (p->Codec.Node.Class == ADPCM_G726_ID)
	{
		g726_init_state(&p->G726[0]);
		g726_init_state(&p->G726[1]);
	}
	BufferDrop(&p->Data);
	p->Channel = 0;
	return ERR_NONE;
}

static int Create(adpcm* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef ADPCM =
{
	sizeof(adpcm)|CF_ABSTRACT,
	ADPCM_CLASS,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

static const nodedef ADPCM_MS =
{
	0, //parent size
	ADPCM_MS_ID,
	ADPCM_CLASS,
	PRI_DEFAULT,
	NULL,
	NULL,
};

static const nodedef ADPCM_IMA =
{
	0, //parent size
	ADPCM_IMA_ID,
	ADPCM_CLASS,
	PRI_DEFAULT,
	NULL,
	NULL,
};

static const nodedef ADPCM_IMA_QT =
{
	0, //parent size
	ADPCM_IMA_QT_ID,
	ADPCM_CLASS,
	PRI_DEFAULT,
	NULL,
	NULL,
};

static const nodedef ADPCM_G726 =
{
	0, //parent size
	ADPCM_G726_ID,
	ADPCM_CLASS,
	PRI_DEFAULT,
	NULL,
	NULL,
};

void ADPCM_Init()
{
	NodeRegisterClass(&ADPCM);
	NodeRegisterClass(&ADPCM_MS);
	NodeRegisterClass(&ADPCM_IMA);
	NodeRegisterClass(&ADPCM_IMA_QT);
	NodeRegisterClass(&ADPCM_G726);
}

void ADPCM_Done()
{
	NodeUnRegisterClass(ADPCM_MS_ID);
	NodeUnRegisterClass(ADPCM_IMA_ID);
	NodeUnRegisterClass(ADPCM_IMA_QT_ID);
	NodeUnRegisterClass(ADPCM_G726_ID);
	NodeUnRegisterClass(ADPCM_CLASS);
}



