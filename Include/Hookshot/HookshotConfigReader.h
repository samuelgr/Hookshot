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

#include <unordered_map>


namespace Hookshot
{
    class HookshotConfigReader : public Configuration::ConfigurationFileReader
    {
    private:
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Configuration.h" for documentation.

        Configuration::ESectionAction ActionForSection(TStdStringView section) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TIntegerValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TBooleanValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TStringValue& value) override;
        void PrepareForRead(void) override;
        Configuration::EValueType TypeForValue(TStdStringView section, TStdStringView name) override;
    };
}
