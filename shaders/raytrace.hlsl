Texture2D Texture0 : register(t0);
sampler PointSampler : register(s0);
sampler LinearSampler : register(s1);
sampler LinearWrap : register(s2);
sampler LinearBorder : register(s3);

//------------------------------------------------------
struct VSQuadOut
{
  float4 pos : SV_Position;
  float2 uv: TexCoord;
};

//------------------------------------------------------
// outputs a full screen triangle with screen-space coordinates
// input: three empty vertices
// entry-point: vs
VSQuadOut VsQuad(uint vertexID : SV_VertexID)
{
  VSQuadOut result;
  // ID=0 -> Pos=[-1,-1], Tex=[0,0]
  // ID=1 -> Pos=[ 3,-1], Tex=[2,0]
  // ID=2 -> Pos=[-1,-3], Tex=[0,2]
  result.uv = float2((vertexID << 1) & 2, vertexID & 2);
  result.pos = float4(result.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
  return result;
}

//------------------------------------------------------
// entry-point: ps
float4 PsRaytrace(VSQuadOut p) : SV_Target
{
  return Texture0.Sample(LinearSampler, p.uv);
  // float2 dim = float2(800, 600);
  // float2 xx = 2 * (p.pos.xy / dim - 0.5);
  // float f = 0.8 + 0.2 * atan2(xx.y, xx.x);
  // float r = length(xx);
  // return r;
}
