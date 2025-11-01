/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file Globals.h
 *   Declaration of a namespace for storing and retrieving global data.
 *   Intended for miscellaneous data elements with no other suitable place.
 **************************************************************************************************/

#pragma once

#include "ApiWindows.h"

#ifndef HOOKSHOT_SKIP_CONFIG
#include <Infra/Core/Configuration.h>

#include "HookshotConfigReader.h"
#endif

#include <string_view>

namespace Hookshot
{
  namespace Globals
  {
    /// Enumerates the possible ways Hookshot can be loaded.
    enum class ELoadMethod
    {
      /// Executed directly. This is the default value and is applicable for the executable form of
      /// Hookshot.
      Executed,

      /// Injected. An executable form of Hookshot injected this form of Hookshot into the current
      /// process.
      Injected,

      /// Loaded as a library. Some executable loaded Hookshot using a standard dynamic library
      /// loading technique.
      LibraryLoaded,
    };

#ifndef HOOKSHOT_SKIP_CONFIG
    /// Retrieves the Hookshot configuration data object.
    /// Only useful if IsConfigurationDataValid returns `true`.
    const Infra::Configuration::ConfigurationData& GetConfigurationData(void);
#endif

    /// Retrieves the method by which this form of Hookshot was loaded.
    /// @return Method by which Hookshot was loaded.
    ELoadMethod GetHookshotLoadMethod(void);

    /// Retrieves a string representation of the method by which this form of Hookshot was loaded.
    /// @return String representation of the method by which Hookshot was loaded.
    std::wstring_view GetHookshotLoadMethodString(void);

    /// Performs run-time initialization.
    /// @param [in] loadMethod Hookshot library load method.
    void Initialize(ELoadMethod loadMethod);
  }; // namespace Globals
} // namespace Hookshot
