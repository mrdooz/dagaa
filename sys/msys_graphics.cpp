#include "msys_graphics.hpp"
#include "msys_libc.h"
#include "_win32/msys_filewatcherOS.hpp"
#include "gpu_objects.hpp"
#include "msys_utils.hpp"
#include <shaders/out/tl_common_VsQuad.vso.hpp>

#if WITH_FILE_UTILS
#include <sys/msys_file.hpp>
#endif

#define DEFAULTS_TO_ZERO(type, val)
#define HR(x)                                                                                      \
  if (FAILED(x))                                                                                   \
    return 0;

DXGraphics* g_Graphics;
ObjectHandle EMPTY_OBJECT_HANDLE;

//-----------------------------------------------------------------------------
DXGraphics::DXGraphics()
{
  msys_zeromem((void*)&_resources[0], sizeof(_resources));
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
  return EMPTY_OBJECT_HANDLE;
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
  else
  {
    ASSERT(!"Unable to map buffer!");
  }
}

//-----------------------------------------------------------------------------
void DXGraphics::Clear()
{
  float black[4] = {0.1f, 0.1f, 0.1f, 0.1f};
  _context->ClearRenderTargetView(_renderTargetView, black);
  _context->ClearDepthStencilView(
      _depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
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
    D3D11_BIND_FLAG bindFlag,
    u32 cpuAccessFlags = 0)
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
  desc->CPUAccessFlags = cpuAccessFlags;

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
  ObjectHandle h = ObjectHandle(type, idx);
  return h;
}

//-----------------------------------------------------------------------------
void DXGraphics::SetPixelShader(ObjectHandle h)
{
  ASSERT(h.type == ObjectHandle::PixelShader);
  _context->PSSetShader(GetResource<ID3D11PixelShader>(h), nullptr, 0);
}

//-----------------------------------------------------------------------------
void DXGraphics::SetVertexShader(ObjectHandle h)
{
  ASSERT(h.type == ObjectHandle::VertexShader);
  _context->VSSetShader(GetResource<ID3D11VertexShader>(h), 0, 0);
}

//-----------------------------------------------------------------------------
void DXGraphics::SetConstantBuffer(ObjectHandle h, ShaderType type, int slot)
{
  ID3D11Buffer* cbs[] = { GetResource<ID3D11Buffer>(h) };
  switch (type)
  {
    case ShaderType::VertexShader: _context->VSSetConstantBuffers(slot, 1, cbs); break;
    case ShaderType::PixelShader: _context->PSSetConstantBuffers(slot, 1, cbs); break;
  }
}

//-----------------------------------------------------------------------------
void DXGraphics::SetGpuObjects(const GpuObjects& objects)
{
  u32 strides[] = { objects._vb.userdata };
  u32 ofs[] = { 0 };
  ID3D11Buffer* vbs[] = { GetResource<ID3D11Buffer>(objects._vb) };

  _context->IASetPrimitiveTopology(objects._topology);
  _context->IASetIndexBuffer(GetResource<ID3D11Buffer>(objects._ib), DXGI_FORMAT_R32_UINT, 0);
  _context->IASetInputLayout(GetResource<ID3D11InputLayout>(objects._layout));
  _context->IASetVertexBuffers(0, 1, vbs, strides, ofs);

  _context->VSSetShader(GetResource<ID3D11VertexShader>(objects._vs), NULL, 0);
  _context->PSSetShader(GetResource<ID3D11PixelShader>(objects._ps), NULL, 0);
}

//-----------------------------------------------------------------------------
void DXGraphics::SetGpuState(const GpuState& state)
{
  float blendFactor[4] = {1, 1, 1, 1};
  _context->OMSetBlendState(
      GetResource<ID3D11BlendState>(state._blendState), blendFactor, 0xffffffff);
  _context->RSSetState(GetResource<ID3D11RasterizerState>(state._rasterizerState));
  _context->OMSetDepthStencilState(GetResource<ID3D11DepthStencilState>(state._depthStencilState), 0);

  ID3D11SamplerState* sampler = GetResource<ID3D11SamplerState>(_samplers[DXGraphics::Linear]);
  _context->PSSetSamplers(1, 1, &sampler);
}

