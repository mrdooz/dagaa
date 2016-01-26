#pragma once

#if WITH_FILE_WATCHER
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

struct FileWatcherWin32
{
  ~FileWatcherWin32();

  typedef int WatchId;
  typedef std::function<bool(const char* filename, const char* buf, int len)> cbFileChanged;

  struct AddFileWatchResult
  {
    WatchId watchId = -1;
    bool initialResult = true;
  };

  AddFileWatchResult AddFileWatch(
      const std::string& filename, bool initialCallback, const cbFileChanged& cb);
  void RemoveFileWatch(WatchId id);

  void Tick();

private:
  struct CallbackContext
  {
    std::string fullPath;
    std::string filename;
    cbFileChanged cb;
    WatchId id;
  };

  struct WatchedDir
  {
    HANDLE dirHandle;
    OVERLAPPED overlapped;
    std::vector<CallbackContext*> callbacks;
  };

  std::unordered_map<std::string, WatchedDir*> _watchesByDir;
  std::unordered_map<const CallbackContext*, u32> _lastUpdate;

  WatchId _nextId = 0;

  enum
  {
    BUF_SIZE = 16 * 1024
  };
  char _buf[BUF_SIZE];
};

extern FileWatcherWin32* g_FileWatcher;
#endif
