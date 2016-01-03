#include "msys_math.hpp"
#include "msys_libc.h"

float Distance(const vec2& lhs, const vec2& rhs)
{
  float dx = lhs.x - rhs.x;
  float dy = lhs.y - rhs.y;
  return msys_sqrtf(dx * dx + dy * dy);
}

float Length(const vec2& a)
{
  return msys_sqrtf(a.x * a.x + a.y * a.y);
}

vec2 Normalize(const vec2& a)
{
  float len = Length(a);
  if (len == 0)
    return a;
  return 1.0f / len * a;
}

float Dot(const vec2& a, const vec2& b)
{
  return a.x * b.x + a.y * b.y;
}

vec2 operator*(const vec2& v, const mtx2x2& m)
{
  return vec2{
    v.x * m._m11 + v.y * m._m21,
    v.x * m._m12 + v.y * m._m22
  };
}
