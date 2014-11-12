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
 * $Id: overlay.c 551 2006-01-09 11:55:09Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

int OverlayDefaultBlit(overlay* p, const constplanes Data, const constplanes DataLast)
{
	planes Planes;
	int Result = p->Lock(p,Planes,1);
	if (Result==ERR_NONE)
	{
		BlitImage(p->Soft,Planes,Data,DataLast,p->Output.Format.Video.Pitch,-1);
		p->Unlock(p);
	}
	return Result;
}

int OverlayEnum(overlay* p, int* No, datadef* Param)
{
	return VOutEnum(p,No,Param);
}

static int Create(overlay* p)
{
	p->Node.Enum = (nodeenum)OverlayEnum;
	p->Node.Get = (nodeget)OverlayGet;
	p->Node.Set = (nodeset)OverlaySet;
	p->Blit = (ovlblit)OverlayDefaultBlit;
	p->Update = (ovlfunc)OverlayUpdateAlign;

	p->Caps = -1;
	p->Soft = NULL;
	p->Primary = 1;
	p->Overlay = 0;
	p->DoPowerOff = 0;

	memset(&p->Backup,0,sizeof(idctbackup));
	memset(&p->OrigFX,0,sizeof(blitfx));
	p->OrigFX.ScaleX = SCALE_ONE;
	p->OrigFX.ScaleY = SCALE_ONE;
	p->AutoPrerotate = 0;
	p->ColorKey = RGB_NULL;
	p->Aspect.Num = 0;
	p->Aspect.Den = 1;
	p->Output.Type = PACKET_VIDEO;

	return ERR_NONE;
}

static void Delete(overlay* p)
{
	BlitRelease(p->Soft);
	ReleaseModule(&p->Module);
}

void OverlayClearBorder(overlay* p)
{
	planes Planes;
	p->Dirty = 1;
	if (p->Lock && p->Lock(p,Planes,0) == ERR_NONE)
	{
		uint32_t c = RGBToFormat(CRGB(0,0,0),&p->Output.Format.Video.Pixel);
		rect Viewport;
		VirtToPhy(&p->Viewport,&Viewport,&p->Output.Format.Video);

		FillColor(Planes[0],p->Output.Format.Video.Pitch,Viewport.x,Viewport.y,
			Viewport.Width,p->DstAlignedRect.y-Viewport.y,
			p->Output.Format.Video.Pixel.BitCount,c);

		FillColor(Planes[0],p->Output.Format.Video.Pitch,Viewport.x,p->DstAlignedRect.y,
			p->DstAlignedRect.x-Viewport.x,p->DstAlignedRect.Height,
			p->Output.Format.Video.Pixel.BitCount,c);

		FillColor(Planes[0],p->Output.Format.Video.Pitch,p->DstAlignedRect.x+p->DstAlignedRect.Width,p->DstAlignedRect.y,
			(Viewport.x+Viewport.Width)-(p->DstAlignedRect.x+p->DstAlignedRect.Width),p->DstAlignedRect.Height,
			p->Output.Format.Video.Pixel.BitCount,c);

		FillColor(Planes[0],p->Output.Format.Video.Pitch,Viewport.x,p->DstAlignedRect.y+p->DstAlignedRect.Height,
			Viewport.Width,(Viewport.y+Viewport.Height)-(p->DstAlignedRect.y+p->DstAlignedRect.Height),
			p->Output.Format.Video.Pixel.BitCount,c);

		p->Unlock(p);
	}
}

