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

#include <cstdint>


namespace Hookshot
{
    /// Enumeration of possible error conditions that arise when attempting to create and inject a process.
    enum EInjectResult : uint32_t
    {
        // Success
        InjectResultSuccess = 0,                                    ///< All operations succeeded.

        // Unknown failure
        InjectResultFailure,                                        ///< Unknown error.

        // Issues creating the process
        InjectResultErrorCreateProcess,                             ///< Creation of the new process failed.
        InjectResultErrorDetermineMachineProcess,                   ///< Determination of the machine type of the new process failed.
        InjectResultErrorArchitectureMismatch,                      ///< New process cannot be injected due to an architecture mismatch with the running Hookshot binary.

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
        InjectResultErrorCannotGenerateInjectCodeFilename,          ///< Failed to compute the name of the file holding injected code.
        InjectResultErrorCannotGenerateLibraryFilename,             ///< Failed to compute the name of the Hookshot library to inject.
        InjectResultErrorCannotLoadInjectCode,                      ///< Failed to load the file containing injection code.
        InjectResultErrorMalformedInjectCodeFile,                   ///< Failed to inject because the file containing inject code is malformed.
        InjectResultErrorInsufficientTrampolineSpace,               ///< Failed to inject due to insufficient space available for storing the old trampoline code.
        InjectResultErrorInsufficientCodeSpace,                     ///< Failed to inject due to insufficient allocated space for the code region.
        InjectResultErrorInsufficientDataSpace,                     ///< Failed to inject due to insufficient allocated space for the data region.
        InjectResultErrorInternalInvalidParams,                     ///< Failed to inject due to an internal issue resulting in invalid injection parameters.
        InjectResultErrorSetFailedRead,                             ///< Failed to inject due to a failed attempt to retrieve existing code from the injected process.
        InjectResultErrorSetFailedWrite,                            ///< Failed to inject due to a failed attempt to write new code into the injected process.
        InjectResultErrorRunFailedResumeThread,                     ///< Failed to run injected code due to the main thread of the injected process not waking up.
        InjectResultErrorRunFailedSync,                             ///< Failed to synchronize with injected code due to an issue reading from or writing to injected process memory.
        InjectResultErrorRunFailedSuspendThread,                    ///< Failed to place the injected process back into a suspended state after running the injected code.
        InjectResultErrorUnsetFailed,                               ///< Failed to inject due to a failed attempt to return the trampoline region to its original content.

        // Issues spawning a new instance of Hookshot due to an architecture mismatch.
        InjectResultErrorCannotGenerateExecutableFilename,          ///< Failed to compute the name of the executable to spawn.
        InjectResultErrorInterProcessCommunicationFailed,           ///< Failed to perform inter-process communication.
        InjectResultErrorCreateHookshotProcessFailed,               ///< Failed to spawn a new Hookshot instance.

        // Issues encountered while running injection code to initialize the injected process.
        InjectResultErrorCannotLocateRequiredFunctions,             ///< Failed to locate required functions in the address space of the injected process.
        InjectResultErrorCannotWriteRequiredFunctionLocations,      ///< Failed to write the locations of the required functions into the address space of the injected process.
        InjectResultErrorCannotReadStatus,                          ///< Failed to read status information from the injected process.
        InjectResultErrorCannotLoadLibrary,                         ///< Failed to load the Hookshot library in the injected process.
        InjectResultErrorMalformedLibrary,                          ///< Loaded Hookshot library is malformed.
        InjectResultErrorLibraryInitFailed,                         ///< Loaded Hookshot library failed to initialize.

        // Maximum value in this enumeration.
        InjectResultMaximumValue                                    ///< Sentinel value, not used as an error code.
    };
}
