#include "texturelib2.hpp"
#include "../sys/msys_graphics.hpp"

namespace TextureLib
{
  enum Command : u8
  {
    CreateTexture,
    Modulate,
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

};

namespace TextureLib
{
  template <typename T>
  T Read(const char** ptr)
  {
    T tmp = *(T*)(*ptr);
    *ptr += sizeof(T);
    return tmp;
  }

  void GenerateTexture(const char* buf, int len)
  {
      
    const char* cur = buf;
    const char* end = cur + len;
    while (cur != end)
    {
      Command cmd = Read<Command>(&cur);

      if (cmd == CreateTexture)
      {
        CreateTextureVars vars = Read<CreateTextureVars>(&cur);
        g_Graphics->CreateRenderTarget(vars.w, vars.h, &vars.col);
      }
      else if (cmd == Modulate)
      {
        ModulateVars vars = Read<ModulateVars>(&cur);
      }
    }
  }
}
