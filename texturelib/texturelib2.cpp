#include "texturelib2.hpp"
#include "../sys/msys_graphics.hpp"
#include "../sys/msys_containers.hpp"
#include <shaders/out/tl_common_VsQuad.vso.hpp>
#include <shaders/out/tl_basic_PsFill.pso.hpp>
#include <shaders/out/tl_arith_PsModulate.pso.hpp>
#include <shaders/out/tl_basic_PsStore.pso.hpp>

#include <sys/msys_file.hpp>

namespace TextureLib
{
  enum class ShaderOps
  {
    Load = 1,
    Store = 2,

    Fill = 16,

    Modulate = 64,
  };

 static FixedLinearMap<u8, ObjectHandle, 256> g_Shaders;
 ObjectHandle g_vsFullScreen;

 //-----------------------------------------------------------------------------
  template <typename T>
  T Read(const char** ptr)
  {
    T tmp = *(T*)(*ptr);
    *ptr += sizeof(T);
    return tmp;
  }

  //-----------------------------------------------------------------------------
  bool Init()
  {
    g_vsFullScreen = g_Graphics->CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
      tl_common_VsQuad_bin,
      sizeof(tl_common_VsQuad_bin),
      ObjectHandle::VertexShader);

#define LOAD_PS(cmd, filename, bin) \
    g_Shaders[(u8)cmd] = g_Graphics->CreateShader( \
      FW_STR("shaders/out/" filename ".pso"), \
      bin,  \
      sizeof(bin), \
      ObjectHandle::PixelShader);

    LOAD_PS(ShaderOps::Store, "tl_basic_PsStoreD", tl_basic_PsStore_bin);
    LOAD_PS(ShaderOps::Fill, "tl_basic_PsFillD", tl_basic_PsFill_bin);
    LOAD_PS(ShaderOps::Modulate, "tl_arith_PsModulateD", tl_arith_PsModulate_bin);

    ObjectHandle vs = g_Graphics->CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
      tl_common_VsQuad_bin,
      sizeof(tl_common_VsQuad_bin),
      ObjectHandle::VertexShader);

    struct VmPrg
    {
      u8 version = 1;
      u8 texturesUsed = 0;
    };

    BinaryReader r;
    r.Init("texturelib/texture.dat");
    VmPrg prg = r.Read<VmPrg>();

    ObjectHandle renderTargets[256];

    for (int i = 0; i < prg.texturesUsed; ++i)
    {
      u32 black = 0;
      renderTargets[i] = g_Graphics->CreateRenderTarget(1024, 1024, &black);
    }

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_Graphics->GetResource<ID3D11VertexShader>(vs), 0, 0);

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

    ObjectHandle cb = g_Graphics->CreateBuffer(D3D11_BIND_CONSTANT_BUFFER, 1024, true, nullptr);

    while (!r.Eof())
    {
      u8 opIdx = r.Read<u8>();
      u8 rtIdx = r.Read<u8>();

      u8 numInputTextures = r.Read<u8>();
      for (int i = 0; i < numInputTextures; ++i)
      {
        u8 textureIdx = r.Read<u8>();
        // Set shader resource
        ID3D11ShaderResourceView* srv =
          g_Graphics->GetResource<ID3D11ShaderResourceView>(renderTargets[textureIdx]);
        ctx->PSSetShaderResources(0, 1, &srv);
      }

      u16 cbufferSize = r.Read<u16>();

      // Copy data to the cbuffer
      g_Graphics->CopyToBuffer(cb, r.Buf(), cbufferSize);
      r.Skip(cbufferSize);

      ctx->Draw(6, 0);

    }
    return true;
  }
 }
