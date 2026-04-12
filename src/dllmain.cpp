// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "format_string.h"

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
      debug_msg(L"DllMain DLL_PROCESS_ATTACH called");
      break;
    case DLL_THREAD_ATTACH:
      debug_msg(L"DllMain DLL_THREAD_ATTACH called");
      break;
    case DLL_THREAD_DETACH:
      debug_msg(L"DllMain DLL_THREAD_DETACH called");
      break;
    case DLL_PROCESS_DETACH:
      debug_msg(L"DllMain DLL_PROCESS_DETACH called");
      break;
    default:
      debug_msg(L"DllMain unknown reason called");
      break;
  }
  return TRUE;
}