int OverlayUpdateAlign(overlay* p)
{
	rect OldGUI = p->GUIAlignedRect;
	rect Old = p->DstAlignedRect;

	DEBUG_MSG4(DEBUG_TEST,T("BLIT Viewport:%d %d %d %d"),p->Viewport.x,p->Viewport.y,p->Viewport.Width,p->Viewport.Height);

	VirtToPhy(&p->Viewport,&p->DstAlignedRect,&p->Output.Format.Video);
	VirtToPhy(NULL,&p->SrcAlignedRect,&p->Input.Format.Video);

	BlitRelease(p->Soft);
	p->Soft = BlitCreate(&p->Output.Format.Video,&p->Input.Format.Video,&p->FX,&p->Caps);

	BlitAlign(p->Soft,&p->DstAlignedRect, &p->SrcAlignedRect);
	PhyToVirt(&p->DstAlignedRect,&p->GUIAlignedRect,&p->Output.Format.Video);

	//DEBUG_MSG4(DEBUG_TEST,T("BLIT DstRect:%d %d %d %d"),p->DstAlignedRect.x,p->DstAlignedRect.y,p->DstAlignedRect.Width,p->DstAlignedRect.Height);
	//DEBUG_MSG4(DEBUG_TEST,T("BLIT SrcRect:%d %d %d %d"),p->SrcAlignedRect.x,p->SrcAlignedRect.y,p->SrcAlignedRect.Width,p->SrcAlignedRect.Height);

	if (!EqRect(&Old,&p->DstAlignedRect) && p->Show && p->Primary)
	{
		WinInvalidate(&OldGUI,0);
		WinInvalidate(&p->Viewport,1);
		WinValidate(&p->GUIAlignedRect);
	}

	if (p->Show && (p->FullScreenViewport || !p->Primary))
		OverlayClearBorder(p);

	return ERR_NONE;
}

static int Blit(overlay* p,const packet* Packet,const flowstate* State)
{
	int Result = ERR_NONE;

	if (Packet)
	{
		if (State->DropLevel)
		{
			++p->Dropped;
			return ERR_NONE;
		}

		if (State->CurrTime >= 0)
		{
			if (!p->Play && State->CurrTime == p->LastTime)
				return ERR_BUFFER_FULL;

			if (Packet->RefTime >= (State->CurrTime + SHOWAHEAD))
				return ERR_BUFFER_FULL;

			p->LastTime = State->CurrTime;
		}
		else
			p->LastTime = Packet->RefTime;

		p->CurrTime = State->CurrTime;

		if (State->CurrTime != TIME_RESEND)
			++p->Total;

		if (p->Soft && p->Inited && p->Show && Packet->Data[0])
		{
			const constplanes* LastData = &Packet->LastData;
			if (p->Dirty)
				LastData = NULL;

		/* { static tick_t Last = 0;
			DebugMessage("%d %d",Packet->RefTime,Packet->RefTime-Last);
			Last = Packet->RefTime; } */

			Result = p->Blit(p,Packet->Data,*LastData);
			
			if (Result == ERR_NONE)
				p->Dirty = 0;
		}
	}
	else
	if (State->DropLevel)
		++p->Dropped;

	return Result;
}

int OverlayGet(overlay* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case OUT_INPUT: GETVALUE(p->Pin,pin); break;
	case OUT_OUTPUT|PIN_FORMAT: GETVALUE(p->Output,packetformat); break;
	case OUT_INPUT|PIN_FORMAT: GETVALUE(p->Input,packetformat); break;
	case OUT_INPUT|PIN_PROCESS: GETVALUE((packetprocess)Blit,packetprocess); break;
	case OUT_TOTAL: GETVALUE(p->Total,int); break;
	case OUT_DROPPED: GETVALUE(p->Dropped,int); break;
	case VOUT_PRIMARY: GETVALUE(p->Primary,bool_t); break;
	case VOUT_OVERLAY: GETVALUE(p->Overlay,bool_t); break;
	case VOUT_IDCT: GETVALUE(p->AccelIDCT,idct*); break;
	case VOUT_VISIBLE: GETVALUE(p->Visible,bool_t); break;
	case VOUT_CLIPPING: GETVALUE(p->Clipping,bool_t); break;
	case VOUT_FX: GETVALUE(p->OrigFX,blitfx); break;
	case VOUT_VIEWPORT: GETVALUE(p->Viewport,rect); break;
	case VOUT_FULLSCREEN: GETVALUE(p->FullScreenViewport,bool_t); break;
	case VOUT_OUTPUTRECT: GETVALUECOND(p->GUIAlignedRect,rect,!p->Disabled); break;
	case VOUT_AUTOPREROTATE: GETVALUE(p->AutoPrerotate,bool_t); break;
	case VOUT_UPDATING: GETVALUE(p->Updating,bool_t); break;
	case VOUT_PLAY: GETVALUE(p->Play,bool_t); break;
	case VOUT_CAPS: GETVALUE(p->Caps,int); break;
	case VOUT_COLORKEY: GETVALUECOND(p->ColorKey,rgbval_t,!p->Disabled && p->ColorKey!=RGB_NULL); break;
	case VOUT_ASPECT: GETVALUE(p->Aspect,fraction); break;
	case FLOW_BACKGROUND: GETVALUECOND(p->Background,bool_t,p->DoPowerOff); break;
	}
	return Result;
}

