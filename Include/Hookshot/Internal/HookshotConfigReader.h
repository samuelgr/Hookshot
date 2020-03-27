/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotConfigReader.h
 *   Declaration of Hookshot-specific configuration reading functionality.
 *****************************************************************************/

#pragma once

#include "Configuration.h"

#include <string_view>


namespace Hookshot
{
    class HookshotConfigReader : public Configuration::ConfigurationFileReader
    {
    private:
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Configuration.h" for documentation.

        Configuration::ESectionAction ActionForSection(std::wstring_view section) override;
        bool CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TIntegerValue& value) override;
        bool CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TBooleanValue& value) override;
        bool CheckValue(std::wstring_view section, std::wstring_view name, const Configuration::TStringValue& value) override;
        void PrepareForRead(void) override;
        Configuration::EValueType TypeForValue(std::wstring_view section, std::wstring_view name) override;
    };
}
