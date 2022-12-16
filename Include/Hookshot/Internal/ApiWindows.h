/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2022
 **************************************************************************//**
 * @file ApiWindows.h
 *   Common header file for the correct version of the Windows API, along with
 *   declarations of supporting functions.
 *****************************************************************************/

#pragma once


#define NOMINMAX

#include <sdkddkver.h>
#include <windows.h>


namespace Hookshot
{
    // -------- FUNCTIONS -------------------------------------------------- //

    /// Retrieves the proper address of a Windows API function.
    /// Many Windows API functions have been moved to lower-level binaries.
    /// See https://docs.microsoft.com/en-us/windows/win32/win7appqual/new-low-level-binaries for more information.
    /// If possible, use the address in the lower-level binary as the original function, otherwise just use the static address.
    /// @param [in] funcName API function name.
    /// @param [in] funcStaticAddress Static address of the function.
    /// @return Recommended address to use for the Windows API function, which could be the same as the static address.
    void* GetWindowsApiFunctionAddress(const char* const funcName, void* const funcStaticAddress);
}
