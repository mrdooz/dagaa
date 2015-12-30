//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

// static int mirand  = 1;

int msys_rand(int* seed)
{
  seed[0] = seed[0] * 0x343FD + 0x269EC3;
  return ((seed[0] >> 16) & 32767);
}

float msys_frand(int* seed)
{
  seed[0] = seed[0] * 0x343FD + 0x269EC3;
  // seed[0] *= 16807;

  const unsigned int a = (((unsigned int)seed[0]) >> 9) | 0x3f800000;
  const float res = (*((float*)&a)) - 1.0f;

  return (res);
}

float msys_sfrand(int* seed)
{
  seed[0] = seed[0] * 0x343FD + 0x269EC3;
  // seed[0] *= 16807;

  const unsigned int a = (((unsigned int)seed[0]) >> 9) | 0x40000000;
  const float res = (*((float*)&a)) - 3.0f;

  return (res);
}
