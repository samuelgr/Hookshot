/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Harness.cpp
 *   Implementation of Hookshot test harness, including program entry point.
 *****************************************************************************/

#include "Harness.h"
#include "TestCase.h"

#include <cstddef>
#include <hookshot.h>
#include <map>
#include <string>


namespace HookshotTest
{
    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Harness.h" for documentation.

    Harness& Harness::GetInstance(void)
    {
        static Harness harness;
        return harness;
    }


    // -------- INSTANCE METHODS ------------------------------------------- //
    // See "Harness.h" for documentation.

    void Harness::RegisterTestCaseInternal(ITestCase* const testCase, const char* const name)
    {
        if ((NULL != name) && ('\0' != name[0]) && (0 == testCases.count(name)))
            testCases[name] = testCase;
    }

    // --------

    int Harness::RunAllTestsInternal(Hookshot::IHookConfig* const hookConfig)
    {
        int numFailingTests = 0;
        
        for (auto testCaseIterator = testCases.begin(); testCaseIterator != testCases.end(); ++testCaseIterator)
        {
            const std::string& name = testCaseIterator->first;
            ITestCase* const testCase = testCaseIterator->second;

            if (false == testCase->RunTest(hookConfig))
                numFailingTests += 1;
        }

        return numFailingTests;
    }
}


// -------- ENTRY POINT ---------------------------------------------------- //

/// Runs all tests cases.
/// @return Number of failing tests (0 means all tests passed).
int main(int argc, const char* argv[])
{
    Hookshot::IHookConfig* hookConfig = HookshotLibraryInitialize();
    return HookshotTest::Harness::RunAllTests(hookConfig);
}
