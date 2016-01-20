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
