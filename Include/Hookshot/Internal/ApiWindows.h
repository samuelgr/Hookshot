/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file ApiWindows.h
 *   Common header file for the correct version of the Windows API, along with declarations of
 *   supporting functions.
 **************************************************************************************************/

#pragma once

// Windows header files are sensitive to include order. Compilation will fail if the order is
// incorrect. Top-level macros and headers must come first, followed by headers for other parts
// of system functionality.

// clang-format off

#define NOMINMAX
#include <sdkddkver.h>
#include <windows.h>

// clang-format on

#include <commctrl.h>
#include <psapi.h>
#include <shlobj.h>
#include <shlwapi.h>

namespace Hookshot
{
  /// Retrieves the proper address of a Windows API function. Many Windows API functions have been
  /// moved to lower-level binaries. See
  /// https://docs.microsoft.com/en-us/windows/win32/win7appqual/new-low-level-binaries for more
  /// information. If possible, use the address in the lower-level binary as the original function,
  /// otherwise just use the static address.
  /// @param [in] funcName API function name.
  /// @param [in] funcStaticAddress Static address of the function.
  /// @return Recommended address to use for the Windows API function, which could be the same as
  /// the static address.
  void* GetWindowsApiFunctionAddress(const char* const funcName, void* const funcStaticAddress);
} // namespace Hookshot
