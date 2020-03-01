/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file TestPattern.h
 *   Common pattern definitions for Hookshot test cases.
 *****************************************************************************/

#pragma once

#include "TestCase.h"

#include <cstddef>
#include <tchar.h>


// -------- CONSTANTS ------------------------------------------------------ //

/// Expected result of a call to an original version of a function.
/// Test cases should compare the result received from what should be an original function invocation to this value.
static constexpr size_t kOriginalFunctionResult = 1111111;

/// Expected result of a call to a hooked version of a function.
/// Test cases should compare the result received from what should be hook function invocation to this value.
static constexpr size_t kHookFunctionResult = 2222222;


// -------- TYPE DEFINITIONS ----------------------------------------------- //

/// Signature of a Hookshot test function.
/// Most, if not all, test functions are written in assembly.
typedef size_t(__cdecl* THookshotTestFunc)(void);


// -------- TEST PATTERN MACROS -------------------------------------------- //

/// Encapsulates the logic that implements a Hookshot test in which a hook is set successfully, thus effectively replacing the original function with the hooked version.
/// This test pattern verifies that Hookshot correcly returns a hook identifier that identifies the hook, sets the hook, and enables access to the original functionality even after setting the hook.
/// The test case name is the macro parameter. It relies on two functions being defined externally, name_Original and name_Hook.
/// Whatever is the control flow, name_Original should return #kOriginalFunctionResult when executed correctly, and name_Hook should likewise return #kHookFunctionResult.
#define HOOKSHOT_HOOK_SET_SUCCESSFUL_TEST(name)                                                                             \
    extern "C" size_t name##_Original(void);                                                                                \
    extern "C" size_t name##_Hook(void);                                                                                    \
    HOOKSHOT_TEST_CASE(HookSetSuccessful_##name)                                                                            \
    {                                                                                                                       \
        const Hookshot::THookID hookId = hookConfig->SetHook(&name##_Original, &name##_Hook);                               \
                                                                                                                            \
        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookId));                                                           \
        HOOKSHOT_TEST_ASSERT(hookId == hookConfig->IdentifyHook(&name##_Original));                                         \
        HOOKSHOT_TEST_ASSERT(kHookFunctionResult == name##_Original());                                                     \
                                                                                                                            \
        THookshotTestFunc originalFunctionalityPtr = (THookshotTestFunc)hookConfig->GetOriginalFunctionForHook(hookId);     \
                                                                                                                            \
        HOOKSHOT_TEST_ASSERT(nullptr != originalFunctionalityPtr);                                                          \
        HOOKSHOT_TEST_ASSERT(kOriginalFunctionResult == originalFunctionalityPtr());                                        \
        HOOKSHOT_TEST_PASSED;                                                                                               \
    }
