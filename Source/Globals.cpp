/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Globals.cpp
 *   Implementation of accessors and mutators for global data items. Intended for miscellaneous
 *   data elements with no other suitable place.
 **************************************************************************************************/

#include "Globals.h"

#include "ApiWindows.h"
#include "Message.h"
#include "Strings.h"

#include "GitVersionInfo.generated.h"

#ifndef HOOKSHOT_SKIP_CONFIG
#include "Configuration.h"
#include "HookshotConfigReader.h"
#endif

#include <mutex>
#include <string_view>

namespace Hookshot
{
  namespace Globals
  {
    /// Holds all static data that falls under the global category.
    /// Used to make sure that globals are initialized as early as possible so that values are
    /// available during dynamic initialization. Implemented as a singleton object.
    class GlobalData
    {
    public:

      /// Pseudohandle of the current process.
      HANDLE gCurrentProcessHandle;

      /// PID of the current process.
      DWORD gCurrentProcessId;

      /// Handle of the instance that represents the running form of Hookshot.
      HINSTANCE gInstanceHandle;

      /// Method by which Hookshot was loaded into the current process.
      ELoadMethod gLoadMethod;

      /// Holds information about the current system, as retrieved from Windows.
      SYSTEM_INFO gSystemInformation;

    private:

      GlobalData(void)
          : gCurrentProcessHandle(GetCurrentProcess()),
            gCurrentProcessId(GetProcessId(GetCurrentProcess())),
            gInstanceHandle(nullptr),
            gLoadMethod(ELoadMethod::Executed),
            gSystemInformation()
      {
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (LPCWSTR)&GlobalData::GetInstance,
            &gInstanceHandle);
        GetNativeSystemInfo(&gSystemInformation);
      }

      GlobalData(const GlobalData& other) = delete;

    public:

      /// Returns a reference to the singleton instance of this class.
      /// @return Reference to the singleton instance.
      static GlobalData& GetInstance(void)
      {
        static GlobalData globalData;
        return globalData;
      }
    };

#ifndef HOOKSHOT_SKIP_CONFIG
    /// Enables the log if it is not already enabled.
    /// Regardless, the minimum severity for output is set based on the parameter.
    /// @param [in] logLevel Logging level to configure as the minimum severity for output.
    static void EnableLog(Message::ESeverity logLevel)
    {
      static std::once_flag enableLogFlag;
      std::call_once(
          enableLogFlag,
          [logLevel]() -> void
          {
            Message::CreateAndEnableLogFile();
          });

      Message::SetMinimumSeverityForOutput(logLevel);
    }

    /// Enables the log, if it is configured in the configuration file.
    static void EnableLogIfConfigured(void)
    {
      const int64_t logLevel =
          GetConfigurationData()
              .GetFirstIntegerValue(
                  Configuration::kSectionNameGlobal, Strings::kStrConfigurationSettingNameLogLevel)
              .value_or(0);

      if (logLevel > 0)
      {
        // Offset the requested severity so that 0 = disabled, 1 = error, 2 = warning, etc.
        const Message::ESeverity configuredSeverity = static_cast<Message::ESeverity>(
            logLevel + static_cast<int64_t>(Message::ESeverity::LowerBoundConfigurableValue));
        EnableLog(configuredSeverity);
      }
    }

    const Configuration::ConfigurationData& GetConfigurationData(void)
    {
      static Configuration::ConfigurationData configData;

      static std::once_flag readConfigFlag;
      std::call_once(
          readConfigFlag,
          []() -> void
          {
            HookshotConfigReader configReader;

            configData =
                configReader.ReadConfigurationFile(Strings::kStrHookshotConfigurationFilename);

            if (true == configData.HasReadErrors())
            {
              EnableLog(Message::ESeverity::Error);

              Message::Output(
                  Message::ESeverity::Error,
                  L"Errors were encountered during configuration file reading.");
              for (const auto& readError : configData.GetReadErrorMessages())
                Message::OutputFormatted(Message::ESeverity::Error, L"    %s", readError.c_str());

              Message::Output(
                  Message::ESeverity::ForcedInteractiveWarning,
                  L"Errors were encountered during configuration file reading. See log file on the Desktop for more information.");
            }
          });

      return configData;
    }
#endif

    HANDLE GetCurrentProcessHandle(void)
    {
      return GlobalData::GetInstance().gCurrentProcessHandle;
    }

    DWORD GetCurrentProcessId(void)
    {
      return GlobalData::GetInstance().gCurrentProcessId;
    }

    ELoadMethod GetHookshotLoadMethod(void)
    {
      return GlobalData::GetInstance().gLoadMethod;
    }

    std::wstring_view GetHookshotLoadMethodString(void)
    {
      switch (GlobalData::GetInstance().gLoadMethod)
      {
        case ELoadMethod::Executed:
          return L"EXECUTED";

        case ELoadMethod::Injected:
          return L"INJECTED";

        case ELoadMethod::LibraryLoaded:
          return L"LIBRARY_LOADED";

        default:
          return L"UNKNOWN";
      }
    }

    HINSTANCE GetInstanceHandle(void)
    {
      return GlobalData::GetInstance().gInstanceHandle;
    }

    const SYSTEM_INFO& GetSystemInformation(void)
    {
      return GlobalData::GetInstance().gSystemInformation;
    }

    SVersionInfo GetVersion(void)
    {
      constexpr uint16_t kVersionStructured[] = {GIT_VERSION_STRUCT};
      static_assert(4 == _countof(kVersionStructured), "Invalid structured version information.");

      return {
          .major = kVersionStructured[0],
          .minor = kVersionStructured[1],
          .patch = kVersionStructured[2],
          .flags = kVersionStructured[3],
          .string = _CRT_WIDE(GIT_VERSION_STRING)};
    }

    void Initialize(ELoadMethod loadMethod)
    {
      GlobalData::GetInstance().gLoadMethod = loadMethod;

#ifndef HOOKSHOT_SKIP_CONFIG
      EnableLogIfConfigured();
#endif
    }
  } // namespace Globals
} // namespace Hookshot
