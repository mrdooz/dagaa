#include "texturelib.hpp"
#include <shaders/out/tl_common_VsQuad.vso.hpp>
#include <shaders/out/tl_arith_PsModulate.pso.hpp>
#include <shaders/out/tl_basic_PsCopy.pso.hpp>
#include <shaders/out/tl_basic_PsFill.pso.hpp>
#include <shaders/out/tl_noise_PsNoise.pso.hpp>
#include <shaders/out/tl_noise_PsBrick.pso.hpp>
#include <shaders/out/tl_gradient_PsLinearGradient.pso.hpp>
#include <shaders/out/tl_gradient_PsRadialGradient.pso.hpp>

#include <sys/msys_file.hpp>
#include <sys/msys_libc.h>
#include <sys/msys_utils.hpp>
#include <sys/msys_containers.hpp>
#include <sys/msys_graphics.hpp>
#include <sys/msys_math.hpp>

#include <vector>

#define LOAD_FROM_FILE 1

namespace TextureLib
{
  static const int TEXTURE_SIZE = 1024;
  static bool g_Initialized = false;

  enum class ShaderOps
  {
    Copy = 1,

    Fill = 16,
    RadialGradient = 17,
    LinearGradient = 18,
    Noise = 20,
    Brick = 21,

    Modulate = 64,
  };

  static FixedLinearMap<u8, ObjectHandle, 256> g_Shaders;
  static ObjectHandle g_vsFullScreen;
  static ObjectHandle g_cbCommon;

  static struct
  {
    vec2 dim;
  } cbCommon;

  struct
  {
    ObjectHandle hRt;
    DXGraphics::RenderTarget* rt;
    ID3D11ShaderResourceView* srv;
    ID3D11RenderTargetView* rtv;
  } renderTargets[256];

  static const int NUM_AUX_TEXTURES = 16;

#if WITH_TEXTURE_UPDATE
  static HANDLE g_eventNewData = INVALID_HANDLE_VALUE;
  static HANDLE g_eventClose = INVALID_HANDLE_VALUE;
  static HANDLE g_updateThread = INVALID_HANDLE_VALUE;

  static char g_newData[4096];
  static int g_newDataSize;

  DWORD WINAPI ThreadProc(LPVOID lpParameter)
  {
    BOOL fConnected = FALSE;
    DWORD dwThreadId = 0;
    LPTSTR pipeName = TEXT("\\\\.\\pipe\\texturepipe");

    const int BUF_SIZE = 4096;
    char readBuf[BUF_SIZE];
    // char writeBuf[BUF_SIZE];

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
  static void SetupState(const D3D11_VIEWPORT& viewport)
  {
    ID3D11DeviceContext* ctx = g_Graphics->_context;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_Graphics->GetResource<ID3D11VertexShader>(g_vsFullScreen), 0, 0);

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

    cbCommon.dim.x = (float)TEXTURE_SIZE;
    cbCommon.dim.y = (float)TEXTURE_SIZE;
    g_Graphics->CopyToBuffer(g_cbCommon, (void*)&cbCommon, sizeof(cbCommon));
    ID3D11Buffer* cbCommonPtr = g_Graphics->GetResource<ID3D11Buffer>(g_cbCommon);
    ctx->PSSetConstantBuffers(0, 1, &cbCommonPtr);
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
      ObjectHandle hRt = g_Graphics->CreateRenderTarget(TEXTURE_SIZE, TEXTURE_SIZE, true);
      DXGraphics::RenderTarget* rt = g_Graphics->GetResource<DXGraphics::RenderTarget>(hRt);
      renderTargets[i].hRt = hRt;
      renderTargets[i].rt = rt;
      renderTargets[i].srv = g_Graphics->GetResource<ID3D11ShaderResourceView>(rt->srv);
      renderTargets[i].rtv = g_Graphics->GetResource<ID3D11RenderTargetView>(rt->rtv);
    }

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    SetupState(CD3D11_VIEWPORT(0.f, 0.f, (float)TEXTURE_SIZE, (float)TEXTURE_SIZE));

    ObjectHandle cb = g_Graphics->CreateBuffer(D3D11_BIND_CONSTANT_BUFFER, 1024, true, nullptr);
    ID3D11Buffer* resCb = g_Graphics->GetResource<ID3D11Buffer>(cb);

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
      if (cbufferSize)
      {
        // Copy data to the cbuffer
        g_Graphics->CopyToBuffer(cb, reader.Buf(), cbufferSize);
        reader.Skip(cbufferSize);
        ID3D11Buffer* cbPtr = g_Graphics->GetResource<ID3D11Buffer>(cb);
        ctx->PSSetConstantBuffers(1, 1, &cbPtr);
      }

