/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file DependencyProtect.cpp
 *   Implementation of dependency protection functionality.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"

#include <cstdint>
#include <intrin.h>
#include <unordered_map>


namespace Hookshot
{
    // -------- GLOBALS ---------------------------------------------------- //
    // See "DependencyProtect.h" for documentation.
    // Despite being declared const, these are updated behind-the-scenes.

    namespace Windows
    {
        extern const decltype(&CloseHandle) ProtectedCloseHandle = CloseHandle;
        extern const decltype(&CreateFileMapping) ProtectedCreateFileMapping = CreateFileMapping;
        extern const decltype(&CreateProcess) ProtectedCreateProcess = CreateProcess;
        extern const decltype(&DuplicateHandle) ProtectedDuplicateHandle = DuplicateHandle;
        extern const decltype(&FlushInstructionCache) ProtectedFlushInstructionCache = FlushInstructionCache;
        extern const decltype(&GetExitCodeProcess) ProtectedGetExitCodeProcess = GetExitCodeProcess;
        extern const decltype(&GetLastError) ProtectedGetLastError = GetLastError;
        extern const decltype(&GetModuleHandleEx) ProtectedGetModuleHandleEx = GetModuleHandleEx;
        extern const decltype(&GetProcAddress) ProtectedGetProcAddress = GetProcAddress;
        extern const decltype(&IsDebuggerPresent) ProtectedIsDebuggerPresent = IsDebuggerPresent;
        extern const decltype(&LoadLibrary) ProtectedLoadLibrary = LoadLibrary;
        extern const decltype(&MapViewOfFile) ProtectedMapViewOfFile = MapViewOfFile;
        extern const decltype(&MessageBox) ProtectedMessageBox = MessageBox;
        extern const decltype(&OutputDebugString) ProtectedOutputDebugString = OutputDebugString;
        extern const decltype(&ResumeThread) ProtectedResumeThread = ResumeThread;
        extern const decltype(&SetLastError) ProtectedSetLastError = SetLastError;
        extern const decltype(&TerminateProcess) ProtectedTerminateProcess = TerminateProcess;
        extern const decltype(&UnmapViewOfFile) ProtectedUnmapViewOfFile = UnmapViewOfFile;
        extern const decltype(&VirtualAlloc) ProtectedVirtualAlloc = VirtualAlloc;
        extern const decltype(&VirtualFree) ProtectedVirtualFree = VirtualFree;
        extern const decltype(&VirtualQuery) ProtectedVirtualQuery = VirtualQuery;
        extern const decltype(&VirtualProtect) ProtectedVirtualProtect = VirtualProtect;
        extern const decltype(&WaitForSingleObject) ProtectedWaitForSingleObject = WaitForSingleObject;
    }


    // -------- INTERNAL VARIABLES ----------------------------------------- //

    static std::unordered_map<const void*, const void**> protectedDependencies = {
        {(const void*)Windows::ProtectedCloseHandle, (const void**)&Windows::ProtectedCloseHandle},
        {(const void*)Windows::ProtectedCreateFileMapping, (const void**)&Windows::ProtectedCreateFileMapping},
        {(const void*)Windows::ProtectedDuplicateHandle, (const void**)&Windows::ProtectedDuplicateHandle},
        {(const void*)Windows::ProtectedFlushInstructionCache, (const void**)&Windows::ProtectedFlushInstructionCache},
        {(const void*)Windows::ProtectedGetExitCodeProcess, (const void**)&Windows::ProtectedGetExitCodeProcess},
        {(const void*)Windows::ProtectedGetLastError, (const void**)&Windows::ProtectedGetLastError},
        {(const void*)Windows::ProtectedGetModuleHandleEx, (const void**)&Windows::ProtectedGetModuleHandleEx},
        {(const void*)Windows::ProtectedGetProcAddress, (const void**)&Windows::ProtectedGetProcAddress},
        {(const void*)Windows::ProtectedIsDebuggerPresent, (const void**)&Windows::ProtectedIsDebuggerPresent},
        {(const void*)Windows::ProtectedLoadLibrary, (const void**)&Windows::ProtectedLoadLibrary},
        {(const void*)Windows::ProtectedMapViewOfFile, (const void**)&Windows::ProtectedMapViewOfFile},
        {(const void*)Windows::ProtectedMessageBox, (const void**)&Windows::ProtectedMessageBox},
        {(const void*)Windows::ProtectedOutputDebugString, (const void**)&Windows::ProtectedOutputDebugString},
        {(const void*)Windows::ProtectedResumeThread, (const void**)&Windows::ProtectedResumeThread},
        {(const void*)Windows::ProtectedSetLastError, (const void**)&Windows::ProtectedSetLastError},
        {(const void*)Windows::ProtectedTerminateProcess, (const void**)&Windows::ProtectedTerminateProcess},
        {(const void*)Windows::ProtectedUnmapViewOfFile, (const void**)&Windows::ProtectedUnmapViewOfFile},
        {(const void*)Windows::ProtectedVirtualAlloc, (const void**)&Windows::ProtectedVirtualAlloc},
        {(const void*)Windows::ProtectedVirtualFree, (const void**)&Windows::ProtectedVirtualFree},
        {(const void*)Windows::ProtectedVirtualQuery, (const void**)&Windows::ProtectedVirtualQuery},
        {(const void*)Windows::ProtectedVirtualProtect, (const void**)&Windows::ProtectedVirtualProtect},
        {(const void*)Windows::ProtectedWaitForSingleObject, (const void**)&Windows::ProtectedWaitForSingleObject},
    };


    // -------- FUNCTIONS -------------------------------------------------- //
    // See "DependencyProtect.h" for documentation.

    void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress)
    {
        if (0 != protectedDependencies.count(oldAddress))
        {
            const void** const pointerToUpdate = protectedDependencies.at(oldAddress);

            protectedDependencies.erase(oldAddress);
            protectedDependencies.insert({newAddress, pointerToUpdate});

            *pointerToUpdate = newAddress;
            _mm_mfence();
        }
    }
}
