#include "msys_file.hpp"

#if WITH_FILE_UTILS

using namespace std;

//------------------------------------------------------------------------------
class ScopedHandle
{
public:
  ScopedHandle(HANDLE h) : _handle(h) {}
  ~ScopedHandle() {
    if (_handle != INVALID_HANDLE_VALUE)
      CloseHandle(_handle);
  }
  operator bool() { return _handle != INVALID_HANDLE_VALUE; }
  operator HANDLE() { return _handle; }
  HANDLE handle() const { return _handle; }
private:
  HANDLE _handle;
};

//------------------------------------------------------------------------------
bool LoadFile(const char* filename, vector<char> *buf)
{
  ScopedHandle h(CreateFileA(
      filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
  if (!h)
    return false;

  DWORD size = GetFileSize(h, NULL);
  if (size > 0)
  {
    buf->resize(size);
    DWORD res;
    if (!ReadFile(h, &(*buf)[0], size, &res, NULL))
      return false;
  }
  return true;
}

//------------------------------------------------------------------------------
LineReader::LineReader(const char* filename)
{
  LoadFile(filename, &buf);
}

//------------------------------------------------------------------------------
bool LineReader::Eof() const
{
  return idx == buf.size();
}

//------------------------------------------------------------------------------
string LineReader::Next()
{
  if (Eof())
    return string();

  // find next newline or eof from the current position
  const char* next = strchr(&buf[idx], '\r');
  const char* cur = &buf[idx];
  size_t len;
  if (next)
  {
    len = next - cur;
    idx += len + 2;
  }
  else
  {
    len = buf.size() - idx;
    idx = buf.size();
  }
  return string(cur, len);
}

#endif