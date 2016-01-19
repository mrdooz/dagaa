#include "tl_common.hlsl"

cbuffer cbModulate : register(c1)
{
  float2 factor;
};

//------------------------------------------------------
// entry-point: ps
float4 PsModulate(VSQuadOut p) : SV_Target
{
  float2 uv = p.uv;
  float4 colA = Texture0.Sample(LinearSampler, uv);
  float4 colB = Texture1.Sample(LinearSampler, uv);

  return factor.x * colA + factor.y * colB;
}
