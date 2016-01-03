#include "texturelib.hpp"
#include "../sys/msys_graphics.hpp"
#include "../sys/msys_containers.hpp"
#include <shaders/out/tl_common_VsQuad.vso.hpp>
#include <shaders/out/tl_basic_PsCopy.pso.hpp>
#include <shaders/out/tl_basic_PsFill.pso.hpp>
#include <shaders/out/tl_arith_PsModulate.pso.hpp>

#include <sys/msys_file.hpp>
#include <sys/msys_utils.hpp>
#include <sys/msys_libc.h>

#include <vector>

namespace TextureLib
{
  static const int TEXTURE_SIZE = 1024;

  enum class ShaderOps
  {
    Copy = 1,

    Fill = 16,

    Modulate = 64,
  };

 static FixedLinearMap<u8, ID3D11PixelShader*, 256> g_Shaders;
 ObjectHandle g_vsFullScreen;

 struct
 {
   ObjectHandle hTexture;
   ObjectHandle hSrv;
   ObjectHandle hRt;
   ID3D11ShaderResourceView* srv;
   ID3D11RenderTargetView* rt;
 } renderTargets[256];

static  const int NUM_AUX_TEXTURES = 16;

#if WITH_TEXTURE_UPDATE

 static HANDLE g_eventNewData = INVALID_HANDLE_VALUE;
 static HANDLE g_eventClose = INVALID_HANDLE_VALUE;
 static HANDLE g_updateThread = INVALID_HANDLE_VALUE;
 
 static char g_newData[4096];
 static int g_newDataSize;

 DWORD WINAPI ThreadProc(LPVOID lpParameter)
 { 
   BOOL   fConnected = FALSE;
   DWORD  dwThreadId = 0;
   LPTSTR pipeName = TEXT("\\\\.\\pipe\\texturepipe");

   const int BUF_SIZE = 4096;
   char readBuf[BUF_SIZE];
   //char writeBuf[BUF_SIZE];

   while (true)
   {
     HANDLE hPipe = CreateNamedPipe(pipeName,
       PIPE_ACCESS_DUPLEX,
       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
       PIPE_UNLIMITED_INSTANCES, // max. instances
       BUF_SIZE,                 // output buffer size
       BUF_SIZE,                 // input buffer size
       0,                        // client time-out
       NULL);                    // default security attribute

     BOOL res = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    

     // XXX: meh, for a proper shutdown, I need to to async reads, and WaitForMultipleObjects..
     while (true)
     {
       DWORD bytesRead = 0;
       res = ReadFile(hPipe, readBuf, BUF_SIZE, &bytesRead, NULL);
       if (!res)
         break;

       memcpy(g_newData, readBuf, bytesRead);
       g_newDataSize = bytesRead;
       SetEvent(g_eventNewData);
     }

     DisconnectNamedPipe(hPipe);
     CloseHandle(hPipe);
   }

   return 1;
 }
#endif

 //-----------------------------------------------------------------------------
  template <typename T>
  T Read(const char** ptr)
  {
    T tmp = *(T*)(*ptr);
    *ptr += sizeof(T);
    return tmp;
  }

  //-----------------------------------------------------------------------------
  void GenerateTexture(const BinaryReader& reader)
  {
    struct VmPrg
    {
      u8 version = 1;
      u8 texturesUsed = 0;
    };

    VmPrg prg = reader.Read<VmPrg>();

    void* nullViews[16];
    msys_memset(nullViews, 0, sizeof(nullViews));

    // create any required outputs
    for (int i = NUM_AUX_TEXTURES; i < prg.texturesUsed; ++i)
    {
      u32 black = 0;
      ObjectHandle hTexture, hRt, hSrv;
      g_Graphics->CreateRenderTarget(TEXTURE_SIZE, TEXTURE_SIZE, &black, &hTexture, &hRt, &hSrv);
      renderTargets[i].hTexture = hTexture;
      renderTargets[i].hSrv = hSrv;
      renderTargets[i].hRt = hRt;
      renderTargets[i].srv = g_Graphics->GetResource<ID3D11ShaderResourceView>(hSrv);
      renderTargets[i].rt = g_Graphics->GetResource<ID3D11RenderTargetView>(hRt);
    }

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_Graphics->GetResource<ID3D11VertexShader>(g_vsFullScreen), 0, 0);

