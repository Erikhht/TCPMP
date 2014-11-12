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
 * $Id: sincos.c 591 2006-01-17 12:28:08Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#define PI 3.14159265358979324

NOINLINE double floor(double i);
NOINLINE double ceil(double i)
{
	if (i<0)
		return -floor(-i);
	else
	{
		double d = floor(i);
		if (d!=i)
			d+=1.0;
		return d;
	}
}

NOINLINE double floor(double i)
{
	if (i<0) 
		return -ceil(-i);
	else
		return (double)(int)i;
}

NOINLINE double sin(double i)
{
	int n,k;
	double j,sum;
	int sign = 0;

	if (i<0)
	{
		sign = 1;
		i = -i;
	}

	i *= 1.0/(2*PI);
	i -= floor(i);
	if (i>=0.5)
	{
		i -= 0.5;
		sign ^= 1;
	}

	i *= 2*PI;
	k = 1;
	j = sum = i;
	for (n=3;n<14;n+=2)
	{
		k *= (n-1)*n;
		j *= i*i;
		if (n & 2)
			sum -= j/k;
		else
			sum += j/k;
	}

	return sign ? -sum:sum;
}

NOINLINE double cos(double i)
{
	return sin(i+PI/2);
}
