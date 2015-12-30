//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#ifndef _MSYS_H_
#define _MSYS_H_

#include "msys_types.h"
#include "msys_libc.h"
#include "msys_random.h"
#include "msys_malloc.h"
#include "msys_sound.h"
#include "msys_timer.h"
#include "msys_thread.h"
#include "msys_font.h"
#include "msys_debug.h"

#if WITH_OPENGL
#include "msys_glext.h"
#endif

#include <dxgi.h>
#include <d3d11.h>

int msys_init(intptr h);
void msys_end(void);

struct DXGraphics
{
  int Init(HWND h, u32 width, u32 height);
  void Clear();
  void Present();

  D3D_FEATURE_LEVEL _featureLevel;
  ID3D11Device* _device;
  ID3D11DeviceContext* _context;

  IDXGISwapChain* _swapChain;
  ID3D11RenderTargetView* _renderTargetView;
  ID3D11Texture2D* _backBuffer;

  ID3D11Texture2D* _depthStencilBuffer;
  ID3D11DepthStencilView* _depthStencilView;

  bool _vsync;
};

extern DXGraphics g_Graphics;

#endif
