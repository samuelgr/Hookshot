/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file HookResult.h
 *   Declaration of result codes that arise during the setting of hooks.
 *****************************************************************************/

#pragma once

#include <cstdint>


namespace Hookshot
{
    /// Enumeration of possible error conditions that arise when attempting to create and inject a process.
    enum EHookResult : uint32_t
    {
        // Success
        HookResultSuccess = 0,                                      ///< All operations succeeded.

        // Unknown failure
        HookResultFailure,                                          ///< Unknown error.

        // Issues encountered while trying to load the modules that contain hooks
        HookResultInsufficientMemoryFilenames,                      ///< Insufficient buffer space is available to generate hook module filenames.
        HookResultCannotLoadHookModule,                             ///< Failed to load the module that contains hooks.

        // Maximum value in this enumeration.
        HookResultMaximumValue                                      ///< Sentinel value, not used as an error code.
    };
}
