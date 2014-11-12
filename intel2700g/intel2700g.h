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
 * $Id: intel2700g.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __INTEL2700G_H
#define __INTEL2700G_H

#define INTEL2700G_ID		FOURCC('2','7','0','G')
#define INTEL2700G_IDCT_ID	VOUT_IDCT_CLASS(INTEL2700G_ID)

#define INTEL2700G_VSYNC		0x200
#define INTEL2700G_IDCTROTATE	0x201
#define INTEL2700G_CLOSE_WMP	0x202

#define INTEL2700G_LOCKED		0x400

void Intel2700G_Init();
void Intel2700G_Done();

#endif
