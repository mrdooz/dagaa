#if WITH_FILE_UTILS
#include "shader_manifest_loader.hpp"
#include <sys/msys_file.hpp>
#include <sys/object_handle.hpp>
#include <sys/msys_graphics.hpp>
#include <sys/gpu_objects.hpp>
#include <sys/_win32/msys_filewatcherOS.hpp>

ShaderManifestLoader* g_ShaderManifestLoader;
using namespace std;
using namespace manifest;

static ObjectHandle g_cbCommon;

static struct
{
  vec2 dim;
  float time;
} cbCommon;

static string VarToString(const char* mem, const Var& v)
{
  char buf[256];
  switch (v.type)
  {
    case Var::Type::Integer:
      sprintf(buf, "name: %s type: integer value: %d\n", v.name.c_str(), *(int*)&mem[v.memOffset]);
      break;
    case Var::Type::Float:
      sprintf(buf, "name: %s type: float value: %f\n", v.name.c_str(), *(float*)&mem[v.memOffset]);
      break;
    case Var::Type::Vec2:
      sprintf(buf, "name: %s type: vec2 value: %f,%f\n", v.name.c_str(), 
        *(float*)&mem[v.memOffset], *(float*)&mem[v.memOffset+4]);
      break;
    case Var::Type::Vec3:
      sprintf(buf,
          "name: %s type: vec3 value: %f,%f,%f\n",
          v.name.c_str(),
          *(float*)&mem[v.memOffset],
          *(float*)&mem[v.memOffset + 4],
          *(float*)&mem[v.memOffset + 8]);
      break;
    case Var::Type::Color:
      sprintf(buf,
        "name: %s type: color value: %f,%f,%f,%f\n",
        v.name.c_str(),
        *(float*)&mem[v.memOffset],
        *(float*)&mem[v.memOffset + 4],
        *(float*)&mem[v.memOffset + 8],
        *(float*)&mem[v.memOffset + 12]);
      break;
    default:
      ASSERT(!"Unsupported type");
      break;
  }

  return buf;
}

//------------------------------------------------------------------------------
static bool ExtractTag(const char* str, const char* tag, string* res)
{
  const char* valueStart = strstr(str, tag);
  if (!valueStart)
    return false;

  valueStart += strlen(tag);

  const char* end = str + strlen(str);

  // skip leading whitespace
  while (valueStart != end && isspace(*valueStart))
    ++valueStart;

  // traverse until first whitespace (or eol)
  const char* valueEnd = valueStart;
  while (valueEnd != end && !isspace(*valueEnd))
    ++valueEnd;

  res->assign(valueStart, valueEnd);
  return true;
}

//------------------------------------------------------------------------------
namespace manifest
{
  //------------------------------------------------------------------------------
  static int varSize(Var::Type type)
  {
    switch (type)
    {
    case Var::Type::Integer: return 4;
    case Var::Type::Float: return 4;
    case Var::Type::Vec2: return 8;
    case Var::Type::Vec3: return 12;
    case Var::Type::Color: return 16;
    }
    ASSERT(!"Unknown type");
    return 0;
  }

  //------------------------------------------------------------------------------
  void Shader::Render()
  {
    RenderGui();

    ID3D11DeviceContext* ctx = g_Graphics->_context;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //ctx->RSSetViewports(1, &viewport);
    g_Graphics->SetVertexShader(g_Graphics->_vsFullScreen);

    GpuState state;
    state.Create();
    g_Graphics->SetGpuState(state);

    cbCommon.dim.x = (float)g_Graphics->_backBufferWidth;
    cbCommon.dim.y = (float)g_Graphics->_backBufferHeight;
    cbCommon.time = 0;

    ObjectHandle hCommon = g_ShaderManifestLoader->_cbCommon;
    g_Graphics->CopyToBuffer(hCommon, (void*)&cbCommon, sizeof(cbCommon));
    g_Graphics->SetConstantBuffer(hCommon, ShaderType::PixelShader, 0);

    g_Graphics->CopyToBuffer(cb, memory.data(), memory.size());
    g_Graphics->SetConstantBuffer(cb, ShaderType::PixelShader, 1);

    g_Graphics->SetPixelShader(ps);
    g_Graphics->SetDefaultRenderTarget();
    ctx->Draw(6, 0);
  }

