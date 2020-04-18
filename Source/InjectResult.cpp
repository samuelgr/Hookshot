/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file InjectResult.cpp
 *   String definitions for each possible process injection result code.
 *****************************************************************************/

#pragma once

#include "InjectResult.h"

#include <string_view>


namespace Hookshot
{
    // -------- FUNCTIONS -------------------------------------------------- //
    // See "InjectResult.h" for documentation.

    std::wstring_view InjectResultString(EInjectResult injectResult)
    {
        switch (injectResult)
        {
        case EInjectResult::InjectResultSuccess:
            return L"Success";
        case EInjectResult::InjectResultFailure:
            return L"Unknown error";
        case EInjectResult::InjectResultErrorCreateProcess:
            return L"Error creating a new process";
        case EInjectResult::InjectResultErrorDetermineMachineProcess:
            return L"Error determining the new process architecture";
        case EInjectResult::InjectResultErrorArchitectureMismatch:
            return L"New process architecture mismatch";
        case EInjectResult::InjectResultErrorNotAuthorized:
            return L"Not authorized to inject the new process";
        case EInjectResult::InjectResultErrorCannotDetermineAuthorization:
            return L"Error while checking for authorization";
        case EInjectResult::InjectResultErrorLoadNtDll:
            return L"Error loading ntdll.dll";
        case EInjectResult::InjectResultErrorNtQueryInformationProcessUnavailable:
            return L"Error locating NtQueryInformationProcess in ntdll.dll";
        case EInjectResult::InjectResultErrorNtQueryInformationProcessFailed:
            return L"NtQueryInformationProcess call failed";
        case EInjectResult::InjectResultErrorReadProcessPEBFailed:
            return L"Error reading new process PEB";
        case EInjectResult::InjectResultErrorReadDOSHeadersFailed:
            return L"Error reading new process DOS headers";
        case EInjectResult::InjectResultErrorReadNTHeadersFailed:
            return L"Error reading new process NT headers";
        case EInjectResult::InjectResultErrorVirtualAllocFailed:
            return L"Error allocating virtual memory in the new process";
        case EInjectResult::InjectResultErrorVirtualProtectFailed:
            return L"Error protecting virtual memory in the new process";
        case EInjectResult::InjectResultErrorCannotGenerateLibraryFilename:
            return L"Error generating library filename";
        case EInjectResult::InjectResultErrorCannotLoadInjectCode:
            return L"Error loading the injection payload";
        case EInjectResult::InjectResultErrorMalformedInjectCodeFile:
            return L"Malformed injection payload";
        case EInjectResult::InjectResultErrorInsufficientTrampolineSpace:
            return L"Insufficient injection payload trampoline space";
        case EInjectResult::InjectResultErrorInsufficientCodeSpace:
            return L"Insufficient injection payload code space";
        case EInjectResult::InjectResultErrorInsufficientDataSpace:
            return L"Insufficient injection payload data space";
        case EInjectResult::InjectResultErrorInternalInvalidParams:
            return L"Internal error during injection";
        case EInjectResult::InjectResultErrorSetFailedRead:
            return L"Error reading memory during injection payload transfer";
        case EInjectResult::InjectResultErrorSetFailedWrite:
            return L"Error writing memory during injection payload transfer";
        case EInjectResult::InjectResultErrorRunFailedResumeThread:
            return L"Error resuming the main thread in the new process";
        case EInjectResult::InjectResultErrorRunFailedSync:
            return L"Error synchronizing with the injection payload code";
        case EInjectResult::InjectResultErrorRunFailedSuspendThread:
            return L"Error suspending the main thread in the new process";
        case EInjectResult::InjectResultErrorUnsetFailed:
            return L"Error restoring the new process entry point original contents";
        case EInjectResult::InjectResultErrorCannotGenerateExecutableFilename:
            return L"Error generating executable filename";
        case EInjectResult::InjectResultErrorInterProcessCommunicationFailed:
            return L"Error communicating with the executable";
        case EInjectResult::InjectResultErrorCreateHookshotProcessFailed:
            return L"Error creating a new process running the executable";
        case EInjectResult::InjectResultErrorCannotLocateRequiredFunctions:
            return L"Error locating required API functions in the new process";
        case EInjectResult::InjectResultErrorCannotWriteRequiredFunctionLocations:
            return L"Error writing required API function locations";
        case EInjectResult::InjectResultErrorCannotReadStatus:
            return L"Error reading status information from the injection payload";
        case EInjectResult::InjectResultErrorCannotLoadLibrary:
            return L"Error loading library from within the new process";
        case EInjectResult::InjectResultErrorMalformedLibrary:
            return L"Library loaded from within the new process is malformed";
        case EInjectResult::InjectResultErrorLibraryInitFailed:
            return L"Error initializing library from within the new process";
        default:
            return L"Unknown result";
        }
    }
}
