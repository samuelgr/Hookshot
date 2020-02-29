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
    class HookStore : public IHookConfig
    {
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Enforces serialized access to all parts of the hook data structure.
        concurrency::reader_writer_lock lock;

        /// Maps from target function address to hook identifier.
        std::unordered_map<const void*, THookID> hookMap;
        
        /// Trampoline storage.
        /// Used internally to implement hooks.
        std::vector<TrampolineStore> trampolines;

#ifdef HOOKSHOT64
        /// Maps from target function base address to trampoline storage index.
        /// In 64-bit mode, TrampolineStore objects are placed close to target functions.
        /// For each target function, the base address of its associated memory region is computed and used as a key to this map.
        /// TrampolineStore objects are appended to the storage vector as normal, and the index of each such created TrampolineStore object is the value.
        std::unordered_map<void*, int> trampolineStoreMap;
#endif


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        HookStore(void);

        /// Copy constructor. Should never be invoked.
        HookStore(const HookStore&) = delete;

        
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Hookshot.h" for documentation.

        const void* GetOriginalFunctionForHook(const THookID hook) override;
        THookID IdentifyHook(const void* targetFunc) override;
        THookID SetHook(const void* hookFunc, void* targetFunc) override;
    };
}
