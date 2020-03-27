/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookSetFail.cpp
 *   Test cases that follow the HOOK_SET_FAIL pattern.
 *****************************************************************************/

#include "CpuInfo.h"
#include "TestPattern.h"

#include "Hookshot/Hookshot.h"


namespace HookshotTest
{
    // -------- TEST CASES ------------------------------------------------- //
    // Each is implemented in a source file named identically to the test name.
    // See source files for documentation.

    HOOKSHOT_HOOK_SET_FAIL_TEST(InvalidInstruction, Hookshot::EHookshotResult::HookshotResultFailCannotSetHook);
    HOOKSHOT_HOOK_SET_FAIL_TEST_CONDITIONAL(JumpForwardTooFar, Hookshot::EHookshotResult::HookshotResultFailCannotSetHook, CpuInfo::Is64BitLongModeEnabled());
    HOOKSHOT_HOOK_SET_FAIL_TEST(OneByteFunction, Hookshot::EHookshotResult::HookshotResultFailCannotSetHook);
}
