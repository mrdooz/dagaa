#pragma once

#if WITH_FILE_UTILS
struct ShaderManifestLoader
{
  bool Create();
  bool Destroy();

  bool AddManifest(const char* manifest);
};

extern ShaderManifestLoader g_ShaderManifestLoader;
#endif