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

#include <concrt.h>
#include <cstdint>
#include <hookshot.h>
#include <string>
#include <unordered_map>
#include <vector>


namespace Hookshot
{
    /// Holds information about hooks and provides an interface a hook module can use to configure them.
    /// Validates hooks that are requested to be set, and causes hooks to be set when they are added to this data structure.
    /// Enforces serialization between threads as needed.
    class HookStore : public IHookConfig
    {
    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Second-level data structure type used to map from function names to hook identifiers.
        using TFunctionMap = std::unordered_map<THookString, THookID>;

        /// Top-level data structure type used to map from DLL names to second-level data structure types.
        using TDllMap = std::unordered_map<THookString, TFunctionMap&>;

        
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Enforces serialized access to all parts of the hook data structure.
        /// The vast majority of accesses will be read accesses, so these are allowed to occur in parallel.
        concurrency::reader_writer_lock lock;

        /// Top-level map data structure.
        /// Maps DLL name to the second-level map data structure that corresponds to that DLL.
        TDllMap mapDllNameToFunctionMap;

        /// Second-level map data structure.
        /// Each element is scoped at the level of a single DLL, mapping from function name to hook identifier.
        /// Elements may be inserted at the end as more DLLs are added, but they are never removed.
        std::vector<TFunctionMap> mapFunctionNameToHookID;


    public:
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See "Hookshot.h" for documentation.

        virtual EHookStatus GetHookStatus(const THookID hook);
        virtual TFunc GetOriginalFunctionForHook(const THookID hook);
        virtual THookID IdentifyHook(const THookString dllName, const THookString exportFuncName);
        virtual THookID SetHook(const TFunc hookFunc, const THookString dllName, const THookString exportFuncName);
    };
}
