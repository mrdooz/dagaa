#pragma once
#include "object_handle.hpp"
#include <utility>

struct GenTexture;
//-----------------------------------------------------------------------------
struct DXGraphics
{
  DXGraphics();
  int Init(HWND h, u32 width, u32 height);
  void Close();
  void Clear();
  void Present();

  ObjectHandle CreateRenderTarget(int width, int height, u32* col);

  std::pair<ObjectHandle, ObjectHandle> CreateTexture(const GenTexture* texture);
  void UpdateTexture(const GenTexture* texture, ObjectHandle h);

  ObjectHandle CreateShader(const char* filename, const void* buf, int len, ObjectHandle::Type type);

  ObjectHandle CreateBlendState(const D3D11_BLEND_DESC& desc);
  ObjectHandle CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc);
  ObjectHandle CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc);

  int CreateDevice(HWND h, u32 width, u32 height);
  void CreateDefaultStates();

  ObjectHandle ReserveHandle(ObjectHandle::Type type);
  void UpdateHandle(ObjectHandle handle, void* buf);

  template <typename T>
  T* GetResource(ObjectHandle h)
  {
    return (T*)_resources[h.id].ptr;
  }

  ObjectHandle CreateSamplerState(const D3D11_SAMPLER_DESC& desc);
  ObjectHandle AddResource(ObjectHandle::Type type, void* resource);
  int FindFreeResource(int start);

  bool CreateBufferInner(
      D3D11_BIND_FLAG bind, int size, bool dynamic, const void* data, ID3D11Buffer** buffer);

  ObjectHandle CreateBuffer(
    D3D11_BIND_FLAG bind, int size, bool dynamic, const void* buf);

  template <typename T>
  T* MapWriteDiscard(ObjectHandle h, int* pitch = nullptr);
  HRESULT Map(ObjectHandle h, UINT sub, D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE *res);
  void Unmap(ObjectHandle h, UINT sub = 0);

  void CopyToBuffer(ObjectHandle h, const void* data, u32 len);

  enum Samplers
  {
    Point,
    Linear,
    LinearWrap,
    LinearBorder,
  };

  ObjectHandle _samplers[4];

  D3D_FEATURE_LEVEL _featureLevel;
  ID3D11Device* _device;
  ID3D11DeviceContext* _context;

  IDXGISwapChain* _swapChain;
  ID3D11RenderTargetView* _renderTargetView;
  ID3D11Texture2D* _backBuffer;

  ID3D11Texture2D* _depthStencilBuffer;
  ID3D11DepthStencilView* _depthStencilView;

  CD3D11_VIEWPORT _viewport;

  struct Resource
  {
    enum Flags {
      FlagsFree = (1 << 0),
    };

    void* ptr;
    u32 type : 8;
    u32 flags : 24;
  };

  enum { MAX_NUM_RESOURCES = 1 << 12 };
  Resource _resources[MAX_NUM_RESOURCES];
  int _firstFreeResource = -1;

  bool _vsync = true;
};

extern ObjectHandle g_DefaultBlendState;
extern ObjectHandle g_DefaultRasterizerState;
extern ObjectHandle g_DefaultDepthStencilState;
extern ObjectHandle g_DepthDisabledState;

extern DXGraphics* g_Graphics;

