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
        extern const volatile decltype(&CloseHandle) ProtectedCloseHandle;
        extern const volatile decltype(&CreateFileMapping) ProtectedCreateFileMapping;
        extern const volatile decltype(&CreateProcess) ProtectedCreateProcess;
        extern const volatile decltype(&DuplicateHandle) ProtectedDuplicateHandle;
        extern const volatile decltype(&FindClose) ProtectedFindClose;
        extern const volatile decltype(&FindFirstFileEx) ProtectedFindFirstFileEx;
        extern const volatile decltype(&FindNextFile) ProtectedFindNextFile;
        extern const volatile decltype(&FlushInstructionCache) ProtectedFlushInstructionCache;
        extern const volatile decltype(&GetExitCodeProcess) ProtectedGetExitCodeProcess;
        extern const volatile decltype(&GetLastError) ProtectedGetLastError;
        extern const volatile decltype(&GetModuleHandleEx) ProtectedGetModuleHandleEx;
        extern const volatile decltype(&GetProcAddress) ProtectedGetProcAddress;
        extern const volatile decltype(&IsDebuggerPresent) ProtectedIsDebuggerPresent;
        extern const volatile decltype(&LoadLibrary) ProtectedLoadLibrary;
        extern const volatile decltype(&MessageBox) ProtectedMessageBox;
        extern const volatile decltype(&MapViewOfFile) ProtectedMapViewOfFile;
        extern const volatile decltype(&OutputDebugString) ProtectedOutputDebugString;
        extern const volatile decltype(&QueryFullProcessImageName) ProtectedQueryFullProcessImageName;
        extern const volatile decltype(&ResumeThread) ProtectedResumeThread;
        extern const volatile decltype(&SetLastError) ProtectedSetLastError;
        extern const volatile decltype(&TerminateProcess) ProtectedTerminateProcess;
        extern const volatile decltype(&UnmapViewOfFile) ProtectedUnmapViewOfFile;
        extern const volatile decltype(&VirtualAlloc) ProtectedVirtualAlloc;
        extern const volatile decltype(&VirtualFree) ProtectedVirtualFree;
        extern const volatile decltype(&VirtualQuery) ProtectedVirtualQuery;
        extern const volatile decltype(&VirtualProtect) ProtectedVirtualProtect;
        extern const volatile decltype(&WaitForSingleObject) ProtectedWaitForSingleObject;
    }


    // -------- FUNCTIONS -------------------------------------------------- //

    /// Updates the address of a protected dependency.
    /// Behind the scenes, this modifies one of the above seemingly-constant function pointers to point somewhere else.
    /// Has no effect if the specified old address is not actually one of the protected dependencies.
    /// @param [in] dependency Address of one of the above function pointers whose value needs to be updated.
    /// @param [in] newAddress New address to which the function pointer should point.
    void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress);
}
