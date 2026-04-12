#include "pch.h"

#include "format_string.h"

#include "pybind11_helper.h"

#define MAX_PATH_TC 1024

namespace py = pybind11;

// -------------------------------------------------------
// WFX Callback Functions
// -------------------------------------------------------

struct PluginInfo
{
  int PluginNr;
  tProgressProcW progressProc = 0;
  tLogProcW logProc = 0;
  tRequestProcW requestProc = 0;
};

PluginInfo pluginInfo;



// -------------------------------------------------------
// Globaler State
// -------------------------------------------------------

static py::scoped_interpreter* g_interp = nullptr;
static py::object               g_plugin;  // Instanz der Plugin-Klasse

// Handle → Python-Iterator (für parallele FsFindFirst-Aufrufe)
static std::unordered_map<HANDLE, py::object> g_iterators;
static std::atomic<uintptr_t>                 g_nextHandle{ 1 };



PYBIND11_EMBEDDED_MODULE(tcbridge, m)
{
  m.def("debug_msg", [](const std::wstring& msg)
        {
          OutputDebugStringW(L"debug_msg called from python: ");
          OutputDebugStringW(msg.c_str());
          OutputDebugStringW(L"\n");
        });

  // Fortschrittsbalken in TC anzeigen
  // Rückgabe: 1 wenn User abgebrochen hat, 0 sonst
  m.def("progress", [](const std::wstring& src,
                       const std::wstring& dst,
                       int percent) -> int
        {
          if (pluginInfo.progressProc)
          {
            return pluginInfo.progressProc(pluginInfo.PluginNr,
                                           (WCHAR*)src.c_str(),
                                           (WCHAR*)dst.c_str(),
                                           percent);
          }
          return 0;
        });

  // Log-Meldung an TC schicken
  // msg_type: MSGTYPE_CONNECT=1, MSGTYPE_DISCONNECT=2,
  //           MSGTYPE_DETAILS=3, MSGTYPE_TRANSFERCOMPLETE=4,
  //           MSGTYPE_IMPORTANTERROR=5
  m.def("log", [](int msg_type, const std::wstring& msg)
        {
          if (pluginInfo.logProc)
          {
            pluginInfo.logProc(pluginInfo.PluginNr, msg_type, (WCHAR*)msg.c_str());
          }
        });

  // TC nach Text fragen (z.B. Passwort-Dialog)
  // Rückgabe: eingegebener Text oder None wenn abgebrochen
  m.def("request_text", [](const std::wstring& caption,
                           const std::wstring& prompt,
                           const std::wstring& default_val) -> py::object
        {
          WCHAR buf[MAX_PATH_TC] = {};
          wcscpy_s(buf, default_val.c_str());
          if (pluginInfo.requestProc)
          {
            BOOL ok = pluginInfo.requestProc(pluginInfo.PluginNr,
                                             RT_Other,
                                             (WCHAR*)caption.c_str(),
                                             (WCHAR*)prompt.c_str(),
                                             buf,
                                             MAX_PATH_TC);
            if (ok)
            {
              return py::reinterpret_steal<py::str>(
                PyUnicode_FromWideChar(buf, -1));
            }
          }
          return py::none();
        });

  // Konstanten für log msg_type
  m.attr("MSGTYPE_CONNECT") = 1;
  m.attr("MSGTYPE_DISCONNECT") = 2;
  m.attr("MSGTYPE_DETAILS") = 3;
  m.attr("MSGTYPE_TRANSFERCOMPLETE") = 4;
  m.attr("MSGTYPE_IMPORTANTERROR") = 5;

  // Konstanten für FsGetFile/FsPutFile Rückgabewerte
  m.attr("FS_FILE_OK") = FS_FILE_OK;
  m.attr("FS_FILE_EXISTS") = FS_FILE_EXISTS;
  m.attr("FS_FILE_NOTFOUND") = FS_FILE_NOTFOUND;
  m.attr("FS_FILE_READERROR") = FS_FILE_READERROR;
  m.attr("FS_FILE_WRITEERROR") = FS_FILE_WRITEERROR;
  m.attr("FS_FILE_USERABORT") = FS_FILE_USERABORT;
}

