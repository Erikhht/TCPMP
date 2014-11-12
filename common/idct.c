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
 * $Id: idct.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable Params[] = 
{
	{ IDCT_FORMAT,		TYPE_BINARY, DF_HIDDEN, sizeof(video) },
	{ IDCT_OUTPUT,		TYPE_PACKET, DF_OUTPUT },
	{ IDCT_ROUNDING,	TYPE_BOOL, DF_HIDDEN },
	{ IDCT_SHIFT,		TYPE_INT, DF_HIDDEN },
	{ IDCT_MODE,		TYPE_INT, DF_HIDDEN },
	{ IDCT_BUFFERCOUNT,	TYPE_INT, DF_HIDDEN },
	{ IDCT_BUFFERWIDTH,	TYPE_INT, DF_HIDDEN },
	{ IDCT_BUFFERHEIGHT,TYPE_INT, DF_HIDDEN },

	DATATABLE_END(IDCT_CLASS)
};

static const datatable CodecParams[] = 
{
	{ CODECIDCT_INPUT,	TYPE_PACKET, DF_INPUT },
	{ CODECIDCT_IDCT,	TYPE_NODE, DF_OUTPUT, IDCT_CLASS },

	DATATABLE_END(CODECIDCT_CLASS)
};

int IDCTEnum(void* p, int* No, datadef* Param)
{
	if (FlowEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,Params);
}

static void DummyGMC(idct* p,const idct_gmc* gmc)
{
}

static void DummyInter8x8GMC(idct* p,void* Data,int Length)
{
	p->Inter8x8(p,Data,Length);
}

static void DummyMCompGMC(idct* p,int x,int y)
{
	static const int Zero[6] = {0,0,0,0,0,0};
	p->MComp16x16(p,Zero,NULL);
}

static int IDCTCreate(idct* p)
{
	p->Enum = IDCTEnum;
	p->GMC = (idctgmc)DummyGMC;
	p->Inter8x8GMC = (idctinter)DummyInter8x8GMC;
	p->MCompGMC = (idctprocess)DummyMCompGMC;
	return ERR_NONE;
}

static const nodedef IDCT =
{
	sizeof(idct)|CF_ABSTRACT,
	IDCT_CLASS,
	FLOW_CLASS,
	PRI_DEFAULT,
	(nodecreate)IDCTCreate,
};

int IDCTBackup(idct* p,idctbackup* Backup)
{
	int No;
	planes Planes;
	video Format;

	memset(Backup,0,sizeof(idctbackup));

	if (p->Get(p,IDCT_FORMAT,&Backup->Format,sizeof(video))!=ERR_NONE || !Backup->Format.Pixel.Flags ||
		p->Get(p,IDCT_BUFFERWIDTH,&Backup->Width,sizeof(int))!=ERR_NONE ||
		p->Get(p,IDCT_BUFFERHEIGHT,&Backup->Height,sizeof(int))!=ERR_NONE ||
		p->Get(p,IDCT_BUFFERCOUNT,&Backup->Count,sizeof(int))!=ERR_NONE ||
		p->Get(p,IDCT_SHOW,&Backup->Show,sizeof(int))!=ERR_NONE)
		return ERR_INVALID_DATA;

	p->Get(p,IDCT_SHIFT,&Backup->Shift,sizeof(int)); // optional
	p->Get(p,IDCT_MODE,&Backup->Mode,sizeof(int));

	for (No=0;No<Backup->Count;++No)
	{
		idctbufferbackup* Buffer = Backup->Buffer+No;

		Buffer->FrameNo = -1;
		p->Get(p,IDCT_FRAMENO+No,&Buffer->FrameNo,sizeof(int));

		if (p->Lock(p,No,Planes,&Buffer->Brightness,&Format) == ERR_NONE)
		{
			Buffer->Format = Format;
			DefaultPitch(&Buffer->Format);
			if (SurfaceAlloc(Buffer->Buffer,&Buffer->Format) == ERR_NONE)
				SurfaceCopy(&Format,&Buffer->Format,Planes,Buffer->Buffer,NULL);
			p->Unlock(p,No);
		}
	}

	p->Set(p,IDCT_FORMAT,NULL,0);
	return ERR_NONE;
}

