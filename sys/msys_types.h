//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#ifndef _MSYS_TYPES_H_
#define _MSYS_TYPES_H_

// ---- types --------------------------------------------

typedef __int64 sint64;
typedef unsigned __int64 uint64;

typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

typedef uint32 intptr;

// ---- types --------------------------------------------

#ifdef __cplusplus
#define MSYS_INLINE __forceinline
#else
#define MSYS_INLINE
#endif
#define MSYS_ALIGN16 __declspec(align(16))

#endif
