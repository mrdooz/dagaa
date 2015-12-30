//--------------------------------------------------------------------------//
// iq . 2003/2008 . code for 64 kb intros by RGBA                           //
//--------------------------------------------------------------------------//

#if WITH_OPENGL

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <GL/gl.h>
#include "glext.h"

#include <string.h>

//--- d a t a ---------------------------------------------------------------
#include "msys_glext.h"

static char* funciones = {
    // multitexture
    "glActiveTextureARB\x0"
    "glClientActiveTextureARB\x0"
    "glMultiTexCoord4fvARB\x0"
    // programs
    "glDeleteProgramsARB\x0"
    "glBindProgramARB\x0"
    "glProgramStringARB\x0"
    "glProgramLocalParameter4fvARB\x0"
    "glProgramEnvParameter4fvARB\x0"
    // textures 3d
    "glTexImage3D\x0"
    // vbo-ibo
    "glBindBufferARB\x0"
    "glBufferDataARB\x0"
    "glBufferSubDataARB\x0"
    "glDeleteBuffersARB\x0"

    // shader
    "glCreateProgram\x0"
    "glCreateShader\x0"
    "glShaderSource\x0"
    "glCompileShader\x0"
    "glAttachShader\x0"
    "glLinkProgram\x0"
    "glUseProgram\x0"
    "glUniform4fv\x0"
    "glUniform1i\x0"
    "glGetUniformLocationARB\x0"
    "glGetObjectParameterivARB\x0"
    "glGetInfoLogARB\x0"

    "glLoadTransposeMatrixf\x0"

    //"glIsRenderbufferEXT\x0"
    "glBindRenderbufferEXT\x0"
    "glDeleteRenderbuffersEXT\x0"
    //"glGenRenderbuffersEXT\x0"
    "glRenderbufferStorageEXT\x0"
    //"glGetRenderbufferParameterivEXT\x0"
    //"glIsFramebufferEXT\x0"
    "glBindFramebufferEXT\x0"
    "glDeleteFramebuffersEXT\x0"
    //"glGenFramebuffersEXT\x0"
    "glCheckFramebufferStatusEXT\x0"
    "glFramebufferTexture1DEXT\x0"
    "glFramebufferTexture2DEXT\x0"
    "glFramebufferTexture3DEXT\x0"
    "glFramebufferRenderbufferEXT\x0"
    //"glGetFramebufferAttachmentParameterivEXT\x0"
    "glGenerateMipmapEXT\x0"

};

void* msys_oglfunc[NUMFUNCIONES];

//--- c o d e ---------------------------------------------------------------

int msys_glextInit(void)
{
  char* str = funciones;
  for (int i = 0; i < NUMFUNCIONES; i++)
  {
    msys_oglfunc[i] = wglGetProcAddress(str);

    str += 1 + strlen(str);

    if (!msys_oglfunc[i])
      return (0);
  }

  return (1);
}
#endif