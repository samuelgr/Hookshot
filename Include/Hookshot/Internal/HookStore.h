/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
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
    class HookStore : public IHookshot
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
        static std::unordered_map<void*, std::vector<int>> trampolineStoreMap;
#endif


    public:
        // -------- CLASS METHODS ------------------------------------------ //

        /// Internal version of #CreateHook. Intended to be used within Hookshot only.
        /// Can be used to create hooks that are for internal Hookshot use and hooks requested by API users.
        /// @param [in] originalFunc Address of the function that should be hooked.
        /// @param [in] hookFunc Hook function that should be invoked instead of the original function.
        /// @param [in] isInternal If `true`, identifies the requested hook as being for internal Hookshot use.
        /// @param [out] originalFuncAfterHook For internal hooks only, this is a pointer to be filled with what would ordinarily be returned by #GetOriginalFunction.
        /// @return Result of the operation.
        static EResult __fastcall CreateHookInternal(void* originalFunc, const void* hookFunc, const bool isInternal, const void** originalFuncAfterHook);


        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "HookshotTypes.h" for documentation.

        EResult __fastcall CreateHook(void* originalFunc, const void* hookFunc) override;
        EResult __fastcall DisableHookFunction(const void* originalOrHookFunc) override;
        const void* __fastcall GetOriginalFunction(const void* originalOrHookFunc) override;
        EResult __fastcall ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc) override;
    };
}
