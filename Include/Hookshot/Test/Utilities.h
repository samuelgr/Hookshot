/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 *************************************************************************//**
 * @file Utilities.h
 *   Declaration of test utility functions.
 **************************************************************************************************/

#pragma once

#include <sal.h>

namespace HookshotTest
{
  /// Prints the specified message and appends a newline.
  /// @param [in] str Message string.
  void Print(const wchar_t* const str);

  /// Formats and prints the specified message.
  /// @param [in] format Message string, possibly with format specifiers.
  void PrintFormatted(_Printf_format_string_ const wchar_t* const format, ...);
} // namespace HookshotTest
