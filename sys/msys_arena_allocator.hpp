#pragma once

class ArenaAllocator
{
public:
  ArenaAllocator();
  ~ArenaAllocator();

  bool Init(void* start, void* end);
  void Reset();
  void* Alloc(u32 size);

  template<typename T> T* Alloc(u32 count)
  {
    return (T*)Alloc(count * sizeof(T), alignment);
  }

  template<typename T> T* NewArray(u32 count)
  {
    T* mem = (T*)Alloc(count * sizeof(T), alignment);
    for (u32 i = 0; i < count; ++i)
      new(mem + i)T();
    return mem;
  }

  template<typename T, typename... Args> T* New(const Args&... args)
  {
    T* mem = (T*)Alloc(sizeof(T));
    return new(mem)T{args...};
  }

private:

  u8* _mem = nullptr;
  u32 _idx = 0;
  u32 _capacity = 0;
  u32 alignment = 4;
};

extern ArenaAllocator g_ScratchMemory;
