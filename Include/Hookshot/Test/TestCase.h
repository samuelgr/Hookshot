/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TestCase.h
 *   Declaration of Hookshot test case interface.
 *****************************************************************************/

#pragma once

#include <hookshot.h>
#include <tchar.h>


namespace HookshotTest
{
    /// Test case interface.
    class ITestCase
    {
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Test case name.
        const TCHAR* const name;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Initialization constructor. Constructs a test case object with an associated test case name, and registers it with the harness.
        /// @param [in] name Test case name.
        ITestCase(const TCHAR* const name);


        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //

        /// Runs the test case represented by this object.
        /// @param [in] hookConfig Hookshot configuration interface object to use.
        /// @return Indication of the test result, via the appropriate test result macros.
        virtual bool RunTest(Hookshot::IHookConfig* const hookConfig) const = 0;
    };

    /// Concrete test case object template.
    /// Each test case created by #HOOKSHOT_TEST_CASE instantiates an object of this type with a different template parameter.
    /// @tparam kName Name of the test case.
    template <const TCHAR* kName> class TestCase : public ITestCase
    {
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        inline TestCase(void) : ITestCase(kName)
        {
            // Nothing to do here.
        }


        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See above for documentation.

        bool RunTest(Hookshot::IHookConfig* const hookConfig) const override;
    };
}

/// If creating test functions that serve either as either hook or target functions, mark them with this macro.
/// Also make sure that each call to such functions use different parameter values.
/// This is to ensure the optimizer does not optimize away the function calls, either by inlining or result caching, both of which are behaviors that will break the tests.
#define HOOKSHOT_TEST_HELPER_FUNCTION       __declspec(noinline)

/// Exit from a test case and indicate a passing result.
#define HOOKSHOT_TEST_PASSED                return true

/// Exit from a test case and indicate a failing result.
#define HOOKSHOT_TEST_FAILED                return false

/// Recommended way of creating Hookshot test cases.  Just provide the test case name.
/// Automatically instantiates the proper test case object and registers it with the harness.
/// Treat this macro as a function declaration; the test case is the function body.
/// A variable called "hookConfig" is available for configuring Hookshot for test purposes.
#define HOOKSHOT_TEST_CASE(name)                                                                        \
    constexpr TCHAR _kName##name[] = _T(#name);                                                         \
    HookshotTest::TestCase<_kName##name> _testCaseInstance_##name;                                      \
    bool HookshotTest::TestCase<_kName##name>::RunTest(Hookshot::IHookConfig* const hookConfig) const
