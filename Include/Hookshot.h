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
#include <string>
#include <tchar.h>


namespace Hookshot
{
    /// Opaque handle used to identify hooks.
    typedef int THookID;

    /// Address type for identifying the starting address of functions.
    /// Must be cast somehow before being invoked.
    typedef void* TFunc;

    /// Enumeration of possible errors when setting a hook.
    /// These are the negative values returned in place of #THookID, where applicable.
    enum EHookError : THookID
    {
        HookErrorInvalidArgument = -1,                              ///< An argument that was supplied is invalid.
        HookErrorNotFound = -2,                                     ///< Requested hook could not be found.
        HookErrorInitializationFailed = -3,                         ///< Internal initialization steps failed.
        HookErrorLibraryNotLoaded = -4,                             ///< Cannot set a hook because the requested DLL is not loaded.
        HookErrorAllocationFailed = -5,                             ///< Unable to allocate a new hook data structure.
        HookErrorDuplicate = -6,                                    ///< Specified function is already hooked.
        HookErrorResolveModuleName = -7,                            ///< Internal error resolving the absolute path for the specified DLL.
        HookErrorResolveFunctionName = -8,                          ///< Internal error resolving the name of the requested exported function.
        HookErrorFunctionNotExported = -9,                          ///< Cannot set a hook because the requested DLL does not export the requested function.
        HookErrorSetFailed = -10,                                   ///< Trampoline failed to set the hook (debugging required).
        HookErrorMinimumValue = -11                                 ///< Sentinel value, not used as an error code.
    };
    
    /// Interface provided by Hookshot that the hook library can use to configure hooks.
    /// Hookshot creates an object that implements this interface and supplies it to the hook library during initialization.
    /// That instance remains valid throughout the execution of the application.
    /// Its methods can be called at any time and are completely concurrency-safe.
    class IHookConfig
    {
    public:
        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //
        
        /// Retrieves and returns an address that, when invoked as a function, calls the original (i.e. un-hooked) version of the hooked function.
        /// This is useful for accessing the original behavior of the function being hooked.
        /// It is up to the caller to ensure that invocations to the returned address satisfy all calling convention and parameter type requirements of the original function.
        /// The returned address is not the original entry point of the hooked function but rather a trampoline address that Hookshot created when installing the hook.
        /// @param [in] hook Opaque handle that identifies the hook in question.
        /// @return Address that can be invoked to access the functionality of the original function, or `NULL` in the event of an error.
        virtual const TFunc GetOriginalFunctionForHook(const THookID hook) = 0;

        /// Identifies the hook associated with the target function, if one is defined.
        /// @param [in] targetFunc Address of the function that was previously hooked.
        /// @return Opaque handle used to identify the previously-installed hook, or a member of #EHookError to indicate an error.
        virtual THookID IdentifyHook(const TFunc targetFunc) = 0;
        
        /// Causes Hookshot to attempt to install a hook on the specified function.
        /// Once installed, the hook cannot be modified or deleted.
        /// The hook library can emulate modification or deletion by modifying the behavior of the supplied hook function based on runtime conditions.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @param [in,out] targetFunc Address of the function that should be hooked.
        /// @return Opaque handle used to identify the newly-installed hook, or a member of #EHookError to indicate an error.
        virtual THookID SetHook(const TFunc hookFunc, TFunc targetFunc) = 0;
    };
}
