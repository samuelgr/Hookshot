/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Harness.h
 *   Declaration of Hookshot test harness, including program entry point.
 *****************************************************************************/

#pragma once

#include "TestCase.h"

#include <cstdarg>
#include <hookshot.h>
#include <map>
#include <string>
#include <tchar.h>


namespace HookshotTest
{
    /// Registers and runs all Hookshot tests. Reports results. Implemented as a singleton object.
    /// Test cases are run in alphabetical order by name, irrespective of the order in which they are registered.
    class Harness
    {
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

#ifdef UNICODE
        /// Holds all registered test cases in alphabetical order.
        /// This is the Unicode version.
        std::map<std::wstring, const ITestCase*> testCases;
#else
        /// Holds all registered test cases in alphabetical order.
        /// This is the non-Unicode version.
        std::map<std::string, const ITestCase*> testCases;
#endif


        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.  Objects cannot be constructed externally.
        Harness(void) = default;

        /// Copy constructor. Should never be invoked.
        Harness(const Harness&) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Returns a reference to the singleton instance of this class.
        /// Not intended to be invoked externally.
        /// @return Reference to the singleton instance.
        static Harness& GetInstance(void);

    public:
        /// Prints the specified message and appends a newline.
        /// For use inside a test case, but print requests should be through the appropriate #ITestCase methods.
        /// @param [in] testCase Test case object.
        /// @param [in] str Message string.
        static void PrintFromTestCase(const ITestCase* const testCase, const TCHAR* const str);

        /// Formats and prints the specified message and appends a newline.
        /// For use inside a test case, but print requests should be through the appropriate #ITestCase methods.
        /// @param [in] testCase Test case object.
        /// @param [in] format Message string, possibly with format specifiers.
        /// @param [in] args Variable argument list.
        static void PrintVarArgFromTestCase(const ITestCase* const testCase, const TCHAR* const format, va_list args);

        /// Registers a test case to be run by the harness.
        /// Typically, registration happens automatically using the #HOOKSHOT_TEST_CASE macro, which is the recommended way of creating test cases.
        /// @param [in] testCase Test case object to register (appropriate instances are created automatically by the #HOOKSHOT_TEST_CASE macro).
        /// @param [in] name Name of the test case (the value of the parameter passed to the #HOOKSHOT_TEST_CASE macro).
        static inline void RegisterTestCase(const ITestCase* const testCase, const TCHAR* const name)
        {
            GetInstance().RegisterTestCaseInternal(testCase, name);
        }

        /// Runs all tests registered by the harness.
        /// Typically invoked only once by the entry point to the test program.
        /// @param [in] hookConfig Hookshot configuration interface object to send to test cases (obtained during Hookshot initialization by the entry point).
        static inline int RunAllTests(Hookshot::IHookConfig* const hookConfig)
        {
            return GetInstance().RunAllTestsInternal(hookConfig);
        }


    private:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Internal implementation of test case registration.
        /// @param [in] testCase Test case object to register.
        /// @param [in] name Name of the test case.
        void RegisterTestCaseInternal(const ITestCase* const testCase, const TCHAR* const name);

        /// Internal implementation of running all tests.
        /// @param [in] hookConfig Hookshot configuration interface object to send to test cases.
        /// @return Number of failing tests.
        int RunAllTestsInternal(Hookshot::IHookConfig* const hookConfig);
    };
}
