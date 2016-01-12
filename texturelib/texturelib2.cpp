#include "texturelib2.hpp"
#include "../sys/msys_graphics.hpp"
#include "../sys/msys_containers.hpp"
#include <shaders/out/tl_common_VsQuad.vso.hpp>
#include <shaders/out/tl_basic_PsFill.pso.hpp>
#include <shaders/out/tl_arith_PsModulate.pso.hpp>

namespace TextureLib
{
  enum Command : u8
  {
    CreateTexture,
    Modulate,
    NumCommands
  };

 struct CreateTextureVars
 {
   int id;
   s16 w;
   s16 h;
   u32 col;
 };

 struct ModulateVars
 {
   int idA;
   int idB;
   float factorA;
   float factorB;
 };

 static FixedLinearMap<Command, ObjectHandle, NumCommands> g_Shaders;
 ObjectHandle g_vsFullScreen;

 //-----------------------------------------------------------------------------
  template <typename T>
  T Read(const char** ptr)
  {
    T tmp = *(T*)(*ptr);
    *ptr += sizeof(T);
    return tmp;
  }

  //-----------------------------------------------------------------------------
  bool Init()
  {
    g_vsFullScreen = g_Graphics->CreateShader(FW_STR("shaders/out/tl_common_VsQuadD.vso"),
      tl_common_VsQuad_bin,
      sizeof(tl_common_VsQuad_bin),
      ObjectHandle::VertexShader);

#define LOAD_PS(cmd, filename, bin) \
    g_Shaders[cmd] = g_Graphics->CreateShader( \
      FW_STR("shaders/out/" filename ".pso"), \
      bin,  \
      sizeof(bin), \
      ObjectHandle::PixelShader);

    LOAD_PS(Command::CreateTexture, "tl_basic_PsFillD", tl_basic_PsFill_bin);
    LOAD_PS(Command::Modulate, "tl_arith_PsModulateD", tl_arith_PsModulate_bin);

    return true;
  }

  //-----------------------------------------------------------------------------
  void GenerateTexture(const char* buf, int len)
  {
    FixedLinearMap<int, ObjectHandle, 256> renderTargets;
      
    const char* cur = buf;
    const char* end = cur + len;
    while (cur != end)
    {
      Command cmd = Read<Command>(&cur);

      if (cmd == CreateTexture)
      {
        CreateTextureVars vars = Read<CreateTextureVars>(&cur);
        renderTargets[vars.id] = g_Graphics->CreateRenderTarget(vars.w, vars.h, &vars.col);
      }
      else if (cmd == Modulate)
      {
        ModulateVars vars = Read<ModulateVars>(&cur);
      }
    }
  }
}
