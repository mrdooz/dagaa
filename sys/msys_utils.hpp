#pragma once

//------------------------------------------------------------------------------
struct BinaryReader
{
  BinaryReader(const char* buf, int len) : buf(buf), len(len), idx(0) {}

  bool Eof() const { return idx >= len; }

  template <typename T>
  T Read() const
  {
    if (Eof())
      return T();
    T tmp = *(T*)(buf + idx);
    idx += sizeof(T);
    return tmp;
  }

  int FilePos() const { return idx; }

  void Skip(int len) const { idx += len; }

  const char* Buf() const { return buf + idx; }
  const char* buf;
  int len;
  mutable int idx;
};

//------------------------------------------------------------------------------
u32 FnvHash(const char* str, int len, u32 d = 0);
u32 SizeFromFormat(DXGI_FORMAT format);

//------------------------------------------------------------------------------
template <typename T>
u32 CalcHash(const T& t)
{
  return FnvHash((const char*)&t, sizeof(T));
}