int IDCTRestore(idct* p,idctbackup* Backup)
{
	int No;
	if (p && Backup->Format.Pixel.Flags)
	{
		int Brightness;
		planes Planes;
		video Format;
		blitfx FX;

		memset(&FX,0,sizeof(FX));
		FX.ScaleX = SCALE_ONE;
		FX.ScaleY = SCALE_ONE;

		p->Set(p,IDCT_MODE,&Backup->Mode,sizeof(int));
		p->Set(p,IDCT_SHIFT,&Backup->Shift,sizeof(int));
		p->Set(p,IDCT_BUFFERWIDTH,&Backup->Width,sizeof(int));
		p->Set(p,IDCT_BUFFERHEIGHT,&Backup->Height,sizeof(int));
		p->Set(p,IDCT_FORMAT,&Backup->Format,sizeof(video));
		p->Set(p,IDCT_BUFFERCOUNT,&Backup->Count,sizeof(int));

		for (No=0;No<Backup->Count;++No)
		{
			idctbufferbackup* Buffer = Backup->Buffer+No;

			p->Set(p,IDCT_FRAMENO+No,&Buffer->FrameNo,sizeof(int));

			if (Buffer->Buffer[0] && p->Lock(p,No,Planes,&Brightness,&Format) == ERR_NONE)
			{
				FX.Direction = CombineDir(Buffer->Format.Direction, 0, Format.Direction);
				FX.Brightness = Brightness - Buffer->Brightness;
					
				SurfaceCopy(&Buffer->Format,&Format,Buffer->Buffer,Planes,&FX);

				p->Unlock(p,No);
			}
		}

		p->Set(p,IDCT_SHOW,&Backup->Show,sizeof(int));
	}

	for (No=0;No<Backup->Count;++No)
		SurfaceFree(Backup->Buffer[No].Buffer);

	memset(Backup,0,sizeof(idctbackup));
	return ERR_NONE;
}

//-----------------------------------------------------------------------------------------------

int CodecIDCTEnum(void* p, int* No, datadef* Param)
{
	if (FlowEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,CodecParams);
}

static int Discontinuity(codecidct* p)
{
	p->RefTime = TIME_UNKNOWN;
	p->Dropping = 1;
	p->Show = -1;
	if (p->IDCT.Ptr)
		p->IDCT.Ptr->Drop(p->IDCT.Ptr);
	if (p->Discontinuity)
		p->Discontinuity(p);
	return ERR_NONE;
}

static int Flush(codecidct* p)
{
	Discontinuity(p);
	p->RefUpdated = 0;
	p->FrameEnd = 0;
	p->Dropping = 0;
	BufferClear(&p->Buffer);
	if (p->Flush)
		p->Flush(p);
	return ERR_NONE;
}

static int UpdateCount(codecidct* p)
{
	if (p->IDCT.Ptr)
	{
		p->IDCT.Ptr->Get(p->IDCT.Ptr,IDCT_BUFFERCOUNT,&p->IDCT.Count,sizeof(int));
	
		if (p->UpdateCount)
			p->UpdateCount(p);

		if (p->IDCT.Width>0 && p->IDCT.Height>0 && p->IDCT.Count<p->MinCount)
		{
			p->IDCT.Ptr->Set(p->IDCT.Ptr,IDCT_FORMAT,NULL,0);
			return ERR_OUT_OF_MEMORY;
		}
	}
	return ERR_NONE;
}

static bool_t Prepair(codecidct* p)
{
	int Result;
	idct* IDCT = p->IDCT.Ptr;
	p->IDCT.Count = 0;

	if (!IDCT || !p->IDCT.Format.Pixel.Flags)
		return ERR_NONE;

	Result = IDCT->Set(IDCT,IDCT_BUFFERWIDTH,&p->IDCT.Width,sizeof(int));
	if (Result != ERR_NONE) return Result;
	Result = IDCT->Set(IDCT,IDCT_BUFFERHEIGHT,&p->IDCT.Height,sizeof(int));
	if (Result != ERR_NONE) return Result;
	Result = IDCT->Set(IDCT,IDCT_FORMAT,&p->IDCT.Format,sizeof(video));
	if (Result != ERR_NONE) return Result;

	IDCT->Set(IDCT,IDCT_BUFFERCOUNT,&p->MinCount,sizeof(int));
	if (p->DefCount > p->MinCount) // optional
	{
		DisableOutOfMemory();
		IDCT->Set(IDCT,IDCT_BUFFERCOUNT,&p->DefCount,sizeof(int));
		EnableOutOfMemory();
	}
	IDCT->Drop(IDCT);

	return UpdateCount(p);
}

