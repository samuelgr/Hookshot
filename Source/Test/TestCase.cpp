/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
 **************************************************************************//**
 * @file TestCase.cpp
 *   Implementation of Hookshot test case interface.
 *****************************************************************************/

#include "Harness.h"
#include "TestCase.h"

#include <string_view>


namespace HookshotTest
{
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "TestCase.h" for documentation.

    ITestCase::ITestCase(std::wstring_view name)
    {
        Harness::RegisterTestCase(this, name);
    }
}
