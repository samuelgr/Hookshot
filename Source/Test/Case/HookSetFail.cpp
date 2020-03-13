/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookSetSuccessful.cpp
 *   Test cases that follow the HOOK_SET_FAIL pattern.
 *****************************************************************************/

#include "CpuInfo.h"
#include "TestPattern.h"


// -------- TEST CASES ----------------------------------------------------- //
// Each is implemented in a source file named identically to the test name.
// See source files for documentation.

HOOKSHOT_HOOK_SET_FAIL_TEST_CONDITIONAL(JumpForwardTooFar, CpuInfo::Is64BitLongModeEnabled());
HOOKSHOT_HOOK_SET_FAIL_TEST(OneByteFunction);
