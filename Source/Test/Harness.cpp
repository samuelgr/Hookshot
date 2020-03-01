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

#include <cstdarg>
#include <cstddef>
#include <hookshot.h>
#include <map>
#include <string>
#include <windows.h>


namespace HookshotTest
{
    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Prints the specified message and appends a newline.
    /// If a debugger is present, outputs a debug string, otherwise writes to standard output.
    /// @param [in] str Message string.
    static void Print(const TCHAR* const str)
    {
        if (IsDebuggerPresent())
        {
            OutputDebugString(str);
            OutputDebugString(_T("\n"));
        }
        else
        {
            _putts(str);
        }
    }

    /// Formats and prints the specified message and appends a newline.
    /// @param [in] format Message string, possibly with format specifiers.
    /// @param [in] args Variable argument list.
    static void PrintVarArg(const TCHAR* const format, va_list args)
    {
        TCHAR formattedStringBuffer[1024];
        _vstprintf_s(formattedStringBuffer, _countof(formattedStringBuffer), format, args);
        Print(formattedStringBuffer);
    }
    
    /// Formats and prints the specified message.
    /// @param [in] format Message string, possibly with format specifiers.
    static void PrintFormatted(const TCHAR* const format, ...)
    {
        va_list args;
        va_start(args, format);
        PrintVarArg(format, args);
        va_end(args);
    }

    
    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Harness.h" for documentation.

    Harness& Harness::GetInstance(void)
    {
        static Harness harness;
        return harness;
    }

    // --------

    void Harness::PrintFromTestCase(const TCHAR* const name, const TCHAR* const str)
    {
        Print(str);
    }

    // --------

    void Harness::PrintVarArgFromTestCase(const TCHAR* const name, const TCHAR* const format, va_list args)
    {
        PrintVarArg(format, args);
    }


    // -------- INSTANCE METHODS ------------------------------------------- //
    // See "Harness.h" for documentation.

    void Harness::RegisterTestCaseInternal(ITestCase* const testCase, const TCHAR* const name)
    {
        if ((NULL != name) && ('\0' != name[0]) && (0 == testCases.count(name)))
            testCases[name] = testCase;
    }

    // --------

    int Harness::RunAllTestsInternal(Hookshot::IHookConfig* const hookConfig)
    {
        int numFailingTests = 0;
        
        switch(testCases.size())
        {
        case 0:
            Print(_T("\nNo tests defined!\n"));
            return -1;

        case 1:
            Print(_T("\nRunning 1 test...\n"));
            break;

        default:
            PrintFormatted(_T("\nRunning %d tests...\n"), testCases.size());
            break;
        }

        Print(_T("================================================================================"));
        
        for (auto testCaseIterator = testCases.begin(); testCaseIterator != testCases.end(); ++testCaseIterator)
        {
            const auto& name = testCaseIterator->first;
            ITestCase* const testCase = testCaseIterator->second;
            
            PrintFormatted(_T("[ %-8s ] %s"), _T("RUN"), name.c_str());
            
            const bool testCasePassed = testCase->Run(hookConfig);
            if (true != testCasePassed)
                numFailingTests += 1;

            PrintFormatted(_T("[ %8s ] %s%s"), (true == testCasePassed ? _T("PASS") : _T("FAIL")), name.c_str(), (testCaseIterator == --testCases.end() ? _T("") : _T("\n")));
        }

        Print(_T("================================================================================"));
        
        switch (numFailingTests)
        {
        case 0:
            Print(_T("\nAll tests passed!\n"));
            break;

        case 1:
            Print(_T("\n1 test failed.\n"));
            break;

        default:
            PrintFormatted(_T("\n%d tests failed.\n"), numFailingTests);
            break;
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
