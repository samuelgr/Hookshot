/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file DependencyProtect.cpp
 *   Implementation of dependency protection functionality.
 **************************************************************************************************/

#pragma once

#include <intrin.h>

#include <cstdint>
#include <unordered_map>

#include "ApiWindows.h"
#include "DebugAssert.h"

namespace Hookshot
{
  /// Registry of all protected dependencies. Key is the current known address of the protected
  /// dependency. Value is the address of the protected dependency pointer.
  static std::unordered_map<const void*, const void* volatile*> protectedDependencies;

  /// Initializes a protected dependency pointer.
  /// Returns the address passed in after registering the protected dependency in the registry.
  /// @param [in] address Initial address of the protected dependency function.
  /// @param [in] protectedDependencyPointer Address of the protected dependency function pointer.
  static const void* InitializeProtectedDependencyAddress(
      const void* address, const void* volatile* protectedDependencyPointer)
  {
    DebugAssert(
        0 == protectedDependencies.count(address),
        "Initializing a protected dependency that already exists.");

    protectedDependencies[address] = protectedDependencyPointer;
    return address;
  }

  /// Retrieves a protected dependency pointer for Windows protected dependency functions.
  /// Many Windows API functions have been moved to lower-level binaries.
  /// See https://docs.microsoft.com/en-us/windows/win32/win7appqual/new-low-level-binaries for more
  /// information. If possible, use the address in the lower-level binary as the original function,
  /// otherwise just use the static address.
  /// @param [in] funcQualifiedName Fully-qualified function name.
  /// @param [in] funcBaseName Base name of the function without any scoping qualifiers.
  /// @param [in] funcStaticAddress Static address of the function.
  /// @return Address to use for the initial protected dependency pointer.
  static const void* GetInitialAddress_Windows(
      const char* const funcQualifiedName,
      const char* const funcBaseName,
      void* const funcStaticAddress)
  {
    return GetWindowsApiFunctionAddress(funcBaseName, funcStaticAddress);
  }
} // namespace Hookshot

/// Defines and initializes a type-safe protected dependency pointer.
/// First parameter specifies the namespace path, and second the function name.
/// Initial address is returned by a function GetInitialAddress_nspace(const char* const
/// funcQualifiedName, const char* const funcBaseName, void* const funcStaticAddress). Therefore,
/// this function must exists for each protected dependency namespace. Simplest implementation is
/// just to return funcStaticAddress.
#define PROTECTED_DEPENDENCY(qualpath, nspace, func)                                               \
  extern const volatile decltype(&qualpath::func) nspace##_##func =                                \
      (decltype(&qualpath::func))InitializeProtectedDependencyAddress(                             \
          GetInitialAddress_##nspace(#qualpath "::" #func, #func, &qualpath::func),                \
          (const void* volatile*)&nspace##_##func)

// Variables are imported from "DependencyProtect.h" and defined.

#include "DependencyProtect.h"

namespace Hookshot
{
  void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress)
  {
    if (0 != protectedDependencies.count(oldAddress))
    {
      DebugAssert(
          0 == protectedDependencies.count(newAddress),
          "New protected dependency address already exists.");

      const void* volatile* const pointerToUpdate = protectedDependencies.at(oldAddress);

      protectedDependencies.erase(oldAddress);
      protectedDependencies.insert({newAddress, pointerToUpdate});

      *pointerToUpdate = newAddress;
      _mm_mfence();
    }
  }
} // namespace Hookshot
