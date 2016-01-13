#include "tl_common.hlsl"

cbuffer cbFill : register(c0)
{
  float4 col;
};

//------------------------------------------------------
// entry-point: ps
float4 PsFill(VSQuadOut p) : SV_Target
{
  return col;
}

//------------------------------------------------------
// entry-point: ps
float4 PsStore(VSQuadOut p)  : SV_Target
{
  return Texture0.Sample(LinearSampler, p.uv);
}
