//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#include "sys/msys.h"
#include "intro.h"

typedef struct
{
  long mTo;
  int mSeed;
} IntroObject;

//--------------------------------------------------------
static IntroObject intro;

int intro_init(int xr, int yr, int nomusic, IntroProgressDelegate* pd)
{
  // progress report, (from 0 to 200)
  pd->func(pd->obj, 0);

  // init your stuff here (mzk player, intro, ...)
  // remember to call pd->func() regularly to update the loading bar

  pd->func(pd->obj, 200);

  intro.mTo = msys_timerGet();
  return 1;
}

void intro_end(void)
{
  // deallicate your stuff here
}

//---------------------------------------------------------------------
int intro_run()
{
  float time = (1.0f / 1000.0f) * (float)(msys_timerGet() - intro.mTo);

  return 0;
}
