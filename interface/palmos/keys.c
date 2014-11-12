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
 * $Id: keys.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"
#if defined(TARGET_PALMOS)

#include "../../common/palmos/pace.h"

#ifdef HAVE_PALMONE_SDK
#include <Common/System/palmOneChars.h>
#include <Common/System/palmOneNavigator.h>
#endif

#ifdef HAVE_SONY_SDK
#include <SonyChars.h>
#include <Libraries/SonyRmcLib.h>
#endif

bool_t KeyHold(EventPtr Event)
{
	return (Event->eType==keyDownEvent && (Event->data.keyDown.modifiers & autoRepeatKeyMask)) || 
		Event->eType==keyHoldEvent;
}

static int KeyDir(EventPtr Event)
{
	if (Event->eType==keyDownEvent)
		return (Event->data.keyDown.modifiers & (willSendUpKeyMask|autoRepeatKeyMask))?1:2; 
	if (Event->eType==keyHoldEvent)
		return 1;
	return -1;
}

#ifdef HAVE_PALMONE_SDK
static int NOINLINE PalmNav(EventPtr Event,int Change,int Bit)
{
	if (Event->data.keyDown.chr != vchrNavChange) 
		return 0;
	Bit &= Event->data.keyDown.keyCode;
	if ((Event->data.keyDown.modifiers & autoRepeatKeyMask) && Bit)
		return 1;
	if (Event->data.keyDown.keyCode & Change)
		return Bit ? 1:-1;
	return 0;
}
#endif

int KeySelect(EventPtr Event)
{
#ifdef HAVE_SONY_SDK
	if (Event->eType==keyDownEvent)
	{
		if (Event->data.keyDown.chr == vchrJogPush ||
			Event->data.keyDown.chr == vchrJogPushRepeat) return 1;
		if (Event->data.keyDown.chr == vchrJogRelease)	return -1;
	}
#endif
	if ((Event->eType==keyHoldEvent || Event->eType==keyDownEvent) && 
		Event->data.keyDown.chr == vchrHardRockerCenter)
		return 1; // avoid keyUpEvent because vchrRocketCenter still generated

	if (Event->data.keyDown.chr == vchrRockerCenter)
		return KeyDir(Event);

#ifdef HAVE_PALMONE_SDK
	return PalmNav(Event,navChangeSelect,navBitSelect);
#endif
	return 0;
}

int KeyLeft(EventPtr Event)
{
#ifdef HAVE_SONY_SDK
	if (Event->data.keyDown.chr == vchrJogLeft)
		return KeyDir(Event);
#endif
	if (Event->data.keyDown.chr == vchrRockerLeft)
		return KeyDir(Event);
#ifdef HAVE_PALMONE_SDK
	return PalmNav(Event,navChangeLeft,navBitLeft);
#else
	return 0;
#endif
}

int KeyRight(EventPtr Event)
{
#ifdef HAVE_SONY_SDK
	if (Event->data.keyDown.chr == vchrJogRight)
		return KeyDir(Event);
#endif
	if (Event->data.keyDown.chr == vchrRockerRight)
		return KeyDir(Event);
#ifdef HAVE_PALMONE_SDK
	return PalmNav(Event,navChangeRight,navBitRight);
#else
	return 0;
#endif
}

int KeyUp(EventPtr Event)
{
	if (Event->data.keyDown.chr == vchrRockerUp ||
		Event->data.keyDown.chr == vchrPageUp)
		return KeyDir(Event);
#ifdef HAVE_PALMONE_SDK
	return PalmNav(Event,navChangeUp,navBitUp);
#else
	return 0;
#endif
}

int KeyDown(EventPtr Event)
{
	if (Event->data.keyDown.chr == vchrRockerDown ||
	    Event->data.keyDown.chr == vchrPageDown)
		return KeyDir(Event);
#ifdef HAVE_PALMONE_SDK
	return PalmNav(Event,navChangeDown,navBitDown);
#else
	return 0;
#endif
}

#ifdef HAVE_SONY_SDK
static int NOINLINE RmcKey(EventPtr Event,int Code)
{
	if (GetRmcKey(Event->data.keyDown.keyCode) == Code)
	{
		if (Event->data.keyDown.chr == vchrRmcKeyPush)
			return 1;
		if (Event->data.keyDown.chr == vchrRmcKeyRelease)
			return -1;
	}
	return 0;
}
#endif

int KeyMediaPlay(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyPlay);
#endif
	return i;
}

int KeyMediaStop(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyStop);
#endif
	return i;
}

int KeyMediaVolumeUp(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyUp);
#endif
	return i;
}

int KeyMediaVolumeDown(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyDown);
#endif
	return i;
}

int KeyMediaNext(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyFfPlay);
#endif
	return i;
}

int KeyMediaPrev(EventPtr Event)
{
	int i = 0;
#ifdef HAVE_SONY_SDK
	i = RmcKey(Event,rmcKeyFrPlay);
#endif
	return i;
}

#endif
