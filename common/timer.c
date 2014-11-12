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
 * $Id: timer.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable Params[] = 
{
	{ TIMER_TIME,	TYPE_INT, DF_HIDDEN },
	{ TIMER_SPEED,	TYPE_FRACTION, DF_HIDDEN },
	{ TIMER_PLAY,	TYPE_BOOL, DF_HIDDEN },

	DATATABLE_END(TIMER_CLASS)
};

int TimerEnum(void* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static const nodedef Timer =
{
	CF_ABSTRACT,
	TIMER_CLASS,
	NODE_CLASS,
	PRI_DEFAULT,
};

//--------------------------------------------------------------------------------------

typedef struct systimer
{
	node Node;
	void* Section;
	bool_t Play;
	fraction Speed;
	fraction SpeedTime;
	tick_t TickStart;
	int TimeRef;

} systimer;

static int Get(systimer* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_PLAY: GETVALUE(p->Play,bool_t); break;
	case TIMER_SPEED: GETVALUE(p->Speed,fraction); break;
	case TIMER_TIME:
		assert(Size == sizeof(tick_t));
		LockEnter(p->Section);
		if (p->Speed.Num==0) // benchmark mode
			*(tick_t*)Data = TIME_BENCH;
		else
		if (p->Play)
			*(tick_t*)Data = p->TickStart + Scale(GetTimeTick()-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
		else
			*(tick_t*)Data = p->TickStart;
		LockLeave(p->Section);
		Result = ERR_NONE;
		break;
	}
	return Result;
}

static int Set(systimer* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case TIMER_TIME:
		assert(Size == sizeof(tick_t));
		LockEnter(p->Section);
		p->TickStart = *(tick_t*)Data;
		p->TimeRef = GetTimeTick();
		LockLeave(p->Section);
		Result = ERR_NONE;
		break;

	case TIMER_SPEED:
		assert(Size == sizeof(fraction));
		if (!EqFrac(&p->Speed, (const fraction*)Data))
		{
			LockEnter(p->Section);
			if (p->Play)
			{
				int t = GetTimeTick();
				p->TickStart += Scale(t-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
				p->TimeRef = t;
			}
			p->Speed = *(fraction*)Data;
			p->SpeedTime = p->Speed;
			p->SpeedTime.Num *= TICKSPERSEC;
			p->SpeedTime.Den *= GetTimeFreq();
			LockLeave(p->Section);
		}
		Result = ERR_NONE;
		break;
			
	case TIMER_PLAY:
		assert(Size == sizeof(bool_t));
		if (p->Play != *(bool_t*)Data)
		{
			int t;
			LockEnter(p->Section);
			t = GetTimeTick();
			p->Play = *(bool_t*)Data;
			if (!p->Play) // save time
				p->TickStart += Scale(t-p->TimeRef,p->SpeedTime.Num,p->SpeedTime.Den);
			p->TimeRef = t;
			LockLeave(p->Section);
		}
		Result = ERR_NONE;
		break;
	}
	return Result;
}

static int Create(systimer* p)
{
	p->Node.Enum = (nodeenum)TimerEnum;
	p->Node.Get = (nodeget)Get;
	p->Node.Set = (nodeset)Set;
	p->Speed.Den = p->Speed.Num = 1;
	p->SpeedTime.Num = TICKSPERSEC;
	p->SpeedTime.Den = GetTimeFreq();
	p->Section = LockCreate();
	return ERR_NONE;
}

static void Delete(systimer* p)
{
	LockDelete(p->Section);
}

static const nodedef SysTimer =
{
	sizeof(systimer)|CF_GLOBAL,
	SYSTIMER_ID,
	TIMER_CLASS,
	PRI_MAXIMUM,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void Timer_Init()
{
	NodeRegisterClass(&Timer);
	NodeRegisterClass(&SysTimer);
}

void Timer_Done()
{
	NodeUnRegisterClass(SYSTIMER_ID);
	NodeUnRegisterClass(TIMER_CLASS);
}

