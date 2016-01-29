#if WITH_FILE_UTILS
#include "shader_manifest_loader.hpp"
#include <sys/msys_file.hpp>
#include <sys/msys_math.hpp>

ShaderManifestLoader g_ShaderManifestLoader;
using namespace std;

namespace manifest
{
  struct Value
  {
    int iVal;
    float fVal;
    vec2 v2Val;
    vec3 v3Val;
    color cVal;
  };

  struct Var
  {
    enum class Type
    {
      Integer,
      Float,
      Vec2,
      Vec3,
      Color,
    };

    enum Flags
    {
      FLAG_MINMAX = (1 << 0)
    };

    Type type;
    u32 flags = 0;
    void* data = nullptr;
    int len;
    Value minValue, maxValue;
  };

  struct Shader
  {
    void RenderGui();
    string name;
    string shaderFile;
    vector<Var> vars;
  };

  void Shader::RenderGui()
  {
    for (const Var& var : vars)
    {
      switch (var.type)
      {
        case Var::Type::Integer: break;
        case Var::Type::Float: break;
        case Var::Type::Vec2: break;
        case Var::Type::Vec3: break;
        case Var::Type::Color: break;
        default: break;
      }
    }
  }

  vector<Shader> shaders;
}

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
static string ExtractTagValue(const char* str, const char* tag)
{
  const char* valueStart = strstr(str, tag);
  if (!valueStart)
    return string();

  valueStart += strlen(tag);

  const char* end = str + strlen(str);

  // skip leading whitespace
  while (valueStart != end && isspace(*valueStart))
    ++valueStart;

  // traverse until first whitespace (or eol)
  const char* valueEnd = valueStart;
  while (valueEnd != end && !isspace(*valueEnd))
    ++valueEnd;

  return string(valueStart, valueEnd);
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
      else if (inCbuffer)
      {
        const char* s = str.c_str();
        string name = ExtractTagValue(s, "name:");
        string type = ExtractTagValue(s, "type:");
        string range = ExtractTagValue(s, "range:");
      }
    }
  }

  return true;
}

#endif