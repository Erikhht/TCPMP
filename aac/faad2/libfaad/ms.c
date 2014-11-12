/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: ms.c,v 1.17 2004/09/04 14:56:28 menno Exp $
**/

#include "common.h"
#include "structs.h"

#include "syntax.h"
#include "ms.h"
#include "is.h"
#include "pns.h"

void ms_decode(ic_stream *ics, ic_stream *icsr, uint16_t frame_len)
{
    int b, sfb;
    uint16_t nshort = frame_len/8;

    if (ics->ms_mask_present >= 1)
    {
		real_t *l_spec = ics->buffer;
		real_t *r_spec = icsr->buffer;
		ic_group *g;
		ic_group *gr = icsr->group;

        for (g = ics->group; g != ics->group_end; g++) 
        {
            for (b = 0; b < g->window_group_length; b++)
            {
                for (sfb = 0; sfb < ics->max_sfb; sfb++)
                {
                    /* If intensity stereo coding or noise substitution is on
                       for a particular scalefactor band, no M/S stereo decoding
                       is carried out.
                     */
                    if ((g->sfb[sfb].ms_used || ics->ms_mask_present == 2) &&
                        !is_intensity(icsr, gr, sfb) && !is_noise(ics, g, sfb))
                    {
						int i;
                        for (i = ics->swb_offset[sfb]; i < ics->swb_offset[sfb+1]; i++)
                        {
                            real_t tmp = l_spec[i] - r_spec[i];
                            l_spec[i] = l_spec[i] + r_spec[i];
                            r_spec[i] = tmp;
                        }
                    }
                }

				l_spec += nshort;
				r_spec += nshort;
            }
        }
    }
}