static NOINLINE void UpdateInputFormat(overlay* p)
{
	if (p->Inited && !p->UpdateInputFormat && p->PrefDirection != p->Input.Format.Video.Direction && p->Pin.Node)
	{	
		packetformat Format = p->Input;
		if ((p->PrefDirection ^ Format.Format.Video.Direction) & DIR_SWAPXY)
			SwapInt(&Format.Format.Video.Width,&Format.Format.Video.Height);
		Format.Format.Video.Direction = p->PrefDirection;

		p->UpdateInputFormat = 1;
		p->Pin.Node->Set(p->Pin.Node,p->Pin.No|PIN_FORMAT,&Format,sizeof(Format));
		p->UpdateInputFormat = 0;
	}
}

int OverlayUpdateFX(overlay* p, bool_t ForceUpdate)
{
	fraction Aspect = p->Aspect;
	blitfx LastFX = p->FX;
	int Width,Height,ScaleX,ScaleY;

	if (!p->Inited || p->Updating) return ERR_NONE;

	if (Aspect.Num==0) // source
	{
		Aspect.Num = p->Input.Format.Video.Aspect;
		Aspect.Den = ASPECT_ONE;
		if (Aspect.Num == ASPECT_ONE || !Aspect.Num)
			Aspect.Num = DefaultAspect(p->Input.Format.Video.Width,p->Input.Format.Video.Height);
	}
	else
	if (Aspect.Num<0)
	{
		// screen aspect ratio -> pixel aspect ratio
		if (p->InputDirection & DIR_SWAPXY)
		{
			Aspect.Num *= -p->Input.Format.Video.Width;
			Aspect.Den *= p->Input.Format.Video.Height;
		}
		else
		{
			Aspect.Num *= -p->Input.Format.Video.Height;
			Aspect.Den *= p->Input.Format.Video.Width;
		}
	}

	p->FX = p->OrigFX;
	p->FX.Flags |= p->SetFX;
	p->FX.Flags &= ~p->ClearFX;
	p->FX.Direction = CombineDir(p->InputDirection,p->OrigFX.Direction,p->Output.Format.Video.Direction);
	p->PrefDirection = CombineDir(p->FX.Direction,0,p->Input.Format.Video.Direction);

	if (p->OrigFX.ScaleX<=0)
	{
		int v = p->OrigFX.ScaleX;

		Width = p->Viewport.Width;
		Height = p->Viewport.Height;

		if ((p->InputDirection ^ p->OrigFX.Direction) & DIR_SWAPXY)
			SwapInt(&Width,&Height);

		if (p->InputDirection & DIR_SWAPXY)
		{
			ScaleX = Scale(Width,SCALE_ONE,p->Input.Format.Video.Width);
			ScaleY = Scale(Height,SCALE_ONE,Scale(p->Input.Format.Video.Height,Aspect.Num,Aspect.Den));
		}
		else
		{
			ScaleX = Scale(Width,SCALE_ONE,Scale(p->Input.Format.Video.Width,Aspect.Num,Aspect.Den));
			ScaleY = Scale(Height,SCALE_ONE,p->Input.Format.Video.Height);
		}

		if ((p->InputDirection & DIR_SWAPXY) && (v == -1 || v == -2))
			v = -3-v; // swap 'fit width' and 'fit height'

		if (v==-3)
		{
			//todo: fill screen, but always using the fullscreen aspect ratio!
			p->FX.ScaleY = ScaleY;
			p->FX.ScaleX = ScaleX;
		}
		else
		{
			if ((v == -2) || (v!=-1 && v!=-4 && ScaleX>ScaleY) || (v==-4 && ScaleX<ScaleY))
				ScaleX = ScaleY;
			
			if (v<-4)
				ScaleX = Scale(ScaleX,-v,SCALE_ONE);

			p->FX.ScaleY = ScaleX;
			p->FX.ScaleX = Scale(ScaleX,Aspect.Num,Aspect.Den);

			if (p->InputDirection & DIR_SWAPXY)
				SwapInt(&p->FX.ScaleX,&p->FX.ScaleY);
		}
	}
	else
	{
		// prefer source horizontal scaling (because of smooth scale option)
		if (p->InputDirection & DIR_SWAPXY)
			p->FX.ScaleY = Scale(p->FX.ScaleY,Aspect.Num,Aspect.Den);
		else
			p->FX.ScaleX = Scale(p->FX.ScaleX,Aspect.Num,Aspect.Den);
	}

	if (p->Output.Format.Video.Pixel.Flags & PF_PIXELDOUBLE)
	{
		p->FX.ScaleX >>= 1;
		p->FX.ScaleY >>= 1;
	}

	if ((p->FX.Flags & BLITFX_ONLYDIFF) && p->FX.ScaleX < (SCALE_ONE*2)/3)
		p->FX.Flags &= ~BLITFX_ONLYDIFF;

	if (p->ForceUpdate)
	{
		p->ForceUpdate = 0;
		ForceUpdate = 1;
	}

	if (ForceUpdate || !EqBlitFX(&p->FX,&LastFX))
	{
		p->Update(p);
		p->Dirty = 1;
		p->LastTime = -1;
		UpdateInputFormat(p);
	}

	return ERR_NONE;
}

