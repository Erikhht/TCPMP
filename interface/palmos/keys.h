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
 * $Id: keys.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __KEYS_H
#define __KEYS_H

// 5-way d-pad keys
int KeySelect(EventPtr Event);
int KeyUp(EventPtr Event);
int KeyDown(EventPtr Event);
int KeyLeft(EventPtr Event);
int KeyRight(EventPtr Event);

bool_t KeyHold(EventPtr Event);

// media keys
int KeyMediaPlay(EventPtr Event);
int KeyMediaStop(EventPtr Event);
int KeyMediaVolumeUp(EventPtr Event);
int KeyMediaVolumeDown(EventPtr Event);
int KeyMediaNext(EventPtr Event);
int KeyMediaPrev(EventPtr Event);

#endif
