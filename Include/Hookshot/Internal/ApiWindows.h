/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file ApiWindows.h
 *   Common header file for the correct version of the Windows API, along with declarations of
 *   supporting functions.
 **************************************************************************************************/

#pragma once

// Windows header files are sensitive to include order. Compilation will fail if the order is
// incorrect. Top-level macros and headers must come first, followed by headers for other parts
// of system functionality.

// clang-format off

#define NOMINMAX
#include <sdkddkver.h>
#include <windows.h>

// clang-format on

#include <commctrl.h>
#include <psapi.h>
#include <shlobj.h>
#include <shlwapi.h>
