/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file NotifyOnLibraryLoad.cpp
 *   Implementation of an event notification for when a DLL is loaded.
 **************************************************************************************************/

#include <set>

#include <Infra/Core/Message.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "InternalHook.h"

namespace Hookshot
{
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryA);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryExA);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryW);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryExW);

  /// Determines if the library loading flags specified mean that the library is being loaded as
  /// executable code.
  /// @param [in] dwFlags LoadLibrary API flags to check.
  /// @return `true` if the flags indicate loading as executable code, `false` otherwise.
  static inline bool IsLoadingAsExecutableCode(DWORD dwFlags)
  {
    return (
        0 ==
        (dwFlags &
         (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
          LOAD_LIBRARY_AS_IMAGE_RESOURCE)));
  }

  HMODULE LoadLibraryInternal(
      const wchar_t* entryPointName, LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    if (false == IsLoadingAsExecutableCode(dwFlags))
      return InternalHook_LoadLibraryExW::Original(lpLibFileName, hFile, dwFlags);

    static std::set<std::wstring, Infra::Strings::CaseInsensitiveLessThanComparator<wchar_t>>
        seenLibraries;

    HMODULE loadResult = InternalHook_LoadLibraryExW::Original(lpLibFileName, hFile, dwFlags);
    if (NULL != loadResult)
    {
      Infra::TemporaryString loadedModuleFileName;
      loadedModuleFileName.UnsafeSetSize(GetModuleFileNameW(
          loadResult, loadedModuleFileName.Data(), loadedModuleFileName.Capacity()));

      // If the library is not newly-loaded then both it and all of its dependencies were already
      // previously loaded, so no new notifications need to be generated.
      if (true == seenLibraries.emplace(loadedModuleFileName.AsStringView()).second)
      {
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Info,
            L"%s: Requested %s, loaded %s",
            entryPointName,
            lpLibFileName,
            loadedModuleFileName.AsCString());
      }
    }

    return loadResult;
  }

  void* InternalHook_LoadLibraryA::OriginalFunctionAddress(void)
  {
    return GetWindowsApiFunctionAddress("LoadLibraryA", &LoadLibraryA);
  }

  HMODULE InternalHook_LoadLibraryA::Hook(LPCSTR lpLibFileName)
  {
    Infra::TemporaryString lpLibFileNameWide = lpLibFileName;
    return LoadLibraryInternal(GetFunctionName(), lpLibFileNameWide.AsCString(), NULL, 0);
  }

  void* InternalHook_LoadLibraryExA::OriginalFunctionAddress(void)
  {
    return GetWindowsApiFunctionAddress("LoadLibraryExA", &LoadLibraryExA);
  }

  HMODULE InternalHook_LoadLibraryExA::Hook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    Infra::TemporaryString lpLibFileNameWide = lpLibFileName;
    return LoadLibraryInternal(GetFunctionName(), lpLibFileNameWide.AsCString(), hFile, dwFlags);
  }

  void* InternalHook_LoadLibraryW::OriginalFunctionAddress(void)
  {
    return GetWindowsApiFunctionAddress("LoadLibraryW", &LoadLibraryW);
  }

  HMODULE InternalHook_LoadLibraryW::Hook(LPCWSTR lpLibFileName)
  {
    return LoadLibraryInternal(GetFunctionName(), lpLibFileName, NULL, 0);
  }

  void* InternalHook_LoadLibraryExW::OriginalFunctionAddress(void)
  {
    return GetWindowsApiFunctionAddress("LoadLibraryExW", &LoadLibraryExW);
  }

  HMODULE InternalHook_LoadLibraryExW::Hook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    return LoadLibraryInternal(GetFunctionName(), lpLibFileName, hFile, dwFlags);
  }

} // namespace Hookshot
