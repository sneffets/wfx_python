#include "pch.h"

#include "format_string.h"

// -------------------------------------------------------
// WFX Callback Functions
// -------------------------------------------------------

int __stdcall ProgressProcW(int PluginNr, WCHAR* SourceName,
                            WCHAR* TargetName, int PercentDone)
{
  OutputDebugStringW(L"ProgressProcW called\n");
  return 0;
}

struct PluginInfo
{
  int PluginNr;
  tProgressProcW progressProc;
  tLogProcW logProc;
  tRequestProcW requestProc;

  PluginInfo(tProgressProcW progressProc) : progressProc(progressProc)
  {

  }
};

PluginInfo pluginInfo(ProgressProcW);

std::filesystem::path map_filename(WCHAR* pathToMap)
{
  constexpr auto mapRootTo = L"C:\\";
  std::filesystem::path returnPath(mapRootTo);
  std::wstring w_pathToMap;

  // leading '\\' => operator/= would remove subpaths from map
  // so we skip leading '\\'
  w_pathToMap = (pathToMap[0] == L'\\') ? (pathToMap + 1) : pathToMap;

  returnPath /= w_pathToMap;

  return returnPath;
}

// -------------------------------------------------------
// WFX Exports
// -------------------------------------------------------

int __stdcall FsInitW(int PluginNr, tProgressProcW pProgressProcW,
                      tLogProcW pLogProcW, tRequestProcW pRequestProcW)
{
  debug_msg(L"FsInitW called with PluginNr=", PluginNr);
  pluginInfo.PluginNr = PluginNr;
  pluginInfo.progressProc = pProgressProcW;
  pluginInfo.logProc = pLogProcW;
  pluginInfo.requestProc = pRequestProcW;

  ////
  //// some tests with getting user infos
  //// 
  //WCHAR test[256] = { 0 };

  //wcscat_s(test, L"DefaultUser");

  //pRequestProcW(PluginNr, RT_UserName, NULL, (WCHAR*)L"Username:", test, 256);
  //debug_msg(L"pRequestProcW returned ", test, L" as username");
  //test[0] = 0; // else it would be suggested as default
  //pRequestProcW(PluginNr, RT_Password, NULL, (WCHAR*)L"Pasword:", test, 256);
  //debug_msg(L"pRequestProcW returned ", test, L" as pasword");
  return 0;
}

void __stdcall FsSetDefaultParams(FsDefaultParamStruct* dps)
{
  debug_msg(L"FsSetDefaultParams called");
  // INI-Pfad an Plugin weitergeben damit es seine Config lesen kann
  memset(dps, 0, sizeof(FsDefaultParamStruct));
}

HANDLE __stdcall FsFindFirstW(WCHAR* path, WIN32_FIND_DATA* data)
{
  if (path == nullptr || data == nullptr)
  {
    debug_msg(L"FsFindFirstW invalid parameters");
    return INVALID_HANDLE_VALUE;
  }

  debug_msg(L"FsFindFirstW called for ", path);


  std::filesystem::path p = map_filename(path);
  p /= L"*";
  const WCHAR* ptr = p.c_str();

  debug_msg(L"FsFindFirstW mapping to ", ptr);

  auto h = FindFirstFileW(ptr, data);
  if (h == INVALID_HANDLE_VALUE)
  {
    auto err = GetLastError();
    SetLastError(err);
  }

  return h;


  SetLastError(ERROR_NO_MORE_FILES);

  return INVALID_HANDLE_VALUE;
}

BOOL __stdcall FsFindNextW(HANDLE hdl, WIN32_FIND_DATA* data)
{
  debug_msg(L"FsFindNextW called");

  auto h = FindNextFileW(hdl, data);

  return h;
  return FALSE;
}

int __stdcall FsFindClose(HANDLE hdl)
{
  debug_msg(L"FsFindClose called");

  return FindClose(hdl);

  return 0;
}

int __stdcall FsGetFileW(WCHAR* RemoteName, WCHAR* LocalName, int CopyFlags,
                         RemoteInfoStruct* ri)
{
  std::filesystem::path p = map_filename(RemoteName);

  debug_msg(L"FsGetFileW called with RemoteName=", RemoteName, L" and LocalName=", LocalName,
            L" RemoteName mapped to ", p.c_str(), L" CopyFlags=", CopyFlags);

  //FS_COPYFLAGS_OVERWRITE 1
  //FS_COPYFLAGS_RESUME 2
  //FS_COPYFLAGS_MOVE 4

  BOOL overWrite = (CopyFlags & FS_COPYFLAGS_OVERWRITE) != 0;
  // test overwrite mechanism
  // if(!overWrite)
  //  return FS_FILE_EXISTS;

  debug_msg(overWrite ? L"Overwrite!" : L"No overwritte");

  const BOOL success = CopyFileW(p.c_str(), LocalName, !overWrite);

  debug_msg(success ? L"Copied." : L"Copy failed!");

  for (int i = 0; i < 10; i++)
  {
    pluginInfo.progressProc(pluginInfo.PluginNr, RemoteName, LocalName, i*10);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
  }

  if (success == 0)
  {
    return FS_FILE_EXISTS;
  }

  return FS_FILE_OK;

  // return FS_FILE_READERROR;
}

void __stdcall FsGetDefRootName(char* DefRootName, int maxlen) // No Wide-Char version available
{
  debug_msg(L"FsGetDefRootName called");
  strcpy_s(DefRootName, maxlen, "WFX Python bridge");
}
