/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file HookSetFail.cpp
 *   Test cases that follow the HOOK_SET_FAIL pattern.
 **************************************************************************************************/

#include "CpuInfo.h"
#include "Hookshot.h"
#include "TestPattern.h"

namespace HookshotTest
{
  // Each is implemented in a source file named identically to the test name.

  HOOKSHOT_HOOK_SET_FAIL_TEST(InvalidInstruction, Hookshot::EResult::FailCannotSetHook);
  HOOKSHOT_HOOK_SET_FAIL_TEST_CONDITIONAL(
      JumpForwardTooFar, Hookshot::EResult::FailCannotSetHook, CpuInfo::Is64BitLongModeEnabled());
  HOOKSHOT_HOOK_SET_FAIL_TEST(MixedPadding, Hookshot::EResult::FailCannotSetHook);
  HOOKSHOT_HOOK_SET_FAIL_TEST(OneByteFunction, Hookshot::EResult::FailCannotSetHook);
} // namespace HookshotTest