    CD3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.f, 0.f, (float)TEXTURE_SIZE, (float)TEXTURE_SIZE);
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

    while (!reader.Eof())
    {
      u8 opIdx = reader.Read<u8>();
      u8 rtIdx = reader.Read<u8>();

      // Set all the input textures
      u8 numInputTextures = reader.Read<u8>();
      for (int i = 0; i < numInputTextures; ++i)
      {
        u8 textureIdx = reader.Read<u8>();
        ctx->PSSetShaderResources(i, 1, &renderTargets[textureIdx].srv);
      }

      u16 cbufferSize = reader.Read<u16>();

      // Copy data to the cbuffer
      g_Graphics->CopyToBuffer(cb, reader.Buf(), cbufferSize);
      reader.Skip(cbufferSize);

      ID3D11Buffer* xx = g_Graphics->GetResource<ID3D11Buffer>(cb);
      ctx->PSSetConstantBuffers(0, 1, &xx);

      // Set render target and draw
      ctx->OMSetRenderTargets(1, &renderTargets[rtIdx].rt, nullptr);
      ctx->PSSetShader(g_Shaders[opIdx], nullptr, 0);
      ctx->Draw(6, 0);

      // unset views
      if (numInputTextures)
      {
        ctx->PSSetShaderResources(0, numInputTextures, (ID3D11ShaderResourceView**)nullViews);
      }
      ctx->OMSetRenderTargets(1, (ID3D11RenderTargetView**)nullViews, nullptr);
    }

    g_Graphics->ReleaseResource(cb);

    // Free any temporary render targets
    for (int i = NUM_AUX_TEXTURES; i < prg.texturesUsed; ++i)
    {
      g_Graphics->ReleaseResource(renderTargets[i].hTexture);
      g_Graphics->ReleaseResource(renderTargets[i].hRt);
      g_Graphics->ReleaseResource(renderTargets[i].hSrv);
    }
  }
  

  //-----------------------------------------------------------------------------
  bool Init()
  {
    g_vsFullScreen = g_Graphics->CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
      tl_common_VsQuad_bin,
      sizeof(tl_common_VsQuad_bin),
      ObjectHandle::VertexShader);

#define LOAD_PS(cmd, filename, bin)                                                                \
  {                                                                                                \
    ObjectHandle h = g_Graphics->CreateShader(                                                     \
        FW_STR("shaders/out/" filename ".pso"), bin, sizeof(bin), ObjectHandle::PixelShader);      \
    g_Shaders[(u8)cmd] = g_Graphics->GetResource<ID3D11PixelShader>(h);                            \
  }

    LOAD_PS(ShaderOps::Copy, "tl_basic_PsCopyD", tl_basic_PsCopy_bin);
    LOAD_PS(ShaderOps::Fill, "tl_basic_PsFillD", tl_basic_PsFill_bin);
    LOAD_PS(ShaderOps::Modulate, "tl_arith_PsModulateD", tl_arith_PsModulate_bin);

    memset(renderTargets, 0, sizeof(renderTargets));

    // create the aux textures, and the required outputs
    for (int i = 255, j = 0; j <= NUM_AUX_TEXTURES; i = j++)
    {
      u32 black = 0;
      ObjectHandle hTexture, hRt, hSrv;
      g_Graphics->CreateRenderTarget(TEXTURE_SIZE, TEXTURE_SIZE, &black, &hTexture, &hRt, &hSrv);
      renderTargets[i].hTexture = hTexture;
      renderTargets[i].hSrv = hSrv;
      renderTargets[i].hRt = hRt;
      renderTargets[i].srv = g_Graphics->GetResource<ID3D11ShaderResourceView>(hSrv);
      renderTargets[i].rt = g_Graphics->GetResource<ID3D11RenderTargetView>(hRt);
    }

#if WITH_TEXTURE_UPDATE
    g_eventNewData = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif

    DWORD threadId;
    g_updateThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &threadId);

    return true;
  }

  //-----------------------------------------------------------------------------
  void Tick()
  {
#if WITH_TEXTURE_UPDATE
    if (WaitForSingleObject(g_eventNewData, 0) == WAIT_OBJECT_0)
    {
      BinaryReader r(g_newData, g_newDataSize);
      GenerateTexture(r);
    }
#endif

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_Graphics->GetResource<ID3D11VertexShader>(g_vsFullScreen), 0, 0);

    CD3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.f, 0.f, (float)TEXTURE_SIZE, (float)TEXTURE_SIZE);
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

    ctx->PSSetShaderResources(0, 1, &renderTargets[0xff].srv);
    ctx->OMSetRenderTargets(1, &g_Graphics->_renderTargetView, nullptr);
    ctx->PSSetShader(g_Shaders[(u8)ShaderOps::Copy], nullptr, 0);
    ctx->Draw(6, 0);

  }

  //-----------------------------------------------------------------------------
  void Close()
  {
#if WITH_TEXTURE_UPDATE
    CloseHandle(g_eventNewData);
#endif
  }

 }
