/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotConfiguration.h
 *   Declaration of Hookshot-specific configuration functionality.
 *****************************************************************************/

#pragma once

#include "Configuration.h"


namespace Hookshot
{
    class HookshotConfigurationReader : public Configuration::ConfigurationFileReader
    {
    protected:
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Configuration.h" for documentation.

        Configuration::ESectionAction ActionForSection(TStdStringView section) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TIntegerValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TBooleanValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const Configuration::TStringValue& value) override;
        Configuration::EValueType TypeForValue(TStdStringView section, TStdStringView name) override;
    };
}
