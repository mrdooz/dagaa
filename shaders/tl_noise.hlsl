#include "tl_common.hlsl"
#include "tl_noiselib.hlsl"

cbuffer cbNoise : register(b1)
{
  int numOctaves;
  float scale;
  float freq_scale;
  float intensity_scale;
};

//------------------------------------------------------
// entry-point: ps
float4 PsNoise(VSQuadOut p) : SV_Target
{
  float2 xy = -0.5 + p.pos.xy / dim;
  float4 col = float4(0,0,0,0);

  float cur_intensity = 0.5;
  float cur_scale = scale;
  for (int i = 0; i < numOctaves; ++i)
  {
    col += cur_intensity * (0.5 + 0.5 * snoise(xy / cur_scale));
    cur_intensity *= intensity_scale;
    cur_scale *= freq_scale;
  }
  return col;
}

//------------------------------------------------------
// entry-point: ps
float4 PsBrick(VSQuadOut p) : SV_Target
{
  float c = 0.1;
  float s = 0.01;
  float2 w = float2(0.13, 0.1);

  float oddOfs = 40;
  float oddY = fmod(p.pos.y / dim.y, 2 * w.y) > w.y ? oddOfs : 0;
  float x = fmod((p.pos.x + oddY) / dim.x, w.x);
  float y = fmod(p.pos.y / dim.y, w.y);

  float colX = min(smoothstep(0, s, x), 1 - smoothstep(w.x-s, w.x, x));
  float colY = min(smoothstep(0, s, y), 1 - smoothstep(w.y-s, w.y, y));
  return c * min(colX, colY);
}
