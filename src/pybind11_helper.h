#pragma once

#include "pch.h"

namespace py = pybind11;

static void showPythonError(py::error_already_set& e) noexcept
{
  try
  {
    std::string msg = e.what(); // immer verfügbar, wirft nie

    // Versuche vollständigen Traceback zu bekommen
    try
    {
      py::module_ tb = py::module_::import("traceback");
      py::module_ io = py::module_::import("io");
      py::object  sio = io.attr("StringIO")();
      tb.attr("print_exc")(py::arg("file") = sio);
      msg = sio.attr("getvalue")().cast<std::string>();
    }
    catch (...) { } // Fallback auf e.what() wenn Traceback fehlschlägt

    MessageBoxA(0, msg.c_str(), "Python Error", MB_ICONERROR);
  }
  catch (...) { } // absoluter Fallback – nie crashen
}

// Kein noexcept, expliziter Rückgabetyp statt auto
template<typename Func, typename T>
T pyCall(Func&& f, T fallback)
{
  try
  {
    return f();
  }
  catch (py::error_already_set& e)
  {
    showPythonError(e);
    return fallback;
  }
  catch (py::stop_iteration&)
  {
    return fallback;
  }
  catch (std::exception& e)
  {
    MessageBoxA(0, e.what(), "WFX Bridge Error", MB_ICONERROR);
    return fallback;
  }
  catch (...)
  {
    MessageBoxA(0, "Unknown error", "WFX Bridge Error", MB_ICONERROR);
    return fallback;
  }
}
