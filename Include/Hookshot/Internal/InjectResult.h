/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
 **************************************************************************//**
 * @file InjectResult.h
 *   Declaration of result codes that arise during process injection.
 *****************************************************************************/

#pragma once

#include <string_view>


namespace Hookshot
{
    // -------- TYPE DEFINITIONS ------------------------------------------- //

    /// Enumeration of possible error conditions that arise when attempting to create and inject a process.
    enum class EInjectResult
    {
        // Success
        Success = 0,                                                ///< All operations succeeded.

        // Unknown failure
        Failure,                                                    ///< Unknown error.

        // Issues creating the process
        ErrorCreateProcess,                                         ///< Creation of the new process failed.
        ErrorDetermineMachineProcess,                               ///< Determination of the machine type of the new process failed.
        ErrorArchitectureMismatch,                                  ///< New process cannot be injected due to an architecture mismatch with the running Hookshot binary.

        // Issues with authorization.
        ErrorNotAuthorized,                                         ///< Hookshot is not authorized to act on the new process.
        ErrorCannotDetermineAuthorization,                          ///< Hookshot encountered an error while determining if it is authorized to act on the new process.

        // Issues determining the base address of the process' executable image
        ErrorLoadNtDll,                                             ///< Attempt to dynamically load `ntdll.dll` failed.
        ErrorNtQueryInformationProcessUnavailable,                  ///< Attempt to locate `NtQueryInformationProcess` within `ntdll.dll` failed.
        ErrorNtQueryInformationProcessFailed,                       ///< Call to `NtQueryInformationProcess` failed to retrieve the desired information.
        ErrorReadProcessPEBFailed,                                  ///< Virtual memory read of the process PEB failed.

        // Issues determining the entry point address of a process
        ErrorReadDOSHeadersFailed,                                  ///< Failed to read DOS headers from the process' executable image.
        ErrorReadNTHeadersFailed,                                   ///< Failed to read NT headers from the process' executable image.

        // Issues preparing to inject code or data into the target process
        ErrorVirtualAllocFailed,                                    ///< Failed to allocate virtual memory for code and data in the target process.
        ErrorVirtualProtectFailed,                                  ///< Failed to set protection values for code and data in the target process.

        // Issues actually injecting code or data into the target process
        ErrorCannotGenerateLibraryFilename,                         ///< Failed to compute the name of the Hookshot library to inject.
        ErrorCannotLoadInjectCode,                                  ///< Failed to load the file containing injection code.
        ErrorMalformedInjectCodeFile,                               ///< Failed to inject because the file containing inject code is malformed.
        ErrorInsufficientTrampolineSpace,                           ///< Failed to inject due to insufficient space available for storing the old trampoline code.
        ErrorInsufficientCodeSpace,                                 ///< Failed to inject due to insufficient allocated space for the code region.
        ErrorInsufficientDataSpace,                                 ///< Failed to inject due to insufficient allocated space for the data region.
        ErrorInternalInvalidParams,                                 ///< Failed to inject due to an internal issue resulting in invalid injection parameters.
        ErrorSetFailedRead,                                         ///< Failed to inject due to a failed attempt to retrieve existing code from the injected process.
        ErrorSetFailedWrite,                                        ///< Failed to inject due to a failed attempt to write new code into the injected process.
        ErrorRunFailedResumeThread,                                 ///< Failed to run injected code due to the main thread of the injected process not waking up.
        ErrorRunFailedSync,                                         ///< Failed to synchronize with injected code due to an issue reading from or writing to injected process memory.
        ErrorRunFailedSuspendThread,                                ///< Failed to place the injected process back into a suspended state after running the injected code.
        ErrorUnsetFailed,                                           ///< Failed to inject due to a failed attempt to return the trampoline region to its original content.

        // Issues spawning a new instance of Hookshot due to an architecture mismatch
        ErrorCannotGenerateExecutableFilename,                      ///< Failed to compute the name of the executable to spawn.
        ErrorInterProcessCommunicationFailed,                       ///< Failed to perform inter-process communication.
        ErrorCreateHookshotProcessFailed,                           ///< Failed to spawn a new Hookshot instance.

        // Issues encountered while running injection code to initialize the injected process
        ErrorCannotLocateRequiredFunctions,                         ///< Failed to locate required functions in the address space of the injected process.
        ErrorCannotWriteRequiredFunctionLocations,                  ///< Failed to write the locations of the required functions into the address space of the injected process.
        ErrorCannotReadStatus,                                      ///< Failed to read status information from the injected process.
        ErrorCannotLoadLibrary,                                     ///< Failed to load the Hookshot library in the injected process.
        ErrorMalformedLibrary,                                      ///< Loaded Hookshot library is malformed.
        ErrorLibraryInitFailed,                                     ///< Loaded Hookshot library failed to initialize.

        // Maximum value in this enumeration
        MaximumValue                                                ///< Sentinel value, not used as an error code.
    };


    // -------- FUNCTIONS -------------------------------------------------- //

    /// Retrieves a string that explains the injection result code.
    /// @param [in] injectResult Injection result code.
    /// @return Explanatory string.
    std::wstring_view InjectResultString(EInjectResult injectResult);
}
