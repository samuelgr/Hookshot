/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Hookshot.h
 *   Public interface for interacting with Hookshot to configure hooks.
 *   Intended to be included externally by hook libraries.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <tchar.h>


namespace Hookshot
{
    /// Opaque handle used to identify hooks.
    typedef int32_t THookID;

    /// Address type for identifying the starting address of functions.
    /// Must be cast somehow before being invoked.
    typedef const void* TFunc;

    /// Enumeration of possible hook statuses.
    enum EHookStatus : uint32_t
    {
        // Success
        HookStatusSuccess = 0,                                      ///< Hook was installed successfully.

        // Argument errors
        HookStatusOutOfRange,                                       ///< Argument provided (i.e. the hook handle) is out of range.

        // Other hook statuses
        HookStatusLibraryNotYetLoaded,                              ///< Hook is present, but the associated DLL has not yet been loaded.

        // Maximum value in this enumeration
        HookStatusMaximumValue                                      ///< Sentinel value, not used as an error code.
    };
    
    /// Interface provided by Hookshot that the hook library can use to configure hooks.
    /// Hookshot creates an object that implements this interface and supplies it to the hook library during initialization.
    /// That instance remains valid throughout the execution of the application.
    /// Its methods can be called at any time and are completely concurrency-safe.
    class IHookConfig
    {
    public:
        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //

        /// Checks the status of a hook that was previously installed.
        /// Guaranteed not to serialize.
        /// @param [in] hook Opaque handle that identifies the hook in question.
        /// @return Indication of the status of the identified hook.
        virtual EHookStatus GetHookStatus(const THookID hook) = 0;
        
        /// Retrieves and returns an address that, when invoked as a function, calls the original (i.e. un-hooked) version of the hooked function.
        /// This is useful for accessing the original behavior of the function being hooked.
        /// It is up to the caller to ensure that invocations to the returned address satisfy all calling convention and parameter type requirements of the original function.
        /// The returned address is not the original entry point of the hooked function but rather a trampoline address that Hookshot created when installing the hook.
        /// Guaranteed not to serialize.
        /// @param [in] hook Opaque handle that identifies the hook in question.
        /// @return Address that can be invoked to access the functionality of the original function, or `NULL` in the event of an error (invoke GetHookStatus for more information).
        virtual TFunc GetOriginalFunctionForHook(const THookID hook) = 0;

        /// Looks up the handle of a previously-installed hook.
        /// Serialized with respect to other hook operations that also serialize and so should be called sparingly, if at all.
        /// It is recommended that the hook library record hook identifiers in convenient locations to avoid the need to call this method.
        /// @param [in] dllName Name of the DLL that exports the function that was hooked.
        /// @param [in] exportFuncName Exported function name of the function that was hooked.
        virtual THookID IdentifyHook(const TCHAR* const dllName, const TCHAR* const exportFuncName) = 0;
        
        /// Causes Hookshot to attempt to install a hook on the specified function.
        /// Once installed, the hook cannot be modified or deleted.
        /// The hook library can emulate modification or deletion by modifying the behavior of the supplied hook function based on runtime conditions.
        /// Serialized with respect to other hook operations that also serialize.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @param [in] dllName Name of the DLL that exports the function to be hooked.
        /// @param [in] exportFuncName Exported function name of the function to be hooked.
        /// @return Opaque handle used to identify the newly-installed hook, or a negative value to indicate an error.
        virtual THookID SetHook(const TFunc hookFunc, const TCHAR* const dllName, const TCHAR* const exportFuncName) = 0;
    };
}
