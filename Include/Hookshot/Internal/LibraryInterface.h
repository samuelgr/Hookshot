/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file LibraryInterface.h
 *   Declaration of support functionality for Hookshot's library interface.
 **************************************************************************************************/

#pragma once

#include <string_view>

#include "Globals.h"
#include "HookshotInterface.h"

namespace Hookshot
{
  namespace LibraryInterface
  {
    /// Retrieves the Hookshot interface object pointer that can be passed to external clients.
    /// @return Hook interface object pointer.
    IHookshot* GetHookshotInterfacePointer(void);

    /// Performs common top-level initialization operations. Idempotent. Any initialization steps
    /// that must happen irrespective of how this library was loaded should go here. Will fail if
    /// the initialization attempt is inappropriate, duplicate, and so on.
    /// @param [in] loadMethod Hookshot library load method.
    /// @return `true` if successful, `false` otherwise.
    bool Initialize(Globals::ELoadMethod loadMethod);

    /// Attempts to load and initialize all applicable hook modules.
    /// @return Number of hook modules successfully loaded.
    int LoadHookModules(void);

    /// Attempts to load and initialize all applicable inject-only libraries.
    /// @return Number of inject-only libraries successfully loaded.
    int LoadInjectOnlyLibraries(void);
  } // namespace LibraryInterface
} // namespace Hookshot