      // Set render target and draw
      ctx->OMSetRenderTargets(1, &renderTargets[rtIdx].rtv, nullptr);
      ASSERT(g_Shaders.contains(opIdx));
      ctx->PSSetShader(g_Graphics->GetResource<ID3D11PixelShader>(g_Shaders[opIdx]), nullptr, 0);
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
      g_Graphics->ReleaseResource(renderTargets[i].hRt);
    }
  }

#if WITH_FILE_UTILS
  static bool OnShaderChanged(const char* filename, const char* buf, int len)
  {
    if (!g_Initialized)
      return true;

    std::vector<char> texBuf;
    if (LoadFile("c:/users/magnus/documents/test1.dat", &texBuf))
    {
      BinaryReader r(texBuf.data(), (int)texBuf.size());
      GenerateTexture(r);
    }
    return true;
  }
#else
  static bool OnShaderChanged(const char* filename, const char* buf, int len)
  {
    return true;
  }
#endif
  //-----------------------------------------------------------------------------
  bool Init()
  {
    g_vsFullScreen = g_Graphics->CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
        tl_common_VsQuad_bin,
        sizeof(tl_common_VsQuad_bin),
        ObjectHandle::VertexShader,
        nullptr);

    g_cbCommon =
        g_Graphics->CreateBuffer(D3D11_BIND_CONSTANT_BUFFER, sizeof(cbCommon), true, nullptr);
    ID3D11Buffer* commonCb = g_Graphics->GetResource<ID3D11Buffer>(g_cbCommon);

#define LOAD_PS(cmd, filename, bin)                                                                \
  g_Shaders[(u8)cmd] = g_Graphics->CreateShader(FW_STR("shaders/out/" filename ".pso"),            \
      bin,                                                                                         \
      sizeof(bin),                                                                                 \
      ObjectHandle::PixelShader,                                                                   \
      OnShaderChanged);

    LOAD_PS(ShaderOps::Copy, "tl_basic_PsCopyD", tl_basic_PsCopy_bin);
    LOAD_PS(ShaderOps::Fill, "tl_basic_PsFillD", tl_basic_PsFill_bin);
    LOAD_PS(ShaderOps::RadialGradient,
        "tl_gradient_PsRadialGradientD",
        tl_gradient_PsRadialGradient_bin);
    LOAD_PS(ShaderOps::LinearGradient,
        "tl_gradient_PsLinearGradientD",
        tl_gradient_PsLinearGradient_bin);
    //LOAD_PS(ShaderOps::Noise, "tl_noise_PsNoiseD", tl_noise_PsNoise_bin);
    LOAD_PS(ShaderOps::Noise, "tl_noise_PsBrickD", tl_noise_PsBrick_bin);
    LOAD_PS(ShaderOps::Modulate, "tl_arith_PsModulateD", tl_arith_PsModulate_bin);

    memset(renderTargets, 0, sizeof(renderTargets));

    // create the aux textures, and the required outputs
    for (int i = 255, j = 0; j <= NUM_AUX_TEXTURES; i = j++)
    {
      ObjectHandle hRt = g_Graphics->CreateRenderTarget(TEXTURE_SIZE, TEXTURE_SIZE, true);
      DXGraphics::RenderTarget* rt = g_Graphics->GetResource<DXGraphics::RenderTarget>(hRt);
      renderTargets[i].hRt = hRt;
      renderTargets[i].rt = rt;
      renderTargets[i].srv = g_Graphics->GetResource<ID3D11ShaderResourceView>(rt->srv);
      renderTargets[i].rtv = g_Graphics->GetResource<ID3D11RenderTargetView>(rt->rtv);
    }

#if WITH_TEXTURE_UPDATE
    g_eventNewData = CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD threadId;
    g_updateThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &threadId);
#endif

#if LOAD_FROM_FILE && WITH_FILE_UTILS
    std::vector<char> buf;
    if (LoadFile("c:/users/magnus/documents/test1.dat", &buf))
    {
      BinaryReader r(buf.data(), (int)buf.size());
      GenerateTexture(r);
    }
#endif

    g_Initialized = true;
    return true;
  }

  //-----------------------------------------------------------------------------
  void Tick()
  {
#if WITH_TEXTURE_UPDATE

#if !LOAD_FROM_FILE
    if (WaitForSingleObject(g_eventNewData, 0) == WAIT_OBJECT_0)
    {
      BinaryReader r(g_newData, g_newDataSize);
      GenerateTexture(r);
    }
#endif

#endif

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    SetupState(CD3D11_VIEWPORT(
        0.f, 0.f, (float)g_Graphics->_backBufferWidth, (float)g_Graphics->_backBufferHeight));
    ctx->PSSetShaderResources(0, 1, &renderTargets[0xff].srv);
    ctx->OMSetRenderTargets(1, &g_Graphics->_renderTargetView, nullptr);
    ctx->PSSetShader(
        g_Graphics->GetResource<ID3D11PixelShader>(g_Shaders[(u8)ShaderOps::Copy]), nullptr, 0);
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
