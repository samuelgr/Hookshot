/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookStore.h
 *   Data structure declaration for holding information about hooks.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "HookshotTypes.h"
#include "Trampoline.h"
#include "TrampolineStore.h"

#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <vector>


namespace Hookshot
{
    /// Holds information about hooks and provides an interface a hook module can use to configure them.
    /// Enforces serialization between threads as needed.
    /// This is a global data structure accessed using an interface object.
    class HookStore
    {
    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Enforces serialized access to all parts of the hook data structure.
        static std::shared_mutex hookStoreMutex;

        /// Maps from function address (either original or target) to trampoline address.
        static std::unordered_map<const void*, Trampoline*> functionToTrampoline;

        /// Maps from trampoline address to original function address.
        static std::unordered_map<Trampoline*, const void*> trampolineToOriginalFunction;
        
        /// Trampoline storage.
        /// Used internally to implement hooks.
        static std::vector<TrampolineStore> trampolines;

#ifdef HOOKSHOT64
        /// Maps from target function base address to trampoline storage index.
        /// In 64-bit mode, TrampolineStore objects are placed close to target functions.
        /// For each target function, the base address of its associated memory region is computed and used as a key to this map.
        /// TrampolineStore objects are appended to the storage vector as normal, and the index of each such created TrampolineStore object is the value.
        static std::unordered_map<void*, int> trampolineStoreMap;
#endif


    public:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Causes Hookshot to attempt to install a hook on the specified function.
        /// @param [in,out] originalFunc Address of the function that should be hooked.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @return Result of the operation.
        EResult CreateHook(void* originalFunc, const void* hookFunc);

        /// Disables the hook function associated with the specified hook.
        /// On success, going forward all invocations of the original function will execute as if not hooked at all, and Hookshot no longer associates the hook function with the hook.
        /// To re-enable the hook, use #ReplaceHookFunction and identify the hook by its original function address.
        /// @param [in] originalOrHookFunc Address of either the original function or the current hook function (it does not matter which) currently associated with the hook.
        /// @return Result of the operation.
        EResult DisableHookFunction(const void* originalOrHookFunc);

        /// Retrieves and returns an address that, when invoked as a function, calls the original (i.e. un-hooked) version of the hooked function.
        /// This is useful for accessing the original behavior of the function being hooked.
        /// It is up to the caller to ensure that invocations to the returned address satisfy all calling convention and parameter type requirements of the original function.
        /// The returned address is not the original entry point of the hooked function but rather a trampoline address that Hookshot created when installing the hook.
        /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
        /// @return Address that can be invoked to access the functionality of the original function, or `nullptr` in the event that a hook cannot be found matching the specified function.
        const void* GetOriginalFunction(const void* originalOrHookFunc);

        /// Modifies an existing hook by replacing its hook function.
        /// The existing hook is identified either by the address of the original function or the address of the current hook function.
        /// On success, Hookshot associates the new hook function with the hook and forgets about the old hook function.
        /// @param [in] originalOrHookFunc Address of either the original function or the hook function (it does not matter which) currently associated with the hook.
        /// @return Result of the operation.
        EResult ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc);
    };
}
