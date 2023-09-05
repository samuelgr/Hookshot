/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file ApiWindowsShell.h
 *   Common header file for the correct version of the Windows API along with additional shell
 *   functionality.
 **************************************************************************************************/

#pragma once

// Windows header files are sensitive to include order. Compilation will fail if the order is
// incorrect.

// clang-format off

#include "ApiWindows.h"

#include <commctrl.h>
#include <psapi.h>
#include <shlobj.h>
#include <shlwapi.h>

// clang-format on
