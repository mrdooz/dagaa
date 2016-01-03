#pragma once

#include "../sys/msys_math.hpp"

#include <tuple>
using namespace std;

//----------------------------------------------------------------------------
struct GenTexture
{
  GenTexture(int w, int h) : width(w), height(h), data(new color[w*h]) {}

  void Set(int x, int y, float v) { data[y*width + x] = color{ v, v, v }; }
  void Set(int x, int y, const color& c) { data[y*width + x] = c; }
  color Get(int x, int y) const { return data[y*width+x]; }
  int width;
  int height;
  color* data;
};

//----------------------------------------------------------------------------
template<typename T, int N>
struct FixedArray
{
  T& push_back(const T& elem) { data[used] = elem; return data[used++]; }
  int size() const { return used; }
  const T& back() const { return data[used-1]; }

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
    for (int i = 0; i < data.size(); ++i)
    {
      if (data[i].first == key)
        return data[i].second;
    }

    return data.push_back(pair<K,V>{key, V()}).second;
  }

  bool contains(const K& key) const
  {
    for (int i = 0; i < data.size(); ++i)
    {
      if (data[i].first == key)
        return true;
    }

    return false;
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

//----------------------------------------------------------------------------
struct BaseFun;
struct Cell
{
  enum Type
  {
    Symbol,
    Texture,
    Color,
    Vec2,
    Float,
    List,
    Func,
  };

  Cell() {}
  Cell(Type type) : type(type) {}
  Cell(float val) : type(Float), fVal(val) {}
  Cell(const color& col) : type(Color), cVal(col) {}
  Cell(const vec2& val) : type(Vec2), vVal(val) {}
  Cell(GenTexture* tex) : type(Texture), tVal(tex) {}
  Cell(BaseFun* func) : type(Func), func(func) {}

  Type type;
  InplaceString symbol;
  color cVal;
  float fVal;
  vec2 vVal;
  GenTexture* tVal;
  FixedArray<Cell*, 16> list;
  BaseFun* func;
};

//----------------------------------------------------------------------------
namespace detail
{
  // tag type, used to index into Cell lists
  template <size_t... Is>
  struct _indices
  {
  };

  // Recursively inherits from itself until N == 0, building up a parameter pack containing 0..N-1
  template <size_t N, size_t... Is>
  struct _indices_builder : _indices_builder<N - 1, N - 1, Is...>
  {
  };

  // The base case where we define the type tag
  template <size_t... Is>
  struct _indices_builder<0, Is...>
  {
    using type = _indices<Is...>;
  };

  // Overloaded functions used to extract the correct value from Cells
  template <typename T>
  inline T _get_var(Cell& cell, int n);

  template <>
  inline float _get_var(Cell& cell, int n)
  {
    return cell.list[n]->fVal;
  }

  template <>
  inline const color& _get_var(Cell& cell, int n)
  {
    return cell.list[n]->cVal;
  }

  template <>
  inline const vec2& _get_var(Cell& cell, int n)
  {
    return cell.list[n]->vVal;
  }

  template <>
  inline GenTexture* _get_var(Cell& cell, int n)
  {
    return cell.list[n]->tVal;
  }

  template <typename... T, size_t... N>
  tuple<T...> _get_args(Cell& cell, _indices<N...>)
  {
    return tuple<T...>(_get_var<T>(cell, N)...);
  }

  template <typename... T>
  tuple<T...> _get_args(Cell& cell)
  {
    return _get_args<T...>(cell, typename _indices_builder<sizeof...(T)>::type());
  }

  template <typename Ret, typename... Args, size_t... N>
  Ret _lift(Ret(*fun)(Args...), tuple<Args...> args, _indices<N...>)
  {
    return fun(get<N>(args)...);
  }

  template <typename Ret, typename... Args>
  Ret _lift(Ret(*fun)(Args...), tuple<Args...> args)
  {
    return _lift(fun, args, typename _indices_builder<sizeof...(Args)>::type());
  }
}

//----------------------------------------------------------------------------
struct BaseFun
{
  virtual ~BaseFun() {}
  virtual Cell Apply(Cell& cell) = 0;
};

//----------------------------------------------------------------------------
template <typename Ret, typename... Args>
class Fun : public BaseFun
{
private:
  typedef Ret(*_fun_type)(Args...);
  //using _fun_type =  function<Ret(Args...)>;
  _fun_type _fun;

public:
  Fun(_fun_type fun) : _fun(fun) {}

  Cell Apply(Cell& cell)
  {
    // args are returned as a tuple, so they need to be converted to a parameter pack
    auto& args = detail::_get_args<Args...>(cell);
    return Cell(detail::_lift(_fun, args));
  }
};

//----------------------------------------------------------------------------

typedef FixedLinearMap<InplaceString, Cell, 256> Environment;

// This fancy env will be needed if I ever implement lambdas
#if 0
struct Environment
{
  Environment* FindEnvForVar(const InplaceString& var)
  {
    if (env.contains(var))
      return this;

    if (!outer)
      return nullptr;

    return !outer ? nullptr : outer->FindEnvForVar(var);

  }

  FixedLinearMap<InplaceString, Cell, 256> env;
  Environment* outer = nullptr;
};
#endif
//----------------------------------------------------------------------------

typedef FixedArray<InplaceString, 1024> TokenArray;
TokenArray Tokenize(const char* str, const char* end);
Cell* ParseTokens(InplaceString*& start, InplaceString*& end);
Cell Eval(const Cell& cell, Environment& env);

template <typename Ret, typename... Args>
BaseFun* Register(Ret(*fun)(Args...))
{
  return new Fun<Ret, Args...>{ fun };
}

//----------------------------------------------------------------------------
void InitDefaultEnv(Environment* defaultEnv);
Cell EvalString(const char* str, Environment& env);
