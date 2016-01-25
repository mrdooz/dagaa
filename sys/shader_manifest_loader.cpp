#include "shader_manifest_loader.hpp"
#include <lib/file_utils.hpp>

using namespace tokko;

ShaderManifestLoader tokko::g_ShaderManifestLoader;

//------------------------------------------------------------------------------
bool ShaderManifestLoader::Create()
{
  return true;
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::Destroy()
{
  return true;
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::AddManifest(const char* manifest)
{
  LineReader reader(manifest);

  bool inShader = false;
  bool inCbuffer = false;
  while (!reader.Eof())
  {
    string str = reader.Next();

    if (str == "shader-begin")
    {
      inShader = true;
    }
    else if (str == "shader-end")
    {
      inShader = false;
    }

    if (inShader)
    {
      if (str == "cbuffer-begin")
      {
        inCbuffer = true;
      }
      else if (str == "cbuffer-end")
      {
        inCbuffer = false;
      }

      if (inCbuffer)
      {

      }
    }



    OutputDebugStringA(str.c_str());
    OutputDebugStringA("\n");
  }

  return true;
}
