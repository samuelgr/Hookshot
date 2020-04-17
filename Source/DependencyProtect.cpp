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
        extern const volatile decltype(&CloseHandle) ProtectedCloseHandle = CloseHandle;
        extern const volatile decltype(&CreateFileMapping) ProtectedCreateFileMapping = CreateFileMapping;
        extern const volatile decltype(&CreateProcess) ProtectedCreateProcess = CreateProcess;
        extern const volatile decltype(&DuplicateHandle) ProtectedDuplicateHandle = DuplicateHandle;
        extern const volatile decltype(&FindClose) ProtectedFindClose = FindClose;
        extern const volatile decltype(&FindFirstFileEx) ProtectedFindFirstFileEx = FindFirstFileEx;
        extern const volatile decltype(&FindNextFile) ProtectedFindNextFile = FindNextFile;
        extern const volatile decltype(&FlushInstructionCache) ProtectedFlushInstructionCache = FlushInstructionCache;
        extern const volatile decltype(&GetExitCodeProcess) ProtectedGetExitCodeProcess = GetExitCodeProcess;
        extern const volatile decltype(&GetLastError) ProtectedGetLastError = GetLastError;
        extern const volatile decltype(&GetModuleHandleEx) ProtectedGetModuleHandleEx = GetModuleHandleEx;
        extern const volatile decltype(&GetProcAddress) ProtectedGetProcAddress = GetProcAddress;
        extern const volatile decltype(&IsDebuggerPresent) ProtectedIsDebuggerPresent = IsDebuggerPresent;
        extern const volatile decltype(&LoadLibrary) ProtectedLoadLibrary = LoadLibrary;
        extern const volatile decltype(&MapViewOfFile) ProtectedMapViewOfFile = MapViewOfFile;
        extern const volatile decltype(&MessageBox) ProtectedMessageBox = MessageBox;
        extern const volatile decltype(&OutputDebugString) ProtectedOutputDebugString = OutputDebugString;
        extern const volatile decltype(&QueryFullProcessImageName) ProtectedQueryFullProcessImageName = QueryFullProcessImageName;
        extern const volatile decltype(&ResumeThread) ProtectedResumeThread = ResumeThread;
        extern const volatile decltype(&SetLastError) ProtectedSetLastError = SetLastError;
        extern const volatile decltype(&TerminateProcess) ProtectedTerminateProcess = TerminateProcess;
        extern const volatile decltype(&UnmapViewOfFile) ProtectedUnmapViewOfFile = UnmapViewOfFile;
        extern const volatile decltype(&VirtualAlloc) ProtectedVirtualAlloc = VirtualAlloc;
        extern const volatile decltype(&VirtualFree) ProtectedVirtualFree = VirtualFree;
        extern const volatile decltype(&VirtualQuery) ProtectedVirtualQuery = VirtualQuery;
        extern const volatile decltype(&VirtualProtect) ProtectedVirtualProtect = VirtualProtect;
        extern const volatile decltype(&WaitForSingleObject) ProtectedWaitForSingleObject = WaitForSingleObject;
    }


    // -------- INTERNAL VARIABLES ----------------------------------------- //

    static std::unordered_map<const void*, const void* volatile*> protectedDependencies = {
        {Windows::ProtectedCloseHandle, (const void* volatile*)&Windows::ProtectedCloseHandle},
        {Windows::ProtectedCreateFileMapping, (const void* volatile*)&Windows::ProtectedCreateFileMapping},
        {Windows::ProtectedCreateProcess, (const void* volatile*)&Windows::ProtectedCreateProcess},
        {Windows::ProtectedDuplicateHandle, (const void* volatile*)&Windows::ProtectedDuplicateHandle},
        {Windows::ProtectedFindClose, (const void* volatile*)&Windows::ProtectedFindClose},
        {Windows::ProtectedFindFirstFileEx, (const void* volatile*)&Windows::ProtectedFindFirstFileEx},
        {Windows::ProtectedFindNextFile, (const void* volatile*)&Windows::ProtectedFindNextFile},
        {Windows::ProtectedFlushInstructionCache, (const void* volatile*)&Windows::ProtectedFlushInstructionCache},
        {Windows::ProtectedGetExitCodeProcess, (const void* volatile*)&Windows::ProtectedGetExitCodeProcess},
        {Windows::ProtectedGetLastError, (const void* volatile*)&Windows::ProtectedGetLastError},
        {Windows::ProtectedGetModuleHandleEx, (const void* volatile*)&Windows::ProtectedGetModuleHandleEx},
        {Windows::ProtectedGetProcAddress, (const void* volatile*)&Windows::ProtectedGetProcAddress},
        {Windows::ProtectedIsDebuggerPresent, (const void* volatile*)&Windows::ProtectedIsDebuggerPresent},
        {Windows::ProtectedLoadLibrary, (const void* volatile*)&Windows::ProtectedLoadLibrary},
        {Windows::ProtectedMapViewOfFile, (const void* volatile*)&Windows::ProtectedMapViewOfFile},
        {Windows::ProtectedMessageBox, (const void* volatile*)&Windows::ProtectedMessageBox},
        {Windows::ProtectedOutputDebugString, (const void* volatile*)&Windows::ProtectedOutputDebugString},
        {Windows::ProtectedQueryFullProcessImageName, (const void* volatile*)&Windows::ProtectedQueryFullProcessImageName},
        {Windows::ProtectedResumeThread, (const void* volatile*)&Windows::ProtectedResumeThread},
        {Windows::ProtectedSetLastError, (const void* volatile*)&Windows::ProtectedSetLastError},
        {Windows::ProtectedTerminateProcess, (const void* volatile*)&Windows::ProtectedTerminateProcess},
        {Windows::ProtectedUnmapViewOfFile, (const void* volatile*)&Windows::ProtectedUnmapViewOfFile},
        {Windows::ProtectedVirtualAlloc, (const void* volatile*)&Windows::ProtectedVirtualAlloc},
        {Windows::ProtectedVirtualFree, (const void* volatile*)&Windows::ProtectedVirtualFree},
        {Windows::ProtectedVirtualQuery, (const void* volatile*)&Windows::ProtectedVirtualQuery},
        {Windows::ProtectedVirtualProtect, (const void* volatile*)&Windows::ProtectedVirtualProtect},
        {Windows::ProtectedWaitForSingleObject, (const void* volatile*)&Windows::ProtectedWaitForSingleObject},
    };


    // -------- FUNCTIONS -------------------------------------------------- //
    // See "DependencyProtect.h" for documentation.

    void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress)
    {
        if (0 != protectedDependencies.count(oldAddress))
        {
            const void* volatile* const pointerToUpdate = protectedDependencies.at(oldAddress);

            protectedDependencies.erase(oldAddress);
            protectedDependencies.insert({newAddress, pointerToUpdate});

            *pointerToUpdate = newAddress;
            _mm_mfence();
        }
    }
}
