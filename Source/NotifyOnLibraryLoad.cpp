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

#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

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

  /// Representation of the status of notifications for a specific library for which notifications
  /// were requested. Encapsulates all of the concurrency control needed to ensure thread safety in
  /// the event that multiple threads are all loading libraries and sending notifications
  /// concurrently.
  class NotificationStatus
  {
  public:

    enum class EDecision : uint8_t
    {
      /// The library represented by this object should be processed and notifications sent to the
      /// subscribers.
      ProcessThisLibrary,

      /// The library represented by this object should be skipped. No notifications need to be sent
      /// by the caller.
      SkipThisLibrary,

      /// The caller should terminate all processing of notification handlers.
      TerminateAllProcessing,
    };

    inline NotificationStatus(void) : notificationState(kNotificationStateNew) {}

    inline NotificationStatus(const NotificationStatus& other)
        : notificationState(other.notificationState.load(std::memory_order_relaxed))
    {}

    inline NotificationStatus(NotificationStatus&& other) noexcept
        : notificationState(other.notificationState.load(std::memory_order_relaxed))
    {}

    /// Determines if notifications for the library represented by this object were already fully
    /// processed.
    /// @return `true` if so, `false` otherwise.
    inline bool NotificationsWereAlreadyProcessed(void)
    {
      return (kNotificationStateDone == notificationState.load(std::memory_order_relaxed));
    }

    /// Decides how the calling thread should process notifications for the library represented by
    /// this object. This method also implements concurrency control; calling it is sufficient to
    /// "claim ownership" of notification handling for the library associated with this object.
    /// @return Decision enumerator indicating what the caller should do. If the decision is to
    /// process the notifications for the library associated with this object, the caller is marked
    /// as the owner of that library in a thread-safe way.
    EDecision HowShouldNotificationsBeProcessed(void)
    {
      DWORD currentState = kNotificationStateNew;
      DWORD claimedByThisThreadState = GetCurrentThreadId();

      const bool newlyClaimedByThisThread = notificationState.compare_exchange_strong(
          currentState,
          claimedByThisThreadState,
          std::memory_order_acq_rel,
          std::memory_order_relaxed);

      if (true == newlyClaimedByThisThread)
      {
        // The caller is the first to get to the library represented by this object and therefore
        // has successfully claimed it.
        return EDecision::ProcessThisLibrary;
      }
      else if (claimedByThisThreadState == currentState)
      {
        // The calling thread has already claimed the library represented by this object, and the
        // caller has itself been called recursively. Further notifications by this thread will be
        // sent by the initial call, not by the direct caller.
        return EDecision::TerminateAllProcessing;
      }

      while (kNotificationStateDone != currentState)
      {
        // Another thread is already processing the notifications, so this thread will block until
        // it is done. This is necessary to avoid one thread potentially returning from LoadLibrary
        // while another thread is still processing notifications for a dependency. For example, if
        // both libraries A and B depend on C, and both A and B are loaded via LoadLibrary in a
        // different thread, without the blocking, one of the threads might skip notifications for C
        // and directly return while the other is still sending notifications for C.

        WaitOnAddress(
            reinterpret_cast<volatile void*>(&notificationState),
            &currentState,
            sizeof(currentState),
            INFINITE);
        currentState = notificationState.load(std::memory_order_acquire);
      }

      return EDecision::SkipThisLibrary;
    }

    /// Marks the processing of notifications as "completed" for the library whose notification
    /// status is represented by this object.
    inline void FinishProcessingNotifications(void) noexcept
    {
      notificationState.store(kNotificationStateDone, std::memory_order_release);
      WakeByAddressAll(reinterpret_cast<void*>(&notificationState));
    }

  private:

    /// Notifications to the subscribed handlers have not yet been sent.
    static constexpr DWORD kNotificationStateNew = 0;

    /// Notifications to the subscribed handlers have already been sent.
    static constexpr DWORD kNotificationStateDone = static_cast<DWORD>(-1);

    /// Notification state. Will either be one of the above constants or the ID of the thread
    /// processing notifications.
    std::atomic<DWORD> notificationState;
  };

  /// Handler information for a single library to for which a notification is to be sent when it is
  /// loaded.
  struct SHandlers
  {
    /// Status of sending notifications to the handlers for the library represented by this object.
    NotificationStatus status;

    /// Handlers subscribed to the notification for the library represented by this object.
    concurrency::concurrent_vector<
        std::function<void(IHookshot* hookshot, const wchar_t* modulePath)>>
        handlers;
  };

  /// Top-level data structure for holding all subscription information for notifications when a
  /// library is loaded. Keyed by library path, which is case-insensitive.
  static concurrency::concurrent_unordered_map<
      std::wstring,
      SHandlers,
      Infra::Strings::CaseInsensitiveHasher<wchar_t>,
      Infra::Strings::CaseInsensitiveEqualityComparator<wchar_t>>
      subscribersByLibraryPath;

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

    subscribersByLibraryPath[libraryPath].handlers.push_back(std::move(handlerFunc));
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

    // The call to the system's LoadLibrary may have loaded the requested library but also some
    // dependencies of it, any of which could trigger a notification needing to be sent to
    // subscribers.

    for (auto& subscribersForLibrary : subscribersByLibraryPath)
    {
      if (true == subscribersForLibrary.second.status.NotificationsWereAlreadyProcessed()) continue;

      const wchar_t* subscribedLibraryPath = subscribersForLibrary.first.c_str();
      HMODULE subscribedLibraryHandle = NULL;
      if (0 ==
          Protected::Windows_GetModuleHandleExW(
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              subscribedLibraryPath,
              &subscribedLibraryHandle))
        continue;

      switch (subscribersForLibrary.second.status.HowShouldNotificationsBeProcessed())
      {
        case NotificationStatus::EDecision::SkipThisLibrary:
          continue;
        case NotificationStatus::EDecision::TerminateAllProcessing:
          return loadResult;
        default:
          break;
      }

      Infra::TemporaryString loadedModulePath;
      loadedModulePath.UnsafeSetSize(Protected::Windows_GetModuleFileNameW(
          subscribedLibraryHandle, loadedModulePath.Data(), loadedModulePath.Capacity()));
      if (false == loadedModulePath.Empty())
      {
        for (auto& subscribedHandler : subscribersForLibrary.second.handlers)
          subscribedHandler(
              LibraryInterface::GetHookshotInterfacePointer(), loadedModulePath.AsCString());
      }

      subscribersForLibrary.second.status.FinishProcessingNotifications();
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
