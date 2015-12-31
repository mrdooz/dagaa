#pragma once

#pragma pack(push, 1)
struct ObjectHandle
{
  enum Type : u16
  {
    Invalid,
    VertexShader,
    PixelShader,
    BlendState,
    RasterizeState,
    DepthStencilState,
  };

  ObjectHandle() : type(Invalid), id(0) {}
  ObjectHandle(Type type, u16 id) : type(type), id(id) {}

  bool IsValid() const { return type != Invalid; }

  union {
    struct {
      Type type : 4;
      u16 id   : 12;
    };
    u16 raw;
  };
};
#pragma pack(pop)

static_assert(sizeof(ObjectHandle) <= sizeof(u16), "ObjectHandle too large");

extern ObjectHandle g_EmptyHandle;

//-----------------------------------------------------------------------------
struct DXGraphics
{
  int Init(HWND h, u32 width, u32 height);
  void Close();
  void Clear();
  void Present();

  ObjectHandle CreateShader(const char* filename, const void* buf, int len, ObjectHandle::Type type);
  ObjectHandle ReserveHandle(ObjectHandle::Type type);
  void UpdateHandle(ObjectHandle handle, const void* buf);

  template <typename T>
  T* GetResource(ObjectHandle h)
  {
    return (T*)_resourceData[h.id];
  }

  D3D_FEATURE_LEVEL _featureLevel;
  ID3D11Device* _device;
  ID3D11DeviceContext* _context;

  IDXGISwapChain* _swapChain;
  ID3D11RenderTargetView* _renderTargetView;
  ID3D11Texture2D* _backBuffer;

  ID3D11Texture2D* _depthStencilBuffer;
  ID3D11DepthStencilView* _depthStencilView;

  enum { MAX_NUM_RESOURCES = 1 << 12 };
  const void* _resourceData[MAX_NUM_RESOURCES];
  u8 _resourceType[MAX_NUM_RESOURCES];
  int _resourceCount = 0;

  bool _vsync = true;
};

extern DXGraphics* g_Graphics;
