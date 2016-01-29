#pragma once

#pragma pack(push, 1)
struct ObjectHandle
{
  enum
  {
    NUM_TYPE_BITS = 8,
    NUM_ID_BITS = 14,
    NUM_PADDING_BITS = 32 - (NUM_TYPE_BITS + NUM_ID_BITS),
  };

  enum Type : u8
  {
    Invalid,
    InputLayout,
    VertexShader,
    PixelShader,

    IndexBuffer,
    VertexBuffer,
    ConstantBuffer,

    BlendState,
    RasterizeState,
    DepthStencilState,
    Sampler,

    Texture,
    RenderTargetView,
    DepthStencilView,
    ShaderResourceView,

    // NB: These are special types, that just point to the actual resources
    RenderTargetData,
    TextureData,

  };

  ObjectHandle() : type(Invalid), id(0) {}
  ObjectHandle(Type type, u32 id) : type(type), id(id) {}

  bool IsValid() const { return type != Invalid; }

  union {
    struct {
      u32 type : NUM_TYPE_BITS;
      u32 id : NUM_ID_BITS;
      u32 padding : NUM_PADDING_BITS;
      u32 userdata;
    };
    u64 raw;
  };
};
#pragma pack(pop)

static_assert(sizeof(ObjectHandle) <= sizeof(u64), "ObjectHandle too large");

extern ObjectHandle g_EmptyHandle;
