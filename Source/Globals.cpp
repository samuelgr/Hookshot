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

#include <Infra/Core/Message.h>
#include <Infra/Core/ProcessInfo.h>

#include "ApiWindows.h"

#include "GitVersionInfo.generated.h"

#ifndef HOOKSHOT_SKIP_CONFIG
#include <Infra/Core/Configuration.h>

#include "HookshotConfigReader.h"
#include "Strings.h"
#endif

#include <mutex>
#include <string_view>

INFRA_DEFINE_PRODUCT_NAME_FROM_RESOURCE(
    Infra::ProcessInfo::GetThisModuleInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME);
INFRA_DEFINE_PRODUCT_VERSION_FROM_GIT_VERSION_INFO();

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

      /// Method by which Hookshot was loaded into the current process.
      ELoadMethod gLoadMethod;

    private:

      GlobalData(void) : gLoadMethod(ELoadMethod::Executed) {}

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
    static void EnableLog(Infra::Message::ESeverity logLevel)
    {
      static std::once_flag enableLogFlag;
      std::call_once(
          enableLogFlag,
          [logLevel]() -> void
          {
            Infra::Message::CreateAndEnableLogFile(Strings::GetHookshotLogFilename());
          });

      Infra::Message::SetMinimumSeverityForOutput(logLevel);
    }

    /// Enables the log, if it is configured in the configuration file.
    static void EnableLogIfConfigured(void)
    {
      const int64_t logLevel = GetConfigurationData()
                                   .GetFirstIntegerValue(
                                       Infra::Configuration::kSectionNameGlobal,
                                       Strings::kStrConfigurationSettingNameLogLevel)
                                   .value_or(0);

      if (logLevel > 0)
      {
        // Offset the requested severity so that 0 = disabled, 1 = error, 2 = warning, etc.
        const Infra::Message::ESeverity configuredSeverity = static_cast<Infra::Message::ESeverity>(
            logLevel +
            static_cast<int64_t>(Infra::Message::ESeverity::LowerBoundConfigurableValue));
        EnableLog(configuredSeverity);
      }
    }

    const Infra::Configuration::ConfigurationData& GetConfigurationData(void)
    {
      static Infra::Configuration::ConfigurationData configData;

      static std::once_flag readConfigFlag;
      std::call_once(
          readConfigFlag,
          []() -> void
          {
            HookshotConfigReader configReader;

            configData =
                configReader.ReadConfigurationFile(Strings::GetHookshotConfigurationFilename());

            if (true == configData.HasReadErrors())
            {
              EnableLog(Infra::Message::ESeverity::Error);

              Infra::Message::Output(
                  Infra::Message::ESeverity::Error,
                  L"Errors were encountered during configuration file reading.");
              for (const auto& readError : configData.GetReadErrorMessages())
                Infra::Message::OutputFormatted(
                    Infra::Message::ESeverity::Error, L"    %s", readError.c_str());

              Infra::Message::Output(
                  Infra::Message::ESeverity::ForcedInteractiveWarning,
                  L"Errors were encountered during configuration file reading. See log file on the Desktop for more information.");
            }
          });

      return configData;
    }
#endif

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

    void Initialize(ELoadMethod loadMethod)
    {
      GlobalData::GetInstance().gLoadMethod = loadMethod;

#ifndef HOOKSHOT_SKIP_CONFIG
      EnableLogIfConfigured();
#endif
    }
  } // namespace Globals
} // namespace Hookshot
