/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file RemoteProcessInjector.h
 *   Interface declaration for requesting IPC-based process injection.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "InjectResult.h"

#include <cstdint>


namespace Hookshot
{
    namespace RemoteProcessInjector
    {
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Defines the structure of the shared memory that communicates between two instances of Hookshot.
        /// One instance fills the required input information, and the other performs the requested tasks and fills status output.
        /// To ensure safety, all values are 64-bit integers.
        struct SInjectRequest
        {
            uint64_t processHandle;                                 ///< Handle of the process to inject, as a 64-bit integer.  Must be valid for the instance of Hookshot that performs the injection.
            uint64_t threadHandle;                                  ///< Handle of the main thread in the process to inject, as a 64-bit integer.  Must be valid for the instance of Hookshot that performs the injection.
            bool enableDebugFeatures;                               ///< If `true`, signals to the injected process that a debugger is present, so certain debug features should be enabled.
            uint64_t injectionResult;                               ///< EInjectionResult value, as a 64-bit integer.  Indicates the result of the injection attempt.
            uint64_t extendedInjectionResult;                       ///< Extended injection result, as a 64-bit integer.
        };


        // -------- FUNCTIONS ---------------------------------------------- //

        /// Spawns a Hookshot executable and uses IPC to request that it injects the specified process.
        /// @param [in] processHandle Handle to the process to inject.
        /// @param [in] threadHandle Handle to the main thread of the process to inject.
        /// @param [in] switchArchitecture If `true`, specifies that the injection must cross a processor architecture boundary (i.e. 32-bit -> 64-bit or vice versa).
        /// @return Indicator of the result of the operation.
        EInjectResult InjectProcess(const HANDLE processHandle, const HANDLE threadHandle, const bool switchArchitecture, const bool enableDebugFeatures);
    }
}
