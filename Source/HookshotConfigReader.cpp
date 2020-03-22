/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotConfigReader.cpp
 *   Implementation of Hookshot-specific configuration reading functionality.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Configuration.h"
#include "HookshotConfigReader.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <unordered_map>
#include <string>
#include <string_view>


namespace Hookshot
{
    // -------- INTERNAL VARIABLES ----------------------------------------- //
    
    /// Holds the layout of the Hookshot configuration file that is known statically.
    /// At compile time, this encompasses all settings except for the dynamically-determined section whose name matches the file name of the currently-running executable, which is filled in at runtime.
    static Configuration::TConfigurationFileLayout configurationFileLayout = {
        ConfigurationFileLayoutSection(Configuration::ConfigurationData::kSectionNameGlobal, {
            ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigurationSettingNameHookModule, Configuration::EValueType::StringMultiValue),
            ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigurationSettingNameInject, Configuration::EValueType::StringMultiValue),
            ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigurationSettingNameLogLevel, Configuration::EValueType::Integer),
        }),
    };

    /// Holds the section name for the per-executable settings.
    /// This is dynamically set to the name of the currently-running executable.
    std::wstring executableSpecificSectionName;
    
    /// If `true`, indicates that the hookshot configuration file layout definition has been augmented with runtime information.
    static bool configurationFileLayoutIsComplete = false;


    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Completes the Hookshot configuration file layout, if not already done, by adding to it a section whose name is determined at runtime.
    /// Supports the same settings as those common settings applied to all executables in a particular directory, but in this case is specific to one executable.
    /// The section name in question is the same as the base name of the currently-running executable.
    static inline void CompleteConfigurationFileLayout(void)
    {
        if (false == configurationFileLayoutIsComplete)
        {
            configurationFileLayout.insert(ConfigurationFileLayoutSection(Strings::kStrExecutableBaseName, {
                ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigurationSettingNameHookModule, Configuration::EValueType::StringMultiValue),
                ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigurationSettingNameInject, Configuration::EValueType::StringMultiValue),
            }));

            configurationFileLayoutIsComplete = true;
        }
    }


    // -------- CONCRETE INSTANCE METHODS ---------------------------------- //
    // See "Configuration.h" for documentation.

    Configuration::ESectionAction HookshotConfigReader::ActionForSection(std::wstring_view section)
    {
        if (0 != configurationFileLayout.count(section))
            return Configuration::ESectionAction::Read;

        return Configuration::ESectionAction::Skip;
    }

    // --------

    bool HookshotConfigReader::CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TIntegerValue& value)
    {
        return (value >= 0);
    }

    // --------

    bool HookshotConfigReader::CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TBooleanValue& value)
    {
        return false;
    }

    // --------

    bool HookshotConfigReader::CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TStringValue& value)
    {
        return true;
    }

    // --------

    void HookshotConfigReader::PrepareForRead(void)
    {
        CompleteConfigurationFileLayout();
    }

    // --------
    
    Configuration::EValueType HookshotConfigReader::TypeForValue(std::wstring_view section, std::wstring_view name)
    {
        auto sectionLayout = configurationFileLayout.find(section);
        if (configurationFileLayout.end() == sectionLayout)
            return Configuration::EValueType::Error;

        auto settingInfo = sectionLayout->second.find(name);
        if (sectionLayout->second.end() == settingInfo)
            return Configuration::EValueType::Error;

        return settingInfo->second;
    }
}
