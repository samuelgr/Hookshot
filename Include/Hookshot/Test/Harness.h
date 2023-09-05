/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file Harness.h
 *   Declaration of Hookshot test harness, including program entry point.
 **************************************************************************************************/

#pragma once

#include "TestCase.h"

#include <map>
#include <string>
#include <string_view>

#include "Hookshot.h"

namespace HookshotTest
{
  /// Registers and runs all Hookshot tests. Reports results. Implemented as a singleton object.
  /// Test cases are run in alphabetical order by name, irrespective of the order in which they are
  /// registered.
  class Harness
  {
  public:

    /// Registers a test case to be run by the harness.
    /// Typically, registration happens automatically using the #HOOKSHOT_TEST_CASE macro, which is
    /// the recommended way of creating test cases.
    /// @param [in] testCase Test case object to register (appropriate instances are created
    /// automatically by the #HOOKSHOT_TEST_CASE macro).
    /// @param [in] name Name of the test case (the value of the parameter passed to the
    /// #HOOKSHOT_TEST_CASE macro).
    static inline void RegisterTestCase(const ITestCase* const testCase, std::wstring_view name)
    {
      GetInstance().RegisterTestCaseInternal(testCase, name);
    }

    /// Runs all tests registered by the harness whose names begin with the specified prefix, where
    /// empty prefix matches all tests. Typically invoked only once by the entry point to the test
    /// program.
    /// @param [in] hookshot Hookshot interface object, used to create hooks.
    /// @param [in] prefixToMatch Prefix against which to compare test case names.
    /// @return Number of failing tests.
    static inline int RunTestsWithMatchingPrefix(
        Hookshot::IHookshot* hookshot, std::wstring_view prefixToMatch = L"")
    {
      return GetInstance().RunTestsWithMatchingPrefixInternal(hookshot, prefixToMatch);
    }

  private:

    Harness(void) = default;

    Harness(const Harness&) = delete;

    /// Returns a reference to the singleton instance of this class.
    /// Not intended to be invoked externally.
    /// @return Reference to the singleton instance.
    static Harness& GetInstance(void);

    /// Internal implementation of test case registration.
    /// @param [in] testCase Test case object to register.
    /// @param [in] name Name of the test case.
    void RegisterTestCaseInternal(const ITestCase* const testCase, std::wstring_view name);

    /// Internal implementation of running all tests whose names begin with the specified prefix.
    /// @param [in] hookshot Hookshot interface object, used to create hooks.
    /// @param [in] prefixToMatch Prefix against which to compare test case names.
    /// @return Number of failing tests.
    int RunTestsWithMatchingPrefixInternal(
        Hookshot::IHookshot* hookshot, std::wstring_view prefixToMatch);

    /// Holds all registered test cases in alphabetical order.
    std::map<std::wstring_view, const ITestCase*> testCases;
  };
} // namespace HookshotTest
