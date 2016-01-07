#include "msys_graphics.hpp"
#include "msys_libc.h"
#include "_win32/msys_filewatcherOS.hpp"
#include "../texturelib/texturelib.hpp"

#define DEFAULTS_TO_ZERO(type, val)

DXGraphics* g_Graphics;
ObjectHandle g_EmptyHandle;

#define HR(x)                                                                                      \
  if (FAILED(x))                                                                                   \
    return 0;

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
  //desc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
ObjectHandle DXGraphics::AddResource(ObjectHandle::Type type, void* resource)
{
  int idx = _firstFreeResource != -1 ? _firstFreeResource : FindFreeResource(0);
  assert(idx != -1);
  _firstFreeResource = -1;

  _resources[idx] = Resource{resource, type, 0};
  return ObjectHandle(type, idx);
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateRenderTarget(int width, int height, u32* col)
{
  D3D11_TEXTURE2D_DESC desc;
  InitTextureDesc(&desc,
    width,
    height,
    D3D11_USAGE_DEFAULT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    D3D11_BIND_SHADER_RESOURCE);

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
    data.pSysMem = (void*)tmp;
    data.SysMemPitch = sizeof(u32) * width;
    data.SysMemSlicePitch = data.SysMemPitch * height;
  }

  ID3D11Texture2D* t;
  HRESULT hr = _device->CreateTexture2D(&desc, tmp ? &data : nullptr, &t);
  delete [] tmp;
  if (FAILED(hr))
    return g_EmptyHandle;

  ObjectHandle hTexture = AddResource(ObjectHandle::RenderTarget, t);

  // create SRV
  D3D_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_TEXTURE2D;
  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(dim, desc.Format);
  ID3D11ShaderResourceView* srv;
  if (FAILED(_device->CreateShaderResourceView(t, &srvDesc, &srv)))
    return g_EmptyHandle;

  ObjectHandle hSrv = AddResource(ObjectHandle::ShaderResourceView, srv);
  return hSrv;
}

//-----------------------------------------------------------------------------
pair<ObjectHandle, ObjectHandle> DXGraphics::CreateTexture(const GenTexture* texture)
{
  D3D11_TEXTURE2D_DESC desc;
  InitTextureDesc(&desc,
      texture->width,
      texture->height,
      D3D11_USAGE_DEFAULT,
      DXGI_FORMAT_R32G32B32A32_FLOAT,
      D3D11_BIND_SHADER_RESOURCE);

  D3D11_SUBRESOURCE_DATA data;
  data.pSysMem = (void*)texture->data;
  data.SysMemPitch = 4 * sizeof(float) * texture->width;
  data.SysMemSlicePitch = data.SysMemPitch * texture->height;

  ID3D11Texture2D* t;
  if (SUCCEEDED(_device->CreateTexture2D(&desc, &data, &t)))
  {
    int n = _resourceCount;
    _resourceData[_resourceCount] = t;
    _resourceType[_resourceCount++] = ObjectHandle::Texture;

    // create SRV
    D3D_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_TEXTURE2D;
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(dim, desc.Format);
    if (SUCCEEDED(_device->CreateShaderResourceView(
            t, &srvDesc, (ID3D11ShaderResourceView**)&_resourceData[_resourceCount])))
    {
      _resourceType[_resourceCount++] = ObjectHandle::ShaderResourceView;
      return make_pair(ObjectHandle(ObjectHandle::Texture, n), ObjectHandle(ObjectHandle::ShaderResourceView, n+1));
    }
  }

  return make_pair(g_EmptyHandle, g_EmptyHandle);
}

//-----------------------------------------------------------------------------
void DXGraphics::UpdateTexture(const GenTexture* texture, ObjectHandle h)
{
  _context->UpdateSubresource(GetResource<ID3D11Resource>(h),
    0,
    nullptr,
    (void*)texture->data,
    4 * sizeof(float) * texture->width,
    4 * sizeof(float) * texture->width * texture->height);
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
    if (SUCCEEDED(_device->CreateVertexShader(
            buf, len, nullptr, (ID3D11VertexShader**)&_resourceData[_resourceCount])))
    {
      _resourceType[_resourceCount] = type;
      return ObjectHandle(type, _resourceCount++);
    }
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
    if (SUCCEEDED(_device->CreatePixelShader(
            buf, len, nullptr, (ID3D11PixelShader**)&_resourceData[_resourceCount])))
    {
      _resourceType[_resourceCount] = type;
      return ObjectHandle(type, _resourceCount++);
    }
#endif
  }
  return g_EmptyHandle;
}

//-----------------------------------------------------------------------------
ObjectHandle DXGraphics::ReserveHandle(ObjectHandle::Type type)
{
  _resourceType[_resourceCount] = type;
  return ObjectHandle(type, _resourceCount++);
}

//-----------------------------------------------------------------------------
void DXGraphics::UpdateHandle(ObjectHandle handle, const void* buf)
{
  _resourceData[handle.id] = buf;
}

//-----------------------------------------------------------------------------
int DXGraphics::Init(HWND h, u32 width, u32 height)
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

  CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
  _samplers[Linear] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  _samplers[LinearWrap] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  _samplers[LinearBorder] = CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  _samplers[Point] = CreateSamplerState(samplerDesc);

  _context->OMSetRenderTargets(1, &_renderTargetView, _depthStencilView);
  return 1;
}

//-----------------------------------------------------------------------------
void DXGraphics::Close()
{
#if WITH_DX_CLEANUP
  for (int i = 0; i < _resourceCount; ++i)
  {
#define RELEASE_RESOURCE(type, klass)                                                              \
  case ObjectHandle::type: ((klass*)_resourceData[i])->Release(); break;
    switch (_resourceType[i])
    {
      RELEASE_RESOURCE(VertexShader, ID3D11VertexShader);
      RELEASE_RESOURCE(PixelShader, ID3D11PixelShader);
      RELEASE_RESOURCE(BlendState, ID3D11BlendState);
      RELEASE_RESOURCE(RasterizeState, ID3D11RasterizerState);
      RELEASE_RESOURCE(DepthStencilState, ID3D11DepthStencilState);
    }
  }

  _depthStencilView->Release();
  _depthStencilBuffer->Release();
  _backBuffer->Release();
  _renderTargetView->Release();
  _swapChain->Release();

  _context->Release();
  _device->Release();
#endif
}

//------------------------------------------------------------------------------
ObjectHandle DXGraphics::CreateSamplerState(const D3D11_SAMPLER_DESC& desc)
{
  if (SUCCEEDED(_device->CreateSamplerState(&desc, (ID3D11SamplerState**)&_resourceData[_resourceCount])))
  {
    _resourceType[_resourceCount] = ObjectHandle::Sampler;
    return ObjectHandle(ObjectHandle::Sampler, _resourceCount++);
  }
  return g_EmptyHandle;
}
