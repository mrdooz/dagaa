//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

// clang-format off

float msys_log2f( const float x )
{
    float res;

    _asm fld    dword ptr [x]
    _asm fld1
    _asm fxch   st(1)
    _asm fyl2x
    _asm fstp   dword ptr [res]

    return res;
}

float msys_expf( const float x )
{
    float res;

    _asm fld     dword ptr [x]
    _asm fldl2e
    _asm fmulp   st(1), st(0)
    _asm fld1
    _asm fld     st(1)
    _asm fprem
    _asm f2xm1
    _asm faddp   st(1), st(0)
    _asm fscale
    _asm fxch    st(1)
    _asm fstp    st(0)
    _asm fstp    dword ptr [res]

    return res;
}

float msys_fmodf( const float x, const float y )
{
    float res;

    _asm fld     dword ptr [y]
    _asm fld     dword ptr [x]
    _asm fprem
    _asm fxch    st(1)
    _asm fstp    st(0)
    _asm fstp    dword ptr [res]

    return res;
}

float msys_powf( const float x, const float y )
{
    float res;

    _asm fld     dword ptr [y]
    _asm fld     dword ptr [x]
    _asm fyl2x
    _asm fld1
    _asm fld     st(1)
    _asm fprem
    _asm f2xm1
    _asm faddp   st(1), st(0)
    _asm fscale
    _asm fxch
    _asm fstp    st(0)
    _asm fstp    dword ptr [res];

    return res;
}

static short opc1 = 0x043f ;     // floor

int msys_ifloorf( const float x )
{
    int res;
    short tmp;

    _asm fstcw   word  ptr [tmp]
    _asm fld     dword ptr [x]
    _asm fldcw   word  ptr [opc1]
    _asm fistp   dword ptr [res]
    _asm fldcw   word  ptr [tmp]

    return res;
}

// clang-format on
