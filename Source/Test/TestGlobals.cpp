/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file TestGlobals.cpp
 *   Implementation of global data for use in test cases.
 **************************************************************************************************/

#include "Hookshot.h"

namespace HookshotTest
{
  Hookshot::IHookshot* HookshotInterface(void)
  {
    static Hookshot::IHookshot* const hookshotInterface = HookshotLibraryInitialize();
    return hookshotInterface;
  }
} // namespace HookshotTest