static int UpdateInputDirection(overlay* p,bool_t Update)
{
	if (p->Inited && !p->Updating)
	{
		p->InputDirection = p->Input.Format.Video.Direction;
		p->PreRotate = 0;

		if (p->AutoPrerotate && !(p->Input.Format.Video.Pixel.Flags & PF_NOPREROTATE))
		{
			rect r;
			PhyToVirt(NULL,&r,&p->Input.Format.Video);

			// portrait?
			if (r.Width < r.Height)
			{
				p->PreRotate = 1;
				if (p->InputDirection & DIR_SWAPXY)
					p->InputDirection ^= DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
				else
					p->InputDirection ^= DIR_SWAPXY | DIR_MIRRORUPDOWN;
			}
		}

		if (Update)
			OverlayUpdateFX(p,0);
	}
	return ERR_NONE;
}

int OverlayUpdateShow(overlay* p,bool_t Temp)
{
	bool_t Show = !p->Disabled && p->Visible && (p->ColorKey != RGB_NULL || !p->Clipping || !p->Primary);
	if (!p->Updating && p->Show != Show)
	{
		p->Show = Show;
		p->Dirty = 1;
		p->LastTime = -1;

		if (p->Inited && !Temp)
		{
			if (p->Primary && p->Overlay && p->ColorKey != RGB_NULL && !p->Show)
			{
				// clear colorkey before turning off overlay
				WinInvalidate(&p->Viewport,1);
				WinUpdate(); 
			}

			if (p->UpdateShow)
				p->UpdateShow(p);

			if (p->Primary)
			{
				if (p->Overlay)
				{
					if (p->ColorKey != RGB_NULL && p->Show)
						WinInvalidate(&p->Viewport,1);
				}
				else
				if (p->Show)
				{
					WinInvalidate(&p->Viewport,1); // redraw border (zoom may have changed)
					WinValidate(&p->GUIAlignedRect);
				}
				else
				{
					WinInvalidate(&p->Viewport,0); // redraw other windows
					WinValidate(&p->Viewport); // own window is fine
				}
			}
		}
	}
	return ERR_NONE;
}