  //------------------------------------------------------------------------------
  void Shader::RenderGui()
  {
    ImGui::Begin(name.c_str());
    for (const Var& var : vars)
    {
      switch (var.type)
      {
      case Var::Type::Integer:
      {
        ImGui::InputInt(var.name.c_str(), (int*)&memory[var.memOffset]);
        break;
      }
      case Var::Type::Float:
      {
        if (var.flags & Var::FLAG_MINMAX)
          ImGui::SliderFloat(var.name.c_str(),
              (float*)&memory[var.memOffset],
              var.minValue.fVal,
              var.maxValue.fVal);
        else
          ImGui::InputFloat(var.name.c_str(), (float*)&memory[var.memOffset]);

        break;
      }
      case Var::Type::Vec2:
      {
        if (var.flags & Var::FLAG_MINMAX)
          ImGui::SliderFloat2(var.name.c_str(),
            (float*)&memory[var.memOffset],
            var.minValue.fVal,
            var.maxValue.fVal);
        else
          ImGui::InputFloat2(var.name.c_str(), (float*)&memory[var.memOffset]);
        break;
      }
      case Var::Type::Vec3:
      {
        ImGui::InputFloat3(var.name.c_str(), (float*)&memory[var.memOffset]);
        break;
      }
      case Var::Type::Color:
      {
        ImGui::InputFloat4(var.name.c_str(), (float*)&memory[var.memOffset]);
        break;
      }
      default: break;
      }
    }
    ImGui::End();
  }
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::Create()
{
  g_ShaderManifestLoader = new ShaderManifestLoader();
  return g_ShaderManifestLoader->Init();
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::Destroy()
{
  delete g_ShaderManifestLoader;
  g_ShaderManifestLoader = nullptr;
  return true;
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::Init()
{
  // load full screen vertex shader
  _cbCommon = g_Graphics->CreateBuffer(D3D11_BIND_CONSTANT_BUFFER, sizeof(cbCommon), true);
  return true;
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::AddManifest(const char* manifest)
{
  FileWatcherWin32::AddFileWatchResult res = g_FileWatcher->AddFileWatch(manifest,
      true,
      [this](const char* filename, const char* buf, int len) { return UpdateManifest(filename); });

  return res.initialResult;
}

namespace path
{
  void Split(const char* path, string* root, string* rest)
  {
    // returns the part (filename) after the last slash
    const char* p = strrchr(path, '/');
    const char* end = path + strlen(path);
    if (!p)
    {
      *root = path;
      *rest = string();
      return;
    }

    root->assign(path, p);
    rest->assign(p+1, end - p);
  }

  void SplitExt(const char* filename, string* root, string* ext)
  {
  }

  string Normalize(const char* path)
  {
    string res(path);
    int len = strlen(path);
    for (int i = 0; i < len; ++i)
    {
      if (res[i] == '\\')
        res[i] = '/';
    }
    return res;
  }
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::UpdateManifest(const char* manifest)
{
  string p = path::Normalize(manifest);
  string a, b;
  path::Split(p.c_str(), &a, &b);
  LineReader reader(manifest);

  bool inShader = false;
  bool inCbuffer = false;

  Shader* curShader = nullptr;
  while (!reader.Eof())
  {
    string str = reader.Next();
    const char* s = str.c_str();

    if (strstr(s, "shader-begin"))
    {
      string name, file;
      ExtractTag(s, "name:", &name);
      ExtractTag(s, "file:", &file);

      // check if the shader exists
      auto it = _shaders.find(name);
      if (it == _shaders.end())
      {
        curShader = new Shader{ name, file };
      }
      else
      {
        curShader = it->second;
        for (const Var& var : curShader->vars)
        {
          string str = VarToString(curShader->memory.data(), var);
        }
        curShader->vars.clear();
        g_Graphics->ReleaseResource(curShader->ps);
        g_Graphics->ReleaseResource(curShader->cb);
      }

      curShader->ps = g_Graphics->LoadPixelShader(file.c_str());
      inShader = true;
    }
    else if (str == "shader-end")
    {
      _shaders[curShader->name] = curShader;
      curShader = nullptr;
      inShader = false;
    }
    else if (str == "cbuffer-begin")
    {
      inCbuffer = true;
    }
    else if (str == "cbuffer-end")
    {
      // calc required memory, set var offsets, and set default values
      int ofs = 0;
      int slotsLeft = 4;
      for (Var& var : curShader->vars)
      {
        int requiredSlots = varSize(var.type) / sizeof(float);
        if (requiredSlots > slotsLeft)
        {
          // check if we need to add padding
          ofs += (requiredSlots - slotsLeft) * sizeof(float);
          slotsLeft = 4;
        }
        var.memOffset = ofs;
        ofs += varSize(var.type);
        slotsLeft -= requiredSlots;

        if (slotsLeft == 0)
          slotsLeft = 4;
      }
      curShader->memory.resize(ofs);
      curShader->cb = g_Graphics->CreateBuffer(D3D11_BIND_CONSTANT_BUFFER, ofs, true, nullptr);

      // initialize the variables
      for (Var& var : curShader->vars)
      {
        switch (var.type)
        {
          case Var::Type::Integer: *(int*)&curShader->memory[var.memOffset] = int(); break;
          case Var::Type::Float: *(float*)&curShader->memory[var.memOffset] = float(); break;
          case Var::Type::Vec2: *(vec2*)&curShader->memory[var.memOffset] = vec2(); break;
          case Var::Type::Vec3: *(vec3*)&curShader->memory[var.memOffset] = vec3(); break;
          case Var::Type::Color: *(color*)&curShader->memory[var.memOffset] = color(); break;
        }
      }

      inCbuffer = false;
    }
    else
    {
      if (inCbuffer)
      {
        Var var;
        if (ExtractTag(s, "name:", &var.name))
        {
          string type, minValue, maxValue;
          bool hasType = ExtractTag(s, "type:", &type);
          bool hasMin = ExtractTag(s, "min:", &minValue);
          bool hasMax = ExtractTag(s, "max:", &maxValue);

          if (type == "integer")
          {
            var.type = Var::Type::Integer;
          }
          else if (type == "float" || type == "vec2")
          {
            if (hasMin && hasMin)
            {
              var.minValue.fVal = (float)atof(minValue.c_str());
              var.maxValue.fVal = (float)atof(maxValue.c_str());
              var.flags |= Var::FLAG_MINMAX;
            }

            var.type = type == "float" ? Var::Type::Float : Var::Type::Vec2;
          }

          if (var.type != Var::Type::Unknown)
          {
            curShader->vars.push_back(var);
          }
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void ShaderManifestLoader::Render(float time)
{
  if (!_shaders.empty())
    _shaders.begin()->second->Render();
}

#endif