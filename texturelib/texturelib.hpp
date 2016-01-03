#pragma once

struct BinaryReader;

namespace TextureLib
{
  bool Init();
  void Close();
  void GenerateTexture(const BinaryReader& reader);
  void Tick();
}
