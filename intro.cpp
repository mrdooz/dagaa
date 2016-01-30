//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#include "sys/msys.h"
#include "sys/msys_graphics.hpp"
#include "intro.h"
#include "sys/shader_manifest_loader.hpp"

typedef struct
{
  long mTo;
  int mSeed;
} IntroObject;

//--------------------------------------------------------
static IntroObject intro;

#if WITH_FILE_WATCHER
#define FW_STR(x) x
#else
#define FW_STR(x) ""
#endif

int intro_init(int xr, int yr, int nomusic, IntroProgressDelegate* pd)
{
#if WITH_FILE_UTILS
  ShaderManifestLoader::Create();
  g_ShaderManifestLoader->AddManifest("c:/projects/dagaa/shaders/out/tl_gradient.manifest");
#endif
  // progress report, (from 0 to 200)
  pd->func(pd->obj, 0);

  // init your stuff here (mzk player, intro, ...)
  // remember to call pd->func() regularly to update the loading bar

  pd->func(pd->obj, 200);

  intro.mTo = msys_timerGet();
  return 1;
}

//-----------------------------------------------------------------------------
void intro_end(void)
{
}

//-----------------------------------------------------------------------------
int intro_run(ObjectHandle texture)
{
  float time = (1.0f / 1000.0f) * (float)(msys_timerGet() - intro.mTo);
  return 0;
}
