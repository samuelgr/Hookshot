/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file TestMain.cpp
 *   Entry point for the test executable.
 **************************************************************************************************/

#include <Infra/Test/Harness.h>

int wmain(int argc, const wchar_t* argv[])
{
  return Infra::Test::Harness::RunTestsWithMatchingPrefix(((argc > 1) ? argv[1] : L""));
}
