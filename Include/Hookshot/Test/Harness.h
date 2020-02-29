/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Harness.h
 *   Declaration of Hookshot test harness, including program entry point.
 *****************************************************************************/

#pragma once

#include "TestCase.h"

#include <hookshot.h>
#include <map>
#include <string>


namespace HookshotTest
{
    /// Registers and runs all Hookshot tests.  Reports results.  Implemented as a singleton object.
    /// Test cases are run in alphabetical order by name, irrespective of the order in which they are registered.
    class Harness
    {
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        std::map<std::string, ITestCase*> testCases;


        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.  Harnesses cannot be constructed externally.
        Harness(void) = default;

        /// Copy constructor. Should never be invoked.
        Harness(const Harness&) = delete;

        
        // -------- CLASS METHODS ------------------------------------------ //

        /// Returns a reference to the singleton instance of this class.
        /// Not intended to be invoked externally.
        /// @return Reference to the singleton instance.
        static Harness& GetInstance(void);

    public:
        /// Registers a test case to be run by the harness.
        /// Typically, registration happens automatically using the #HOOKSHOT_TEST_CASE macro, which is the recommended way of creating test cases.
        /// @param [in] testCase Test case object to register (appropriate instances are created automatically by the #HOOKSHOT_TEST_CASE macro).
        /// @param [in] name Name of the test case (the value of the parameter passed to the #HOOKSHOT_TEST_CASE macro).
        static inline void RegisterTestCase(ITestCase* const testCase, const char* const name)
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
        void RegisterTestCaseInternal(ITestCase* const testCase, const char* const name);

        /// Internal implementation of running all tests.
        /// @param [in] hookConfig Hookshot configuration interface object to send to test cases.
        /// @return Number of failing tests.
        int RunAllTestsInternal(Hookshot::IHookConfig* const hookConfig);
    };
}
