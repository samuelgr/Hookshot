/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file HookshotConfigReader.cpp
 *   Implementation of Hookshot-specific configuration reading functionality.
 **************************************************************************************************/

#include "HookshotConfigReader.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include <Infra/Core/Configuration.h>
#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "Strings.h"

namespace Hookshot
{
  using namespace ::Infra::Configuration;

  /// Holds the layout of the Hookshot configuration file that is known statically. At compile time,
  /// this encompasses all settings except for the dynamically-determined section whose name matches
  /// the file name of the currently-running executable, which is filled in at runtime.
  static TConfigurationFileLayout configurationFileLayout = {
      ConfigurationFileLayoutSection(
          kSectionNameGlobal,
          {
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameHookModule, EValueType::StringMultiValue),
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameInject, EValueType::StringMultiValue),
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameLogLevel, EValueType::Integer),
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameUseConfiguredHookModules,
                  EValueType::Boolean),
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameLoadHookModulesFromHookshotDirectory,
                  EValueType::Boolean),
          }),
  };

  /// Holds the section name for the per-executable settings. This is dynamically set to the name of
  /// the currently-running executable.
  std::wstring executableSpecificSectionName;

  /// If `true`, indicates that the hookshot configuration file layout definition has been augmented
  /// with runtime information.
  static bool configurationFileLayoutIsComplete = false;

  /// Completes the Hookshot configuration file layout, if not already done, by adding to it a
  /// section whose name is determined at runtime. Supports the same settings as those common
  /// settings applied to all executables in a particular directory, but in this case is specific to
  /// one executable. The section name in question is the same as the base name of the
  /// currently-running executable.
  static inline void CompleteConfigurationFileLayout(void)
  {
    if (false == configurationFileLayoutIsComplete)
    {
      configurationFileLayout.insert(ConfigurationFileLayoutSection(
          Infra::ProcessInfo::GetExecutableBaseName(),
          {
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameHookModule, EValueType::StringMultiValue),
              ConfigurationFileLayoutNameAndValueType(
                  Strings::kStrConfigurationSettingNameInject, EValueType::StringMultiValue),
          }));

      configurationFileLayoutIsComplete = true;
    }
  }

  EAction HookshotConfigReader::ActionForSection(std::wstring_view section)
  {
    if (0 != configurationFileLayout.count(section)) return EAction::Process;

    return EAction::Skip;
  }

  EAction HookshotConfigReader::ActionForValue(
      std::wstring_view section, std::wstring_view name, const TIntegerView value)
  {
    if (value >= 0) return EAction::Process;

    return EAction::Error;
  }

  EAction HookshotConfigReader::ActionForValue(
      std::wstring_view section, std::wstring_view name, const TBooleanView value)
  {
    return EAction::Process;
  }

  EAction HookshotConfigReader::ActionForValue(
      std::wstring_view section, std::wstring_view name, const TStringView value)
  {
    return EAction::Process;
  }

  void HookshotConfigReader::BeginRead(void)
  {
    CompleteConfigurationFileLayout();
  }

  EValueType HookshotConfigReader::TypeForValue(std::wstring_view section, std::wstring_view name)
  {
    auto sectionLayout = configurationFileLayout.find(section);
    if (configurationFileLayout.end() == sectionLayout) return EValueType::Error;

    auto settingInfo = sectionLayout->second.find(name);
    if (sectionLayout->second.end() == settingInfo) return EValueType::Error;

    return settingInfo->second;
  }
} // namespace Hookshot