static int SetIDCT(codecidct* p, idct* Dst)
{
	int Count;
	blitfx FX;
	planes SrcPlanes,DstPlanes;
	int SrcBrightness,DstBrightness;
	int SrcShift,DstShift;
	int No;
	int Result;
	idct* Src = p->IDCT.Ptr;

	if (Src == Dst)
		return ERR_NONE;

	if (!Src)
	{
		p->IDCT.Ptr = Dst;
		Result= Prepair(p);
		if (Result != ERR_NONE)
			p->IDCT.Ptr = NULL;
		return Result;
	}

	if (Dst)
	{
		assert(NodeIsClass(Dst->Class,IDCT_CLASS));

		if (Src->Get(Src,IDCT_SHIFT,&SrcShift,sizeof(SrcShift))!=ERR_NONE)
			SrcShift = 0;
		Dst->Set(Dst,IDCT_SHIFT,&SrcShift,sizeof(SrcShift));
		if (Dst->Get(Dst,IDCT_SHIFT,&DstShift,sizeof(DstShift))!=ERR_NONE)
			DstShift = 0;

		Result = Dst->Set(Dst,IDCT_MODE,&p->IDCT.Mode,sizeof(int));
		if (Result != ERR_NONE)	return Result;
		Result = Dst->Set(Dst,IDCT_BUFFERWIDTH,&p->IDCT.Width,sizeof(int));
		if (Result != ERR_NONE)	return Result;
		Result = Dst->Set(Dst,IDCT_BUFFERHEIGHT,&p->IDCT.Height,sizeof(int));
		if (Result != ERR_NONE)	return Result;
		Result = Dst->Set(Dst,IDCT_FORMAT,&p->IDCT.Format,sizeof(video));
		if (Result != ERR_NONE)	return Result;
		Result = Dst->Set(Dst,IDCT_BUFFERCOUNT,&p->MinCount,sizeof(int));
		if (Result != ERR_NONE)	return Result;

		if (Src->Get(Src,IDCT_BUFFERCOUNT,&Count,sizeof(Count))==ERR_NONE && Count>p->MinCount)
			Dst->Set(Dst,IDCT_BUFFERCOUNT,&Count,sizeof(Count)); // optional

		memset(&FX,0,sizeof(FX));
		FX.ScaleX = SCALE_ONE;
		FX.ScaleY = SCALE_ONE;
		
		No = SrcShift - DstShift;
		if (No>0)
		{
			FX.ScaleX <<= No;
			FX.ScaleY <<= No;
		}
		else if (No<0)
		{
			FX.ScaleX >>= No;
			FX.ScaleY >>= No;
		}

		for (No=0;No<Count;++No)
		{
			video SrcFormat,DstFormat;
			int FrameNo = -1;
			Src->Get(Src,IDCT_FRAMENO+No,&FrameNo,sizeof(FrameNo));
			Dst->Set(Dst,IDCT_FRAMENO+No,&FrameNo,sizeof(FrameNo));

			if (Src->Lock(Src,No,SrcPlanes,&SrcBrightness,&SrcFormat) == ERR_NONE)
			{
				if (Dst->Lock(Dst,No,DstPlanes,&DstBrightness,&DstFormat) == ERR_NONE)
				{
					FX.Direction = CombineDir(SrcFormat.Direction, 0, DstFormat.Direction);
					FX.Brightness = DstBrightness - SrcBrightness;
					
					SurfaceCopy(&SrcFormat,&DstFormat,SrcPlanes,DstPlanes,&FX);

					Dst->Unlock(Dst,No);
				}
				Src->Unlock(Src,No);
			}
		}

		if (Src->Get(Src,IDCT_SHOW,&No,sizeof(No))==ERR_NONE)
			Dst->Set(Dst,IDCT_SHOW,&No,sizeof(No));
	}

	Src->Set(Src,IDCT_FORMAT,NULL,0);
	p->IDCT.Ptr = Dst;
	UpdateCount(p); // can't and shouldn't fail here (src already cleared and dst pointer saved)
	return ERR_NONE;
}

int CodecIDCTSetCount(codecidct* p, int Count)
{
	if (!p->IDCT.Ptr)
		return ERR_NONE;
	p->IDCT.Ptr->Set(p->IDCT.Ptr,IDCT_BUFFERCOUNT,&Count,sizeof(int));
	return UpdateCount(p);
}

