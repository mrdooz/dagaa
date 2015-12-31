#include "msys_graphics.hpp"
#include "msys_libc.h"
#include "_win32/msys_filewatcherOS.hpp"

DXGraphics* g_Graphics;
ObjectHandle g_EmptyHandle;

#define HR(x)                                                                                      \
  if (FAILED(x))                                                                                   \
    return 0;

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
  swapChainDesc.SampleDesc.Quality = 0;

  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = h;
  // TODO(magnus): pass in info
  swapChainDesc.Windowed = true;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swapChainDesc.Flags = 0;

  // Get the IDXGIFactory that created the device, and use that to create the swap chain
  IDXGIDevice* dxgiDevice = 0;
  HR(_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
  IDXGIAdapter* dxgiAdapter = 0;
  HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

  // Finally got the IDXGIFactory interface.
  IDXGIFactory* dxgiFactory = 0;
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
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.MipLevels = 1;
  depthStencilDesc.ArraySize = 1;
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

  depthStencilDesc.SampleDesc.Count = 1;
  depthStencilDesc.SampleDesc.Quality = 0;

  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depthStencilDesc.CPUAccessFlags = 0;
  depthStencilDesc.MiscFlags = 0;

  HR(_device->CreateTexture2D(&depthStencilDesc, 0, &_depthStencilBuffer));
  HR(_device->CreateDepthStencilView(_depthStencilBuffer, 0, &_depthStencilView));

  _context->OMSetRenderTargets(1, &_renderTargetView, _depthStencilView);
  return 1;
}

//-----------------------------------------------------------------------------
void DXGraphics::Close()
{
#if WITH_DX_RELEASE
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
