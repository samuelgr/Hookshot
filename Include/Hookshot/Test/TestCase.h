/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
 **************************************************************************//**
 * @file TestCase.h
 *   Declaration of Hookshot test case interface.
 *****************************************************************************/

#pragma once

#include "Hookshot/Hookshot.h"


namespace HookshotTest
{
    /// Test case interface.
    class ITestCase
    {
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Initialization constructor. Constructs a test case object with an associated test case name, and registers it with the harness.
        /// @param [in] name Test case name.
        ITestCase(const wchar_t* const name);


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Prints the specified message and appends a newline.
        /// Available to be called directly from the body of a test case.
        /// @param [in] str Message string.
        void Print(const wchar_t* const str) const;

        /// Formats and prints the specified message and appends a newline.
        /// Available to be called directly from the body of a test case.
        /// @param [in] format Message string, possibly with format specifiers.
        void PrintFormatted(const wchar_t* const format, ...) const;


        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //

        /// Performs run-time checks to determine if the test case represented by this object can be run.
        /// If not, it will be skipped.
        virtual bool CanRun(void) const = 0;


        /// Runs the test case represented by this object.
        /// Implementations are generated when test cases are created using the #HOOKSHOT_TEST_CASE macro.
        /// @param [in] hookshot Hookshot interface object, used to create hooks.
        /// @return Indication of the test result, via the appropriate test result macros.
        virtual bool Run(Hookshot::IHookshot* hookshot) const = 0;
    };

    /// Concrete test case object template.
    /// Each test case created by #HOOKSHOT_TEST_CASE instantiates an object of this type with a different template parameter.
    /// @tparam kName Name of the test case.
    template <const wchar_t* kName> class TestCase : public ITestCase
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

        bool CanRun(void) const override;
        bool Run(Hookshot::IHookshot* hookshot) const override;
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
#define HOOKSHOT_TEST_ASSERT(expr)          do {if (!(expr)) {PrintFormatted(L"%s(%d): Assertion failed: %s", __FILEW__, __LINE__, L#expr); return false;}} while (0)

/// Recommended way of creating Hookshot test cases that execute conditionally.
/// Requires a test case name and a condition, which evaluates to a value of type bool.
/// If the condition ends up being false, which can be determined at runtime, the test case is skipped.
/// Automatically instantiates the proper test case object and registers it with the harness.
/// Treat this macro as a function declaration; the test case is the function body.
/// Inside the function body, the variable `hookshot` is available as a pointer to a Hookshot interface object.
#define HOOKSHOT_TEST_CASE_CONDITIONAL(name, cond) \
    static constexpr wchar_t kHookshotTestName__##name[] = L#name; \
    HookshotTest::TestCase<kHookshotTestName__##name>  hookshotTestCaseInstance__##name; \
    bool HookshotTest::TestCase<kHookshotTestName__##name>::CanRun(void) const { return (cond); } \
    bool HookshotTest::TestCase<kHookshotTestName__##name>::Run(Hookshot::IHookshot* hookshot) const

/// Recommended way of creating Hookshot test cases that execute unconditionally.
/// Just provide the test case name.
#define HOOKSHOT_TEST_CASE(name)            HOOKSHOT_TEST_CASE_CONDITIONAL(name, true)
