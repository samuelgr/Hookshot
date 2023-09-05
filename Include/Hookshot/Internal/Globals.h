/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file Globals.h
 *   Declaration of a namespace for storing and retrieving global data.
 *   Intended for miscellaneous data elements with no other suitable place.
 **************************************************************************************************/

#pragma once

#include "ApiWindows.h"

#ifndef HOOKSHOT_SKIP_CONFIG
#include "Configuration.h"
#include "HookshotConfigReader.h"
#endif

#include <string_view>

namespace Hookshot
{
  namespace Globals
  {

    /// Version information structure.
    struct SVersionInfo
    {
      /// Major version number.
      uint16_t major;

      /// Minor version number.
      uint16_t minor;

      /// Patch level.
      uint16_t patch;

      union
      {
        /// Complete view of the flags element of structured version information.
        uint16_t flags;

        // Per Microsoft documentation, bit fields are ordered from low bit to high bit.
        // See https://docs.microsoft.com/en-us/cpp/cpp/cpp-bit-fields for more information.
        struct
        {
          /// Whether or not the working directory was dirty when the binary was built.
          uint16_t isDirty : 1;

          /// Unused bits, reserved for future use.
          uint16_t reserved : 3;

          /// Number of commits since the most recent official version tag.
          uint16_t commitDistance : 12;
        };
      };

      /// String representation of the version information, including any suffixes. Guaranteed
      /// to be null-terminated.
      std::wstring_view string;
    };

    static_assert(
        sizeof(SVersionInfo) == ((4 * sizeof(uint16_t)) + sizeof(std::wstring_view)),
        "Version information structure size constraint violation.");

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
    const Configuration::ConfigurationData& GetConfigurationData(void);
#endif

    /// Retrieves a pseudohandle to the current process.
    /// @return Current process pseudohandle.
    HANDLE GetCurrentProcessHandle(void);

    /// Retrieves the PID of the current process.
    /// @return Current process PID.
    DWORD GetCurrentProcessId(void);

    /// Retrieves the method by which this form of Hookshot was loaded.
    /// @return Method by which Hookshot was loaded.
    ELoadMethod GetHookshotLoadMethod(void);

    /// Retrieves a string representation of the method by which this form of Hookshot was loaded.
    /// @return String representation of the method by which Hookshot was loaded.
    std::wstring_view GetHookshotLoadMethodString(void);

    /// Retrieves the handle of the instance that represents the current running form of Hookshot,
    /// be it the library or the bootstrap executable.
    /// @return Instance handle for the loaded module.
    HINSTANCE GetInstanceHandle(void);

    /// Retrieves information on the current system. This includes architecture, page size, and so
    /// on.
    /// @return Reference to a read-only structure containing system information.
    const SYSTEM_INFO& GetSystemInformation(void);

    /// Retrieves and returns version information for this running binary.
    /// @return Version information structure.
    SVersionInfo GetVersion(void);

    /// Performs run-time initialization.
    /// @param [in] loadMethod Hookshot library load method.
    void Initialize(ELoadMethod loadMethod);
  }; // namespace Globals
} // namespace Hookshot
