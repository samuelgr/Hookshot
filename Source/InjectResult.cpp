/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2022
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
        case EInjectResult::Success:
            return L"Success";
        case EInjectResult::Failure:
            return L"Unknown error";
        case EInjectResult::ErrorCreateProcess:
            return L"Error creating a new process";
        case EInjectResult::ErrorDetermineMachineProcess:
            return L"Error determining the new process architecture";
        case EInjectResult::ErrorArchitectureMismatch:
            return L"New process architecture mismatch";
        case EInjectResult::ErrorNotAuthorized:
            return L"Not authorized to inject the new process";
        case EInjectResult::ErrorCannotDetermineAuthorization:
            return L"Error while checking for authorization";
        case EInjectResult::ErrorLoadNtDll:
            return L"Error loading ntdll.dll";
        case EInjectResult::ErrorNtQueryInformationProcessUnavailable:
            return L"Error locating NtQueryInformationProcess in ntdll.dll";
        case EInjectResult::ErrorNtQueryInformationProcessFailed:
            return L"NtQueryInformationProcess call failed";
        case EInjectResult::ErrorReadProcessPEBFailed:
            return L"Error reading new process PEB";
        case EInjectResult::ErrorReadDOSHeadersFailed:
            return L"Error reading new process DOS headers";
        case EInjectResult::ErrorReadNTHeadersFailed:
            return L"Error reading new process NT headers";
        case EInjectResult::ErrorVirtualAllocFailed:
            return L"Error allocating virtual memory in the new process";
        case EInjectResult::ErrorVirtualProtectFailed:
            return L"Error protecting virtual memory in the new process";
        case EInjectResult::ErrorCannotGenerateLibraryFilename:
            return L"Error generating library filename";
        case EInjectResult::ErrorCannotLoadInjectCode:
            return L"Error loading the injection payload";
        case EInjectResult::ErrorMalformedInjectCodeFile:
            return L"Malformed injection payload";
        case EInjectResult::ErrorInsufficientTrampolineSpace:
            return L"Insufficient injection payload trampoline space";
        case EInjectResult::ErrorInsufficientCodeSpace:
            return L"Insufficient injection payload code space";
        case EInjectResult::ErrorInsufficientDataSpace:
            return L"Insufficient injection payload data space";
        case EInjectResult::ErrorInternalInvalidParams:
            return L"Internal error during injection";
        case EInjectResult::ErrorSetFailedRead:
            return L"Error reading memory during injection payload transfer";
        case EInjectResult::ErrorSetFailedWrite:
            return L"Error writing memory during injection payload transfer";
        case EInjectResult::ErrorRunFailedResumeThread:
            return L"Error resuming the main thread in the new process";
        case EInjectResult::ErrorRunFailedSync:
            return L"Error synchronizing with the injection payload code";
        case EInjectResult::ErrorRunFailedSuspendThread:
            return L"Error suspending the main thread in the new process";
        case EInjectResult::ErrorUnsetFailed:
            return L"Error restoring the new process entry point original contents";
        case EInjectResult::ErrorCannotGenerateExecutableFilename:
            return L"Error generating executable filename";
        case EInjectResult::ErrorInterProcessCommunicationFailed:
            return L"Error communicating with the executable";
        case EInjectResult::ErrorCreateHookshotProcessFailed:
            return L"Error creating a new Hookshot executable process";
        case EInjectResult::ErrorCannotLocateRequiredFunctions:
            return L"Error locating required API functions in the new process";
        case EInjectResult::ErrorCannotWriteRequiredFunctionLocations:
            return L"Error writing required API function locations";
        case EInjectResult::ErrorCannotReadStatus:
            return L"Error reading status information from the injection payload";
        case EInjectResult::ErrorCannotLoadLibrary:
            return L"Error loading library from within the new process";
        case EInjectResult::ErrorMalformedLibrary:
            return L"Library loaded from within the new process is malformed";
        case EInjectResult::ErrorLibraryInitFailed:
            return L"Error initializing library from within the new process";
        default:
            return L"Unknown result";
        }
    }
}
