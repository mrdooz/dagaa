#pragma once

#if WITH_FILE_UTILS

#include <string>
#include <vector>
#include <unordered_map>
#include <sys/msys_math.hpp>
#include <sys/object_handle.hpp>

namespace manifest
{
  struct Value
  {
    int iVal = 0;
    float fVal = 0.f;
    vec2 v2Val = vec2{0,0};
    vec3 v3Val = vec3{0,0,0};
    color cVal = color{0,0,0,0};
  };

  struct Var
  {
    enum class Type
    {
      Unknown,
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

    Var() : type(Type::Unknown) {}

    Type type;
    std::string name;
    u32 flags = 0;
    int memOffset = 0;
    int len;
    Value minValue, maxValue;
  };

  struct Shader
  {
    void Render(float time);
    void RenderGui();
    std::string name;
    std::string shaderFile;
    std::string root;
    std::vector<Var> vars;
    std::vector<char> memory;
    ObjectHandle ps;
    ObjectHandle cb;
  };
}

struct ShaderManifestLoader
{
  static bool Create();
  static bool Destroy();

  bool Init();

  bool AddManifest(const char* manifest);
  bool UpdateManifest(const char* manifest);
  void Render(float time);

  manifest::Shader* FindShader(const std::string& name);

  ObjectHandle _cbCommon;
  std::vector<manifest::Shader*> _shaders;
  int _curShader = 0;
};

extern ShaderManifestLoader* g_ShaderManifestLoader;
#endif