static int Updating(overlay* p)
{
	if (!p->Updating)
	{
		UpdateInputDirection(p,0);
		OverlayUpdateFX(p,0);
		OverlayUpdateShow(p,0);
	}
	return ERR_NONE;
}

static int UpdateInput(overlay* p)
{
	if (p->Inited)
	{
		if (p->Show)
		{
			bool_t Old = p->Visible;
			p->Visible = 0;
			p->Show = 0;
			if (p->UpdateShow)
				p->UpdateShow(p); // maybe calls OverlayUpdateShow
			p->Visible = Old;
		}
		p->Done(p);
		BlitRelease(p->Soft);
		p->Soft = NULL;
		p->Inited = 0;
		memset(&p->GUIAlignedRect,0,sizeof(rect));

		if (p->Primary)
			WinInvalidate(&p->Viewport,1);
	}

	if (!p->TurningOff)
		IDCTRestore(NULL,&p->Backup);

	memset(&p->FX,0,sizeof(blitfx));
	memset(&p->DstAlignedRect,0,sizeof(rect));
	memset(&p->SrcAlignedRect,0,sizeof(rect));
	memset(&p->GUIAlignedRect,0,sizeof(rect));

	p->Total = 0;
	p->Dropped = 0;
	p->Disabled = 0;
	p->Updating = 0;
	p->Show = 0;
	p->Dirty = 1;
	p->LastTime = -1;
	p->ColorKey = RGB_NULL;

	if (p->Input.Type == PACKET_VIDEO)
	{
		if (Compressed(&p->Input.Format.Video.Pixel))
			return ERR_INVALID_DATA;

		if (p->Input.Format.Video.Width<=0) 
		{
			p->Disabled = 1;
			p->Input.Format.Video.Width = 2;
		}

		if (p->Input.Format.Video.Height<=0) 
		{
			p->Disabled = 1;
			p->Input.Format.Video.Height = 2;
		}

		if (!p->Background)
		{
			int Result = p->Init(p);
			if (Result != ERR_NONE)
			{
				if (Result == ERR_DEVICE_ERROR)
					ShowError(p->Node.Class,ERR_ID,ERR_DEVICE_ERROR);
				return Result;
			}

			p->Inited = 1;
			p->ForceUpdate = 1;
				
			UpdateInputDirection(p,0);
			OverlayUpdateFX(p,1);
			OverlayUpdateShow(p,0);

			if (p->Primary && !p->TurningOff)
				WinInvalidate(&p->Viewport,1);
		}
	}
	else
	if (p->Input.Type != PACKET_NONE)
		return ERR_INVALID_DATA;

	return ERR_NONE;
}

static int UpdateBackground(overlay* p)
{
	if (p->DoPowerOff)
	{
		p->TurningOff = 1;
		if (p->Background)
		{
			IDCTRestore(NULL,&p->Backup); // free old backup
			if (!p->AccelIDCT || p->AccelIDCT->Get(p->AccelIDCT,IDCT_BACKUP,&p->Backup,sizeof(idctbackup))!=ERR_NONE)
				UpdateInput(p);
		}
		else
		{
			if (p->AccelIDCT && p->Backup.Format.Pixel.Flags)
			{
				p->AccelIDCT->Set(p->AccelIDCT,IDCT_BACKUP,&p->Backup,sizeof(idctbackup));
				p->AccelIDCT->Set(p->AccelIDCT,FLOW_RESEND,NULL,0);
			}
			else
			{
				node* Player = Context()->Player;
				UpdateInput(p);
				if (p->Inited && Player)
					Player->Set(Player,PLAYER_UPDATEVIDEO,NULL,0); 
			}
			IDCTRestore(NULL,&p->Backup); // free backup in any case
		}
		p->TurningOff = 0;
	}
	return ERR_NONE;
}

