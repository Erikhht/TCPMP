/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__PLUGIN_COMMON__DITHER_H
#define FLAC__PLUGIN_COMMON__DITHER_H

#include <stdlib.h> /* for size_t */
#include "defs.h" /* buy FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS for the caller */
#include "FLAC/ordinals.h"

size_t FLAC__plugin_common__pack_pcm_signed_big_endian(FLAC__byte *data, const FLAC__int32 * const input[], unsigned wide_samples, unsigned channels, unsigned source_bps, unsigned target_bps);
size_t FLAC__plugin_common__pack_pcm_signed_little_endian(FLAC__byte *data, const FLAC__int32 * const input[], unsigned wide_samples, unsigned channels, unsigned source_bps, unsigned target_bps);

#endif
