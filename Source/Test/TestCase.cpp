/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file TestCase.cpp
 *   Implementation of Hookshot test case interface.
 *****************************************************************************/

#include "Harness.h"
#include "TestCase.h"

#include <cstdarg>


namespace HookshotTest
{
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "TestCase.h" for documentation.

    ITestCase::ITestCase(const TCHAR* const name)
    {
        Harness::RegisterTestCase(this, name);
    }


    // -------- INSTANCE METHODS ------------------------------------------- //
    // See "TestCase.h" for documentation.

    void ITestCase::Print(const TCHAR* const str) const
    {
        Harness::PrintFromTestCase(this, str);
    }

    // --------

    void ITestCase::PrintFormatted(const TCHAR* const format, ...) const
    {
        va_list args;
        va_start(args, format);
        Harness::PrintVarArgFromTestCase(this, format, args);
        va_end(args);
    }
}