static bool_t PitchChanged(const packetformat* Current, const packetformat* New)
{
	if (Current && Current->Type == PACKET_VIDEO && New && New->Type == PACKET_VIDEO)
	{
		video Tmp = New->Format.Video;
		Tmp.Pitch = Current->Format.Video.Pitch;
		return EqVideo(&Current->Format.Video,&Tmp);
	}
	return 0;
}

int OverlaySet(overlay* p,int No,const void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case NODE_SETTINGSCHANGED:
		p->ForceUpdate = 1;
		break;

	case OUT_INPUT: SETVALUE(p->Pin,pin,ERR_NONE); break;
	case OUT_TOTAL: SETVALUE(p->Total,int,ERR_NONE); break;
	case OUT_DROPPED: SETVALUE(p->Dropped,int,ERR_NONE); break;
	case OUT_INPUT|PIN_FORMAT:
		if (p->Inited && (
			PitchChanged(&p->Input,(const packetformat*)Data) ||
			PacketFormatRotatedVideo(&p->Input,(const packetformat*)Data,DIR_SWAPXY|DIR_MIRRORLEFTRIGHT|DIR_MIRRORUPDOWN)))
		{
			PacketFormatCopy(&p->Input,(const packetformat*)Data);
			UpdateInputDirection(p,0);
			Result = OverlayUpdateFX(p,1);
		}
		else
			SETPACKETFORMAT(p->Input,packetformat,UpdateInput(p)); 
		break;

	case FLOW_BACKGROUND: SETVALUECMP(p->Background,bool_t,UpdateBackground(p),EqBool); break;
	case VOUT_PLAY: SETVALUE(p->Play,bool_t,ERR_NONE); break;
	case VOUT_UPDATING: SETVALUECMP(p->Updating,bool_t,Updating(p),EqBool); break;
	case VOUT_CLIPPING: SETVALUE(p->Clipping,bool_t,OverlayUpdateShow(p,0)); break;
	case VOUT_VISIBLE: SETVALUE(p->Visible,bool_t,OverlayUpdateShow(p,0)); break;
	case VOUT_AUTOPREROTATE: SETVALUE(p->AutoPrerotate,bool_t,UpdateInputDirection(p,1)); break;
	case VOUT_ASPECT: SETVALUECMP(p->Aspect,fraction,OverlayUpdateFX(p,0),EqFrac); break;
	case VOUT_FX: SETVALUE(p->OrigFX,blitfx,OverlayUpdateFX(p,0)); break;
	case VOUT_FULLSCREEN: SETVALUECMP(p->FullScreenViewport,bool_t,OverlayUpdateFX(p,0),EqBool); break;
	case VOUT_VIEWPORT:
		if (Size == sizeof(rect))
		{
			Result = ERR_NONE;
			if (!EqRect(&p->Viewport,(const rect*)Data))
			{
				p->Viewport = *(const rect*)Data;
				p->ForceUpdate = 1; // when in updating
				OverlayUpdateFX(p,1);
			}
		}
		break;

	case FLOW_FLUSH:
		p->Dirty = 1;
		break;

	case VOUT_RESET:
		if (p->Inited && p->Reset)
		{
			p->Reset(p);
			OverlayUpdateFX(p,1);
		}
		Result = ERR_NONE;
		break;
	}
	return Result;
}

static const nodedef Overlay =
{
	sizeof(overlay)|CF_ABSTRACT,
	OVERLAY_CLASS,
	VOUT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void Overlay_Init()
{
	NodeRegisterClass(&Overlay);
}

void Overlay_Done()
{
	NodeUnRegisterClass(OVERLAY_CLASS);
}
