/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file DebugAssert.h
 *   Standard mechanism for accessing debug assertion functionality.
 **************************************************************************************************/

#pragma once

#include <crtdbg.h>

/// Wrapper around debug assertion functionality.
/// Provides an interface like `static_assert` which takes an expression and a narrow-string message
/// compile-time constant literal.
#define DebugAssert(expr, msg)                                                                     \
  _ASSERT_EXPR((expr), L"\n" _CRT_WIDE(msg) L"\n\nFunction:\n" __FUNCTIONW__)
