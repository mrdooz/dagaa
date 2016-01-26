#include "tl_common.hlsl"

// cb-meta-begin: PsLinearGradient
// cb-meta-end
cbuffer cbLinearGradient : register(b1)
{
  float2 pos0;
  float2 pos1;
  float linPower;
};

// cb-meta-begin: PsRadialGradient 
// cb-meta-end
cbuffer cbRadialGradient : register(b1)
{
  float2 center;
  float radPower;
};

//------------------------------------------------------
// full-screen-entry-point: ps
float4 PsLinearGradient(VSQuadOut p) : SV_Target
{
  float2 xy = -0.5 + p.pos.xy / dim;
  float2 v = xy - pos0;
  float2 dir = normalize(pos1 - pos0);
  float2 proj = dot(v, dir) * dir;

  // proj + dist = v => dist = v - proj
  float2 dist = v - proj;
  return 1 - saturate(pow(length(dist), linPower));
}

//------------------------------------------------------
// full-screen-entry-point: ps
float4 PsRadialGradient(VSQuadOut p) : SV_Target
{
  float2 xy = -0.5 + p.pos.xy / dim;
  float2 dist = xy - center;
  return 1 - saturate(pow(length(dist), radPower));
}
