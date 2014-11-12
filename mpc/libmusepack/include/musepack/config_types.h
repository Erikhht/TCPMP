#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

typedef unsigned char mpc_bool_t;
#define TRUE  1
#define FALSE 0

#ifdef _MSC_VER
#define inline __inline
typedef signed long int32_t;
typedef unsigned long uint32_t;  
typedef signed short int16_t; 
typedef unsigned short uint16_t; 
typedef signed __int64 int64_t;
#else
#ifdef __SYMBIAN32__
#ifndef __PORTAB_H
typedef signed long int32_t;
typedef unsigned long uint32_t;  
typedef signed short int16_t; 
typedef unsigned short uint16_t; 
typedef signed long long int64_t;
#endif
#else
#include <stdint.h>
#endif
#define __cdecl
#endif

typedef int16_t mpc_int16_t;
typedef uint16_t mpc_uint16_t;
typedef int32_t mpc_int32_t;
typedef uint32_t mpc_uint32_t;
typedef int64_t mpc_int64_t;

#endif
