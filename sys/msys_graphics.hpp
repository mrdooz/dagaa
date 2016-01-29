#pragma once
#include <sys/msys_math.hpp>
#include "object_handle.hpp"
#if WITH_FILE_WATCHER
#include <sys/_win32/msys_filewatcherOS.hpp>
#endif

struct GpuState;
struct GpuObjects;

//-----------------------------------------------------------------------------
enum class ShaderType
{
  VertexShader,
  PixelShader,
  ComputeShader,
  GeometryShader
};

//-----------------------------------------------------------------------------
struct DXGraphics
{
  struct RenderTarget
  {
    ObjectHandle texture;
    ObjectHandle srv;
    ObjectHandle rtv;
  };

  struct Texture
  {
    ObjectHandle texture;
    ObjectHandle srv;
  };

  DXGraphics();
  int Init(HWND h, u32 width, u32 height);
  void Close();
  void Clear();
  void Present();

  ObjectHandle CreateRenderTarget(int width, int height, bool createSrv);
  ObjectHandle CreateTexture(
      int width, int height, DXGI_FORMAT fmt, void* data, int data_width = -1);

#if WITH_FILE_UTILS
  std::pair<ObjectHandle, ObjectHandle> LoadVertexShader(const char* filename,
      D3D11_INPUT_ELEMENT_DESC* inputElements = nullptr,
      int numElements = 0);

  ObjectHandle LoadPixelShader(const char* filename);
#endif

#if WITH_FILE_WATCHER
  ObjectHandle CreateShader(const char* filename,
      const void* buf,
      int len,
      ObjectHandle::Type type,
      const FileWatcherWin32::cbFileChanged& cbChained = FileWatcherWin32::cbFileChanged());
#else
  ObjectHandle CreateShader(
    const char* filename, const void* buf, int len, ObjectHandle::Type type, const void*);
#endif

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
    if (!h.IsValid())
      return nullptr;
    return (T*)_resources[h.id].ptr;
  }

  template <>
  RenderTarget* GetResource(ObjectHandle h)
  {
    RenderTargetResource* res = (RenderTargetResource*)_resources[h.id].ptr;
    return &res->data;
  }

  void SetShaderResource(ObjectHandle h, ShaderType type, u32 slot = 0);
  void SetScissorRect(const D3D11_RECT& r);
  void SetViewport(const D3D11_VIEWPORT& viewPort);

  ObjectHandle CreateSamplerState(const D3D11_SAMPLER_DESC& desc);
  ObjectHandle AddResource(ObjectHandle::Type type, void* resource, u32 hash = 0);
  void ReleaseResource(ObjectHandle h);
  int FindFreeResource(int start);

  void SetRenderTarget(
      ObjectHandle renderTarget, ObjectHandle depthStencil, const color* clearTarget);

  bool CreateBufferInner(
      D3D11_BIND_FLAG bind, int size, bool dynamic, const void* data, ID3D11Buffer** buffer);

  ObjectHandle CreateBuffer(
    D3D11_BIND_FLAG bind, int size, bool dynamic, const void* buf);

  template <typename T>
  T* MapWriteDiscard(ObjectHandle h, int* pitch = nullptr);
  HRESULT Map(ObjectHandle h, UINT sub, D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE *res);
  void Unmap(ObjectHandle h, UINT sub = 0);

  void CopyToBuffer(ObjectHandle h, const void* data, u32 len);

  void SetConstantBuffer(ObjectHandle h, ShaderType type, int slot);
  void SetGpuObjects(const GpuObjects& objects);
  void SetGpuState(const GpuState& state);
  ObjectHandle FindByHash(u32 hash, ObjectHandle::Type type);

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

  DXGI_SWAP_CHAIN_DESC _swapChainDesc;
  IDXGISwapChain* _swapChain;
  ID3D11RenderTargetView* _renderTargetView;
  ID3D11Texture2D* _backBuffer;

  ID3D11Texture2D* _depthStencilBuffer;
  ID3D11DepthStencilView* _depthStencilView;

  CD3D11_VIEWPORT _viewport;

  ObjectHandle _defaultBackBuffer;
  ObjectHandle _defaultDepthStencil;

  int _backBufferWidth, _backBufferHeight;

  struct Resource
  {
    enum Flags {
      FlagsFree = (1 << 0),
    };

    void* ptr;
    u32 type : 8;
    u32 flags : 24;
    u32 hash;
  };

  template <typename T>
  struct ResourceData
  {
    enum Flags {
      FlagsFree = (1 << 0),
    };
    u32 flags;
    T data;
  };

  typedef ResourceData<RenderTarget> RenderTargetResource;
  typedef ResourceData<Texture> TextureResource;

  template <typename T>
  int FindFreeResourceData(const ResourceData<T>* buf);
  template <typename T>
  void InitResourceData(ResourceData<T>* buf);

  enum { MAX_NUM_RESOURCES = 1 << ObjectHandle::NUM_ID_BITS };
  Resource _resources[MAX_NUM_RESOURCES];
  int _firstFreeResource = -1;
  RenderTargetResource _renderTargets[MAX_NUM_RESOURCES];
  TextureResource _textures[MAX_NUM_RESOURCES];

  bool _vsync = true;
};

template <typename T>
T* DXGraphics::MapWriteDiscard(ObjectHandle h, int* pitch)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (!SUCCEEDED(Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    return nullptr;

  if (pitch)
    *pitch = res.RowPitch;

  return (T*)res.pData;
}

extern ObjectHandle g_DefaultBlendState;
extern ObjectHandle g_DefaultRasterizerState;
extern ObjectHandle g_DefaultDepthStencilState;
extern ObjectHandle g_DepthDisabledState;

extern DXGraphics* g_Graphics;

