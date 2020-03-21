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
#include "UnicodeTypes.h"

#include <tchar.h>
#include <unordered_map>


namespace Hookshot
{
    // -------- INTERNAL VARIABLES ----------------------------------------- //
    
    /// Holds the layout of executable-scoped sections of a Hookshot configuration file.
    /// Hookshot supports a directory-wide section that applies to all contained executables and an executable-specific section.
    /// In both cases the supported settings are the same, and the layout of each such section is captured here.
    static const Configuration::TConfigurationFileSectionLayout kExecutableScopeSectionLayout = {
        ConfigurationFileLayoutNameAndValueType(Strings::kStrConfigFileNameHookModule, Configuration::EValueType::String),
    };
    
    /// Holds the layout of the Hookshot configuration file that is known statically.
    /// At compile time, this encompasses all settings except for the dynamically-determined section whose name matches the file name of the currently-running executable, which is filled in at runtime.
    static Configuration::TConfigurationFileLayout configurationFileLayout = {
        ConfigurationFileLayoutSection(Configuration::ConfigurationData::kSectionNameGlobal, kExecutableScopeSectionLayout),
    };

    /// Holds the section name for the per-executable settings.
    /// This is dynamically set to the name of the currently-running executable.
    TStdString executableSpecificSectionName;
    
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
            TemporaryBuffer<TCHAR> executableBaseName;
            if (false == Strings::FillExecutableBaseName(executableBaseName, executableBaseName.Count()))
                return;

            executableSpecificSectionName = executableBaseName;
            
            configurationFileLayout.insert(ConfigurationFileLayoutSection(executableSpecificSectionName, kExecutableScopeSectionLayout));
            configurationFileLayoutIsComplete = true;
        }
    }


    // -------- CONCRETE INSTANCE METHODS ---------------------------------- //
    // See "Configuration.h" for documentation.

    Configuration::ESectionAction HookshotConfigReader::ActionForSection(TStdStringView section)
    {
        if (0 != configurationFileLayout.count(section))
            return Configuration::ESectionAction::Read;

        return Configuration::ESectionAction::Skip;
    }

    // --------

    bool HookshotConfigReader::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TIntegerValue& value)
    {
        // No integer values are currently supported.
        return false;
    }

    // --------

    bool HookshotConfigReader::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TBooleanValue& value)
    {
        // No Boolean values are currently supported.
        return false;
    }

    // --------

    bool HookshotConfigReader::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TStringValue& value)
    {
        return true;
    }

    // --------

    void HookshotConfigReader::PrepareForRead(void)
    {
        CompleteConfigurationFileLayout();
    }

    // --------
    
    Configuration::EValueType HookshotConfigReader::TypeForValue(TStdStringView section, TStdStringView name)
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
