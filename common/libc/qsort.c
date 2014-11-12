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
 * $Id: qsort.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// cheap fake bubble sort :)))

#if defined(__GNUC__)
#define alloca(size) __builtin_alloca(size)
#else
#include <malloc.h>
#endif

void qsort(void *const base, size_t num, size_t size,int (*cmp)(const void*, const void*))
{
	if (num>1)
	{
		void* tmp = alloca(size);
		int changed;
		do
		{
			char* p  = (char*)base;
			char* pe = p + num*size;
			changed = 0;

			for (p+=size;p!=pe;p+=size)
				if (cmp(p-size,p)>0)
				{
					memcpy(tmp,p,size);
					memcpy(p,p-size,size);
					memcpy(p-size,tmp,size);
					changed = 1;
				}
		}
		while (changed);
	}
}
