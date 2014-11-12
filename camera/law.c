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
 * $Id: law.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "law.h"

typedef struct law
{
	codec Codec;
	buffer Buffer;
	int16_t Table[256];

} law;

static int UpdateInput(law* p)
{
	BufferClear(&p->Buffer);
	if (p->Codec.In.Format.Type == PACKET_AUDIO)
		PacketFormatPCM(&p->Codec.Out.Format,&p->Codec.In.Format,16);
	return ERR_NONE;
}

static int Process(law* p, const packet* Packet, const flowstate* State)
{
	if (!Packet)
		return ERR_NEED_MORE_DATA;

	BufferDrop(&p->Buffer);
	if (!BufferWrite(&p->Buffer,NULL,Packet->Length*2,1024))
		return ERR_OUT_OF_MEMORY;

	p->Codec.Packet.RefTime = Packet->RefTime;
	p->Codec.Packet.Length = p->Buffer.WritePos;
	p->Codec.Packet.Data[0] = p->Buffer.Data;

	{
		uint8_t* s = (uint8_t*)Packet->Data[0];
		uint8_t* se = s + Packet->Length;
		int16_t* d = (int16_t*)p->Buffer.Data;
		for (;s!=se;++s,++d)
			*d = p->Table[*s];
	}
	return ERR_NONE;
}

/* from g711.c by SUN microsystems (unrestricted use) */

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

#define	BIAS		(0x84)		/* Bias for linear code. */

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static INLINE int alaw2linear(int a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = (a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

static INLINE int ulaw2linear(int u_val)
{
	int		t;

	/* Complement to obtain normal u-law value. */
	u_val ^= 0xFF;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= (u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

static int CreateLaw(law* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	return ERR_NONE;
}

static int CreateALaw(law* p)
{
	int n;
	for (n=0;n<256;++n)
		p->Table[n] = (int16_t)alaw2linear(n);
	return ERR_NONE;
}

static int CreateMuLaw(law* p)
{
	int n;
	for (n=0;n<256;++n)
		p->Table[n] = (int16_t)ulaw2linear(n);
	return ERR_NONE;
}

static const nodedef Law =
{
	sizeof(law)|CF_ABSTRACT,
	LAW_CLASS,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)CreateLaw,
	NULL,
};

static const nodedef ALaw =
{
	0, //parent size
	ALAW_ID,
	LAW_CLASS,
	PRI_DEFAULT,
	(nodecreate)CreateALaw,
	NULL,
};

static const nodedef MuLaw =
{
	0, //parent size
	MULAW_MS_ID,
	LAW_CLASS,
	PRI_DEFAULT,
	(nodecreate)CreateMuLaw,
	NULL,
};

void Law_Init()
{
	NodeRegisterClass(&Law);
	NodeRegisterClass(&ALaw);
	NodeRegisterClass(&MuLaw);
}

void Law_Done()
{
	NodeUnRegisterClass(LAW_CLASS);
	NodeUnRegisterClass(ALAW_ID);
	NodeUnRegisterClass(MULAW_MS_ID);
}

