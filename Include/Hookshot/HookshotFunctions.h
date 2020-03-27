/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotFunctions.h
 *   Function prototypes and macros for the public Hookshot interface.
 *   External users should include Hookshot.h instead of this file.
 *****************************************************************************/


#pragma once

#include "HookshotTypes.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    /// Causes Hookshot to attempt to install a hook on the specified function.
    /// @param [in,out] originalFunc Address of the function that should be hooked.
    /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
    /// @return Result of the operation.
    __declspec(dllimport) EResult __fastcall CreateHook(void* originalFunc, const void* hookFunc);

    /// Disables the hook function associated with the specified hook.
    /// On success, going forward all invocations of the original function will execute as if not hooked at all, and Hookshot no longer associates the hook function with the hook.
    /// To re-enable the hook, use #ReplaceHookFunction and identify the hook by its original function address.
    /// @param [in] originalOrHookFunc Address of either the original function or the current hook function (it does not matter which) currently associated with the hook.
    /// @return Result of the operation.
    __declspec(dllimport) EResult __fastcall DisableHookFunction(const void* originalOrHookFunc);

    /// Initializes the Hookshot library.
    /// Hook modules do not need to invoke this function because Hookshot initializes itself before loading them.
    /// If linking directly against the Hookshot library, this function must be invoked before other Hookshot functions.
    __declspec(dllimport) void __fastcall InitializeLibrary(void);

    /// Retrieves and returns an address that, when invoked as a function, calls the original (i.e. un-hooked) version of the hooked function.
    /// This is useful for accessing the original behavior of the function being hooked.
    /// It is up to the caller to ensure that invocations to the returned address satisfy all calling convention and parameter type requirements of the original function.
    /// The returned address is not the original entry point of the hooked function but rather a trampoline address that Hookshot created when installing the hook.
    /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
    /// @return Address that can be invoked to access the functionality of the original function, or `NULL` in the event that a hook cannot be found matching the specified function.
    __declspec(dllimport) const void* __fastcall GetOriginalFunction(const void* originalOrHookFunc);

    /// Modifies an existing hook by replacing its hook function.
    /// The existing hook is identified either by the address of the original function or the address of the current hook function.
    /// On success, Hookshot associates the new hook function with the hook and forgets about the old hook function.
    /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
    /// @return Result of the operation.
    __declspec(dllimport) EResult __fastcall ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc);
}


/// Convenient definition for the entry point of a Hookshot hook module.
/// If building a hook module, use this macro to create a hook module entry point.
#define HOOKSHOT_HOOK_MODULE_ENTRY          extern "C" __declspec(dllexport) void __fastcall HookshotMain(void)
