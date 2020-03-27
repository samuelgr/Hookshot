/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotTypes.h
 *   Types definitions used in the public Hookshot interface.
 *   External users should include Hookshot.h instead of this file.
 *****************************************************************************/

#pragma once


namespace Hookshot
{
    /// Enumeration of possible results from Hookshot functions.
    enum class EResult
    {
        // Success codes.
        Success,                                                        ///< Operation was successful.
        NoEffect,                                                       ///< Operation did not generate an error but had no effect.

        // Boundary value between success and failure.
        BoundaryValue,                                                  ///< Boundary value between success and failure, not used as an error code.

        // Failure codes.
        FailAllocation,                                                 ///< Unable to allocate a new hook data structure.
        FailBadState,                                                   ///< Hookshot is not initialized. Invoke #InitializeLibrary and try again.
        FailCannotSetHook,                                              ///< Failed to set the hook.
        FailDuplicate,                                                  ///< Specified function is already hooked.
        FailInvalidArgument,                                            ///< An argument that was supplied is invalid.
        FailInternal,                                                   ///< Internal error.
        FailNotFound,                                                   ///< Unable to find a hook using the supplied identification.

        // Upper sentinel.
        UpperBoundValue                                                 ///< Upper sentinel value, not used as an error code.
    };

    /// Convenience function used to determine if a hook operation succeeded.
    /// @param [in] result Result code returned from most Hookshot functions.
    /// @return `true` if the identifier represents success, `false` otherwise.
    inline bool SuccessfulResult(const EResult result)
    {
        return (result < EResult::BoundaryValue);
    }
}
