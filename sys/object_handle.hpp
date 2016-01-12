#pragma once

#pragma pack(push, 1)
struct ObjectHandle
{
  enum Type : u16
  {
    Invalid,
    VertexShader,
    PixelShader,

    IndexBuffer,
    VertexBuffer,
    ConstantBuffer,

    BlendState,
    RasterizeState,
    DepthStencilState,
    
    Texture,
    RenderTarget,
    Sampler,
    
    ShaderResourceView,
  };

  ObjectHandle() : type(Invalid), id(0) {}
  ObjectHandle(Type type, u16 id) : type(type), id(id) {}

  bool IsValid() const { return type != Invalid; }

  union {
    struct {
      Type type : 4;
      u16 id : 12;
    };
    u16 raw;
  };
};
#pragma pack(pop)

static_assert(sizeof(ObjectHandle) <= sizeof(u16), "ObjectHandle too large");

extern ObjectHandle g_EmptyHandle;
