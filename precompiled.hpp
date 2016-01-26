#pragma once

#define WITH_TEXTURE_UPDATE 1
#define WITH_IMGUI 1

#ifdef _DEBUG
  // DEBUG
  #ifndef WITH_FILE_WATCHER
  #define WITH_FILE_WATCHER 1
  #endif
  #ifndef WITH_DX_CLEANUP
  #define WITH_DX_CLEANUP 1
  #endif
  #ifndef WITH_FP_VALIDATION
  #define WITH_FP_VALIDATION 1
  #endif
  #ifndef WITH_FILE_UTILS
  #define WITH_FILE_UTILS 1
  #endif
  #ifndef WITH_ASSERT
  #define WITH_ASSERT 1
  #endif
  #ifndef WITH_TEXTURE_UPDATE
  #define WITH_TEXTURE_UPDATE 1
  #endif
  #ifndef WITH_IMGUI
  #define WITH_IMGUI 1
  #endif
#else
  // RELEASE
  #ifndef WITH_FILE_WATCHER
  #define WITH_FILE_WATCHER 0
  #endif
  #ifndef WITH_DX_CLEANUP
  #define WITH_DX_CLEANUP 0
  #endif
  #ifndef WITH_FP_VALIDATION
  #define WITH_FP_VALIDATION 0
  #endif
  #ifndef WITH_FILE_UTILS
  #define WITH_FILE_UTILS 0
  #endif
  #ifndef WITH_ASSERT
  #define WITH_ASSERT 0
  #endif
  #ifndef WITH_TEXTURE_UPDATE
  #define WITH_TEXTURE_UPDATE 0
  #endif
  #ifndef WITH_IMGUI
  #define WITH_IMGUI 0
  #endif
#endif

#if WITH_ASSERT
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

// wrap around file-watch filename, so they get compiled out
#if WITH_FILE_WATCHER
#define FW_STR(x) x
#else
#define FW_STR(x) ""
#endif

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#ifdef _DEBUG
#include <stdarg.h>
#endif

#include <dxgi.h>
#include <d3d11.h>

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "winmm.lib")

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
