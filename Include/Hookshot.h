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

    /// Enumeration of possible errors when setting a hook.
    /// These are the negative values returned in place of #THookID, where applicable.
    enum EHookError : THookID
    {
        HookErrorMinimumValue = -100,                               ///< Lower sentinel value, not used as an error code.

        HookErrorInvalidArgument,                                   ///< An argument that was supplied is invalid.
        HookErrorNotFound,                                          ///< Requested hook could not be found.
        HookErrorInitializationFailed,                              ///< Internal initialization steps failed.
        HookErrorAllocationFailed,                                  ///< Unable to allocate a new hook data structure.
        HookErrorDuplicate,                                         ///< Specified function is already hooked.
        HookErrorSetFailed,                                         ///< Trampoline failed to set the hook (debugging required).

        HookErrorMaximumValue                                       ///< Upper sentinel value, not used as an error code.
    };

    /// Convenience function used to determine if a hook operation succeeded.
    /// @param [in] result Hook identifier returned as the result of any #IHookConfig interface method call.
    /// @return `true` if the identifier represents success, `false` otherwise.
    inline bool SuccessfulResult(const THookID result)
    {
        return (result >= 0);
    }
    
    /// Interface provided by Hookshot that the hook library can use to configure hooks.
    /// Hookshot creates an object that implements this interface and supplies it to the hook library during initialization.
    /// That instance remains valid throughout the execution of the application.
    /// Its methods can be called at any time and are completely concurrency-safe.
    /// However, it is highly recommended that hook identifiers and original function pointers be obtained once and cached.
    /// This is because the implementations of all interface methods very likely involve taking a lock.
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
        virtual const void* GetOriginalFunctionForHook(THookID hook) = 0;

        /// Identifies the hook associated with the target function, if one is defined.
        /// @param [in] targetFunc Address of the function that was previously hooked.
        /// @return Opaque handle used to identify the previously-installed hook, or a member of #EHookError to indicate an error.
        virtual THookID IdentifyHook(const void* targetFunc) = 0;
        
        /// Causes Hookshot to attempt to install a hook on the specified function.
        /// Once installed, the hook cannot be modified or deleted.
        /// The hook library can emulate modification or deletion by modifying the behavior of the supplied hook function based on runtime conditions.
        /// If the caller attempts to set a hook after the program is already initialized and running, then the caller is responsible for making sure no other threads are executing code at the target function address while the hook is being set.
        /// @param [in,out] targetFunc Address of the function that should be hooked.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @return Opaque handle used to identify the newly-installed hook, or a member of #EHookError to indicate an error.
        virtual THookID SetHook(void* targetFunc, const void* hookFunc) = 0;
    };
}


/// Convenient definition for the entry point of a Hookshot hook library.
/// Parameter allows specifying the name of the function parameter of type #IHookConfig.
#define HOOKSHOT_HOOK_LIBRARY_ENTRY(hc)     extern "C" __declspec(dllexport) void __stdcall HookshotMain(Hookshot::IHookConfig* hc)


#ifdef HOOKSHOT_LINK_WITH_LIBRARY
/// Hookshot initialization function to be used when linking directly with the Hookshot library instead of loading it through injection.
/// Must be invoked to initialize the Hookshot library before any functionality is accessed.
/// @return Hook configuration interface object, which remains owned by Hookshot, or `NULL` in the event Hookshot failed to initialize.
__declspec(dllimport) Hookshot::IHookConfig* __stdcall HookshotLibraryInitialize(void);
#endif
