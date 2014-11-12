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
 * $Id: events.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "../common/palmos/pace.h"
#include "events.h"

#define STACKSIZE	4

typedef struct info
{
	Boolean Override;
	int ReadPos;
	int WritePos;
	EventType Stack[STACKSIZE];

	Boolean PenDown;
	PointType PenPos;
	PointType PenStart;
	UInt16 PenObject;
	UInt32 KeyState;

	UInt32 BatteryTick;
	UInt32 BatteryInterval;

} info;

static info Info;

static Err Push(info* p,const EventType* event)
{
	int i = p->WritePos+1;
	if (i == STACKSIZE)
		i = 0;
	if (i == p->ReadPos)
		return evtErrQueueFull;

	memcpy(&p->Stack[p->WritePos],event,sizeof(EventType));
	p->WritePos = i;
	return errNone;
}

static bool_t CheckEvent(info* p)
{
	EventType e;
	Boolean Down;
	PointType Pos;
	UInt32 KeyState;

	EvtGetPen(&Pos.x, &Pos.y, &Down);

	if (p->PenDown != Down)
	{
		p->PenPos = Pos;
		p->PenDown = Down;

		memset(&e,0,sizeof(e));
		e.penDown = Down;
		e.tapCount = 1;
		e.screenX = Pos.x;
		e.screenY = Pos.y;

		if (Down)
		{
			p->PenStart = Pos;
			e.eType = penDownEvent;
		}
		else
		{
			e.eType = penUpEvent;
			e.data.penUp.start = p->PenStart;
			e.data.penUp.end = Pos;
		}
		Push(p,&e);
		return 1;
	}
	else
	if (Down && (p->PenPos.x != Pos.x || p->PenPos.y != Pos.y))
	{
		p->PenPos = Pos;

		memset(&e,0,sizeof(e));
		e.eType = penMoveEvent;
		e.penDown = true;
		e.tapCount = 1;
		e.screenX = Pos.x;
		e.screenY = Pos.y;
		Push(p,&e);
		return 1;
	}

	KeyState = KeyCurrentState();
	if (KeyState != p->KeyState)
	{
		UInt32 Change = KeyState ^ p->KeyState;
		p->KeyState = KeyState;

		if ((Change & keyBitPower) && !(KeyState & keyBitPower))
		{
			DebugMessage("Battery manager override mode off");
			// turning off event override mode
			EvtOverride(0);
		}
	}
	return 0;
}

Boolean EvtGetOverride()
{
	return Info.Override;
}

void EvtOverride(Boolean v)
{
	EventType e;
	info* p = &Info;
	if (v != p->Override)
	{
		p->Override = v;
		if (v)
		{
			EvtGetPen(&p->PenPos.x, &p->PenPos.y, &p->PenDown);
			p->PenStart = p->PenPos;

			do
			{
				EvtGetEvent(&e,evtNoWait);
				if (e.eType == nilEvent)
					break;
			} while(Push(p,&e)==errNone);

			p->KeyState = KeyCurrentState();
			p->BatteryTick = TimGetTicks();
			p->BatteryInterval = SysTicksPerSecond();
		}
		else
		{
			// flush event queue
			EvtResetAutoOffTimer();
			do
			{
				EvtGetEvent(&e,evtNoWait);
			} while (e.eType != nilEvent);
		}
	}
}

void _EvtAddUniqueEventToQueue(const EventType *eventP, UInt32 id,Boolean inPlace)
{
	int i;
	info* p = &Info;
	if (!p->Override)
	{
		EvtAddUniqueEventToQueue(eventP,id,inPlace);
		return;
	}

	i = p->ReadPos;
	while (i!=p->WritePos)
	{
		if (p->Stack[i].eType == eventP->eType)
		{
			memcpy(&p->Stack[i],eventP,sizeof(EventType));
			return;
		}
		if (++i == STACKSIZE)
			i = 0;
	}
	
	Push(&Info,eventP);
}

void _EvtGetEvent(EventType *event, Int32 timeout)
{
	info* p = &Info;
	if (!p->Override)
	{
		EvtGetEvent(event,timeout);
		return;
	}

	for (;;)
	{
		while (CheckEvent(p));

		if (p->ReadPos != p->WritePos)
		{
			memcpy(event,&p->Stack[p->ReadPos],sizeof(EventType));
			if (++p->ReadPos == STACKSIZE)
				p->ReadPos = 0;
			break;
		}

		if (timeout==0)
		{
			memset(event,0,sizeof(EventType));
			event->eType = nilEvent;
			break;
		}

		SysTaskDelay(1);
		if (timeout>0)
			--timeout;
	}
}

Err _EvtEnqueueKey(WChar ascii, UInt16 keycode, UInt16 modifiers)
{
	EventType event;
	if (!Info.Override)
		return EvtEnqueueKey(ascii, keycode, modifiers);

	memset(&event,0,sizeof(event));
	event.eType = keyDownEvent;
	event.data.keyDown.chr = keycode;
	event.data.keyDown.modifiers = modifiers;
	return Push(&Info,&event);
}

Boolean _EvtEventAvail()
{
	if (!Info.Override)
		return (Boolean)(EvtEventAvail() || EvtSysEventAvail(1));
	return 1;
}

void _FrmPopupForm(UInt16 formId)
{
	if (!Info.Override)
		FrmPopupForm(formId);
}

static UInt16 FindObject(FormType* Form,Coord x,Coord y)
{
	UInt16 n,Count = FrmGetNumberOfObjects(Form);
	RectangleType Rect;
	for (n=0;n<Count;++n)
	{
		FrmGetObjectBounds(Form,n,&Rect);
		if (Rect.topLeft.x <= x &&
			Rect.topLeft.y <= y &&
			Rect.topLeft.x + Rect.extent.x > x &&
			Rect.topLeft.y + Rect.extent.y > y)
			return FrmGetObjectId(Form,n);
	}
	return 0;
}

Boolean OSHandleEvent(EventPtr eventP)
{
	if (!Info.Override)
	{
		Err err;
		if (SysHandleEvent(eventP))
			return 1;
		if (MenuHandleEvent(0,eventP,&err))
			return 1;
		return FrmDispatchEvent(eventP);
	}
	else
	{
		info* p = &Info;
		FormType* Form = FrmGetActiveForm();
		FormEventHandlerType* Handler = FrmGetEventHandler(Form);
		if (Handler && Handler(eventP))
			return 1;

		if (eventP->eType == penDownEvent)
			p->PenObject = FindObject(Form,eventP->screenX,eventP->screenY);

		if (eventP->eType == penUpEvent && p->PenObject && FindObject(Form,eventP->screenX,eventP->screenY)==p->PenObject)
		{
			FormObjectKind Kind = FrmGetObjectType(Form,FrmGetObjectIndex(Form,p->PenObject));
			if (Kind == frmControlObj)
			{
				EventType e;
				memset(&e,0,sizeof(e));
				e.eType = ctlSelectEvent;
				e.data.ctlSelect.controlID = p->PenObject;
				Push(p,&e);
			}
		}
		return 0;
	}
}
