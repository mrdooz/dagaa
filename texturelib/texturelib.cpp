#include "texturelib.hpp"
#include <malloc.h>

//----------------------------------------------------------------------------
float Clamp(float minValue, float maxValue, float value)
{
  return min(maxValue, max(minValue, value));
}

//----------------------------------------------------------------------------
template <typename T>
T Lerp(const T& a, const T& b, float t)
{
  t = Clamp(0, 1, t);
  return a + t * (b - a);
}

//----------------------------------------------------------------------------
GenTexture* MakeTexture(float width, float height)
{
  GenTexture* tex = new GenTexture((int)width, (int)height);
  return tex;
}

//----------------------------------------------------------------------------
Cell MakeColor(float r, float g, float b)
{
  return Cell(color{r, g, b, 1});
}

//----------------------------------------------------------------------------
Cell MakeVec2(float x, float y)
{
  return Cell(vec2{x, y});
}

//----------------------------------------------------------------------------
GenTexture* Fill(GenTexture* texture, const color& col)
{
  int len = texture->width * texture->height;
  for (int i = 0; i < len; ++i)
    texture->data[i] = col;

  return texture;
}

//----------------------------------------------------------------------------
vec2 ToCoord(int x, int y, const GenTexture* tex)
{
  return vec2
  {
    2 * (-0.5f + (float)x / tex->width),
    2 * (-0.5f + (float)y / tex->height)
  };
}

//----------------------------------------------------------------------------
pair<int, int> TextureMod(int x, int y, const GenTexture* tex)
{
  while (x < 0)
    x += tex->width;

  while (y < 0)
    y += tex->height;

  return make_pair(x % tex->width, y % tex->height);
}

//----------------------------------------------------------------------------
pair<int, int> ToTexture(const GenTexture* tex, const vec2& v)
{
  int x = (int)(tex->width * (0.5f + v.x / 2.f));
  int y = (int)(tex->height * (0.5f + v.y / 2.f));

  return TextureMod(x, y, tex);
}

//----------------------------------------------------------------------------
color GetFromCoord(const GenTexture* tex, const vec2& v)
{
  int x, y;
  tie(x, y) = ToTexture(tex, v);
  color c00 = tex->Get(x, y);
  color c01 = tex->Get(x, (y+1) % tex->height);
  color c10 = tex->Get((x+1) % tex->width, y);
  color c11 = tex->Get((x+1) % tex->width, (y+1) % tex->height);

  float dx = 2.0f / tex->width;
  float dy = 2.0f / tex->height;

  return c00;
}

//----------------------------------------------------------------------------
GenTexture* PixelInvoker(void* context,
    GenTexture* texture,
    color (*fn)(void* context, GenTexture* texture, const vec2& v))
{
  int w = texture->width;
  int h = texture->height;
  color* tmp = new color[w*h];
  for (int i = 0; i < h; ++i)
  {
    for (int j = 0; j < w; ++j)
    {
      vec2 v = ToCoord(j, i, texture);
      tmp[i*w + j] = fn(context, texture, v);
    }
  }
  memcpy(texture->data, tmp, w * h * sizeof(color));
  delete[] tmp;
  return texture;
}

//----------------------------------------------------------------------------
GenTexture* AddScale(GenTexture* a, GenTexture* b, const color& scaleA, const color& scaleB)
{
  struct Data
  {
    GenTexture* a;
    GenTexture* b;
    color scaleA;
    color scaleB;

    static color Eval(void* ctx, GenTexture*, const vec2& v)
    {
      Data* data = (Data*)ctx;
      color colA = GetFromCoord(data->a, v);
      color colB = GetFromCoord(data->b, v);
      return
        color{ colA.r * data->scaleA.r + colB.r * data->scaleB.r,
        colA.g * data->scaleA.g + colB.g * data->scaleB.g,
        colA.b * data->scaleA.b + colB.b * data->scaleB.b };
    }
  };

  Data data = Data{ a, b, scaleA, scaleB };
  return PixelInvoker((void*)&data, a, Data::Eval);
}

