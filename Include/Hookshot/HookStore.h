/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file HookStore.h
 *   Data structure declaration for holding information about hooks.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "TrampolineStore.h"

#include <concrt.h>
#include <cstdint>
#include <hookshot.h>
#include <unordered_map>
#include <vector>


namespace Hookshot
{
    /// Holds information about hooks and provides an interface a hook module can use to configure them.
    /// Enforces serialization between threads as needed.
    /// This is a global data structure accessed using an interface object.
    class HookStore : public IHookConfig
    {
    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Enforces serialized access to all parts of the hook data structure.
        static concurrency::reader_writer_lock lock;

        /// Maps from target function address to hook identifier.
        static std::unordered_map<const void*, THookID> hookMap;
        
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
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Hookshot.h" for documentation.

        const void* GetOriginalFunctionForHook(THookID hook) override;
        THookID IdentifyHook(const void* targetFunc) override;
        THookID SetHook(void* targetFunc, const void* hookFunc) override;
    };
}
