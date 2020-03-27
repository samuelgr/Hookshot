/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Hookshot.h
 *   Public interface for interacting with Hookshot to configure hooks.
 *   Intended to be included by hook modules.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>


namespace Hookshot
{
    /// Enumeration of possible errors from Hookshot functions.
    enum class EResult
    {
        // Success codes.
        HookshotResultSuccess,                                      ///< Operation was successful.
        HookshotResultNoEffect,                                     ///< Operation did not generate an error but had no effect.

        // Boundary value between success and failure.
        HookshotResultBoundaryValue,                                ///< Boundary value between success and failure, not used as an error code.

        // Failure codes.
        HookshotResultFailAllocation,                               ///< Unable to allocate a new hook data structure.
        HookshotResultFailCannotSetHook,                            ///< Failed to set the hook.
        HookshotResultFailDuplicate,                                ///< Specified function is already hooked.
        HookshotResultFailInvalidArgument,                          ///< An argument that was supplied is invalid.
        HookshotResultFailInternal,                                 ///< Internal error.
        HookshotResultFailNotFound,                                 ///< Unable to find a hook using the supplied identification.

        // Upper sentinal.
        HookshotResultUpperBoundValue                               ///< Upper sentinel value, not used as an error code.
    };
    
    /// Convenience function used to determine if a hook operation succeeded.
    /// @param [in] result Hook identifier returned as the result of any #IHookConfig interface method call.
    /// @return `true` if the identifier represents success, `false` otherwise.
    inline bool SuccessfulResult(const EResult result)
    {
        return (result < EResult::HookshotResultBoundaryValue);
    }
    
    /// Interface provided by Hookshot that the hook module can use to configure hooks.
    /// Hookshot creates an object that implements this interface and supplies it to the hook module during initialization.
    /// That instance remains valid throughout the execution of the application.
    /// Its methods can be called at any time and are completely concurrency-safe.
    /// However, it is highly recommended that hook identifiers and original function pointers be obtained once and cached.
    /// This is because the implementations of all interface methods very likely involve taking a lock.
    class IHookConfig
    {
    public:
        /// Causes Hookshot to attempt to install a hook on the specified function.
        /// @param [in,out] originalFunc Address of the function that should be hooked.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @return Result of the operation.
        virtual EResult CreateHook(void* originalFunc, const void* hookFunc) = 0;
        
        /// Disables the hook function associated with the specified hook.
        /// On success, going forward all invocations of the original function will execute as if not hooked at all, and Hookshot no longer associates the hook function with the hook.
        /// To re-enable the hook, use #ReplaceHookFunction and identify the hook by its original function address.
        /// @param [in] originalOrHookFunc Address of either the original function or the current hook function (it does not matter which) currently associated with the hook.
        /// @return Result of the operation.
        virtual EResult DisableHookFunction(const void* originalOrHookFunc) = 0;

        /// Retrieves and returns an address that, when invoked as a function, calls the original (i.e. un-hooked) version of the hooked function.
        /// This is useful for accessing the original behavior of the function being hooked.
        /// It is up to the caller to ensure that invocations to the returned address satisfy all calling convention and parameter type requirements of the original function.
        /// The returned address is not the original entry point of the hooked function but rather a trampoline address that Hookshot created when installing the hook.
        /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
        /// @return Address that can be invoked to access the functionality of the original function, or `NULL` in the event that a hook cannot be found matching the specified function.
        virtual const void* GetOriginalFunction(const void* originalOrHookFunc) = 0;

        /// Modifies an existing hook by replacing its hook function.
        /// The existing hook is identified either by the address of the original function or the address of the current hook function.
        /// On success, Hookshot associates the new hook function with the hook and forgets about the old hook function.
        /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
        /// @return Result of the operation.
        virtual EResult ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc) = 0;
    };
}


// If building a hook module, create a hook module entry point and do not define this preprocessor symbol.
// Otherwise, define this preprocessor symbol to build a library or executable that depends on the Hookshot library.
#ifdef HOOKSHOT_LINK_WITH_LIBRARY
/// Hookshot initialization function to be used when linking directly with the Hookshot library instead of loading it through injection.
/// Must be invoked to initialize the Hookshot library before any functionality is accessed.
/// @return Hook configuration interface object, which remains owned by Hookshot, or `NULL` in the event Hookshot failed to initialize.
__declspec(dllimport) Hookshot::IHookConfig* __fastcall HookshotLibraryInitialize(void);
#else
/// Convenient definition for the entry point of a Hookshot hook module.
/// Parameter allows specifying the name of the function parameter of type #IHookConfig.
#define HOOKSHOT_HOOK_MODULE_ENTRY(hc)      extern "C" __declspec(dllexport) void __fastcall HookshotMain(Hookshot::IHookConfig* hc)
#endif
