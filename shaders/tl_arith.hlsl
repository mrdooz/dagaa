#include "tl_common.hlsl"

cbuffer cbModulate : register(c0)
{
  float factorA;
  float factorB;
};

//------------------------------------------------------
// entry-point: ps
float4 PsModulate(VSQuadOut p) : SV_Target
{
  float2 uv = p.uv;
  float4 colA = Texture0.Sample(LinearSampler, uv);
  float4 colB = Texture0.Sample(LinearSampler, uv);

  return factorA * colA + factorB * colB;
}
