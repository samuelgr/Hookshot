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


namespace HookshotTest
{
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "TestCase.h" for documentation.

    ITestCase::ITestCase(const char* name)
    {
        Harness::RegisterTestCase(this, name);
    }
}