static void fillFindData(WIN32_FIND_DATA* data, py::object entry)
{
  memset(data, 0, sizeof(WIN32_FIND_DATA));

  // Pflichtfeld: Dateiname
  std::wstring name = entry.attr("name").cast<std::wstring>();
  wcscpy_s(data->cFileName, MAX_PATH - 1, name.c_str());

  // Verzeichnis oder Datei
  bool is_dir = entry.attr("is_dir").cast<bool>();
  data->dwFileAttributes = is_dir ? FILE_ATTRIBUTE_DIRECTORY
    : FILE_ATTRIBUTE_NORMAL;

  // Dateigröße (bei Verzeichnissen 0)
  if (!is_dir)
  {
    int64_t size = entry.attr("size").cast<int64_t>();
    data->nFileSizeLow = static_cast<DWORD>(size & 0xFFFFFFFF);
    data->nFileSizeHigh = static_cast<DWORD>(size >> 32);
  }

  // Zeitstempel – Python datetime → FILETIME
  if (!entry.attr("mtime").is_none())
  {
    py::object mtime = entry.attr("mtime");

    // datetime.timestamp() → Unix-Sekunden als float
    double unixSecs = mtime.attr("timestamp")().cast<double>();

    // Unix-Epoch (1970) → Windows-Epoch (1601): 116444736000000000 * 100ns
    uint64_t ft = static_cast<uint64_t>(unixSecs * 10000000.0)
      + 116444736000000000ULL;

    data->ftLastWriteTime.dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFF);
    data->ftLastWriteTime.dwHighDateTime = static_cast<DWORD>(ft >> 32);

    // TC zeigt auch CreationTime an wenn gesetzt
    data->ftCreationTime = data->ftLastWriteTime;
  }
  else
  {
    // Kein Timestamp verfügbar – TC-Konvention: max FILETIME
    data->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
    data->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
  }
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
  debug_msg(L"creating scoped_interpreter");

  g_interp = new py::scoped_interpreter{};

  py::module_ sys = py::module_::import("sys");
  sys.attr("path").attr("append")("../../python");

  try
  {
    py::module_ mod = py::module_::import("s3plugin");
    g_plugin = mod.attr("S3Plugin")();  // Instanz anlegen
  }
  catch (std::exception& ex)
  {
    std::string s(ex.what());
    debug_msg_a("hi");
  }

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

  return pyCall([&]() -> HANDLE
                {
                  py::object iterator = g_plugin.attr("find_first")(path);
                  if (iterator.is_none())
                  {
                    SetLastError(ERROR_NO_MORE_FILES);
                    return INVALID_HANDLE_VALUE;
                  }

                  // Iterator in Map speichern, Handle zurückgeben
                  HANDLE h = reinterpret_cast<HANDLE>(g_nextHandle++);
                  g_iterators[h] = iterator;

                  // Ersten Eintrag aus Iterator holen und in FindData füllen
                  py::object entry = iterator.attr("__next__")();
                  fillFindData(data, entry);
                  return h;

                }, INVALID_HANDLE_VALUE);

  SetLastError(ERROR_NO_MORE_FILES);

  return INVALID_HANDLE_VALUE;
}

BOOL __stdcall FsFindNextW(HANDLE hdl, WIN32_FIND_DATA* data)
{
  debug_msg(L"FsFindNextW called");
  auto it = g_iterators.find(hdl);
  if (it == g_iterators.end())
  {
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
  }

  return pyCall([&]() -> BOOL
                {
                  try
                  {
                    py::object entry = it->second.attr("__next__")();
                    fillFindData(data, entry);
                    return TRUE;
                  }
                  catch (py::error_already_set& e)
                  {
                    if (e.matches(PyExc_StopIteration))
                    {
                      g_iterators.erase(hdl);
                      SetLastError(ERROR_NO_MORE_FILES);
                      return FALSE;  // direkt return – erreicht pyCall nie
                    }
                    throw; // nur echte Fehler nach außen
                  }
                }, FALSE);
}

int __stdcall FsFindClose(HANDLE hdl)
{
  g_iterators.erase(hdl);
  return 0;
}

int __stdcall FsGetFileW(WCHAR* RemoteName, WCHAR* LocalName, int CopyFlags,
                         RemoteInfoStruct* ri)
{
  debug_msg(L"FsGetFileW called with RemoteName=", RemoteName, L" and LocalName=", LocalName,
            L" RemoteName mapped to ", p.c_str(), L" CopyFlags=", CopyFlags);
  try
  {
    int result = g_plugin.attr("get_file")(RemoteName, LocalName, CopyFlags).cast<int>();
    return result;
  }
  catch (...) { return FS_FILE_READERROR; }
}

void __stdcall FsGetDefRootName(char* DefRootName, int maxlen) // No Wide-Char version available
{
  debug_msg(L"FsGetDefRootName called");
  strcpy_s(DefRootName, maxlen, "WFX Python bridge");
}
