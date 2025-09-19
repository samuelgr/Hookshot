/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file NotifyOnLibraryLoad.h
 *   Declaration of an event notification for when a DLL is loaded.
 **************************************************************************************************/

#pragma once

#include <functional>

#include "HookshotTypes.h"

namespace Hookshot
{
  /// Subscribes to a notification for when a library is loaded. The notification is sent
  /// immediately if the library is already lodaed or at some point in the future when it gets
  /// loaded.
  /// @param [in] libraryPath Library path for which a notification is requested.
  /// @param [in] handlerFunc Handler fucntion object to invoke when the library is loaded.
  /// @return Result of attempting to subscribe to the notification.
  EResult SetNotificationOnLibraryLoad(
      const wchar_t* libraryPath,
      std::function<void(IHookshot* hookshot, const wchar_t* modulePath)> handlerFunc);
} // namespace Hookshot
