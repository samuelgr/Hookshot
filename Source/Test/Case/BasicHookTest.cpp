/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 *****************************************************************************/

#include "TestCase.h"


// -------- HELPER DEFINITIONS --------------------------------------------- //

static constexpr int kOriginalFunctionResult = 1111111;
static constexpr int kHookFunctionResult = 2222222;

HOOKSHOT_TEST_HELPER_FUNCTION int originalFunction(void)
{
    return kOriginalFunctionResult;
}

HOOKSHOT_TEST_HELPER_FUNCTION int hookFunction(void)
{
    return kHookFunctionResult;
}

// -------- TEST CASES ----------------------------------------------------- //

// The most basic imaginable hook test.  Attempts to replace a function.
// Exercises all parts of the Hookshot external API.
HOOKSHOT_TEST_CASE(BasicHookTest)
{
    const Hookshot::THookID hookId = hookConfig->SetHook(&originalFunction, &hookFunction);
    
    HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookId));
    HOOKSHOT_TEST_ASSERT(hookId == hookConfig->IdentifyHook(&originalFunction));
    HOOKSHOT_TEST_ASSERT(kHookFunctionResult == originalFunction());
    
    const auto originalFunctionalityPtr = (decltype(&originalFunction))hookConfig->GetOriginalFunctionForHook(hookId);

    HOOKSHOT_TEST_ASSERT(nullptr != originalFunctionalityPtr);
    HOOKSHOT_TEST_ASSERT(kOriginalFunctionResult == originalFunctionalityPtr());

    HOOKSHOT_TEST_PASSED;
}
