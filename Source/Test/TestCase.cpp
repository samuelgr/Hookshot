/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
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

    ITestCase::ITestCase(const TCHAR* const name) : name(name)
    {
        Harness::RegisterTestCase(this, name);
    }


    // -------- INSTANCE METHODS ------------------------------------------- //
    // See "TestCase.h" for documentation.

    void ITestCase::Print(const TCHAR* const str) const
    {
        Harness::PrintFromTestCase(name, str);
    }

    // --------

    void ITestCase::PrintFormatted(const TCHAR* const format, ...) const
    {
        va_list args;
        va_start(args, format);
        Harness::PrintVarArgFromTestCase(name, format, args);
        va_end(args);
    }
}
