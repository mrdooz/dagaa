#include "msys_graphics.hpp"
#include "msys_libc.h"
#include "_win32/msys_filewatcherOS.hpp"
#include "../texturelib/texturelib.hpp"

#define DEFAULTS_TO_ZERO(type, val)

DXGraphics* g_Graphics;
ObjectHandle g_EmptyHandle;

ObjectHandle g_DefaultBlendState;
ObjectHandle g_DefaultRasterizerState;
ObjectHandle g_DefaultDepthStencilState;
ObjectHandle g_DepthDisabledState;

#define HR(x)                                                                                      \
  if (FAILED(x))                                                                                   \
    return 0;

//------------------------------------------------------------------------------
static u32 FnvHash(const char* str, int len, u32 d = 0)
{
  if (d == 0)
    d = 0x01000193;

  for (int i = 0; i < len; ++i)
  {
    char c = str[i];
    d = ((d * 0x01000193) ^ c) & 0xffffffff;
  }
  return d;
}

//------------------------------------------------------------------------------
template <typename T>
u32 CalcHash(const T& t)
{
  return FnvHash((const char*)&t, sizeof(T));
}

//------------------------------------------------------------------------------
bool DXGraphics::CreateBufferInner(
    D3D11_BIND_FLAG bind, int size, bool dynamic, const void* data, ID3D11Buffer** buffer)
{
  CD3D11_BUFFER_DESC desc(((size + 15) & ~0xf),
    bind,
    dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
    dynamic ? D3D11_CPU_ACCESS_WRITE : 0);

  HRESULT hr;
  if (data)
  {
    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(init_data));
    init_data.pSysMem = data;
    hr = _device->CreateBuffer(&desc, &init_data, buffer);
  }
  else
  {
    hr = _device->CreateBuffer(&desc, nullptr, buffer);
  }

  return SUCCEEDED(hr);
}

//------------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateBuffer(D3D11_BIND_FLAG bind, int size, bool dynamic, const void* buf)
{
  ID3D11Buffer* buffer = 0;
  if (CreateBufferInner(bind, size, dynamic, buf, &buffer))
  {
    if (bind == D3D11_BIND_INDEX_BUFFER)
    {
      return AddResource(ObjectHandle::IndexBuffer, buffer);
    }
    else if (bind == D3D11_BIND_VERTEX_BUFFER)
    {
      return AddResource(ObjectHandle::VertexBuffer, buffer);
    }
    else if (bind == D3D11_BIND_CONSTANT_BUFFER)
    {
      return AddResource(ObjectHandle::ConstantBuffer, buffer);
    }
    else
    {
      // LOG_ERROR_LN("Implement me!");
    }
  }
  return g_EmptyHandle;
}

//------------------------------------------------------------------------------
HRESULT DXGraphics::Map(
  ObjectHandle h,
  UINT sub,
  D3D11_MAP type,
  UINT flags,
  D3D11_MAPPED_SUBRESOURCE *res)
{
  return _context->Map(GetResource<ID3D11Resource>(h), sub, type, flags, res);
}

//------------------------------------------------------------------------------
void DXGraphics::Unmap(ObjectHandle h, UINT sub)
{
  return _context->Unmap(GetResource<ID3D11Resource>(h), sub);
}

