#pragma once
#if WITH_FILE_UTILS
#include <vector>
bool LoadFile(const char* filename, std::vector<char> *buf);

//------------------------------------------------------------------------------
struct LineReader
{
  LineReader(const char* filename);
  std::string Next();
  bool Eof() const;

  std::vector<char> buf;
  size_t idx = 0;
};

#endif