/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Prints the specified message and appends a newline.
        /// Available to be called directly from the body of a test case.
        /// @param [in] str Message string.
        void Print(const TCHAR* const str) const;
        
        /// Formats and prints the specified message and appends a newline.
        /// Available to be called directly from the body of a test case.
        /// @param [in] format Message string, possibly with format specifiers.
        void PrintFormatted(const TCHAR* const format, ...) const;

        
        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //

        /// Runs the test case represented by this object.
        /// Implementations are generated when test cases are created using the #HOOKSHOT_TEST_CASE macro.
        /// @param [in] hookConfig Hookshot configuration interface object to use.
        /// @return Indication of the test result, via the appropriate test result macros.
        virtual bool Run(Hookshot::IHookConfig* const hookConfig) const = 0;
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

        bool Run(Hookshot::IHookConfig* const hookConfig) const override;
    };
}

/// If creating test functions that serve either as either hook or target functions, mark them with this macro.
/// Also, to avoid the optimizer optimizing away multiple calls, make sure that each call to such functions use different parameter values.
#define HOOKSHOT_TEST_HELPER_FUNCTION       __declspec(noinline) static

/// Exit from a test case and indicate a passing result.
#define HOOKSHOT_TEST_PASSED                return true

/// Exit from a test case and indicate a failing result.
#define HOOKSHOT_TEST_FAILED                return false

/// Exit from a test case and indicate a failing result if the expression is false.
#define HOOKSHOT_TEST_ASSERT(expr)          do {if (!(expr)) {PrintFormatted(_T("%s:%d: Assertion failed: %s"), _T(__FILE__), __LINE__, _T(#expr)); return false;}} while(0)

/// Recommended way of creating Hookshot test cases.  Just provide the test case name.
/// Automatically instantiates the proper test case object and registers it with the harness.
/// Treat this macro as a function declaration; the test case is the function body.
/// A variable called "hookConfig" is available for configuring Hookshot for test purposes.
#define HOOKSHOT_TEST_CASE(name)                                                                                \
    static constexpr TCHAR kHookshotTestName__##name[] = _T(#name);                                             \
    HookshotTest::TestCase<kHookshotTestName__##name>  hookshotTestCaseInstance__##name;                        \
    bool HookshotTest::TestCase<kHookshotTestName__##name>::Run(Hookshot::IHookConfig* const hookConfig) const
