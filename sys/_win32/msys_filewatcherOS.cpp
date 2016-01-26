#if WITH_FILE_WATCHER

#include "msys_filewatcherOS.hpp"
#include <sys/msys_file.hpp>

using namespace std;

FileWatcherWin32* g_FileWatcher;

// FILE_NOTIFY_CHANGE_FILE_NAME is needed because photoshop doesn't modify the file directly,
// instead it saves a temp file, and then deletes/renames
static DWORD FILE_NOTIFY_FLAGS = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

//------------------------------------------------------------------------------
static string ReplaceAll(const string& str, char toReplace, char replaceWith)
{
  string res(str);
  for (size_t i = 0; i < res.size(); ++i)
  {
    if (res[i] == toReplace)
      res[i] = replaceWith;
  }
  return res;
}

//------------------------------------------------------------------------------
static string MakeCanonical(const string &str)
{
  // convert back slashes to forward
  return ReplaceAll(str, '\\', '/');
}

//------------------------------------------------------------------------------
bool WideCharToUft8(LPCOLESTR unicode, int len, string *str)
{
  if (!unicode)
    return false;

  char *buf = (char *)_alloca(len * 2 + 1);

  int res = WideCharToMultiByte(CP_UTF8, 0, unicode, len, buf, len * 2 + 1, NULL, NULL);
  if (res == 0)
    return false;

  buf[res] = '\0';

  *str = string(buf);
  return true;
}

//------------------------------------------------------------------------------
FileWatcherWin32::~FileWatcherWin32()
{
  for (auto kv : _watchesByDir)
  {
    WatchedDir* dir = kv.second;
    CloseHandle(dir->dirHandle);
    CloseHandle(dir->overlapped.hEvent);
    for (CallbackContext* c : dir->callbacks)
      delete c;
    delete dir;
  }
  _watchesByDir.clear();
}

//------------------------------------------------------------------------------
FileWatcherWin32::AddFileWatchResult FileWatcherWin32::AddFileWatch(
  const string& filename, bool initialCallback, const cbFileChanged& cb)
{
  // split filename into dir/file
  string canonical = MakeCanonical(filename);
  size_t lastSlash = canonical.rfind('/');
  string head = canonical.substr(0, lastSlash);
  string tail = canonical.substr(lastSlash + 1);

  // check if the directory is already open
  WatchedDir* dir = nullptr;
  auto it = _watchesByDir.find(head);
  if (it == _watchesByDir.end())
  {
    dir = new WatchedDir();
    dir->dirHandle = ::CreateFileA(
      head.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      NULL);

    ZeroMemory(&dir->overlapped, sizeof(OVERLAPPED));
    dir->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BOOL res = ReadDirectoryChangesW(
      dir->dirHandle, _buf, BUF_SIZE, FALSE, FILE_NOTIFY_FLAGS, NULL, &dir->overlapped, NULL);
    if (!res)
    {
      delete dir;
      return AddFileWatchResult();
    }

    _watchesByDir[head] = dir;
  }
  else
  {
    dir = it->second;
  }

  dir->callbacks.push_back(new CallbackContext{ filename, tail, cb, _nextId });

  // Do the initial calback if requested
  AddFileWatchResult res;
  if (initialCallback)
  {
    vector<char> buf;
    LoadFile(filename.c_str(), &buf);
    res.initialResult = cb(filename.c_str(), buf.data(), (int)buf.size());
    res.watchId = _nextId;
  }

  _nextId++;
  return res;
}

//------------------------------------------------------------------------------
void FileWatcherWin32::RemoveFileWatch(WatchId id)
{
  for (auto itDir = _watchesByDir.begin(); itDir != _watchesByDir.end(); )
  {
    WatchedDir* dir = itDir->second;
    for (auto itFile = dir->callbacks.begin(); itFile != dir->callbacks.end(); )
    {
      if ((*itFile)->id == id)
      {
        delete *itFile;
        itFile = dir->callbacks.erase(itFile);
      }
      else
      {
        ++itFile;
      }
    }

    if (dir->callbacks.empty())
    {
      CloseHandle(dir->dirHandle);
      CloseHandle(dir->overlapped.hEvent);
      delete dir;
      itDir = _watchesByDir.erase(itDir);
    }
    else
    {
      ++itDir;
    }
  }
}

//------------------------------------------------------------------------------
void FileWatcherWin32::Tick()
{
  // Check if any of the recently changed files have been idle for long enough to
  // call their callbacks
  u32 now = timeGetTime();

  for (auto it = _lastUpdate.begin(); it != _lastUpdate.end();)
  {
    const CallbackContext* ctx = it->first;
    u32 lastUpdate = it->second;
    if (now - lastUpdate > 1000)
    {
      vector<char> buf;
      LoadFile(ctx->fullPath.c_str(), &buf);
      ctx->cb(ctx->fullPath.c_str(), buf.data(), (int)buf.size());
      it = _lastUpdate.erase(it);
    }
    else
    {
      it++;
    }
  }

  for (auto kv : _watchesByDir)
  {
    WatchedDir* dir = kv.second;
    DWORD bytesTransferred = 0;
    if (GetOverlappedResult(dir->dirHandle, &dir->overlapped, &bytesTransferred, FALSE))
    {
      char* ptr = _buf;
      while (true)
      {
        FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)ptr;

        string filename;
        WideCharToUft8(fni->FileName, fni->FileNameLength / 2, &filename);
        filename = MakeCanonical(filename);

        // Check if this file is being watched, and update its last modified
        for (const CallbackContext* ctx : dir->callbacks)
        {
          if (ctx->filename == filename)
            _lastUpdate[ctx] = now;
        }

        if (fni->NextEntryOffset == 0)
          break;
        ptr += fni->NextEntryOffset;
      }

      // Reset the event, and reapply the watch
      ResetEvent(dir->overlapped.hEvent);
      ReadDirectoryChangesW(
        dir->dirHandle, _buf, BUF_SIZE, FALSE, FILE_NOTIFY_FLAGS, NULL, &dir->overlapped, NULL);
    }
  }
}

#endif