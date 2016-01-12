//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#include "sys/msys.h"
#include "sys/msys_graphics.hpp"
#include "intro.h"
#include "texturelib/texturelib2.hpp"
#include "shaders/out/raytrace_PsRaytrace.pso.hpp"
#include "shaders/out/raytrace_VsQuad.vso.hpp"

ObjectHandle vs, ps;

typedef struct
{
  long mTo;
  int mSeed;
} IntroObject;

//--------------------------------------------------------
static IntroObject intro;

#if WITH_FILE_WATCHER
#define FW_STR(x) x
#else
#define FW_STR(x) ""
#endif

int intro_init(int xr, int yr, int nomusic, IntroProgressDelegate* pd)
{
  // progress report, (from 0 to 200)
  pd->func(pd->obj, 0);

  if (!TextureLib::Init())
    return 1;

  // init your stuff here (mzk player, intro, ...)
  // remember to call pd->func() regularly to update the loading bar

  vs = g_Graphics->CreateShader(FW_STR("shaders/out/raytrace_VsQuadD.vso"),
      raytrace_VsQuad_bin,
      sizeof(raytrace_VsQuad_bin),
      ObjectHandle::VertexShader);
  ps = g_Graphics->CreateShader(FW_STR("shaders/out/raytrace_PsRaytraceD.pso"),
      raytrace_PsRaytrace_bin,
      sizeof(raytrace_PsRaytrace_bin),
      ObjectHandle::PixelShader);

  if (!(vs.IsValid() && ps.IsValid()))
  {
    return 0;
  }

  pd->func(pd->obj, 200);

  intro.mTo = msys_timerGet();
  return 1;
}

void intro_end(void)
{
  // deallicate your stuff here
}

//-----------------------------------------------------------------------------
int intro_run(ObjectHandle texture)
{
  float time = (1.0f / 1000.0f) * (float)(msys_timerGet() - intro.mTo);

  ID3D11DeviceContext* ctx = g_Graphics->_context;
  ctx->IASetInputLayout(nullptr);
  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(g_Graphics->GetResource<ID3D11VertexShader>(vs), 0, 0);
  ctx->PSSetShader(g_Graphics->GetResource<ID3D11PixelShader>(ps), 0, 0);

  CD3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.f, 0.f, 800, 600);
  ctx->RSSetViewports(1, &viewport);

  float blendFactor[4] = { 1, 1, 1, 1 };
  ctx->OMSetBlendState(
      g_Graphics->GetResource<ID3D11BlendState>(g_DefaultBlendState), blendFactor, 0xffffffff);
  ctx->RSSetState(g_Graphics->GetResource<ID3D11RasterizerState>(g_DefaultRasterizerState));
  ctx->OMSetDepthStencilState(
      g_Graphics->GetResource<ID3D11DepthStencilState>(g_DepthDisabledState), 0);

  ID3D11SamplerState* sampler =
      g_Graphics->GetResource<ID3D11SamplerState>(g_Graphics->_samplers[DXGraphics::Linear]);
  ctx->PSSetSamplers(1, 1, &sampler);

  ID3D11ShaderResourceView* srv = g_Graphics->GetResource<ID3D11ShaderResourceView>(texture);
  ctx->PSSetShaderResources(0, 1, &srv);
  ctx->Draw(6, 0);

  return 0;
}
