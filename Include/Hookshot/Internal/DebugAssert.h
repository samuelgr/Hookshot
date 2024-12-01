/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file DebugAssert.h
 *   Standard mechanism for accessing debug assertion functionality.
 **************************************************************************************************/

#pragma once

#include <crtdbg.h>

#include <string>

namespace _DebugTestInternal
{
  /// Type that should be thrown for debug assertions that are expected in tests. Simply contains a
  /// message containing the string that would have been triggered as a full-blown debug assertion.
  /// Intended for use in tests.
  class Assertion
  {
  public:

    inline Assertion(std::wstring&& failureMessage) : failureMessage(std::move(failureMessage)) {}

    std::wstring_view GetFailureMessage(void) const
    {
      return failureMessage;
    }

  private:

    std::wstring failureMessage;
  };

  /// Context manager for expecting debug assertions. When an object of this type is in scope
  /// anywhere in the stack, debug assertions are considered "expected" and thrown as exceptions
  /// instead of causing a program crash. Intended for use in tests.
  class ExpectedAssertionContext
  {
  public:

#ifdef _DEBUG
    inline ExpectedAssertionContext(void)
    {
      expectedAssertionCount += 1;
    }

    inline ~ExpectedAssertionContext(void)
    {
      expectedAssertionCount -= 1;
    }
#else
    constexpr ExpectedAssertionContext(void) = default;
#endif

    ExpectedAssertionContext(const ExpectedAssertionContext& other) = delete;

    ExpectedAssertionContext(ExpectedAssertionContext&& other) = delete;

#ifdef _DEBUG
    static inline bool ShouldThrowAssertionFailureAsException(void)
    {
      return (expectedAssertionCount > 0);
    }
#else
    static consteval bool ShouldThrowAssertionFailureAsException(void)
    {
      return false;
    }
#endif

  private:

#ifdef _DEBUG
    /// Debug assertion expectation counter. Incremented whenever an object is created, decremented
    /// whenever an object is destroyed. If non-zero, debug assertions are expected and should be
    /// thrown as exceptions rather than used to trigger a full-blown application crash due to
    /// assertion failure.
    static inline int expectedAssertionCount = 0;
#endif
  };
} // namespace _DebugTestInternal

/// Type alias intended for use when attempting to catch debug assertions thrown as exceptions.
using DebugAssertionException = _DebugTestInternal::Assertion;

/// Wrapper around debug assertion functionality. Provides an interface like `static_assert` which
/// takes an expression and a compile-time constant narrow-character string literal.
#define DebugAssert(expr, msg)                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (::_DebugTestInternal::ExpectedAssertionContext::ShouldThrowAssertionFailureAsException())  \
    {                                                                                              \
      if (!(expr))                                                                                 \
        throw ::DebugAssertionException(                                                           \
            __FILEW__ L"(" _CRT_WIDE(_CRT_STRINGIZE(__LINE__)) L"): " _CRT_WIDE(msg));             \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _ASSERT_EXPR((expr), L"\n" _CRT_WIDE(msg) L"\n\nFunction:\n" __FUNCTIONW__);                 \
    }                                                                                              \
  }                                                                                                \
  while (0)

/// Begins a context in which a debug assertion may be expected. As long as this is in scope, debug
/// assertions will be thrown as `DebugAssertionException` objects instead of sent to the debugger
/// to trigger a break. Intended only for use in tests, particularly those that intentionally
/// trigger conditions that normally could lead to assertions being triggered. Note that it is not
/// generally possible to require that an assertion be triggered, because assertions typically only
/// trigger in debug builds. This macro should be considered as allowing a possible assertion
/// failure but not causing a test to fail in the absence of an assertion failure.
#define ScopedExpectDebugAssertion()                                                               \
  [[maybe_unused]] ::_DebugTestInternal::ExpectedAssertionContext __expectedDebugAssertionContext
