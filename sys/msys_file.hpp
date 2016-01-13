#pragma once
#if WITH_FILE_UTILS
#include <vector>
bool LoadFile(const char* filename, std::vector<char> *buf);

struct BinaryReader
{
  bool Init(const char* filename)
  {
    return LoadFile(filename, &buf);
  }

  bool Eof() const
  {
    return idx >= (int)buf.size();
  }

  template <typename T>
  T Read()
  {
    if (Eof())
      return T();
    T tmp = *(T*)(buf.data() + idx);
    idx += sizeof(T);
    return tmp;
  }

  int FilePos() const
  {
    return idx;
  }

  void Skip(int len)
  {
    idx += len;
  }

  void* Buf()
  {
    return buf.data() + idx;
  }

  int idx = 0;
  std::vector<char> buf;
};

#endif