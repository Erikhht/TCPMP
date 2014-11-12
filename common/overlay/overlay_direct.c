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
 * $Id: overlay_direct.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WINCE)

typedef struct direct
{
	overlay Overlay;
	planes Planes;
	bool_t KernelMode;
	void* PhyFrameBuffer;

} direct;

static int Init(direct* p)
{
	void* Ptr = NULL;
	video* Output = &p->Overlay.Output.Format.Video;
	video Desktop;

	memset(Output,0,sizeof(video));

	//set default values (for most of the models)
	Output->Width = 240;
	Output->Height = 320;
	Output->Pitch = 480;
	Output->Aspect = ASPECT_ONE;
	Output->Direction = 0;

	if (QueryPlatform(PLATFORM_CAPS) & CAPS_ONLY12BITRGB)
		DefaultRGB(&Output->Pixel,16,4,4,4,1,2,1);
	else
		DefaultRGB(&Output->Pixel,16,5,6,5,0,0,0);

	switch (QueryPlatform(PLATFORM_MODEL))
	{
	case MODEL_BSQUARE_ALCHEMY:
		Ptr = p->PhyFrameBuffer = PhyMemBegin(0x15000000,320*480,1);
		Output->Width = 240;
		Output->Height = 320;
		Output->Pitch = 480;
		break;

	case MODEL_IPAQ_H3600:
	case MODEL_IPAQ_H3700:
		Output->Direction = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
		Ptr = (void*)0xAC050020;
		Output->Width = 320;
		Output->Height = 240;
		Output->Pitch = 640;
		break;

	case MODEL_IPAQ_H3800:
		Output->Direction = DIR_SWAPXY | DIR_MIRRORUPDOWN;
		Ptr = (void*)0xAC050020;
		Output->Width = 320;
		Output->Height = 240;
		Output->Pitch = 640;
		break;

	case MODEL_COMPAQ_AERO_1500:
		QueryDesktop(&Desktop);
		Ptr = (void*)0xAA000000;
		Output->Direction = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
		Output->Width = 320;
		Output->Height = 240;
		Output->Pixel.Flags = PF_PALETTE|PF_INVERTED;
		Output->Pixel.BitCount = Desktop.Pixel.BitCount;
		Output->Pitch = (Output->Width * Output->Pixel.BitCount) >> 3;
		break;

	case MODEL_CASIO_BE300:
	case MODEL_CASIO_E105:
	case MODEL_CASIO_E115:
		Ptr = (void*)0xAA200000;
		Output->Pitch = 512;
		break;

	case MODEL_JORNADA_710:
	case MODEL_JORNADA_720:
		Ptr = p->PhyFrameBuffer = PhyMemBegin(0x48200000,240*1280,1);
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1280;
		break;

	case MODEL_JORNADA_680:
	case MODEL_JORNADA_690: 
		Ptr = (void*)0xB2000000; //only japan???
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1280;
		break;

	case MODEL_MOBILEGEAR2_550:
	case MODEL_MOBILEGEAR2_450:
	case MODEL_MOBILEGEAR2_530:
	case MODEL_MOBILEGEAR2_430:
	case MODEL_MOBILEPRO_780:
	case MODEL_MOBILEPRO_790:
		Ptr = (void*)0xAA180100;
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1280;
		break;

	case MODEL_MOBILEPRO_770:
		Ptr = (void*)0xAA000000;
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1600;
		break;

	case MODEL_JVC_C33:
		Ptr = (void*)0x445C0000;
		Output->Width = 1024;
		Output->Height = 600;
		Output->Pitch = 2048;
		break;

	case MODEL_SIGMARION:
		Ptr = (void*)0xAA000000;
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1280;
		break;

	case MODEL_SIGMARION2:
		Ptr = (void*)0xB0800000;
		Output->Width = 640;
		Output->Height = 240;
		Output->Pitch = 1280;
		break;

	case MODEL_SIGMARION3:
		Ptr = p->PhyFrameBuffer = PhyMemBegin(0x14800000,480*1600,1);
		Output->Width = 800;
		Output->Height = 480;
		Output->Pitch = 1600;
		break;

	case MODEL_INTERMEC_6651:
		Ptr = p->PhyFrameBuffer = PhyMemBegin(0x6C000000,480*1600,1);
		Output->Width = 800;
		Output->Height = 480;
		Output->Pitch = 1600;
		break;
	}

	if (!Ptr)
		return ERR_NOT_SUPPORTED;

	if (((int)Ptr & 15)==0 && (Output->Pitch & 15)==0)
		Output->Pixel.Flags |= PF_16ALIGNED;

	p->Planes[0] = Ptr;
	AdjustOrientation(Output,0);
	p->Overlay.SetFX = BLITFX_AVOIDTEARING;
	return ERR_NONE;
}

static void Done(direct* p)
{
	if (p->PhyFrameBuffer)
	{
		PhyMemEnd(p->PhyFrameBuffer);
		p->PhyFrameBuffer = NULL;
	}
}

static int Reset(direct* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static int Lock(direct* p, planes Planes, bool_t OnlyAligned)
{
	p->KernelMode = KernelMode(1);
	Planes[0] = p->Planes[0];
	return ERR_NONE;
}

static int Unlock(direct* p)
{
	KernelMode(p->KernelMode);
	return ERR_NONE;
}

static int Create(direct* p)
{
	if (Init(p) != ERR_NONE) // check if supported
		return ERR_NOT_SUPPORTED;
	Done(p);

	p->Overlay.Init = (ovlfunc)Init;
	p->Overlay.Done = (ovldone)Done;
	p->Overlay.Reset = (ovlfunc)Reset;
	p->Overlay.Lock = (ovllock)Lock;
	p->Overlay.Unlock = (ovlfunc)Unlock;
	return ERR_NONE;
}

static const nodedef Direct = 
{
	sizeof(direct)|CF_GLOBAL,
	DIRECT_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+90,
	(nodecreate)Create,
};

void OverlayDirect_Init()
{ 
	NodeRegisterClass(&Direct);
}

void OverlayDirect_Done()
{
	NodeUnRegisterClass(DIRECT_ID);
}

#endif
