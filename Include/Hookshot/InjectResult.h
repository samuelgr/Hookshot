/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file InjectResult.h
 *   Declaration of result codes that arise during process injection.
 *****************************************************************************/

#pragma once


namespace Hookshot
{
    /// Enumeration of possible error conditions that arise when attempting to create and inject a process.
    enum EInjectResult : unsigned int
    {
        // Success
        InjectResultSuccess = 0,                                    ///< All operations succeeded.

        // Issues creating the process
        InjectResultErrorCreateProcess,                             ///< Call to `CreateProcess` failed.  `GetLastError` can be used to obtain more details.

        // Issues determining the base address of the process' executable image
        InjectResultErrorLoadNtDll,                                 ///< Attempt to dynamically load `ntdll.dll` failed.
        InjectResultErrorNtQueryInformationProcessUnavailable,      ///< Attempt to locate `NtQueryInformationProcess` within `ntdll.dll` failed.
        InjectResultErrorNtQueryInformationProcessFailed,           ///< Call to `NtQueryInformationProcess` failed to retrieve the desired information.
        InjectResultErrorReadProcessPEBFailed,                      ///< Virtual memory read of the process PEB failed.

        // Issues determining the entry point address of a process
        InjectResultErrorReadDOSHeadersFailed,                      ///< Failed to read DOS headers from the process' executable image.
        InjectResultErrorReadNTHeadersFailed,                       ///< Failed to read NT headers from the process' executable image.

        // Issues preparing to inject code or data into the target process
        InjectResultErrorVirtualAllocFailed,                        ///< Failed to allocate virtual memory for code and data in the target process.
        InjectResultErrorVirtualProtectFailed,                      ///< Failed to set protection values for code and data in the target process.

        // Issues actually injecting code or data into the target process.
        InjectResultErrorInsufficientCodeSpace,                     ///< Failed to inject due to insufficient allocated space for the code region.
        InjectResultErrorInsufficientDataSpace,                     ///< Failed to inject due to insufficient allocated space for the data region.
        InjectResultErrorInternalInvalidParams,                     ///< Failed to inject due to an internal issue resulting in invalid injection parameters.
        InjectResultErrorSetFailedWriteProcessMemory,               ///< Failed to inject due to a failed attempt to set injected code into the injected process.
    };
}
