/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file HookshotConfigReader.h
 *   Declaration of Hookshot-specific configuration reading functionality.
 **************************************************************************************************/

#pragma once

#include <string_view>

#include "Configuration.h"

namespace Hookshot
{
  class HookshotConfigReader : public Configuration::ConfigurationFileReader
  {
  protected:

    // ConfigurationFileReader
    Configuration::EAction ActionForSection(std::wstring_view section) override;
    Configuration::EAction ActionForValue(
        std::wstring_view section,
        std::wstring_view name,
        const Configuration::TIntegerView value) override;
    Configuration::EAction ActionForValue(
        std::wstring_view section,
        std::wstring_view name,
        const Configuration::TBooleanView value) override;
    Configuration::EAction ActionForValue(
        std::wstring_view section,
        std::wstring_view name,
        const Configuration::TStringView value) override;
    void BeginRead(void) override;
    Configuration::EValueType TypeForValue(
        std::wstring_view section, std::wstring_view name) override;
  };
} // namespace Hookshot