int CodecIDCTSetMode(codecidct* p, int Mode, bool_t Value)
{
	if (Value)
		p->IDCT.Mode |= Mode;
	else
		p->IDCT.Mode &= ~Mode;

	if (p->IDCT.Ptr && p->IDCT.Ptr->Set(p->IDCT.Ptr,IDCT_MODE,&p->IDCT.Mode,sizeof(int))!=ERR_NONE)
	{
		if (p->NotSupported.Node)
		{
			pin Pin;
			Pin.No = CODECIDCT_IDCT;
			Pin.Node = &p->Node;
			if (p->NotSupported.Node->Set(p->NotSupported.Node,p->NotSupported.No,&Pin,sizeof(Pin)) == ERR_NONE)
				return ERR_NOT_COMPATIBLE;
		}
		return ERR_NOT_SUPPORTED;
	}
	return ERR_NONE;
}

int CodecIDCTSetFormat(codecidct* p, int Flags, int Width, int Height, int IDCTWidth, int IDCTHeight, int Aspect)
{
	int Result = ERR_NONE;

	Flags |= p->In.Format.Format.Video.Pixel.Flags & PF_NOPREROTATE;

	if (p->IDCT.Format.Pixel.Flags != Flags ||
		p->IDCT.Format.Width != Width || 
		p->IDCT.Format.Height != Height ||
		p->IDCT.Format.Aspect != Aspect ||
		p->IDCT.Width != IDCTWidth ||
		p->IDCT.Height != IDCTHeight)
	{
		p->IDCT.Format.Pixel.Flags = Flags;
		p->IDCT.Format.Width = Width;
		p->IDCT.Format.Height = Height;
		p->IDCT.Format.Aspect = Aspect;
		p->IDCT.Width = IDCTWidth;
		p->IDCT.Height = IDCTHeight;

		p->In.Format.Format.Video.Width = Width;
		p->In.Format.Format.Video.Height = Height;

		if (p->UpdateSize)
			Result = p->UpdateSize(p);

		if (Result == ERR_NONE)
			Result = Prepair(p);
	}

	return Result;
}

static int UpdateInput(codecidct* p)
{
	int Result = ERR_NONE;
	
	Flush(p);

	if (!PacketFormatMatch(p->Node.Class, &p->In.Format))
		PacketFormatClear(&p->In.Format);

	memset(&p->IDCT.Format,0,sizeof(video));
	if (p->UpdateInput)
		Result = p->UpdateInput(p);

	if (Result != ERR_NONE)
		PacketFormatClear(&p->In.Format);

	if (p->In.Format.Type != PACKET_VIDEO && p->IDCT.Ptr)
		p->IDCT.Ptr->Set(p->IDCT.Ptr,IDCT_FORMAT,NULL,0);

	return Result;
}

