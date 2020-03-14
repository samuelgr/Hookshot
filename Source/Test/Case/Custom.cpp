/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Custom.cpp
 *   Test cases that follow the CUSTOM pattern.
 *****************************************************************************/

#include "CpuInfo.h"
#include "TestPattern.h"

#include <cstdio>
#include <cstdlib>
#include <tchar.h>


// -------- MACROS --------------------------------------------------------- //

/// Generates a new function using #FunctionGenerator and returns its address.
#define GENERATE_FUNCTION()                 (&FunctionGenerator<__LINE__>)


// -------- INTERNAL FUNCTIONS --------------------------------------------- //

/// Not intended ever to be called, but can be used to generate original and hook functions.
template <int n> HOOKSHOT_TEST_HELPER_FUNCTION int FunctionGenerator(void)
{
#ifdef UNICODE
    constexpr TCHAR kFunctionName[] = __FUNCTIONW__;
#else
    constexpr TCHAR kFunctionName[] = __FUNCTION__;
#endif

    for (int i = 0; i < 10; ++i)
        srand((unsigned int)(n + i));

    return n;
}

 
// -------- TEST CASES ----------------------------------------------------- //

// Attempts to set the same hook twice.
// Expected result is a failure due to the hook already existing.
HOOKSHOT_CUSTOM_TEST(Duplicate)
{
    const auto originalFunc = GENERATE_FUNCTION();
    const auto hookFunc = GENERATE_FUNCTION();

    HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->SetHook(originalFunc, hookFunc)));
    HOOKSHOT_TEST_ASSERT(Hookshot::EHookError::HookErrorDuplicate == hookConfig->SetHook(originalFunc, hookFunc));

    HOOKSHOT_TEST_PASSED;
}

// Creates a hook cycle of length 2.
// Function A hooks function B (OK), then function B hooks function A (creates a cycle, should fail).
HOOKSHOT_CUSTOM_TEST(Cycle2)
{
    const auto funcA = GENERATE_FUNCTION();
    const auto funcB = GENERATE_FUNCTION();

    HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->SetHook(funcA, funcB)));
    HOOKSHOT_TEST_ASSERT(!Hookshot::SuccessfulResult(hookConfig->SetHook(funcB, funcA)));

    HOOKSHOT_TEST_PASSED;
}

// Creates a hook cycle of length 3.
// Function A hooks function B (OK), function B hooks function C (can go either way, acceptable if this fails), and finally function C hooks function A (creates a cycle, should fail).
HOOKSHOT_CUSTOM_TEST(Cycle3)
{
    const auto funcA = GENERATE_FUNCTION();
    const auto funcB = GENERATE_FUNCTION();
    const auto funcC = GENERATE_FUNCTION();

    HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->SetHook(funcA, funcB)));
    hookConfig->SetHook(funcB, funcC);
    HOOKSHOT_TEST_ASSERT(!Hookshot::SuccessfulResult(hookConfig->SetHook(funcC, funcA)));

    HOOKSHOT_TEST_PASSED;
}
