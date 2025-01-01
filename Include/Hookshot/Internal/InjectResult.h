/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file InjectResult.h
 *   Declaration of result codes that arise during process injection.
 **************************************************************************************************/

#pragma once

#include <string_view>

namespace Hookshot
{
  /// Enumeration of possible error conditions that arise when attempting to create and inject a
  /// process.
  enum class EInjectResult
  {
    /// All operations succeeded.
    Success = 0,

    /// Unknown error.
    Failure,

    /// Creation of the new process failed.
    ErrorCreateProcess,

    /// Determination of the machine type of the new process failed.
    ErrorDetermineMachineProcess,

    /// New process cannot be injected due to an architecture mismatch with the running Hookshot
    /// binary.
    ErrorArchitectureMismatch,

    /// Hookshot is not authorized to act on the new process.
    ErrorNotAuthorized,

    /// Hookshot encountered an error while determining if it is authorized to act on the new
    /// process.
    ErrorCannotDetermineAuthorization,

    /// Failed to use debugger functions to advance the process' early initialization steps.
    ErrorAdvanceProcessFailed,

    /// Attempt to dynamically load `ntdll.dll` failed.
    ErrorLoadNtDll,

    /// Attempt to locate `NtQueryInformationProcess` within `ntdll.dll` failed.
    ErrorNtQueryInformationProcessUnavailable,

    /// Call to `NtQueryInformationProcess` failed to retrieve the desired information.
    ErrorNtQueryInformationProcessFailed,

    /// Virtual memory read of the process PEB failed.
    ErrorReadProcessPEBFailed,

    /// Failed to read DOS headers from the process' executable image.
    ErrorReadDOSHeadersFailed,

    /// Failed to read NT headers from the process' executable image.
    ErrorReadNTHeadersFailed,

    /// Failed to obtain a handle for `mscoree.dll` in the target process.
    ErrorGetModuleHandleClrLibraryFailed,

    /// Failed to locate the CLR entry point in the target process.
    ErrorGetProcAddressClrEntryPointFailed,

    /// Failed to allocate virtual memory for code and data in the target process.
    ErrorVirtualAllocFailed,

    /// Failed to set protection values for code and data in the target process.
    ErrorVirtualProtectFailed,

    /// Failed to compute the name of the Hookshot library to inject.
    ErrorCannotGenerateLibraryFilename,

    /// Failed to load the file containing injection code.
    ErrorCannotLoadInjectCode,

    /// Failed to inject because the file containing inject code is malformed.
    ErrorMalformedInjectCodeFile,

    /// Failed to inject due to insufficient space available for storing the old trampoline code.
    ErrorInsufficientTrampolineSpace,

    /// Failed to inject due to insufficient allocated space for the code region.
    ErrorInsufficientCodeSpace,

    /// Failed to inject due to insufficient allocated space for the data region.
    ErrorInsufficientDataSpace,

    /// Failed to inject due to an internal issue resulting in invalid injection parameters.
    ErrorInternalInvalidParams,

    /// Failed to inject due to a failed attempt to retrieve existing code from the injected
    /// process.
    ErrorSetFailedRead,

    /// Failed to inject due to a failed attempt to write new code into the injected process.
    ErrorSetFailedWrite,

    /// Failed to run injected code due to the main thread of the injected process not waking up.
    ErrorRunFailedResumeThread,

    /// Failed to synchronize with injected code due to an issue reading from or writing to injected
    /// process memory.
    ErrorRunFailedSync,

    /// Failed to place the injected process back into a suspended state after running the injected
    /// code.
    ErrorRunFailedSuspendThread,

    /// Failed to inject due to a failed attempt to return the trampoline region to its original
    /// content.
    ErrorUnsetFailed,

    /// Failed to compute the name of the executable to spawn.
    ErrorCannotGenerateExecutableFilename,

    /// Failed to perform inter-process communication.
    ErrorInterProcessCommunicationFailed,

    /// Failed to spawn a new Hookshot instance.
    ErrorCreateHookshotProcessFailed,

    /// Failed to spawn a new Hookshot instance of the other architecture (for example, spawning a
    /// 32-bit instance from a 64-bit process).
    ErrorCreateHookshotOtherArchitectureProcessFailed,

    /// Failed to locate required functions in the address space of the injected process.
    ErrorCannotLocateRequiredFunctions,

    /// Failed to write the locations of the required functions into the address space of the
    /// injected process.
    ErrorCannotWriteRequiredFunctionLocations,

    /// Failed to read status information from the injected process.
    ErrorCannotReadStatus,

    /// Failed to load the Hookshot library in the injected process.
    ErrorCannotLoadLibrary,

    /// Failed to load the Hookshot library in the injected process of the other architecture (for
    /// example, spawning a 32-bit instance from a 64-bit process).
    ErrorCannotLoadLibraryOtherArchitecture,

    /// Loaded Hookshot library is malformed.
    ErrorMalformedLibrary,

    /// Loaded Hookshot library failed to initialize.
    ErrorLibraryInitFailed,

    /// Sentinel value, not used as an error code.
    MaximumValue
  };

  /// Retrieves a string that explains the injection result code.
  /// @param [in] injectResult Injection result code.
  /// @return Explanatory string.
  std::wstring_view InjectResultString(EInjectResult injectResult);
} // namespace Hookshot
