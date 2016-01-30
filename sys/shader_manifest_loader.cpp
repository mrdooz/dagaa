#if WITH_FILE_UTILS
#include "shader_manifest_loader.hpp"
#include <sys/msys_file.hpp>
#include <sys/object_handle.hpp>
#include <sys/msys_graphics.hpp>
#include <sys/gpu_objects.hpp>
#include <sys/_win32/msys_filewatcherOS.hpp>
#include <algorithm>

ShaderManifestLoader* g_ShaderManifestLoader;
using namespace std;
using namespace manifest;

static ObjectHandle g_cbCommon;
static const char* DATA_DIR = "c:/projects/dagaa/data";

static struct
{
  vec2 dim;
  float time;
} cbCommon;

// just mimicing the Python module naming here
namespace os_path
{
  void SplitLast(const char* path, char delim, string* root, string* rest)
  {
    const char* p = strrchr(path, delim);
    const char* end = path + strlen(path);
    if (!p)
    {
      if (root)
        *root = path;
      if (rest)
        *rest = string();
      return;
    }

    if (root)
      root->assign(path, p);
    if (rest)
      rest->assign(p + 1, end - p);
  }

  void Split(const char* path, string* root, string* rest)
  {
    // returns the part (filename) after the last slash
    SplitLast(path, '/', root, rest);
  }

  void SplitExt(const char* filename, string* root, string* ext)
  {
    SplitLast(filename, '.', root, ext);
  }

