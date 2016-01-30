#include "tl_common.hlsl"

// cb-meta-begin: PsLinearGradient
// linPower: range: 0.1..10
// pos0: range: -1..1
// pos1: range: -1..1
// cb-meta-end
cbuffer cbLinearGradient : register(b1)
{
  float2 pos0;
  float2 pos1;
  float linPower;
};

// cb-meta-begin: PsRadialGradient 
// center: range: -1..1
// radPower: range: 0.1..10
// cb-meta-end
cbuffer cbRadialGradient : register(b1)
{
  float2 center;
  float radPower;
};

// cb-meta-begin: PsTest
// test_linPower: range: 0.1..1
// test_pos0: range: -1..1
// test_pos1: range: -1..1
// test_ofs: range: -1..1
// test_tjong: range: 0.1..10
// test_speed: range: 0.1..1
// test_col: type: color
// cb-meta-end
cbuffer cbTest : register(b1)
{
  float2 test_pos0;
  float2 test_pos1;
  float test_linPower;
  float2 test_tjong;
  float2 test_speed;
  float4 test_col;
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

//------------------------------------------------------
// full-screen-entry-point: ps
float4 PsTest(VSQuadOut p) : SV_Target
{
  float2 xy = -0.5 + p.pos.xy / dim;
  float2 v = xy - test_pos0;
  float2 dir = normalize(test_pos1 - test_pos0);
  float2 proj = dot(v, dir) * dir;

  // proj + dist = v => dist = v - proj
  float2 dist = (v - proj) * pow(2.5, saturate(sin(xy.x * xy.y * time)));
  return 1 - saturate(pow(length(dist), test_linPower)) * test_col;
}
