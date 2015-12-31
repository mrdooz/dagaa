//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#include "msys_malloc.h"
#include "msys_font.h"
#include "msys_types.h"
#include "msys_libc.h"
#include "msys.h"
#include "msys_graphics.hpp"

int msys_init(intptr h)
{
  g_Graphics = new DXGraphics();
  if (!g_Graphics->Init(HWND(h), 800, 600))
    return 0;

  if (!msys_mallocInit())
    return 0;

  msys_fontInit(h);

  return 1;
}

void msys_end(void)
{
  g_Graphics->Close();
  delete g_Graphics;
  msys_mallocEnd();
}