//------------------------------------------------------------------------------
void DXGraphics::CopyToBuffer(ObjectHandle h, const void* data, u32 len)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (SUCCEEDED(Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
  {
    memcpy(res.pData, data, len);
    Unmap(h, 0);
  }
}

//-----------------------------------------------------------------------------
DXGraphics::DXGraphics()
{
  msys_zeromem((void*)&_resources[0], sizeof(_resources));
}

//-----------------------------------------------------------------------------
void DXGraphics::Clear()
{
  float black[4] = {0.1f, 0.1f, 0.1f, 0.1f};
  _context->ClearRenderTargetView(_renderTargetView, black);
}

//-----------------------------------------------------------------------------
void DXGraphics::Present()
{
  _swapChain->Present(_vsync ? 1 : 0, 0);
}

//-----------------------------------------------------------------------------
static void InitTextureDesc(D3D11_TEXTURE2D_DESC* desc,
    u32 width,
    u32 height,
    D3D11_USAGE usage,
    DXGI_FORMAT fmt,
    D3D11_BIND_FLAG bindFlag)
{
  msys_memset(desc, 0, sizeof(D3D11_TEXTURE2D_DESC));
  desc->Width = width;
  desc->Height = height;
  desc->MipLevels = 1;
  desc->ArraySize = 1;

  desc->SampleDesc.Count = 1;
  DEFAULTS_TO_ZERO(desc->SampleDesc.Quality, 0);
  desc->Usage = usage;
  desc->Format = fmt;
  desc->BindFlags = bindFlag;

  DEFAULTS_TO_ZERO(desc->CPUAccessFlags, 0);
  DEFAULTS_TO_ZERO(desc->MiscFlags, 0);
}

//-----------------------------------------------------------------------------
int DXGraphics::FindFreeResource(int start)
{
  for (int i = start; i < MAX_NUM_RESOURCES; ++i)
  {
    if (_resources[i].ptr == nullptr)
      return i;
  }
  return -1;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::AddResource(ObjectHandle::Type type, void* resource, u32 hash)
{
  int idx = _firstFreeResource != -1 ? _firstFreeResource : FindFreeResource(0);
  ASSERT(idx != -1);
  _firstFreeResource = -1;

  _resources[idx] = Resource{resource, type, 0, hash};
  return ObjectHandle(type, idx);
}

//-----------------------------------------------------------------------------
void DXGraphics::SetGpuState(const GpuState& state)
{
  // TODO(magnus): hmm..
  //float blendFactor[4] = { 1, 1, 1, 1 };
  //context->OMSetBlendState(
  //  g_Graphics->GetResource<ID3D11BlendState>(g_DefaultBlendState), blendFactor, 0xffffffff);
  //context->RSSetState(g_Graphics->GetResource<ID3D11RasterizerState>(g_DefaultRasterizerState));
  //context->OMSetDepthStencilState(
  //  g_Graphics->GetResource<ID3D11DepthStencilState>(g_DepthDisabledState), 0);

  //ID3D11SamplerState* sampler =
  //  g_Graphics->GetResource<ID3D11SamplerState>(g_Graphics->_samplers[DXGraphics::Linear]);
  //context->PSSetSamplers(1, 1, &sampler);
}

//-----------------------------------------------------------------------------
void DXGraphics::ReleaseResource(ObjectHandle h)
{
  int idx = h.id;
  if (!_resources[idx].ptr)
    return;

#define RELEASE_RESOURCE(type, klass)                                                              \
  case ObjectHandle::type: ((klass*)_resources[idx].ptr)->Release(); break;

  switch (_resources[idx].type)
  {
    RELEASE_RESOURCE(VertexShader, ID3D11VertexShader);
    RELEASE_RESOURCE(PixelShader, ID3D11PixelShader);

    RELEASE_RESOURCE(IndexBuffer, ID3D11Buffer);
    RELEASE_RESOURCE(VertexBuffer, ID3D11Buffer);
    RELEASE_RESOURCE(ConstantBuffer, ID3D11Buffer);

    RELEASE_RESOURCE(BlendState, ID3D11BlendState);
    RELEASE_RESOURCE(RasterizeState, ID3D11RasterizerState);
    RELEASE_RESOURCE(DepthStencilState, ID3D11DepthStencilState);

    RELEASE_RESOURCE(Texture, ID3D11Texture2D);
    RELEASE_RESOURCE(RenderTarget, ID3D11RenderTargetView);
    RELEASE_RESOURCE(ShaderResourceView, ID3D11ShaderResourceView);

    RELEASE_RESOURCE(Sampler, ID3D11SamplerState);

  default: ASSERT(!"Unhandled resource type!");
  }

  _firstFreeResource = idx;
  _resources[idx].ptr = nullptr;

#undef RELEASE_RESOURCE
}

//-----------------------------------------------------------------------------
bool DXGraphics::CreateRenderTarget(int width,
    int height,
    u32* col,
    ObjectHandle* outTexture,
    ObjectHandle* outRt,
    ObjectHandle* outSrv)
{
  D3D11_TEXTURE2D_DESC desc;

  D3D11_BIND_FLAG bindFlags = D3D11_BIND_RENDER_TARGET;
  if (outSrv)
    bindFlags = (D3D11_BIND_FLAG)(bindFlags | D3D11_BIND_SHADER_RESOURCE);

  InitTextureDesc(&desc, width, height, D3D11_USAGE_DEFAULT, DXGI_FORMAT_R8G8B8A8_UNORM, bindFlags);

  D3D11_SUBRESOURCE_DATA data;
  u32* tmp = nullptr;
  if (col)
  {
    int len = width * height;
    tmp = new u32[len];
    for (int i = 0; i < len; ++i)
    {
      tmp[i] = *col;
    }
    data.pSysMem = tmp;
    data.SysMemPitch = sizeof(u32) * width;
    data.SysMemSlicePitch = data.SysMemPitch * height;
  }

  ID3D11Texture2D* t;
  HRESULT hr = _device->CreateTexture2D(&desc, tmp ? &data : nullptr, &t);
  delete [] tmp;
  if (FAILED(hr))
    return false;

  ObjectHandle hTexture = AddResource(ObjectHandle::Texture, t);

  // TODO: this is broken.. we need to create a rtv as well
  CD3D11_RENDER_TARGET_VIEW_DESC rtDesc =
      CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2D, desc.Format);
  ID3D11RenderTargetView* rtv;
  _device->CreateRenderTargetView(t, &rtDesc, &rtv);
  *outRt = AddResource(ObjectHandle::RenderTarget, rtv);

  // create SRV
  D3D11_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_TEXTURE2D;
  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(dim, desc.Format);
  ID3D11ShaderResourceView* srv;
  if (FAILED(_device->CreateShaderResourceView(t, &srvDesc, &srv)))
    return false;

  *outTexture = hTexture;
  *outSrv = AddResource(ObjectHandle::ShaderResourceView, srv);

  return true;

}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateShader(
    const char* filename, const void* buf, int len, ObjectHandle::Type type)
{
#if WITH_FILE_WATCHER
  ObjectHandle h = ReserveHandle(type);
#endif
  if (type == ObjectHandle::VertexShader)
  {
#if WITH_FILE_WATCHER
    g_FileWatcher->AddFileWatch(filename,
        true,
        [=, this](const char* filename, const char* buf, int len)
        {
          ID3D11VertexShader* vs;
          if (SUCCEEDED(_device->CreateVertexShader(buf, len, nullptr, &vs)))
          {
            UpdateHandle(h, vs);
            return true;
          }
          return false;
        });
    return h;
#else
    ID3D11VertexShader* vs;
    if (SUCCEEDED(_device->CreateVertexShader(buf, len, nullptr, &vs)))
      return AddResource(type, vs);
#endif
  }
  else if (type == ObjectHandle::PixelShader)
  {
#if WITH_FILE_WATCHER
    g_FileWatcher->AddFileWatch(filename,
        true,
        [=, this](const char* filename, const char* buf, int len)
        {
          ID3D11PixelShader* ps;
          if (SUCCEEDED(_device->CreatePixelShader(buf, len, nullptr, &ps)))
          {
            UpdateHandle(h, ps);
            return true;
          }
          return false;
        });
    return h;
#else
    ID3D11PixelShader* ps;
    if (SUCCEEDED(_device->CreatePixelShader(buf, len, nullptr, &ps)))
      return AddResource(type, ps);
#endif
  }
  return g_EmptyHandle;
}

//-----------------------------------------------------------------------------
int DXGraphics::CreateDevice(HWND h, u32 width, u32 height)
{
  u32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
  // NB: The requires enabling "Graphics Tools" on win10
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  // Create device
  HR(D3D11CreateDevice(nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    0,
    flags,
    0,
    0,
    D3D11_SDK_VERSION,
    &_device,
    &_featureLevel,
    &_context));

  // Create swap chain
  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  msys_memset(&swapChainDesc, 0, sizeof(swapChainDesc));

  swapChainDesc.BufferDesc.Width = width;
  swapChainDesc.BufferDesc.Height = height;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  swapChainDesc.SampleDesc.Count = 1;
  DEFAULTS_TO_ZERO(swapChainDesc.SampleDesc.Quality, 0);

  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = h;
  // TODO(magnus): pass in info
  swapChainDesc.Windowed = true;

  // Get the IDXGIFactory that created the device, and use that to create the swap chain
  IDXGIDevice* dxgiDevice;
  HR(_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
  IDXGIAdapter* dxgiAdapter;
  HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

  // Finally got the IDXGIFactory interface.
  IDXGIFactory* dxgiFactory;
  HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

  // Now, create the swap chain.
  HR(dxgiFactory->CreateSwapChain(_device, &swapChainDesc, &_swapChain));

  // Create render target view for backbuffer
  _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer);
  _device->CreateRenderTargetView(_backBuffer, 0, &_renderTargetView);
  _backBuffer->Release();

  dxgiDevice->Release();
  dxgiAdapter->Release();
  dxgiFactory->Release();

  // Create depth/stencil
  D3D11_TEXTURE2D_DESC depthStencilDesc;
  InitTextureDesc(&depthStencilDesc,
    width,
    height,
    D3D11_USAGE_DEFAULT,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    D3D11_BIND_DEPTH_STENCIL);

  HR(_device->CreateTexture2D(&depthStencilDesc, 0, &_depthStencilBuffer));
  HR(_device->CreateDepthStencilView(_depthStencilBuffer, 0, &_depthStencilView));

  _viewport = CD3D11_VIEWPORT(0.f, 0.f, (float)width, (float)height);

  return 1;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::FindByHash(u32 hash, ObjectHandle::Type type)
{
  for (int i = 0; i < MAX_NUM_RESOURCES; ++i)
  {
    if (_resources[i].ptr && _resources[i].type == type && _resources[i].hash == hash)
      return ObjectHandle(type, i);
  }

  return g_EmptyHandle;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc)
{
  u32 hash = CalcHash(desc);
  ObjectHandle h = FindByHash(hash, ObjectHandle::RasterizeState);
  if (h.IsValid())
    return h;

  ID3D11RasterizerState* rasterizerState;
  _device->CreateRasterizerState(&desc, &rasterizerState);
  return AddResource(ObjectHandle::RasterizeState, rasterizerState, hash);
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc)
{
  u32 hash = CalcHash(desc);
  ObjectHandle h = FindByHash(hash, ObjectHandle::DepthStencilState);
  if (h.IsValid())
    return h;

  ID3D11DepthStencilState* depthStencilState;
  _device->CreateDepthStencilState(&desc, &depthStencilState);
  return AddResource(ObjectHandle::DepthStencilState, depthStencilState, hash);
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateBlendState(const D3D11_BLEND_DESC& desc)
{
  u32 hash = CalcHash(desc);
  ObjectHandle h = FindByHash(hash, ObjectHandle::BlendState);
  if (h.IsValid())
    return h;

  ID3D11BlendState* blendState;
  _device->CreateBlendState(&desc, &blendState);
  return AddResource(ObjectHandle::BlendState, blendState, hash);
}

//-----------------------------------------------------------------------------
void DXGraphics::CreateDefaultStates()
{
  g_DefaultBlendState = CreateBlendState(CD3D11_BLEND_DESC(CD3D11_DEFAULT()));
  g_DefaultRasterizerState = CreateRasterizerState(CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT()));
  g_DefaultDepthStencilState = CreateDepthStencilState(CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT()));
  
  D3D11_DEPTH_STENCIL_DESC depthDescDepthDisabled = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  depthDescDepthDisabled = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  depthDescDepthDisabled.DepthEnable = FALSE;
  g_DepthDisabledState = CreateDepthStencilState(depthDescDepthDisabled);

  CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
  _samplers[Linear] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  _samplers[LinearWrap] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  _samplers[LinearBorder] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  _samplers[Point] = CreateSamplerState(samplerDesc);
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::ReserveHandle(ObjectHandle::Type type)
{
  return ObjectHandle(type, FindFreeResource(0));
}

//-----------------------------------------------------------------------------
void DXGraphics::UpdateHandle(ObjectHandle handle, void* buf)
{
  _resources[handle.id] = Resource{buf, handle.type, 0};
}

//-----------------------------------------------------------------------------
int DXGraphics::Init(HWND h, u32 width, u32 height)
{
  _backBufferWidth = width;
  _backBufferHeight = height;
  // TODO(magnus): meh, this is all messed up..
  int res = CreateDevice(h, width, height);
  CreateDefaultStates();
  _context->OMSetRenderTargets(1, &_renderTargetView, _depthStencilView);
  return 1;
}

//-----------------------------------------------------------------------------
void DXGraphics::Close()
{
  for (int i = 0; i < MAX_NUM_RESOURCES; ++i)
  {
    ReleaseResource(ObjectHandle((ObjectHandle::Type)_resources[i].type, i));
  }

  _depthStencilView->Release();
  _depthStencilBuffer->Release();
  _backBuffer->Release();
  _renderTargetView->Release();
  _swapChain->Release();

  _context->Release();
  _device->Release();
}

//------------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateSamplerState(const D3D11_SAMPLER_DESC& desc)
{
  ID3D11SamplerState* ss;
  if (FAILED(_device->CreateSamplerState(&desc, &ss)))
    return g_EmptyHandle;

  return AddResource(ObjectHandle::Sampler, ss);
}