//----------------------------------------------------------------------------
GenTexture* RotateScale(GenTexture* texture, float rotate, const vec2& scale)
{
  color* tmp = new color[texture->width*texture->height];
  mtx2x2 mtx = mtx2x2::Rotate(rotate) * mtx2x2::Scale(scale.x, scale.y);

  return PixelInvoker((void*)&mtx, texture, [](void* ctx, GenTexture* texture, const vec2& v)
  {
    return GetFromCoord(texture, v * (*(mtx2x2*)ctx));
  });
}

//----------------------------------------------------------------------------
GenTexture* Distort(GenTexture* texture, GenTexture* a, GenTexture* b, float scale)
{
  struct Data {
    GenTexture* a;
    GenTexture* b;
    float scale;
    static color Eval(void* ctx, GenTexture* texture, const vec2& v)
    {
      Data* data = (Data*)ctx;
      color rr = GetFromCoord(data->a, v);
      color ss = GetFromCoord(data->b, v);
      mtx2x2 mtx = mtx2x2::Rotate(rr.r) * mtx2x2::Scale(ss.r * data->scale, ss.r * data->scale);
      return GetFromCoord(texture, v * mtx);
    }
  } data = { a, b, scale };
  
  return PixelInvoker((void*)&data, texture, Data::Eval);
}

//----------------------------------------------------------------------------
GenTexture* RadialGradient(GenTexture* texture, const vec2& center, float power)
{
  struct Data {
    vec2 center;
    float power;
    static color Eval(void* ctx, GenTexture* texture, const vec2& v)
    {
      Data* data = (Data*)ctx;
      float r = powf(Distance(v, data->center), data->power);
      return color{ r, r, r, 1 };
    }
  } data = { center, power };

  return PixelInvoker((void*)&data, texture, Data::Eval);
}

//----------------------------------------------------------------------------
GenTexture* Sinus(GenTexture* texture, float freq, float amp, float power)
{
  struct Data {
    float freq;
    float amp;
    float power;
    static color Eval(void* ctx, GenTexture* texture, const vec2& v)
    {
      Data* data = (Data*)ctx;
      float aa = v.y - data->amp * sinf(data->freq * v.x);
      float r = powf(
          Clamp(0, 1, 1 - msys_fabsf(v.y - data->amp * sinf(data->freq * v.x))), data->power);
      return color{ r, r, r, 1 };
    }
  } data = { freq * 3.1415926f, amp, power };

  return PixelInvoker((void*)&data, texture, Data::Eval);
}

//----------------------------------------------------------------------------
GenTexture* LinearGradient(GenTexture* texture,
    const vec2& a,
    const vec2& b,
    float power,
    const color& colorA,
    const color& colorB)
{
  vec2 dir = Normalize(b - a);

  struct Data {
    vec2 a;
    vec2 dir;
    float power;
    color colorA;
    color colorB;
    static color Eval(void* ctx, GenTexture* texture, const vec2& v)
    {
      Data* data = (Data*)ctx;
      vec2 v2 = v - data->a;
      vec2 proj = Dot(v2, data->dir) * data->dir;

      // proj + dist = v => dist = v - proj
      vec2 dist = v2 - proj;
      float t = Clamp(0, 1, powf(Length(dist), data->power));
      return Lerp(data->colorA, data->colorB, t);
    }
  } data = { a, dir, power, colorA, colorB };

  return PixelInvoker((void*)&data, texture, Data::Eval);
}

//----------------------------------------------------------------------------
GenTexture* ColorGradient(GenTexture* texture,
  GenTexture* gradientTexture,
  const color& colorA,
  const color& colorB)
{
  struct Data {
    GenTexture* gradientTexture;
    color colorA;
    color colorB;
    static color Eval(void* ctx, GenTexture* texture, const vec2& v)
    {
      Data* data = (Data*)ctx;
      color grad = GetFromCoord(data->gradientTexture, v);
      float t = grad.r;
      return Lerp(data->colorA, data->colorB, t);
    }
  } data = { gradientTexture, colorA, colorB };

  return PixelInvoker((void*)&data, texture, Data::Eval);
}

