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
    /// Enumeration of possible errors from Hookshot functions.
    enum class EResult
    {
        // Success codes.
        HookshotResultSuccess,                                      ///< Operation was successful.
        HookshotResultNoEffect,                                     ///< Operation did not generate an error but had no effect.

        // Boundary value between success and failure.
        HookshotResultBoundaryValue,                                ///< Boundary value between success and failure, not used as an error code.

        // Failure codes.
        HookshotResultFailAllocation,                               ///< Unable to allocate a new hook data structure.
        HookshotResultFailBadState,                                 ///< Hookshot is not initialized. Invoke #InitializeLibrary and try again.
        HookshotResultFailCannotSetHook,                            ///< Failed to set the hook.
        HookshotResultFailDuplicate,                                ///< Specified function is already hooked.
        HookshotResultFailInvalidArgument,                          ///< An argument that was supplied is invalid.
        HookshotResultFailInternal,                                 ///< Internal error.
        HookshotResultFailNotFound,                                 ///< Unable to find a hook using the supplied identification.

        // Upper sentinal.
        HookshotResultUpperBoundValue                               ///< Upper sentinel value, not used as an error code.
    };

    /// Convenience function used to determine if a hook operation succeeded.
    /// @param [in] result Result code returned from most Hookshot functions.
    /// @return `true` if the identifier represents success, `false` otherwise.
    inline bool SuccessfulResult(const EResult result)
    {
        return (result < EResult::HookshotResultBoundaryValue);
    }
}
