//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#ifndef _MSYS_LIBC_H_
#define _MSYS_LIBC_H_

#include "msys_types.h"
#include <string.h>
#include <math.h>

#define msys_sinf(a) sinf(a)
#define msys_cosf(a) cosf(a)
#define msys_sqrtf(a) sqrtf(a)
#define msys_atanf(a) atanf(a)
#define msys_atan2f(a, b) atan2f(a, b)
#define msys_tanf(a) tanf(a)
#define msys_fabsf(a) fabsf(a)
#define msys_logf(a) logf(a)
#define msys_log10f(a) log10f(a)
#define msys_expf(a) expf(a)
#define msys_memset(a, b, c) memset(a, b, c)
#define msys_memcpy(a, b, c) memcpy(a, b, c)
#define msys_strlen(a) strlen(a)

#include <math.h>

#define msys_sinf(a) sinf(a)
#define msys_cosf(a) cosf(a)
#define msys_sqrtf(a) sqrtf(a)
#define msys_fabsf(a) fabsf(a)
#define msys_atanf(a) atanf(a)
#define msys_atan2f(a, b) atan2f(a, b)
#define msys_tanf(a) tanf(a)

MSYS_INLINE void msys_sincosf(float x, float* r)
{
  _asm fld dword ptr[x];
  _asm fsincos;
  _asm fstp dword ptr[r + 0];
  _asm fstp dword ptr[r + 4];
}

float msys_log2f(const float x);
float msys_expf(const float x);
float msys_fmodf(const float x, const float y);
float msys_powf(const float x, const float y);
float msys_floorf(const float x);
int msys_ifloorf(const float x);

#define msys_zeromem(dst, size) memset(dst, 0, size)
#define msys_memset(a, b, c) memset(a, b, c)
#define msys_memcpy(a, b, c) memcpy(a, b, c)
#define msys_strlen(a) strlen(a)

#endif
