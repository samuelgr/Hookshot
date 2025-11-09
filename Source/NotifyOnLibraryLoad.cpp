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

#include "NotifyOnLibraryLoad.h"

#include <unordered_map>
#include <vector>

#include <Infra/Core/Message.h>
#include <Infra/Core/Mutex.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>
#include <Infra/Core/WindowsUtilities.h>

#include "ApiWindows.h"
#include "DependencyProtect.h"
#include "HookshotTypes.h"
#include "InternalHook.h"
#include "LibraryInterface.h"

namespace Hookshot
{
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryA);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryExA);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryW);
  HOOKSHOT_INTERNAL_HOOK(LoadLibraryExW);

  /// Handler information for a single library to for which a notification is to be sent when it is
  /// loaded.
  struct SHandlers
  {
    /// Whether or not a notification needs to be sent to the handlers in this structure.
    bool needsNotification;

    /// Handlers subscribed to the notification for the library represented by this object.
    std::vector<std::function<void(IHookshot* hookshot, const wchar_t* modulePath)>> handlers;
  };

  /// Top-level data structure for holding all subscription information for notifications when a
  /// library is loaded. Keyed by library path, which is case-insensitive.
  static std::unordered_map<
      std::wstring,
      SHandlers,
      Infra::Strings::CaseInsensitiveHasher<wchar_t>,
      Infra::Strings::CaseInsensitiveEqualityComparator<wchar_t>>
      subscribersByLibraryPath;

  /// Mutex that guards access to the top-level subscription information data structure. It is
  /// recursive so that mutual exclusion between threads is ensured but that notification handlers
  /// can safely load additional libraries from the same thread.
  static Infra::RecursiveMutex subscribersMutex;

  EResult SetNotificationOnLibraryLoad(
      const wchar_t* libraryPath,
      std::function<void(IHookshot* hookshot, const wchar_t* modulePath)> handlerFunc)
  {
    HMODULE alreadyLoadedModuleHandle = NULL;
    if (0 !=
        Protected::Windows_GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, libraryPath, &alreadyLoadedModuleHandle))
    {
      Infra::TemporaryString alreadyLoadedModulePath;
      alreadyLoadedModulePath.UnsafeSetSize(Protected::Windows_GetModuleFileNameW(
          alreadyLoadedModuleHandle,
          alreadyLoadedModulePath.Data(),
          alreadyLoadedModulePath.Capacity()));
      if (true == alreadyLoadedModulePath.Empty()) return EResult::FailInternal;

      handlerFunc(
          LibraryInterface::GetHookshotInterfacePointer(), alreadyLoadedModulePath.AsCString());
      return EResult::Success;
    }

    std::unique_lock lock(subscribersMutex);

    auto subscribersIter =
        subscribersByLibraryPath.emplace(libraryPath, SHandlers{.needsNotification = true}).first;
    subscribersIter->second.handlers.emplace_back(handlerFunc);

    return EResult::Success;
  }

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

  static HMODULE LoadLibraryInternal(
      const wchar_t* entryPointName, LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    HMODULE loadResult = InternalHook_LoadLibraryExW::Original(lpLibFileName, hFile, dwFlags);
    if (false == IsLoadingAsExecutableCode(dwFlags)) return loadResult;

    std::unique_lock lock(subscribersMutex);

    // The call to the system's LoadLibrary may have loaded the requested library but also some
    // dependencies of it, any of which must trigger a notification. The notification only needs to
    // be triggered when a library is newly-loaded.
    for (auto& subscribersForLibrary : subscribersByLibraryPath)
    {
      if (false == subscribersForLibrary.second.needsNotification) continue;

      const wchar_t* subscribedLibraryPath = subscribersForLibrary.first.c_str();
      HMODULE subscribedLibraryHandle = NULL;
      if (0 ==
          Protected::Windows_GetModuleHandleExW(
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              subscribedLibraryPath,
              &subscribedLibraryHandle))
        continue;

      // A notification handler function may itself trigger additional library loads, which means
      // this function can be called again during a handler. The library notification needs to be
      // marked as having been handled before any handlers are called so that recursive instances of
      // this function do not trigger additional notifications.
      subscribersForLibrary.second.needsNotification = false;

      Infra::TemporaryString loadedModulePath;
      loadedModulePath.UnsafeSetSize(Protected::Windows_GetModuleFileNameW(
          subscribedLibraryHandle, loadedModulePath.Data(), loadedModulePath.Capacity()));
      if (true == loadedModulePath.Empty()) continue;

      for (auto& subscribedHandler : subscribersForLibrary.second.handlers)
        subscribedHandler(
            LibraryInterface::GetHookshotInterfacePointer(), loadedModulePath.AsCString());
    }

    return loadResult;
  }

  void* InternalHook_LoadLibraryA::OriginalFunctionAddress(void)
  {
    return Infra::Windows::GetRealApiFunctionAddress("LoadLibraryA", &LoadLibraryA);
  }

  HMODULE InternalHook_LoadLibraryA::Hook(LPCSTR lpLibFileName)
  {
    Infra::TemporaryString lpLibFileNameWide = lpLibFileName;
    return LoadLibraryInternal(GetFunctionName(), lpLibFileNameWide.AsCString(), NULL, 0);
  }

  void* InternalHook_LoadLibraryExA::OriginalFunctionAddress(void)
  {
    return Infra::Windows::GetRealApiFunctionAddress("LoadLibraryExA", &LoadLibraryExA);
  }

  HMODULE InternalHook_LoadLibraryExA::Hook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    Infra::TemporaryString lpLibFileNameWide = lpLibFileName;
    return LoadLibraryInternal(GetFunctionName(), lpLibFileNameWide.AsCString(), hFile, dwFlags);
  }

  void* InternalHook_LoadLibraryW::OriginalFunctionAddress(void)
  {
    return Infra::Windows::GetRealApiFunctionAddress("LoadLibraryW", &LoadLibraryW);
  }

  HMODULE InternalHook_LoadLibraryW::Hook(LPCWSTR lpLibFileName)
  {
    return LoadLibraryInternal(GetFunctionName(), lpLibFileName, NULL, 0);
  }

  void* InternalHook_LoadLibraryExW::OriginalFunctionAddress(void)
  {
    return Infra::Windows::GetRealApiFunctionAddress("LoadLibraryExW", &LoadLibraryExW);
  }

  HMODULE InternalHook_LoadLibraryExW::Hook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
  {
    return LoadLibraryInternal(GetFunctionName(), lpLibFileName, hFile, dwFlags);
  }

} // namespace Hookshot