  string Join(const char* path, const char* next)
  {
    int len = strlen(path);
    if (len)
    {
      if (path[0] != '/')
        return string(path) + '/' + next;
      return string(path) + next;
    }
    return next;
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
static string VarTypeToString(Var::Type type)
{
  switch (type)
  {
    case Var::Type::Integer: return "integer";
    case Var::Type::Float: return "float";
    case Var::Type::Vec2: return "vec2";
    case Var::Type::Vec3: return "vec3";
    case Var::Type::Color: return "color";
  }
  return "unknown";
}

//------------------------------------------------------------------------------
static Var::Type StringToVarType(const string& str)
{
  if (str == "integer")
    return Var::Type::Integer;

  if (str == "float")
    return Var::Type::Float;

  if (str == "vec2")
    return Var::Type::Vec2;

  if (str == "vec3")
    return Var::Type::Vec3;

  if (str == "color")
    return Var::Type::Color;

  return Var::Type::Unknown;
}

//------------------------------------------------------------------------------
static string VarToString(const char* memory, const Var& v)
{
  auto& mem = [&](int ofs) { return &memory[v.memOffset + ofs]; };
  string typeStr = VarTypeToString(v.type);
  const char* type = typeStr.c_str();

  char buf[256];
  switch (v.type)
  {
    case Var::Type::Integer:
      sprintf(buf, "name: %s type: %s value: %d\n", v.name.c_str(), type, *(int*)mem(0));
      break;
    case Var::Type::Float:
      sprintf(buf, "name: %s type: %s value: %f\n", v.name.c_str(), type, *(float*)mem(0));
      break;
    case Var::Type::Vec2:
      sprintf(buf,
          "name: %s type: %s value: %f,%f\n",
          v.name.c_str(),
          type,
          *(float*)mem(0),
          *(float*)mem(4));
      break;
    case Var::Type::Vec3:
      sprintf(buf,
          "name: %s type: %s value: %f,%f,%f\n",
          v.name.c_str(),
          type,
          *(float*)mem(0),
          *(float*)mem(4),
          *(float*)mem(8));
      break;
    case Var::Type::Color:
      sprintf(buf,
        "name: %s type: %s value: %f,%f,%f,%f\n",
        v.name.c_str(),
        type,
        *(float*)mem(0),
        *(float*)mem(4),
        *(float*)mem(8),
        *(float*)mem(12));
      break;
    default:
      ASSERT(!"Unsupported type");
      break;
  }

  return buf;
}

//------------------------------------------------------------------------------
static void StringToVar(Shader* shader, const char* str)
{
  string name, type, value;
  if (ExtractTag(str, "name: ", &name) && ExtractTag(str, "type: ", &type) && ExtractTag(str, "value: ", &value))
  {
    Var::Type varType = StringToVarType(type);
    // check if the var exists
    for (Var& var : shader->vars)
    {
      auto& mem = [&](int ofs) { return &shader->memory[var.memOffset+ofs]; };

      if (var.name == name && var.type == varType)
      {
        switch (varType)
        {
          case Var::Type::Integer:
            sscanf(value.c_str(), "%d", (int*)mem(0));
            break;
          case Var::Type::Float:
            sscanf(value.c_str(), "%f", (float*)mem(0));
            break;
          case Var::Type::Vec2:
            sscanf(value.c_str(), "%f,%f", (float*)mem(0), (float*)mem(4));
            break;
          case Var::Type::Vec3:
            sscanf(value.c_str(), "%f,%f,%f", (float*)mem(0), (float*)mem(4), (float*)mem(8));
            break;
          case Var::Type::Color:
            sscanf(value.c_str(),
                "%f,%f,%f,%f",
                (float*)mem(0),
                (float*)mem(4),
                (float*)mem(8),
                (float*)mem(12));
            break;
        }
      }
    }
  }
  else
  {
    // error
  }
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
  void Shader::Render(float time)
  {
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
    cbCommon.time = time;

    ObjectHandle hCommon = g_ShaderManifestLoader->_cbCommon;
    g_Graphics->CopyToBuffer(hCommon, (void*)&cbCommon, sizeof(cbCommon));
    g_Graphics->SetConstantBuffer(hCommon, ShaderType::PixelShader, 0);

    if (memory.size())
    {
      g_Graphics->CopyToBuffer(cb, memory.data(), memory.size());
      g_Graphics->SetConstantBuffer(cb, ShaderType::PixelShader, 1);
    }

    g_Graphics->SetPixelShader(ps);
    g_Graphics->SetDefaultRenderTarget();
    ctx->Draw(6, 0);
  }

  //------------------------------------------------------------------------------
  void Shader::RenderGui()
  {
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
        if (var.flags & Var::FLAG_MINMAX)
          ImGui::SliderFloat3(var.name.c_str(),
            (float*)&memory[var.memOffset],
            var.minValue.fVal,
            var.maxValue.fVal);
        else
          ImGui::InputFloat3(var.name.c_str(), (float*)&memory[var.memOffset]);
        break;
      }
      case Var::Type::Color:
      {
        ImGui::ColorEdit4(var.name.c_str(), (float*)&memory[var.memOffset]);
        break;
      }
      default: break;
      }
    }
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

//------------------------------------------------------------------------------
Shader* ShaderManifestLoader::FindShader(const string& name)
{
  auto it = find_if(
      _shaders.begin(), _shaders.end(), [&](Shader* shader) { return shader->name == name; });
  return it == _shaders.end() ? nullptr : *it;
}

//------------------------------------------------------------------------------
bool ShaderManifestLoader::UpdateManifest(const char* manifest)
{
  string path = os_path::Normalize(manifest);
  string filename, root, ext;
  os_path::Split(path.c_str(), nullptr, &filename);
  os_path::SplitExt(filename.c_str(), &root, &ext);
  
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

      curShader = new Shader{ name, file, root };
      curShader->ps = g_Graphics->LoadPixelShader(file.c_str());
      inShader = true;
    }
    else if (str == "shader-end")
    {
      // if we had an old version of the shader, copy over any old var values
      Shader* oldShader = FindShader(curShader->name);
      if (oldShader)
      {
        g_Graphics->ReleaseResource(oldShader->ps);
        g_Graphics->ReleaseResource(oldShader->cb);

        for (const Var& oldVar : oldShader->vars)
        {
          for (const Var& var : curShader->vars)
          {
            if (oldVar.name == var.name && oldVar.type == var.type)
            {
              memcpy(&curShader->memory[var.memOffset],
                  &oldShader->memory[oldVar.memOffset],
                  varSize(var.type));
            }
          }
        }

        _shaders.erase(find(_shaders.begin(), _shaders.end(), oldShader));
        delete oldShader;
      }

      // check if we have any saved values
      string filename = os_path::Join(DATA_DIR, curShader->root.c_str()) + "_" + curShader->name + ".vars";
      FILE* f = fopen(filename.c_str(), "rt");
      if (f)
      {
        char buf[1024];
        while (fgets(buf, sizeof(buf), f))
        {
          StringToVar(curShader, buf);
        }
        fclose(f);
      }


      _shaders.push_back(curShader);
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
          case Var::Type::Vec2: *(vec2*)&curShader->memory[var.memOffset] = vec2{0,0}; break;
          case Var::Type::Vec3: *(vec3*)&curShader->memory[var.memOffset] = vec3{0,0,0}; break;
          case Var::Type::Color: *(color*)&curShader->memory[var.memOffset] = color{0,0,0,0}; break;
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

          var.type = StringToVarType(type);
          if (hasMin && hasMin)
          {
            var.minValue.fVal = (float)atof(minValue.c_str());
            var.maxValue.fVal = (float)atof(maxValue.c_str());
            var.flags |= Var::FLAG_MINMAX;
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
  {
    static bool opened = true;
    ImGui::Begin("Shaders", &opened, ImGuiWindowFlags_AlwaysAutoResize);
    Shader* cur = _shaders[_curShader];
    ImGui::Combo("Shader", &_curShader, [](void* data, int idx, const char** out)
    {
      ShaderManifestLoader* self = (ShaderManifestLoader*)data;
      *out = self->_shaders[idx]->name.c_str();
      return true;
    }, this, _shaders.size());

    ImGui::Separator();
    cur->RenderGui();
    ImGui::Separator();
    if (ImGui::Button("Save"))
    {
      Shader* shader = _shaders[_curShader];
      string filename = os_path::Join(DATA_DIR, shader->root.c_str()) + "_" + shader->name + ".vars";
      FILE* f = fopen(filename.c_str(), "wt");
      if (f)
      {
        for (Var& var : shader->vars)
        {
          fputs(VarToString(shader->memory.data(), var).c_str(), f);
        }
        fclose(f);
      }
    }
    ImGui::End();
    cur->Render(time);
  }
}

#endif
