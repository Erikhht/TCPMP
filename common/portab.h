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
 * $Id: portab.h 618 2006-01-28 12:22:07Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PORTAB_H
#define __PORTAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#undef INLINE
#undef NOINLINE
#undef IS_LITTLE_ENDIAN
#undef IS_BIG_ENDIAN

#if defined(__palmos__)

#define TARGET_PALMOS
#define REGISTRY_GLOBAL
#define NO_FLOATINGPOINT

#elif defined(__SYMBIAN32__)

#define TARGET_SYMBIAN
#define REGISTRY_GLOBAL
#define UNICODE
//#define MULTITHREAD

#ifdef __MARM__
#define ARM
#endif

#elif defined(_WIN32_WCE)

#define TARGET_WINCE
#define MULTITHREAD

#elif defined(_WIN32)

#define TARGET_WIN32
#define MULTITHREAD

#else
#error Unsupported target
#endif

#if !defined(NO_CONTEXT)
#define CONFIG_CONTEXT
#endif

#if defined(ARM) || defined(MIPS) || defined(SH3)
#define CONFIG_DYNCODE
#endif

#if defined(_M_IX86)
#define CONFIG_UNALIGNED_ACCESS
#endif

#if defined(ARM) || defined(MIPS) || defined(SH3) || defined(_M_IX86)
#define IS_LITTLE_ENDIAN
#else
#define IS_BIG_ENDIAN
#endif

#define TICKSPERSEC			16384

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define MAX_INT	0x7FFFFFFF

#ifdef _MSC_VER

#if _MSC_VER >= 1400
#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcd.lib")
#pragma comment(linker, "/nodefaultlib:oldnames.lib")
#endif

#ifndef alloca
#define alloca _alloca
#endif
#ifndef inline
#define inline __inline
#endif

typedef signed long int32_t;
typedef unsigned long uint32_t;  
typedef signed short int16_t; 
typedef unsigned short uint16_t; 
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#ifndef _UINTPTR_T_DEFINED
typedef unsigned int uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

#if _MSC_VER >= 1400
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif

#define INLINE __forceinline
#define STDCALL __stdcall
#else

#ifdef TARGET_SYMBIAN
typedef signed long int32_t;
typedef unsigned long uint32_t;  
typedef signed short int16_t; 
typedef unsigned short uint16_t; 
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef uint32_t uintptr_t;
#else
#include <stdint.h>
#endif

#define INLINE inline
#if __GNUC__ >= 3
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE 
#endif
#if defined(TARGET_WINCE) || defined(TARGET_PALMOS) || defined(TARGET_SYMBIAN)
#define __stdcall
#define STDCALL 
#else
#define STDCALL __attribute__((stdcall))
#endif
#endif

#ifdef __MMX__
#define MMX
#endif

#define MAX_TICK MAX_INT

typedef int bool_t;
typedef int tick_t;
typedef int filepos_t;
typedef uint8_t boolmem_t; //unsigned so ":1" bitmode should work

typedef struct guid
{
    uint32_t v1;
    uint16_t v2;
    uint16_t v3;
    uint8_t v4[8];
} guid;

typedef struct fraction
{
	int Num;
	int Den;
} fraction;

typedef struct fraction64
{
	int64_t Num;
	int64_t Den;
} fraction64;

#ifndef ZLIB_INTERNAL
#undef T
#ifdef UNICODE

#if defined(__GNUC__)
#include <wchar.h>
#endif

#if defined(__GNUC__) && defined(__cplusplus)
typedef __wchar_t tchar_t;
#else
typedef unsigned short tchar_t;
#endif

#define tcsstr wcsstr
#define tcslen wcslen
#define tcscmp wcscmp
#define tcsncmp wcsncmp
#define tcschr wcschr
#define tcsrchr wcsrchr
#define T(a) L ## a
#ifdef TARGET_SYMBIAN
extern int wcsncmp(const tchar_t *,const tchar_t *,size_t);
extern tchar_t* wcschr(const tchar_t *, tchar_t);
extern tchar_t* wcsrchr(const tchar_t *, tchar_t);
#endif
#else
typedef char tchar_t;
#define tcsstr strstr
#define tcslen strlen
#define tcscmp strcmp
#define tcsncmp strncmp
#define tcschr strchr
#define tcsrchr strrchr
#define T(a) a
#endif
#endif

#define tcscpy !UNSAFE!
#define tcscat !UNSAFE!
#define stprintf !UNSAFE!
#define vstprintf !UNSAFE!

#if defined(_WIN32)
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLEXPORT
#define DLLIMPORT
#endif

#if !defined(SH3) && !defined(MIPS)
#define _INLINE INLINE
#define _CONST const
#else
#define _INLINE
#define _CONST
#endif