//-----------------------------------------------------------------------------
void DXGraphics::ReleaseResource(ObjectHandle h)
{
  if (!h.IsValid())
    return;

  int idx = h.id;
  if (!_resources[idx].ptr)
    return;

#define RELEASE_RESOURCE(type, klass)                                                              \
  case ObjectHandle::type:                                                                         \
    if (_resources[idx].ptr)                                                                       \
    {                                                                                              \
      ((klass*)_resources[idx].ptr)->Release();                                                    \
      _resources[idx].ptr = nullptr;                                                               \
    }                                                                                              \
    break;

  switch (_resources[idx].type)
  {
    RELEASE_RESOURCE(InputLayout, ID3D11InputLayout);
    RELEASE_RESOURCE(VertexShader, ID3D11VertexShader);
    RELEASE_RESOURCE(PixelShader, ID3D11PixelShader);

    RELEASE_RESOURCE(IndexBuffer, ID3D11Buffer);
    RELEASE_RESOURCE(VertexBuffer, ID3D11Buffer);
    RELEASE_RESOURCE(ConstantBuffer, ID3D11Buffer);

    RELEASE_RESOURCE(BlendState, ID3D11BlendState);
    RELEASE_RESOURCE(RasterizeState, ID3D11RasterizerState);
    RELEASE_RESOURCE(DepthStencilState, ID3D11DepthStencilState);

    RELEASE_RESOURCE(Texture2d, ID3D11Texture2D);
    RELEASE_RESOURCE(RenderTargetView, ID3D11RenderTargetView);
    RELEASE_RESOURCE(DepthStencilView, ID3D11DepthStencilView);
    RELEASE_RESOURCE(ShaderResourceView, ID3D11ShaderResourceView);

    RELEASE_RESOURCE(Sampler, ID3D11SamplerState);

    case ObjectHandle::RenderTargetData:
    {
      RenderTargetResource* rt = (RenderTargetResource*)_resources[idx].ptr;
      ReleaseResource(rt->texture);
      ReleaseResource(rt->rtv);
      if (rt->srv.IsValid())
        ReleaseResource(rt->srv);
      delete rt;
      break;
    }

    case ObjectHandle::TextureData:
    {
      TextureResource* t = (TextureResource*)_resources[idx].ptr;
      ReleaseResource(t->texture);
      ReleaseResource(t->srv);
      delete t;
      break;
    }

    default: ASSERT(!"Unhandled resource type!");
  }

  _firstFreeResource = idx;
  _resources[idx].ptr = nullptr;

#undef RELEASE_RESOURCE
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateRenderTarget(int width, int height, bool createSrv)
{
  D3D11_TEXTURE2D_DESC desc;

  D3D11_BIND_FLAG bindFlags = D3D11_BIND_RENDER_TARGET;
  if (createSrv)
    bindFlags = (D3D11_BIND_FLAG)(bindFlags | D3D11_BIND_SHADER_RESOURCE);

  InitTextureDesc(&desc, width, height, D3D11_USAGE_DEFAULT, DXGI_FORMAT_R8G8B8A8_UNORM, bindFlags);

  ID3D11Texture2D* t;
  if (FAILED(_device->CreateTexture2D(&desc, nullptr, &t)))
    EMPTY_OBJECT_HANDLE;

  RenderTargetResource* rtData = new RenderTargetResource();
  ObjectHandle hData = AddResource(ObjectHandle::RenderTargetData, rtData);
  rtData->texture = AddResource(ObjectHandle::Texture2d, t);

  CD3D11_RENDER_TARGET_VIEW_DESC rtDesc =
      CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2D, desc.Format);
  ID3D11RenderTargetView* rtv;
  _device->CreateRenderTargetView(t, &rtDesc, &rtv);
  rtData->rtv = AddResource(ObjectHandle::RenderTargetView, rtv);

  // create SRV
  if (createSrv)
  {
    D3D11_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_TEXTURE2D;
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(dim, desc.Format);
    ID3D11ShaderResourceView* srv;
    if (FAILED(_device->CreateShaderResourceView(t, &srvDesc, &srv)))
      return EMPTY_OBJECT_HANDLE;
    rtData->srv = AddResource(ObjectHandle::ShaderResourceView, srv);
  }

  return hData;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateTexture(
  int width, int height, DXGI_FORMAT fmt, void* data, int dataWidth)
{
  D3D11_TEXTURE2D_DESC desc;

  D3D11_BIND_FLAG bindFlags = D3D11_BIND_SHADER_RESOURCE;
  InitTextureDesc(&desc,
      width,
      height,
      data ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
      DXGI_FORMAT_R8G8B8A8_UNORM,
      bindFlags,
      data ? D3D11_CPU_ACCESS_WRITE : 0);

  ID3D11Texture2D* t;
  if (FAILED(_device->CreateTexture2D(&desc, nullptr, &t)))
    EMPTY_OBJECT_HANDLE;

  TextureResource* tData = new TextureResource();
  ObjectHandle hData = AddResource(ObjectHandle::TextureData, tData);
  tData->texture = AddResource(ObjectHandle::Texture2d, t);

  // create SRV
  D3D11_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_TEXTURE2D;
  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(dim, desc.Format);
  ID3D11ShaderResourceView* srv;
  if (FAILED(_device->CreateShaderResourceView(t, &srvDesc, &srv)))
    return EMPTY_OBJECT_HANDLE;
  tData->srv = AddResource(ObjectHandle::ShaderResourceView, srv);

  if (data)
  {
    D3D11_MAPPED_SUBRESOURCE resource;
    if (SUCCEEDED(_context->Map(t, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
    {
      uint8_t* src = (uint8_t*)data;
      uint8_t* dst = (uint8_t*)resource.pData;
      const int w = dataWidth == -1 ? width : msys_min(width, dataWidth);
      const int h = height;
      // TODO(magnus): hardcoded pitch
      const int pitch = width * 4; 

      if (pitch == resource.RowPitch)
      {
        memcpy(dst, src, h * pitch);
      }
      else
      {
        for (int i = 0; i < h; ++i)
        {
          memcpy(dst, src, pitch);
          src += pitch;
          dst += resource.RowPitch;
        }
      }
      _context->Unmap(t, 0);
    }
  }

  return hData;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateShader(const char* filename,
    const void* buf,
    int len,
    ObjectHandle::Type type,
#if WITH_FILE_WATCHER
    const FileWatcherWin32::cbFileChanged& cbChained)
#else
    const void*)
#endif
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
            if (cbChained)
              cbChained(filename, buf, len);
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
            if (cbChained)
              cbChained(filename, buf, len);
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
  return EMPTY_OBJECT_HANDLE;
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
  msys_memset(&_swapChainDesc, 0, sizeof(_swapChainDesc));

  _swapChainDesc.BufferDesc.Width = width;
  _swapChainDesc.BufferDesc.Height = height;
  _swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  _swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  _swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  _swapChainDesc.SampleDesc.Count = 1;
  DEFAULTS_TO_ZERO(_swapChainDesc.SampleDesc.Quality, 0);

  _swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  _swapChainDesc.BufferCount = 1;
  _swapChainDesc.OutputWindow = h;
  // TODO(magnus): pass in info
  _swapChainDesc.Windowed = true;

  // Get the IDXGIFactory that created the device, and use that to create the swap chain
  IDXGIDevice* dxgiDevice;
  HR(_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
  IDXGIAdapter* dxgiAdapter;
  HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

  // Finally got the IDXGIFactory interface.
  IDXGIFactory* dxgiFactory;
  HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

  // Now, create the swap chain.
  HR(dxgiFactory->CreateSwapChain(_device, &_swapChainDesc, &_swapChain));

  // Create render target view for backbuffer
  _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer);
  _device->CreateRenderTargetView(_backBuffer, 0, &_renderTargetView);
  AddResource(ObjectHandle::Texture2d, _backBuffer);
  _defaultBackBuffer = AddResource(ObjectHandle::RenderTargetView, _renderTargetView);
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
  _defaultDepthStencil = AddResource(ObjectHandle::DepthStencilView, _depthStencilView);
  AddResource(ObjectHandle::Texture2d, _depthStencilBuffer);

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

  return EMPTY_OBJECT_HANDLE;
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
  _defaultBlendState = CreateBlendState(CD3D11_BLEND_DESC(CD3D11_DEFAULT()));
  _defaultRasterizerState = CreateRasterizerState(CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT()));
  _defaultDepthStencilState = CreateDepthStencilState(CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT()));
  
  D3D11_DEPTH_STENCIL_DESC depthDescDepthDisabled = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  depthDescDepthDisabled = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  depthDescDepthDisabled.DepthEnable = FALSE;
  _depthDisabledState = CreateDepthStencilState(depthDescDepthDisabled);

  CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
  _samplers[Linear] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  _samplers[LinearWrap] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  _samplers[LinearBorder] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  _samplers[Point] = CreateSamplerState(samplerDesc);

  _vsFullScreen = CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
      tl_common_VsQuad_bin,
      sizeof(tl_common_VsQuad_bin),
      ObjectHandle::VertexShader,
      nullptr);
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::ReserveHandle(ObjectHandle::Type type)
{
  return ObjectHandle(type, FindFreeResource(0));
}

//-----------------------------------------------------------------------------
void DXGraphics::UpdateHandle(ObjectHandle handle, void* buf)
{
  _resources[handle.id] = Resource{buf, handle.type};
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

  // Release any resources that aren't added to the resource list
  _swapChain->Release();
  _context->Release();
  _device->Release();
}

//------------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateSamplerState(const D3D11_SAMPLER_DESC& desc)
{
  ID3D11SamplerState* ss;
  if (FAILED(_device->CreateSamplerState(&desc, &ss)))
    return EMPTY_OBJECT_HANDLE;

  return AddResource(ObjectHandle::Sampler, ss);
}

//------------------------------------------------------------------------------
void DXGraphics::SetDefaultRenderTarget()
{
  SetRenderTarget(_defaultBackBuffer, _defaultDepthStencil);
}

//------------------------------------------------------------------------------
void DXGraphics::SetRenderTarget(ObjectHandle rt, ObjectHandle ds)
{
  if (rt.type == ObjectHandle::RenderTargetData)
  {
    rt = GetResource<RenderTargetResource>(rt)->rtv;
  }

  ASSERT(rt.type == ObjectHandle::RenderTargetView || rt == EMPTY_OBJECT_HANDLE);
  ID3D11RenderTargetView* rtViews[] = { GetResource<ID3D11RenderTargetView>(rt) };
  ID3D11DepthStencilView* dsView = ds.IsValid() ? GetResource<ID3D11DepthStencilView>(ds) : nullptr;
  _context->OMSetRenderTargets(1, rtViews, dsView);
}

//------------------------------------------------------------------------------
void DXGraphics::SetShaderResource(ObjectHandle h, ShaderType type, u32 slot)
{
  if (h.type == ObjectHandle::RenderTargetData)
  {
    h = g_Graphics->GetResource<DXGraphics::RenderTargetResource>(h)->srv;
  }
  else if (h.type == ObjectHandle::TextureData)
  {
    h = g_Graphics->GetResource<DXGraphics::TextureResource>(h)->srv;
  }

  ASSERT(h.type == ObjectHandle::ShaderResourceView || h == EMPTY_OBJECT_HANDLE);
  ID3D11ShaderResourceView* views[] = { GetResource<ID3D11ShaderResourceView>(h) };
  
  switch (type)
  {
    case ShaderType::VertexShader: _context->VSSetShaderResources(slot, 1, views); break;
    case ShaderType::PixelShader: _context->PSSetShaderResources(slot, 1, views); break;
    case ShaderType::ComputeShader: _context->CSSetShaderResources(slot, 1, views); break;
    case ShaderType::GeometryShader: _context->GSSetShaderResources(slot, 1, views); break;
  }
}

//------------------------------------------------------------------------------
void DXGraphics::SetViewport(const D3D11_VIEWPORT& viewPort)
{
  _context->RSSetViewports(1, &viewPort);
}

//------------------------------------------------------------------------------
void DXGraphics::SetScissorRect(const D3D11_RECT& r)
{
  _context->RSSetScissorRects(1, &r);
}

//------------------------------------------------------------------------------
#if WITH_FILE_UTILS
std::pair<ObjectHandle, ObjectHandle> DXGraphics::LoadVertexShader(
    const char* filename, D3D11_INPUT_ELEMENT_DESC* inputElements, int numElements)
{
  std::vector<char> buf;
  if (LoadFile(filename, &buf))
  {
    ID3D11VertexShader* vs;
    if (SUCCEEDED(_device->CreateVertexShader(buf.data(), buf.size(), nullptr, &vs)))
    {
      ObjectHandle hVs = AddResource(ObjectHandle::VertexShader, vs);
      if (!inputElements)
        return std::make_pair(hVs, EMPTY_OBJECT_HANDLE);

      u32 ofs = 0;
      for (int i = 0; i < numElements; ++i)
      {
        D3D11_INPUT_ELEMENT_DESC& e = inputElements[i];
        e.AlignedByteOffset = ofs;
        ofs += SizeFromFormat(e.Format);
      }

      ID3D11InputLayout* layout;
      if (SUCCEEDED(_device->CreateInputLayout(
        inputElements, numElements, buf.data(), buf.size(), &layout)))
      {
        return std::make_pair(hVs, AddResource(ObjectHandle::InputLayout, layout));
      }
    }
  }
  return std::make_pair(EMPTY_OBJECT_HANDLE, EMPTY_OBJECT_HANDLE);
}

//------------------------------------------------------------------------------
ObjectHandle DXGraphics::LoadPixelShader(const char* filename)
{
  std::vector<char> buf;
  if (LoadFile(filename, &buf))
  {
    ID3D11PixelShader* ps;
    if (SUCCEEDED(_device->CreatePixelShader(buf.data(), buf.size(), nullptr, &ps)))
    {
      return AddResource(ObjectHandle::PixelShader, ps);
    }
  }
  return EMPTY_OBJECT_HANDLE;
}

#endif

