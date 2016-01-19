#pragma once

template <typename T, typename U>
struct pair
{
  T first;
  U second;
};

//----------------------------------------------------------------------------
template<typename T, int N>
struct FixedArray
{
  T& push_back(const T& elem) { data[used] = elem; return data[used++]; }
  int size() const { return used; }
  const T& back() const { return data[used - 1]; }

  T& operator[](int idx) { return data[idx]; }
  const T& operator[](int idx) const { return data[idx]; }
  T data[N];
  int capacity = N;
  int used = 0;
};

//----------------------------------------------------------------------------
template<typename K, typename V, int N>
struct FixedLinearMap
{
  V& operator[](const K& key)
  {
    V* v = find(key);
    return v ? *v : data.push_back(pair<K, V>{key, V()}).second;
  }

  bool contains(const K& key) const
  {
    return !!find(key);
  }

  V* find(const K& key)
  {
    for (int i = 0; i < data.size(); ++i)
    {
      if (data[i].first == key)
        return &data[i].second;
    }

    return nullptr;
  }

  const V* find(const K& key) const
  {
    for (int i = 0; i < data.size(); ++i)
    {
      if (data[i].first == key)
        return &data[i].second;
    }

    return nullptr;
  }

  FixedArray<pair<K, V>, N> data;
};

//----------------------------------------------------------------------------
struct InplaceString
{
  InplaceString() : start(nullptr), end(nullptr) {}
  InplaceString(const char* start, const char* end) : start(start), end(end) {}
  InplaceString(const char* str) : start(str), end(start + strlen(str)) {}

  bool operator==(const InplaceString& rhs) const
  {
    if (size() != rhs.size())
      return false;

    for (int i = 0; i < size(); ++i)
    {
      if (start[i] != rhs[i])
        return false;
    }

    return true;
  }

  bool operator==(const char* str) const
  {
    InplaceString tmp(str);
    return *this == tmp;
  }

  bool operator!=(const char* str) const
  {
    return !(*this == str);
  }

  char operator[](int idx) const
  {
    return start[idx];
  }

  int find(char ch) const
  {
    for (int i = 0; i < size(); ++i)
    {
      if (start[i] == ch)
        return i;
    }

    return -1;
  }

  operator const char*()
  {
    return start;
  }

  void copy_out(char* buf)
  {
    memcpy(buf, start, size());
    buf[size()] = 0;
  }

  static const int npos = -1;

  int size() const { return end - start; }
  const char* start;
  const char* end;
};
