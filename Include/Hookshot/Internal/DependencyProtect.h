/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file DependencyProtect.h
 *   Declaration of dependency protection functionality.  This is intended to
 *   ensure that certain parts of Hookshot continue to function correctly even
 *   when they are hooked by Hookshot users so that users do not need to worry
 *   about how Hookshot is implemented (for the most part) when setting hooks.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"


namespace Hookshot
{
    // -------- CONSTANTS -------------------------------------------------- //
    // Pointers to protected versions of various API functions.
    // Namespaced by API.

    namespace Windows
    {
        extern const decltype(&CloseHandle) ProtectedCloseHandle;
        extern const decltype(&CreateFileMapping) ProtectedCreateFileMapping;
        extern const decltype(&CreateProcess) ProtectedCreateProcess;
        extern const decltype(&DuplicateHandle) ProtectedDuplicateHandle;
        extern const decltype(&FlushInstructionCache) ProtectedFlushInstructionCache;
        extern const decltype(&GetExitCodeProcess) ProtectedGetExitCodeProcess;
        extern const decltype(&GetLastError) ProtectedGetLastError;
        extern const decltype(&GetModuleHandleEx) ProtectedGetModuleHandleEx;
        extern const decltype(&GetProcAddress) ProtectedGetProcAddress;
        extern const decltype(&IsDebuggerPresent) ProtectedIsDebuggerPresent;
        extern const decltype(&LoadLibrary) ProtectedLoadLibrary;
        extern const decltype(&MessageBox) ProtectedMessageBox;
        extern const decltype(&MapViewOfFile) ProtectedMapViewOfFile;
        extern const decltype(&OutputDebugString) ProtectedOutputDebugString;
        extern const decltype(&ResumeThread) ProtectedResumeThread;
        extern const decltype(&SetLastError) ProtectedSetLastError;
        extern const decltype(&TerminateProcess) ProtectedTerminateProcess;
        extern const decltype(&UnmapViewOfFile) ProtectedUnmapViewOfFile;
        extern const decltype(&VirtualAlloc) ProtectedVirtualAlloc;
        extern const decltype(&VirtualFree) ProtectedVirtualFree;
        extern const decltype(&VirtualQuery) ProtectedVirtualQuery;
        extern const decltype(&VirtualProtect) ProtectedVirtualProtect;
        extern const decltype(&WaitForSingleObject) ProtectedWaitForSingleObject;
    }


    // -------- FUNCTIONS -------------------------------------------------- //

    /// Updates the address of a protected dependency.
    /// Behind the scenes, this modifies one of the above seemingly-constant function pointers to point somewhere else.
    /// Has no effect if the specified old address is not actually one of the protected dependencies.
    /// @param [in] dependency Address of one of the above function pointers whose value needs to be updated.
    /// @param [in] newAddress New address to which the function pointer should point.
    void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress);
}