#ifdef BUILDFIXED
#define _CONSTFIXED
#else
#define _CONSTFIXED _CONST
#endif

//todo: needs more testing, that nothing broke...
//#if defined(MAX_PATH)
//#define MAXPATH MAX_PATH
//#else
#define MAXPATH 256
//#endif
#define MAXMIME 16

#define MAXPLANES 4
typedef void* planes[MAXPLANES];
typedef const void* constplanes[MAXPLANES];

#define FOURCCBE(a,b,c,d) \
	(((uint8_t)(a) << 24) | ((uint8_t)(b) << 16) | \
	((uint8_t)(c) << 8) | ((uint8_t)(d) << 0))

#define FOURCCLE(a,b,c,d) \
	(((uint8_t)(a) << 0) | ((uint8_t)(b) << 8) | \
	((uint8_t)(c) << 16) | ((uint8_t)(d)<< 24))

#ifdef IS_BIG_ENDIAN
#define FOURCC(a,b,c,d) FOURCCBE(a,b,c,d)
#else
#define FOURCC(a,b,c,d) FOURCCLE(a,b,c,d)
#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif

#ifndef sign
#  define sign(x) ((x)<0?-1:1)
#endif

#if defined(TARGET_WINCE)
#define strdup(x) _strdup(x)
#endif

#if defined(__GNUC__)
#define alloca(size) __builtin_alloca(size)

#if defined(TARGET_PALMOS)
extern int rand();
extern void qsort(void* const base,size_t,size_t,int(*cmp)(const void*,const void*));
#endif

#if defined(ARM) && !defined(TARGET_WINCE)
//fixed size stack:
//  symbian
//  palm os
#define SWAPSP
static INLINE void* SwapSP(void* in)
{
	void* out;
	asm volatile(
		"mov %0, sp\n\t"
		"mov sp, %1\n\t"
		: "=&r"(out) : "r"(in) : "cc");
	return out;
}
#endif

#endif //__GNUC__

#ifdef _MSC_VER
#define LIBGCC	\
int64_t __divdi3(int64_t a,int64_t b) { return a/b; } \
int64_t __moddi3(int64_t a,int64_t b) { return a%b; } \
int32_t __divsi3(int32_t a,int32_t b) { return a/b; } \
int32_t __modsi3(int32_t a,int32_t b) { return a%b; } \
uint32_t __udivsi3(uint32_t a,uint32_t b) { return a/b; } \
uint32_t __umodsi3(uint32_t a,uint32_t b) { return a%b; }

#define LIBGCCFLOAT	\
int __fixdfsi(double i) { return (int)i; } \
int64_t __fixdfdi(double i) { return (int64_t)i; } \
int __eqdf2(double a,double b) { return !(a==b); } \
int __nedf2(double a,double b) { return a != b; } \
int __gtdf2(double a,double b) { return a > b; } \
int __gedf2(double a,double b) { return (a>=b)-1; } \
int __ltdf2(double a,double b) { return -(a<b); } \
int __ledf2(double a,double b) { return 1-(a<=b); } \
double __floatsidf(int i) { return (double)i; } \
double __extendsfdf2(float f) { return f; } \
double __negdf2(double a) { return -a; } \
double __divdf3(double a,double b) { return a/b; } \
double __muldf3(double a,double b) { return a*b; } \
double __adddf3(double a,double b) { return a+b; } \
double __subdf3(double a,double b) { return a-b; } \
int __eqsf2(float a,float b) { return !(a==b); } \
int __nesf2(float a,float b) { return a != b; } \
int __gtsf2(float a,float b) { return a > b; } \
int __gesf2(float a,float b) { return (a>=b)-1; } \
int __ltsf2(float a,float b) { return -(a<b); } \
int __lesf2(float a,float b) { return 1-(a<=b); } \
int __fixsfsi(float i) { return (int)i; } \
float __floatsisf(int i) { return (float)i; } \
float __truncdfsf2(double f) { return (float)f; } \
float __negsf2(float a) { return -a; } \
float __divsf3(float a,float b) { return a/b; } \
float __mulsf3(float a,float b) { return a*b; } \
float __addsf3(float a,float b) { return a+b; } \
float __subsf3(float a,float b) { return a-b; }
#else
#define LIBGCC
#ifdef NO_FLOATINGPOINT
#define LIBGCCFLOAT \
double log(double a) { return 0; } \
double pow(double a,double b) { return 0; } \
double sqrt(double a) { return 0; } \
double exp(double a) { return 1.0+a+a*a/2.0+a*a*a/6.0; }
#else
#define LIBGCCFLOAT
#endif
#endif

#endif
