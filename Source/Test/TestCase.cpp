/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file TestCase.cpp
 *   Implementation of Hookshot test case interface.
 **************************************************************************************************/

#include "TestCase.h"

#include <string_view>

#include "Harness.h"

namespace HookshotTest
{
  ITestCase::ITestCase(std::wstring_view name)
  {
    Harness::RegisterTestCase(this, name);
  }
} // namespace HookshotTest
