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
 * $Id: vorbis.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#ifndef __VORBIS_H
#define __VORBIS_H

#define OGGEMBEDDED_HQ_ID	FOURCC('O','G','H','E')
#define OGGEMBEDDED_LQ_ID	FOURCC('O','G','G','E')

void OGGEmbedded_Init();
void OGGEmbedded_Done();

#define OGGPACKET_HQ_ID		FOURCC('O','G','H','P')
#define OGGPACKET_LQ_ID		FOURCC('O','G','G','P')

void OGGPacket_Init();
void OGGPacket_Done();

#define OGG_HQ_ID			FOURCC('O','G','H','V')
#define OGG_LQ_ID			FOURCC('O','G','G','V')

void OGG_Init();
void OGG_Done();

#define VORBIS_HQ_A_ID		FOURCC('V','O','H','B')
#define VORBIS_HQ_V_ID		FOURCC('V','O','H','V')
#define VORBIS_LQ_A_ID		FOURCC('V','O','R','B')
#define VORBIS_LQ_V_ID		FOURCC('V','O','R','V')

void Vorbis_Init();
void Vorbis_Done();

#ifdef _LOW_ACCURACY_
#define OGGPACKET_ID OGGPACKET_LQ_ID
#define OGGEMBEDDED_ID OGGEMBEDDED_LQ_ID
#define OGG_ID OGG_LQ_ID
#define VORBIS_A_ID VORBIS_LQ_A_ID
#define VORBIS_A_PRI PRI_DEFAULT
#define VORBIS_V_ID VORBIS_LQ_V_ID
#define VORBIS_V_PRI PRI_DEFAULT+5
#else
#define OGGPACKET_ID OGGPACKET_HQ_ID
#define OGGEMBEDDED_ID OGGEMBEDDED_HQ_ID
#define OGG_ID OGG_HQ_ID
#define VORBIS_A_ID VORBIS_HQ_A_ID
#define VORBIS_A_PRI PRI_DEFAULT+5
#define VORBIS_V_ID VORBIS_HQ_V_ID
#define VORBIS_V_PRI PRI_DEFAULT
#endif

#endif
