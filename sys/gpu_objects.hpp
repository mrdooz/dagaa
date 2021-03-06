#pragma once
#include "flags.hpp"
#include "object_handle.hpp"

class GraphicsContext;

// Note, the objects here just hold state. The code for actually setting the state
// on the context is found in GraphicsContext (SetGpuState et al)

//------------------------------------------------------------------------------
struct GpuObjects
{
  bool CreateDynamic(u32 ibSize, DXGI_FORMAT ibFormat, u32 vbSize, u32 vbElemSize);

  bool CreateDynamic(u32 ibSize,
      DXGI_FORMAT ibFormat,
      const void* ibData,
      u32 vbSize,
      u32 vbElemSize,
      const void* vbData);

  bool CreateDynamicVb(u32 vbSize, u32 vbElemSize, const void* vbData = nullptr);
  bool CreateDynamicIb(u32 ibSize, DXGI_FORMAT ibFormat, const void* ibData = nullptr);

  bool CreateVertexBuffer(u32 vbSize, u32 vbElemSize, const void* vbData);
  bool CreateIndexBuffer(u32 ibSize, DXGI_FORMAT ibFormat, const void* ibData);

  // bool LoadVertexShader(const char* filename,
  //    const char* entryPoint,
  //    u32 flags = 0,
  //    vector<D3D11_INPUT_ELEMENT_DESC>* elements = nullptr);
  // bool LoadPixelShader(const char* filename, const char* entryPoint);

  D3D11_PRIMITIVE_TOPOLOGY _topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  ObjectHandle _vs;
  ObjectHandle _gs;
  ObjectHandle _ps;
  ObjectHandle _layout;

  ObjectHandle _vb;
  ObjectHandle _ib;

  u32 _vbSize = 0;
  u32 _ibSize = 0;
  u32 _vbElemSize = 0;
  DXGI_FORMAT _ibFormat = DXGI_FORMAT_UNKNOWN;
  u32 _numVerts = 0;
  u32 _numIndices = 0;
};

//------------------------------------------------------------------------------
struct GpuState
{
  // Passing nullptr uses the default settings
  bool Create(const D3D11_DEPTH_STENCIL_DESC* dssDesc = nullptr,
      const D3D11_BLEND_DESC* blendDesc = nullptr,
      const D3D11_RASTERIZER_DESC* rasterizerDesc = nullptr);

  ObjectHandle _depthStencilState;
  ObjectHandle _blendState;
  ObjectHandle _rasterizerState;

  enum Samplers
  {
    Point,
    Linear,
    LinearWrap,
    LinearBorder,
  };

  ObjectHandle _samplers[4];
};

//------------------------------------------------------------------------------
// Bundles Gpu state & objects in a cute little package :)
struct BundleOptions;
struct GpuBundle
{
  bool Create(const BundleOptions& options);
  GpuState state;
  GpuObjects objects;
};

struct BundleOptions
{
  BundleOptions();
  struct OptionFlag
  {
    enum Enum
    {
      DepthStencilDesc = 1 << 0,
      BlendDesc = 1 << 1,
      RasterizerDesc = 1 << 2,
      DynamicVb = 1 << 3,
      DynamicIb = 1 << 4,
      StaticVb = 1 << 5,
      StaticIb = 1 << 6,
    };

    struct Bits
    {
      u32 depthStencilDesc : 1;
      u32 blendDesc : 1;
      u32 rasterizerDesc : 1;
      u32 dynamicVb : 1;
      u32 dynamicIb : 1;
    };
  };

  typedef Flags<OptionFlag> OptionFlags;

  BundleOptions& DepthStencilDesc(const CD3D11_DEPTH_STENCIL_DESC& desc);
  BundleOptions& BlendDesc(const CD3D11_BLEND_DESC& desc);
  BundleOptions& RasterizerDesc(const CD3D11_RASTERIZER_DESC& desc);

  BundleOptions& VertexShader(const char* filename, const char* entrypoint);
  BundleOptions& GeometryShader(const char* filename, const char* entrypoint);
  BundleOptions& PixelShader(const char* filename, const char* entrypoint);
  BundleOptions& ComputeShader(const char* filename, const char* entrypoint);

  BundleOptions& VertexFlags(u32 flags);
  // BundleOptions& InputElements(const vector<D3D11_INPUT_ELEMENT_DESC>& elems);
  // BundleOptions& InputElement(const bristol::CD3D11_INPUT_ELEMENT_DESC& elem);
  BundleOptions& Topology(D3D11_PRIMITIVE_TOPOLOGY topology);

  // TODO: hmm, can the element size be inferred from the input elements?
  BundleOptions& DynamicVb(int numElements, int elementSize);
  BundleOptions& DynamicIb(int numElements, int elementSize);
  BundleOptions& StaticVb(int numElements, int elementSize, void* vertices);
  BundleOptions& StaticIb(int numElements, int elementSize, void* indices);
  // template <typename T>
  // BundleOptions& StaticIb(const vector<T>& elems)
  //{
  //  return StaticIb((int)elems.size(), sizeof(T), (void*)elems.data());
  //}

  CD3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  CD3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
  CD3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());

  const char* vsShaderFile = nullptr;
  const char* gsShaderFile = nullptr;
  const char* psShaderFile = nullptr;
  const char* csShaderFile = nullptr;
  const char* vsEntry = nullptr;
  const char* gsEntry = nullptr;
  const char* psEntry = nullptr;
  const char* csEntry = nullptr;

  u32 vertexFlags = 0;
  // vector<D3D11_INPUT_ELEMENT_DESC> inputElements;

  D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  int vbNumElems = 0;
  int vbElemSize = 0;

  int ibNumElems = 0;
  int ibElemSize = 0;

  void* staticVb = nullptr;
  void* staticIb = nullptr;

  OptionFlags flags;
};
