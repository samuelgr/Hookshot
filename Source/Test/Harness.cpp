/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Harness.cpp
 *   Implementation of Hookshot test harness, including program entry point.
 **************************************************************************************************/

#include "TestCase.h"

#include "Harness.h"

#include <windows.h>

#include <map>
#include <set>

#include "Hookshot.h"
#include "Utilities.h"

namespace HookshotTest
{
  Harness& Harness::GetInstance(void)
  {
    static Harness harness;
    return harness;
  }

  void Harness::RegisterTestCaseInternal(const ITestCase* const testCase, std::wstring_view name)
  {
    if ((false == name.empty()) && (false == testCases.contains(name))) testCases[name] = testCase;
  }

  int Harness::RunTestsWithMatchingPrefixInternal(
      Hookshot::IHookshot* hookshot, std::wstring_view prefixToMatch)
  {
    std::set<const wchar_t*> failingTests;
    unsigned int numExecutedTests = 0;
    unsigned int numSkippedTests = 0;

    switch (testCases.size())
    {
      case 0:
        Print(L"\nNo tests defined!\n");
        return -1;

      default:
        PrintFormatted(
            L"\n%u test%s defined.",
            static_cast<unsigned int>(testCases.size()),
            ((1 == testCases.size()) ? L"" : L"s"));
        break;
    }

    if (true == prefixToMatch.empty())
      Print(L"Running all tests.");
    else
      PrintFormatted(L"Running only tests with \"%s\" as a prefix.", prefixToMatch.data());

    Print(L"\n================================================================================");

    for (auto testCaseIterator = testCases.begin(); testCaseIterator != testCases.end();
         ++testCaseIterator)
    {
      const bool lastTestCase = (testCaseIterator == --testCases.end());
      const auto& name = testCaseIterator->first;
      const ITestCase* const testCase = testCaseIterator->second;

      if (false == name.starts_with(prefixToMatch)) continue;

      if (testCase->CanRun())
      {
        PrintFormatted(L"[ %-9s ] %s", L"RUN", name.data());

        bool testCasePassed = false;
        try
        {
          numExecutedTests += 1;

          testCase->Run(hookshot);
          testCasePassed = true;
        }
        catch (TestFailedException)
        {}

        if (true != testCasePassed) failingTests.insert(name.data());

        PrintFormatted(
            L"[ %9s ] %s%s",
            (true == testCasePassed ? L"PASS" : L"FAIL"),
            name.data(),
            (lastTestCase ? L"" : L"\n"));
      }
      else
      {
        PrintFormatted(L"[  %-8s ] %s%s", L"SKIPPED", name.data(), (lastTestCase ? L"" : L"\n"));
        numSkippedTests += 1;
      }
    }

    Print(L"================================================================================");

    if (numSkippedTests > 0)
      PrintFormatted(
          L"\nFinished running %u test%s (%u skipped).\n",
          numExecutedTests,
          ((1 == numExecutedTests) ? L"" : L"s"),
          numSkippedTests);
    else
      PrintFormatted(
          L"\nFinished running %u test%s.\n",
          numExecutedTests,
          ((1 == numExecutedTests) ? L"" : L"s"));

    const int numFailingTests = static_cast<int>(failingTests.size());

    if (numExecutedTests > 0)
    {
      if (testCases.size() == numSkippedTests)
      {
        Print(L"All tests skipped.\n");
      }
      else
      {
        switch (numFailingTests)
        {
          case 0:
            Print(L"All tests passed!\n");
            break;

          default:
            PrintFormatted(
                L"%u test%s failed:", numFailingTests, ((1 == numFailingTests) ? L"" : L"s"));
            break;
        }
      }

      if (numFailingTests > 0)
      {
        for (std::wstring_view failingTest : failingTests)
          PrintFormatted(L"    %s", failingTest.data());

        Print(L"\n");
      }
    }
    else
    {
      Print(L"No results available.\n");
    }

    return numFailingTests;
  }
} // namespace HookshotTest

/// Runs all tests cases.
/// @return Number of failing tests (0 means all tests passed).
int wmain(int argc, const wchar_t* argv[])
{
  return HookshotTest::Harness::RunTestsWithMatchingPrefix(
      HookshotLibraryInitialize(), ((argc > 1) ? argv[1] : L""));
}
