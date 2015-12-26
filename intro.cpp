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

int intro_do(void)
{
  float time = (1.0f / 1000.0f) * (float)(msys_timerGet() - intro.mTo);

  // animate your intro here

  // render your intro here
  glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBegin(GL_TRIANGLES);
  glColor4f(0.1f, 0.2f, 0.3f, 1.0f);
  glVertex3f(0.0f, 0.0f, 0.0f);
  glVertex3f(msys_sinf(time) + 0.1f * msys_frand(&intro.mSeed),
      msys_cosf(time) + 0.1f * msys_frand(&intro.mSeed),
      0.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glVertex3f(msys_sinf(time + 0.5f) + 0.1f * msys_frand(&intro.mSeed),
      msys_cosf(time + 0.5f) + 0.1f * msys_frand(&intro.mSeed),
      0.0f);
  glEnd();

  return (0);
}