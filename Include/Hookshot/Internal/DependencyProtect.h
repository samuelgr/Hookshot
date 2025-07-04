/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file DependencyProtect.h
 *   Declaration of dependency protection functionality. This is intended to
 *   ensure that certain parts of Hookshot continue to function correctly even
 *   when they are hooked by Hookshot users so that users do not need to worry
 *   about how Hookshot is implemented (for the most part) when setting hooks.
 **************************************************************************************************/

#pragma once

#include "ApiWindows.h"

// This macro is additionally defined in "DependencyProtect.cpp" to instantiate each pointer rather
// than just declare it.
#ifndef PROTECTED_DEPENDENCY

/// Declares a type-safe protected dependency pointer.
/// First parameter specifies the desired namespace for the protected dependency pointer.
/// Second parameter specifies the function's namespace qualifier path, (i.e. where to find the
/// function). Third parameter specifies the function name within its qualifying namespace.
#define PROTECTED_DEPENDENCY(qualpath, nspace, func)                                               \
  extern const volatile decltype(&qualpath::func) nspace##_##func

#endif

namespace Hookshot
{
  namespace Protected
  {
    // Pointers to protected versions of various API functions. Each declaration produces a
    // read-only (but updated behind-the-scenes) function pointer. Naming convention is
    // "[second macro parameter]_[third macro parameter]" for each function pointer.

    PROTECTED_DEPENDENCY(, Windows, CloseHandle);
    PROTECTED_DEPENDENCY(, Windows, CreateFileMappingW);
    PROTECTED_DEPENDENCY(, Windows, CreateProcessW);
    PROTECTED_DEPENDENCY(, Windows, DuplicateHandle);
    PROTECTED_DEPENDENCY(, Windows, FindClose);
    PROTECTED_DEPENDENCY(, Windows, FindFirstFileExW);
    PROTECTED_DEPENDENCY(, Windows, FindNextFileW);
    PROTECTED_DEPENDENCY(, Windows, FlushInstructionCache);
    PROTECTED_DEPENDENCY(, Windows, FormatMessageW);
    PROTECTED_DEPENDENCY(, Windows, GetExitCodeProcess);
    PROTECTED_DEPENDENCY(, Windows, GetLastError);
    PROTECTED_DEPENDENCY(, Windows, GetModuleHandleExW);
    PROTECTED_DEPENDENCY(, Windows, GetProcAddress);
    PROTECTED_DEPENDENCY(, Windows, IsDebuggerPresent);
    PROTECTED_DEPENDENCY(, Windows, LoadLibraryW);
    PROTECTED_DEPENDENCY(, Windows, MessageBoxW);
    PROTECTED_DEPENDENCY(, Windows, MapViewOfFile);
    PROTECTED_DEPENDENCY(, Windows, OutputDebugStringW);
    PROTECTED_DEPENDENCY(, Windows, QueryFullProcessImageNameW);
    PROTECTED_DEPENDENCY(, Windows, ResumeThread);
    PROTECTED_DEPENDENCY(, Windows, SetLastError);
    PROTECTED_DEPENDENCY(, Windows, TerminateProcess);
    PROTECTED_DEPENDENCY(, Windows, UnmapViewOfFile);
    PROTECTED_DEPENDENCY(, Windows, VirtualAlloc);
    PROTECTED_DEPENDENCY(, Windows, VirtualFree);
    PROTECTED_DEPENDENCY(, Windows, VirtualQuery);
    PROTECTED_DEPENDENCY(, Windows, VirtualProtect);
    PROTECTED_DEPENDENCY(, Windows, WaitForSingleObject);
  } // namespace Protected

  /// Updates the address of a protected dependency.
  /// Behind the scenes, this modifies one of the above function pointers to point somewhere else.
  /// Has no effect if the specified old address is not actually one of the protected dependencies.
  /// @param [in] oldAddress Address of one of the above function pointers whose value needs to be
  /// updated.
  /// @param [in] newAddress New address to which the function pointer should point.
  void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress);
} // namespace Hookshot
