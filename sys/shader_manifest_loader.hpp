#pragma once

namespace tokko
{
  struct ShaderManifestLoader
  {
    bool Create();
    bool Destroy();

    bool AddManifest(const char* manifest);

  };

  extern ShaderManifestLoader g_ShaderManifestLoader;
}

