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
        std::unordered_map<TFunc, THookID> hookMap;
        
        /// Trampoline storage.
        /// Used internally to implement hooks.
        std::vector<TrampolineStore> trampolines;

#ifdef HOOKSHOT64
        // TODO: add additional data structures for the 64-bit version.
#endif


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        HookStore(void);

        /// Copy constructor. Should never be invoked.
        HookStore(const HookStore&) = delete;

        
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Hookshot.h" for documentation.

        const TFunc GetOriginalFunctionForHook(const THookID hook) override;
        THookID IdentifyHook(const TFunc targetFunc) override;
        THookID SetHook(const TFunc hookFunc, TFunc targetFunc) override;
    };
}
