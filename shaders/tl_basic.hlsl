#include "tl_common.hlsl"

cbuffer F : register(c0)
{
  float4 col;
};

//------------------------------------------------------
// entry-point: ps
float4 PsFill(VSQuadOut p) : SV_Target
{
  return col;
}
