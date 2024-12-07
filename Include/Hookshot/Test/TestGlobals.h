/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file TestGlobals.h
 *   Declaration of global data for use in test cases.
 **************************************************************************************************/

#pragma once

#include "Hookshot.h"

namespace HookshotTest
{
  /// Retrieves a pointer to the Hookshot interface object that test cases should use. Initialized
  /// once and then reused across all test cases.
  /// @return Pointer to the Hookshot interface object.
  Hookshot::IHookshot* HookshotInterface(void);
} // namespace HookshotTest