static int Process(codecidct* p, const packet* Packet, const flowstate* State)
{
	int Result;
	idct* IDCT = p->IDCT.Ptr;

	if (p->IDCT.Count<=0 && p->IDCT.Width>0 && p->IDCT.Height>0)
		return ERR_INVALID_DATA;

	p->State.CurrTime = State->CurrTime;
	if (State->DropLevel > 1)
		Discontinuity(p);

	if (p->Show>=0) // pending frame?
	{
		Result = IDCT->Send(IDCT,p->RefTime,&p->State);
		if (Result == ERR_BUFFER_FULL)
			return Result;
		p->Show = -1;
	}

	if (!Packet) // end of file or dropped
		return IDCT->Null(IDCT,State,0);

	if ((p->In.Format.Format.Video.Pixel.Flags & PF_FRAGMENTED) && p->FindNext)
	{
		bool_t Processed = 0;

		//DEBUG_MSG2(DEBUG_CODEC,T("%d %d"),Packet->Length,Packet->RefTime);

		for (;;)
		{
			if (!p->FindNext(p))
			{
				if (Processed)
				{
					Result = ERR_NEED_MORE_DATA;
					break;
				}

				p->FrameEnd -= p->Buffer.ReadPos;
				BufferPack(&p->Buffer,0);
				BufferWrite(&p->Buffer,Packet->Data[0],Packet->Length,32768);
				Processed = 1;

				if (Packet->RefTime >= 0)
				{
					if (!p->RefUpdated)
					{
						p->RefTime = Packet->RefTime;
						p->RefUpdated = 1;
//						if (p->IDCT.Count >= 3 && p->FrameTime>0 && p->RefTime >= p->FrameTime)
//							p->RefTime -= p->FrameTime;
					}
				}
				else
					p->RefUpdated = 0;
			}
			else
			{
				//static tick_t Last = 0;
				//DEBUG_MSG2(DEBUG_CODEC,T("%d %d"),p->RefTime,p->RefTime-Last);
				//Last = p->RefTime;

				if (!p->FrameTime && !p->RefUpdated)
					p->RefTime = State->CurrTime;

				p->State.DropLevel = p->RefTime >= 0 && State->CurrTime >= 0 &&  
				                     p->RefTime < (State->CurrTime - p->DropTolerance);

				if (State->DropLevel > 1)
				{
					p->IDCT.Ptr->Null(p->IDCT.Ptr,NULL,0);
					Result = ERR_NONE;
				}
				else
					Result = p->Frame(p,p->Buffer.Data+p->Buffer.ReadPos,p->FrameEnd-p->Buffer.ReadPos); 

				p->Buffer.ReadPos = p->FrameEnd;

				if (p->RefTime >= 0)
					p->RefTime += p->FrameTime; 

				p->RefUpdated = 0;

				if (Result==ERR_NONE && p->Show>=0)
				{
					if (!Processed)
						Result = ERR_BUFFER_FULL; // resend packet next time
					break;
				}
			}
		}
	}
	else
	{
		if (State->DropLevel > 1)
		{
			p->IDCT.Ptr->Null(p->IDCT.Ptr,NULL,0);
			Result = ERR_NONE;
		}
		else
		{
			p->State.DropLevel = State->DropLevel;
			p->RefTime = Packet->RefTime;
//			if (p->IDCT.Count >= 3 && p->FrameTime>0 && p->RefTime >= p->FrameTime)
//				p->RefTime -= p->FrameTime;
			Result = p->Frame(p,Packet->Data[0],Packet->Length);
		}
	}

	if (p->Show>=0 && IDCT->Send(IDCT,p->RefTime,&p->State) != ERR_BUFFER_FULL)
		p->Show = -1;

	return Result;
}

int CodecIDCTSet(codecidct* p, int No, const void* Data, int Size)
{
	node* Advanced;
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case CODECIDCT_INPUT: SETVALUE(p->In.Pin,pin,ERR_NONE); break;
	case CODECIDCT_INPUT|PIN_FORMAT: SETPACKETFORMAT(p->In.Format,packetformat,UpdateInput(p)); break;
	case CODECIDCT_IDCT:
		assert(Size==sizeof(idct*));
		Result = SetIDCT(p,*(idct**)Data);
		break;

	case FLOW_NOT_SUPPORTED: SETVALUE(p->NotSupported,pin,ERR_NONE); break;
	case FLOW_FLUSH: Result = Flush(p); break;
	case NODE_SETTINGSCHANGED:
		Advanced = Context()->Advanced;
		if (Advanced)
			Advanced->Get(Advanced,ADVANCED_DROPTOL,&p->DropTolerance,sizeof(tick_t));
		break;
	}
	return Result;
}

int CodecIDCTGet(codecidct* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case CODECIDCT_INPUT: GETVALUE(p->In.Pin,pin); break;
	case CODECIDCT_INPUT|PIN_FORMAT: GETVALUE(p->In.Format,packetformat); break;
	case CODECIDCT_INPUT|PIN_PROCESS: GETVALUE((packetprocess)Process,packetprocess); break;
	case CODECIDCT_IDCT: GETVALUE(p->IDCT.Ptr,idct*); break;
	}
	return Result;
}

static int Create(codecidct* p)
{
	p->Node.Enum = CodecIDCTEnum;
	p->Node.Get = (nodeget)CodecIDCTGet;
	p->Node.Set = (nodeset)CodecIDCTSet;
	return ERR_NONE;
}

static void Delete(codecidct* p)
{
	PacketFormatClear(&p->In.Format);
	if (p->UpdateInput)
		p->UpdateInput(p);
}

static const nodedef CodecIDCT =
{
	sizeof(codecidct)|CF_ABSTRACT,
	CODECIDCT_CLASS,
	FLOW_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void IDCT_Init()
{
	NodeRegisterClass(&IDCT);
	NodeRegisterClass(&CodecIDCT);
}

void IDCT_Done()
{
	NodeUnRegisterClass(CODECIDCT_CLASS);
	NodeUnRegisterClass(IDCT_CLASS);
}

