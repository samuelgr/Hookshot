/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file HookSetSuccess.cpp
 *   Test cases that follow the HOOK_SET_SUCCESS pattern.
 **************************************************************************************************/

#include "CpuInfo.h"
#include "TestPattern.h"

namespace HookshotTest
{
  // Each is implemented in a source file named identically to the test name.

  HOOKSHOT_HOOK_SET_SUCCESS_TEST(BasicFunction);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(CallSubroutine);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(JumpAbsolutePositionRelative);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST_CONDITIONAL(
      JumpAbsolutePositionRelativeRexW, CpuInfo::Is64BitLongModeEnabled());
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(JumpBackwardRel8);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(JumpForwardRel8);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(JumpBackwardRel32);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(JumpForwardRel32);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(LoopJumpAssist);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(LoopWithinTransplant);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(PositionRelativeAddressGeneration);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(PositionRelativeLoad);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(RelBrAtTransplantEdge);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(RelBrBeforeTransplantEdge);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST(ShortFunctionWithPadding);
  HOOKSHOT_HOOK_SET_SUCCESS_TEST_CONDITIONAL(
      TransactionalMemoryFallback, CpuInfo::FeatureFlags().rtm);
} // namespace HookshotTest
