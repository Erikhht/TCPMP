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
 * $Id: adpcm.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __ADPCM_H
#define __ADPCM_H

#define ADPCM_CLASS		FOURCC('A','D','P','C')
#define ADPCM_MS_ID		FOURCC('A','D','M','S')
#define ADPCM_IMA_ID	FOURCC('A','D','I','M')
#define ADPCM_IMA_QT_ID	FOURCC('A','D','I','Q')
#define ADPCM_G726_ID	FOURCC('G','7','2','6')

extern void ADPCM_Init();
extern void ADPCM_Done();

#endif
