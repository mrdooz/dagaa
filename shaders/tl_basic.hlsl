#include "tl_common.hlsl"

cbuffer cbFill : register(c0)
{
  float4 col;
};

//------------------------------------------------------
// entry-point: ps
float4 PsFill(VSQuadOut p) : SV_Target
{
  // return float4(1,1,0,0);
  return col;
}

//------------------------------------------------------
// entry-point: ps
float4 PsCopy(VSQuadOut p)  : SV_Target
{
  return Texture0.Sample(LinearSampler, p.uv);
}
