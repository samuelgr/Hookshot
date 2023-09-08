/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file TestPattern.h
 *   Common pattern definitions for Hookshot test cases.
 **************************************************************************************************/

#pragma once

#include "TestCase.h"

#include <cstddef>

#include "Hookshot.h"

/// Expected result of a call to an original version of a function.
/// Test cases should compare the result received from what should be an original function
/// invocation to this value.
static constexpr size_t kOriginalFunctionResult = 1111111;

/// Expected result of a call to a hooked version of a function.
/// Test cases should compare the result received from what should be hook function invocation to
/// this value.
static constexpr size_t kHookFunctionResult = (kOriginalFunctionResult << 1);

/// Signature of a Hookshot test function.
/// Most, if not all, test functions are written in assembly.
using THookshotTestFunc = size_t(__fastcall*)(size_t scx, size_t sdx);

/// Encapsulates the logic that implements a Hookshot test in which a hook is set successfully, thus
/// effectively replacing the original function with the hooked version. This test pattern verifies
/// that Hookshot correctly returns a hook identifier that identifies the hook, sets the hook, and
/// enables access to the original functionality even after setting the hook. The test case name is
/// the macro parameter. It relies on two functions being defined externally, name_Original and
/// name_Hook. If written in C/C++, these functions should use C linkage and be declared using the
/// function signature for #THookshotTestFunc. If written in assembly, refer to the assembly
/// language definitions for more information on how to construct a test function easily. Whatever
/// is the control flow, name_Original should return #kOriginalFunctionResult when executed
/// correctly, and name_Hook should likewise return #kHookFunctionResult.
#define HOOKSHOT_HOOK_SET_SUCCESS_TEST_CONDITIONAL(name, cond)                                     \
  extern "C" size_t __fastcall name##_Original(size_t scx, size_t sdx);                            \
  extern "C" size_t __fastcall name##_Hook(size_t scx, size_t sdx);                                \
  HOOKSHOT_TEST_CASE_CONDITIONAL(HookSetSuccess_##name, cond)                                      \
  {                                                                                                \
    HOOKSHOT_TEST_ASSERT(                                                                          \
        Hookshot::SuccessfulResult(hookshot->CreateHook(&name##_Original, &name##_Hook)));         \
    HOOKSHOT_TEST_ASSERT(kHookFunctionResult == name##_Original(kOriginalFunctionResult, 0));      \
    THookshotTestFunc originalFuncPtr =                                                            \
        (THookshotTestFunc)hookshot->GetOriginalFunction(&name##_Hook);                            \
    HOOKSHOT_TEST_ASSERT(nullptr != originalFuncPtr);                                              \
    HOOKSHOT_TEST_ASSERT(                                                                          \
        (THookshotTestFunc)hookshot->GetOriginalFunction(&name##_Original) == originalFuncPtr);    \
    HOOKSHOT_TEST_ASSERT(kOriginalFunctionResult == originalFuncPtr(kOriginalFunctionResult, 0));  \
  }

/// Convenience wrapper that unconditionally runs a test in which the expected result is the hook
/// successfully being set.
#define HOOKSHOT_HOOK_SET_SUCCESS_TEST(name) HOOKSHOT_HOOK_SET_SUCCESS_TEST_CONDITIONAL(name, true)

/// Encapsulates the logic that implements a Hookshot test in which a hook is not successfully set.
/// This test pattern attempts to set a hook and verifies that Hookshot returns the supplied error
/// code. The test case name is a macro parameter. It relies on one function being defined
/// externally, name_Test. If written in C/C++, this function should use C linkage and be declared
/// using the function signature for #THookshotTestFunc. If written in assembly, refer to the
/// assembly language definitions for more information on how to construct a test function easily.
/// Since no hooking actually takes place, and therefore the external function is never executed,
/// its actual behavior is not important.
#define HOOKSHOT_HOOK_SET_FAIL_TEST_CONDITIONAL(name, result, cond)                                \
  extern "C" size_t __fastcall name##_Test(size_t scx, size_t sdx);                                \
  HOOKSHOT_TEST_HELPER_FUNCTION size_t name##_Test_Hook(void)                                      \
  {                                                                                                \
    return __LINE__;                                                                               \
  }                                                                                                \
  HOOKSHOT_TEST_CASE_CONDITIONAL(HookSetFail_##name, cond)                                         \
  {                                                                                                \
    HOOKSHOT_TEST_ASSERT(result == hookshot->CreateHook(&name##_Test, &name##_Test_Hook));         \
  }

/// Convenience wrapper that unconditionally runs a test in which the expected result is the hook
/// failing to be set and the supplied error code being returned.
#define HOOKSHOT_HOOK_SET_FAIL_TEST(name, result)                                                  \
  HOOKSHOT_HOOK_SET_FAIL_TEST_CONDITIONAL(name, result, true)

/// Encapsulates the logic that implements a completely custom Hookshot test.
/// This particular macro just ensures a proper test case naming convention.
#define HOOKSHOT_CUSTOM_TEST_CONDITIONAL(name, cond)                                               \
  HOOKSHOT_TEST_CASE_CONDITIONAL(Custom_##name, cond)

/// Convenience wrapper that unconditionally runs a custom test.
#define HOOKSHOT_CUSTOM_TEST(name) HOOKSHOT_CUSTOM_TEST_CONDITIONAL(name, true)