//----------------------------------------------------------------------------
void InitDefaultEnv(Environment* defaultEnv)
{
  (*defaultEnv)["pi"] = Cell(3.1415f);
  (*defaultEnv)["fill"] = Register(&Fill);
  (*defaultEnv)["col"] = Register(&MakeColor);
  (*defaultEnv)["vec2"] = Register(&MakeVec2);
  (*defaultEnv)["tex"] = Register(&MakeTexture);
  (*defaultEnv)["radial-gradient"] = Register(&RadialGradient);
  (*defaultEnv)["linear-gradient"] = Register(&LinearGradient);
  (*defaultEnv)["color-gradient"] = Register(&ColorGradient);
  (*defaultEnv)["add-scale"] = Register(&AddScale);
  (*defaultEnv)["rotate-scale"] = Register(&RotateScale);
  (*defaultEnv)["distort"] = Register(&Distort);
  (*defaultEnv)["sinus"] = Register(&Sinus);
}

//----------------------------------------------------------------------------
TokenArray Tokenize(const char* str, const char* end)
{
  TokenArray tokens;

  const char* start = nullptr;
  while (str != end)
  {
    char ch = *str;
    if (ch == '(' || ch == ')')
    {
      if (start)
      {
        tokens.push_back(InplaceString{start, str});
        start = nullptr;
      }
      tokens.push_back(InplaceString{str, str + 1});
    }
    else if (ch == ' ' || ch == '\r' || ch == '\n')
    {
      if (start)
      {
        tokens.push_back(InplaceString{start, str});
        start = nullptr;
      }
    }
    else
    {
      if (!start)
        start = str;
    }
    str++;
  }

  return tokens;
}

//----------------------------------------------------------------------------
Cell* ParseTokens(InplaceString*& start, InplaceString*& end)
{
  Cell* res = new Cell();

  if (start == end)
  {
    // error
    return nullptr;
  }

  InplaceString token = *start++;

  if (token == "(")
  {
    res->type = Cell::List;
    // token is a list, so parse all the elements
    while (*start != ")")
    {
      res->list.push_back(ParseTokens(start, end));
    }
    // pop off ')'
    ++start;
    return res;
  }
  else if (token == ")")
  {
    // error
  }
  else
  {
    char ch = token[0];
    // atom: either a int/float or a symbol
    if (isdigit(ch) || ch == '-' || ch == '+' )
    {
      res->type = Cell::Float;
      char* buf = (char*)_alloca(token.size() + 1);
      token.copy_out(buf);
      res->fVal = (float)atof(buf);
    }
    else
    {
      res->type = Cell::Symbol;
      res->symbol = token;
    }
  }
  return res;
}

//----------------------------------------------------------------------------
Cell Eval(const Cell& cell, Environment& env)
{
  // check if cell is an atom, ie either a symbol or a value
  if (cell.type == Cell::Symbol)
    return env[cell.symbol];

  if (cell.type != Cell::List)
    return cell;

  // cell is a list, so check first for special forms
  InplaceString form = cell.list[0]->symbol;
  //if (form == "quote")
  //  return Cell(vector<Cell>(cell.list.begin() + 1, cell.list.end()));

  //if (form == "if")
  //{
  //  bool res = Eval(*cell.list[1], env).nVal > 0;
  //  return res ? Eval(*cell.list[2], env) : Eval(*cell.list[3], env);
  //}

  if (form == "define")
  {
    env[cell.list[1]->symbol] = Eval(*cell.list[2], env);
    return Cell();
  }

  // "begin" - eval all the arguments, and return the last value
  if (form == "begin")
  {
    for (int i = 1; i < cell.list.size() - 1; ++i)
      Eval(*cell.list[i], env);
    return Eval(*cell.list.back(), env);
  }

  // not a special form, so it must be a function call
  Cell fn = Eval(*cell.list[0], env);
  Cell args;
  args.type = Cell::List;
  for (int i = 1; i < cell.list.size(); ++i)
    args.list.push_back(new Cell(Eval(*cell.list[i], env)));

  return fn.func->Apply(args);
}

//----------------------------------------------------------------------------
Cell EvalString(const char* str, Environment& env)
{
  TokenArray tokens = Tokenize(str, str + strlen(str));
  InplaceString* start = &tokens[0];
  InplaceString* end = &tokens[tokens.size()];
  Cell* token = ParseTokens(start, end);
  return Eval(*token, env);
}
