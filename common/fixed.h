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
 * $Id: fixed.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __FIXED_H
#define __FIXED_H

// general fixed point range (-16..16)
#define FIX_FRACBITS	27
// additional fast acc output range limitation (-2..2)
#define ACCFAST_ADJ		3
// additional fast mul b range limitation (-16..16)
#define FIXFAST_ADJ		0
// very bad approx when there is no support for fix_mul (32x32=64)
#define FIX_MUL_ADJ		3

#ifdef FIXED_POINT
// fixed point

#define FIX_SHIFT_A		(FIX_FRACBITS-FIX_FRACBITS/2)
#define FIX_SHIFT_B		(FIX_FRACBITS/2)

typedef int32_t fix_t;
typedef fix_t fixint_t;

#define FIXC(v)				((fix_t)((v)*(1<<FIX_FRACBITS)+((v)>=0?0.5:-0.5)))
#define FIXI(v)				(v)
#if FIX_FRACBITS<28
#define FIX28(v)			RSHIFT_ROUND(v,28-FIX_FRACBITS)
#else
#define FIX28(v)			(v)
#endif

#define ACCFAST_ASHIFT(a,n)	RSHIFT_ROUND(a,FIX_SHIFT_A-ACCFAST_ADJ+(n))
#define ACCFAST_BSHIFT(b)	RSHIFT_ROUND(b,FIX_SHIFT_B)

#define accfast_var(name)		int32_t name
#define accfast_mul(name,a,b)	(name) = (a)*(b)
#define accfast_mla(name,a,b)	(name) += (a)*(b)
#define accfast_neg(name)		(name) = -(name)
#define accfast_get(name,n)		((fix_t)((name)>>(ACCFAST_ADJ-(n))))

#if defined(_MSC_VER) && defined(MIPS)

#define FIXFAST_BSHIFT(b)	((b)<<FIXFAST_ADJ)
#define fixfast_mul(a,b)	((fix_t)(__emul(a,b)>>32) << (32-FIXFAST_ADJ-FIX_FRACBITS))
#define fix_mul(a,b)		((fix_t)(__emul(a,b)>>FIX_FRACBITS))
#define fix_mul64(a,b)		((fix_t)(__emul(a,b)>>FIX_FRACBITS))

#endif

#if defined(_MSC_VER) && defined(_M_IX86)

#if 0
// emulate ARM (just for testing)
#define acc_var(name)		int64_t name
#define acc_mul(name,a,b)	(name) = (int64_t)(a)*(int64_t)(b)
#define acc_mla(name,a,b)	(name) += (int64_t)(a)*(int64_t)(b)
#define acc_neg(name)		(name) = -(name);
#define acc_get(name)		((fix_t)((name) >> FIX_FRACBITS))
#define FIXFAST_BSHIFT(b)	((b)<<FIXFAST_ADJ)
#define fixfast_mul(a,b)	((fix_t)((((int64_t)(a)*(int64_t)(b)) >> 32) << (32-FIXFAST_ADJ-FIX_FRACBITS)))
#endif

#define fix_mul(a,b) _fix_mul(a,b)
#define fix_mul64(a,b) _fix_mul(a,b)
static INLINE fix_t _fix_mul(fix_t a,fix_t b)
{
	__asm 
	{
		mov eax,a
		imul b
		shrd eax, edx, FIX_FRACBITS
	}
}

#endif

#if defined(__GNUC__) && defined(ARM)

#define acc_lo(name)		lo##name
#define acc_hi(name)		hi##name
#define acc_var(name)		int32_t acc_lo(name),acc_hi(name)
#define acc_mul(name,a,b)	asm( "smull %0, %1, %2, %3"	: "=&r" (acc_lo(name)), "=&r" (acc_hi(name)) : "%r" (a), "r" (b))
#define acc_mla(name,a,b)	asm( "smlal %0, %1, %2, %3" : "+r" (acc_lo(name)), "+r" (acc_hi(name)) : "%r" (a), "r" (b))
#define acc_neg(name)		asm ("rsbs %0, %2, #0;\n" \
							     "rsc %1, %3, #0" : "=r" (acc_lo(name)), "=r" (acc_hi(name)) : "r" (acc_lo(name)), "r" (acc_hi(name)) : "cc")
#define acc_get(name)		((fix_t)(((uint32_t)(acc_lo(name)) >> FIX_FRACBITS)|((acc_hi(name)) << (32-FIX_FRACBITS))))

#endif

#if defined(acc_hi)

#ifndef fixfast_mul
#define FIXFAST_BSHIFT(b)	((b)<<FIXFAST_ADJ)
#define fixfast_mul(a,b)	({ acc_var(_tmp); acc_mul(_tmp,(a),(b)); acc_hi(_tmp) << (32-FIXFAST_ADJ-FIX_FRACBITS); })
#endif

#ifndef fix_mul
#define fix_mul(a,b)		({ acc_var(_tmp); acc_mul(_tmp,(a),(b)); acc_get(_tmp); })
#endif

#ifndef fix_mul64
#define fix_mul64(a,b)		({ acc_var(_tmp); acc_mul(_tmp,(a),(b)); acc_get(_tmp); })
#endif

#else

#ifndef fixfast_mul
#ifdef fix_mul
#define FIXFAST_BSHIFT(b)	(b)
#define fixfast_mul(a,b)	fix_mul(a,b)
#else
#define FIXFAST_BSHIFT(b)	RSHIFT_ROUND(b,FIX_SHIFT_B+FIX_MUL_ADJ)
#define fixfast_mul(a,b)	(((a)>>(FIX_SHIFT_A-FIX_MUL_ADJ))*(b))
#endif
#endif

#ifndef fix_mul
#define fix_mul(a,b)		(((a)>>(FIX_SHIFT_A-FIX_MUL_ADJ))*((b)>>(FIX_SHIFT_B+FIX_MUL_ADJ)))
#endif

#ifndef fix_mul64
#define fix_mul64(a,b)		((fix_t)(((int64_t)(a)*(int64_t)(b))>>FIX_FRACBITS))
#endif

#endif

#ifndef fix_1div
#define fix_1div(a)			((fix_t)((((int64_t)1)<<(FIX_FRACBITS*2))/(a)))
#endif

#define checkfix(a)	assert((a)>-0x60000000 && (a)<0x60000000);

#else
// floating point

typedef float fix_t;
typedef int32_t fixint_t; // have to be same size as fix_t
typedef float acclo_t;
typedef int acchi_t;

#define FIXC(v)				((fix_t)(v))
#define FIXI(v)				((int32_t)((v)*(1<<FIX_FRACBITS)))
#define FIX28(v)			((fix_t)(v)/(1<<28))

#define fix_mul(a,b)		((a)*(b))
#define fix_1div(a)			(1.0f/(a))

#define ACCFAST_ASHIFT(a,n)		(a)
#define ACCFAST_BSHIFT(b)		(b)
#define accfast_var(name)		acc_var(name)
#define accfast_mul(name,a,b)	acc_mul(name,a,b)
#define accfast_mla(name,a,b)	acc_mla(name,a,b)
#define accfast_neg(name)		acc_neg(name)
#define accfast_get(name,n)		acc_get(name)

#define FIXFAST_BSHIFT(b)		(b)
#define fixfast_mul(a,b)		fix_mul(a,b)

#define checkfix(a)

#endif

#ifndef acc_mul
#define acc_var(name)		fix_t name
#define acc_mul(name,a,b)	name = fix_mul(a,b)
#define acc_mla(name,a,b)	name += fix_mul(a,b)
#define acc_neg(name)		name = -name
#define acc_get(name)		name
#endif

#endif
