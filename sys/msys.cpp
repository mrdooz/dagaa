//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#include "msys_glext.h"
#include "msys_malloc.h"
#include "msys_font.h"
#include "msys_types.h"
#include "msys_libc.h"
#include "msys.h"

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dxgi.lib")

#define HR(x)                                                                                      \
  if (FAILED(x))                                                                                   \
    return 0;


void DXGraphics::Clear()
{
  float black[4] = {1, 0, 1, 1};
  _context->ClearRenderTargetView(_renderTargetView, black);
}

void DXGraphics::Present()
{
  _swapChain->Present(_vsync ? 1 : 0, 0);
}

int DXGraphics::Init(HWND h, u32 width, u32 height)
{
  u32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if DEBUG
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

DXGraphics g_Graphics;

int msys_directx_init(intptr h)
{
  return g_Graphics.Init(HWND(h), 800, 600);
}

int msys_init(intptr h)
{

#if WITH_OPENGL
  if (!msys_glextInit())
    return 0;
#endif

  if (!msys_directx_init(h))
    return 0;

  if (!msys_mallocInit())
    return 0;

  msys_fontInit(h);

  return 1;
}

void msys_end(void)
{
  msys_mallocEnd();
}
