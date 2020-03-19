/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotConfiguration.cpp
 *   Implementation of Hookshot-specific configuration functionality.
 *****************************************************************************/

#include "Configuration.h"
#include "HookshotConfiguration.h"
#include "UnicodeTypes.h"

#include <tchar.h>
#include <unordered_map>


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ----------------------------------------- //

    /// Section name for listing settings that apply directory-wide.
    /// Hookshot only reads sections named either this or the base name of the currently-running executable.
    static constexpr TCHAR kSectionCommon[] = _T("Common");

    /// Map that enumerates all supported configuration setting names and their value types.
    static const std::unordered_map<TStdStringView, Configuration::EValueType> kNamesAndValueTypes = {
        {_T("HookModule"), Configuration::EValueType::String}            // Specifies the name of a hook module that Hookshot should load.
    };


    // -------- CONCRETE INSTANCE METHODS ---------------------------------- //
    // See "Configuration.h" for documentation.

    Configuration::ESectionAction HookshotConfiguration::ActionForSection(TStdStringView section)
    {
        if (kSectionCommon == section)
            return Configuration::ESectionAction::Read;

        return Configuration::ESectionAction::Skip;
    }

    // --------

    bool HookshotConfiguration::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TIntegerValue& value)
    {
        // No integer values are currently supported.
        return false;
    }

    // --------

    bool HookshotConfiguration::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TBooleanValue& value)
    {
        // No Boolean values are currently supported.
        return false;
    }

    // --------

    bool HookshotConfiguration::CheckValue(TStdStringView section, TStdStringView name, const Configuration::TStringValue& value)
    {
        return true;
    }

    // --------

    Configuration::EValueType HookshotConfiguration::TypeForValue(TStdStringView section, TStdStringView name)
    {
        const auto kNameAndValueType = kNamesAndValueTypes.find(name);

        if (kNamesAndValueTypes.end() == kNameAndValueType)
            return Configuration::EValueType::Error;

        return kNameAndValueType->second;
    }
}
