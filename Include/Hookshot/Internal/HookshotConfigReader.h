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

#include <Infra/Core/Configuration.h>

namespace Hookshot
{
  using namespace ::Infra::Configuration;

  class HookshotConfigReader : public ConfigurationFileReader
  {
  protected:

    // ConfigurationFileReader
    Action ActionForSection(std::wstring_view section) override;
    Action ActionForValue(
        std::wstring_view section, std::wstring_view name, const TIntegerView value) override;
    Action ActionForValue(
        std::wstring_view section, std::wstring_view name, const TBooleanView value) override;
    Action ActionForValue(
        std::wstring_view section, std::wstring_view name, const TStringView value) override;
    void BeginRead(void) override;
    EValueType TypeForValue(std::wstring_view section, std::wstring_view name) override;
  };
} // namespace Hookshot
