#pragma once
#include "msys_libc.h"

struct vec2
{
  union {
    struct {
      float x, y;
    };
    float m[2];
  };

  friend vec2 operator-(const vec2& lhs, const vec2& rhs)
  {
    return vec2{lhs.x - rhs.x, lhs.y - rhs.y};
  }

  friend vec2 operator*(float s, const vec2& rhs)
  {
    return vec2{ s * rhs.x, s * rhs.y };
  }
};

float Distance(const vec2& lhs, const vec2& rhs);
float Length(const vec2& a);
vec2 Normalize(const vec2& a);
float Dot(const vec2& a, const vec2& b);

struct vec3
{
  union {
    struct {
      float x, y, z;
    };
    float m[3];
  };
};

struct vec4
{
  union {
    struct {
      float x, y, z, w;
    };
    float m[4];
  };
};

struct color
{
  color() {}
  color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a)
  {
#if WITH_FP_VALIDATION
    // NaN != NaN
    ASSERT(r == r);
    ASSERT(g == g);
    ASSERT(b == b);
    ASSERT(a == a);
#endif
  }
  union {
    struct {
      float r, g, b, a;
    };
    float m[4];
  };

  friend color operator*(float s, const color& c)
  {
    return color{s * c.r, s * c.g, s * c.b, c.a};
  }

  friend color operator-(const color& lhs, const color& rhs)
  {
    return color{lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a};
  }

  friend color operator+(const color& lhs, const color& rhs)
  {
    return color{ lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a };
  }
};

struct mtx2x2
{
  union{
    struct {
      float _m11, _m12;
      float _m21, _m22;
    };
    float m[4];
  };

  static mtx2x2 Scale(float x, float y)
  {
    return mtx2x2{x, 0.f, 0.f, y};
  }

  static mtx2x2 Rotate(float a)
  {
    float c = msys_cosf(a);
    float s = msys_sinf(a);
    return mtx2x2{ c, -s, s, c };
  }

  friend mtx2x2 operator*(const mtx2x2& a, const mtx2x2& b)
  {
    return mtx2x2{
      a._m11 * b._m11 + a._m12 * b._m21,
      a._m11 * b._m12 + a._m12 * b._m22,
      a._m21 * b._m11 + a._m22 * b._m21,
      a._m21 * b._m12 + a._m22 * b._m22
    };
  }
};

vec2 operator*(const vec2& v, const mtx2x2& m);
