/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file ProcessInjector.cpp
 *   Implementation of all process creation and injection functionality.
 *****************************************************************************/

#include "ApiWindows.h"
#include "CodeInjector.h"
#include "Globals.h"
#include "Inject.h"
#include "InjectResult.h"
#include "Message.h"
#include "ProcessInjector.h"
#include "RemoteProcessInjector.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <psapi.h>
#include <winnt.h>
#include <winternl.h>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "ProcessInjector.h" for documentation.

    HMODULE ProcessInjector::ntdllModuleHandle = NULL;

    NTSTATUS(WINAPI* ProcessInjector::ntdllQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG) = NULL;

    size_t ProcessInjector::systemAllocationGranularity = 0;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "ProcessInjector.h" for documentation.

    EInjectResult ProcessInjector::CreateInjectedProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
    {
        // This method creates processes in suspended state as part of injection functionality.
        // It will allow the new process to run, unless the caller requested a suspended process.
        const bool shouldCreateSuspended = (0 != (dwCreationFlags & CREATE_SUSPENDED)) ? true : false;

        // Attempt to create the new process in suspended state and capture information about it.
        PROCESS_INFORMATION processInfo;
        if (0 == CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, (dwCreationFlags | CREATE_SUSPENDED), lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInfo))
            return EInjectResult::InjectResultErrorCreateProcess;

        *lpProcessInformation = processInfo;

        // Attempt to inject the newly-created process and handle the result.
        return HandleInjectionResult(InjectProcess(processInfo.hProcess, processInfo.hThread, (IsDebuggerPresent() ? true : false)), shouldCreateSuspended, processInfo.hProcess, processInfo.hThread);
    }

    // --------

    EInjectResult ProcessInjector::InjectProcess(const HANDLE processHandle, const HANDLE threadHandle, const bool enableDebugFeatures)
    {
        // First make sure the architectures match between this process and the process being injected.
        // If not, spawn a version of Hookshot that does match and request that it inject the process on behalf of this process.
        EInjectResult operationResult = VerifyMatchingProcessArchitecture(processHandle);

        switch (operationResult)
        {
        case EInjectResult::InjectResultSuccess:
            break;

        case EInjectResult::InjectResultErrorArchitectureMismatch:
            return RemoteProcessInjector::RemoteInjectProcess(processHandle, threadHandle, true, enableDebugFeatures);

        default:
            return operationResult;
        }

        // Architecture matches, so it is safe to proceed.
        const size_t allocationGranularity = GetSystemAllocationGranularity();
        const size_t kEffectiveInjectRegionSize = (InjectInfo::kMaxInjectBinaryFileSize < allocationGranularity) ? allocationGranularity : InjectInfo::kMaxInjectBinaryFileSize;
        void* processBaseAddress = NULL;
        void* processEntryPoint = NULL;
        void* injectedCodeBase = NULL;
        void* injectedDataBase = NULL;

        // Attempt to obtain the base address of the executable image of the new process.
        operationResult = GetProcessImageBaseAddress(processHandle, &processBaseAddress);
        if (EInjectResult::InjectResultSuccess != operationResult)
            return operationResult;

        // Attempt to obtain the entry point address of the new process.
        operationResult = GetProcessEntryPointAddress(processHandle, processBaseAddress, &processEntryPoint);
        if (EInjectResult::InjectResultSuccess != operationResult)
            return operationResult;

        // Allocate code and data areas in the target process.
        // Code first, then data.
        injectedCodeBase = VirtualAllocEx(processHandle, NULL, ((SIZE_T)kEffectiveInjectRegionSize * (SIZE_T)2), MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
        injectedDataBase = (void*)((size_t)injectedCodeBase + kEffectiveInjectRegionSize);

        if (NULL == injectedCodeBase)
            return EInjectResult::InjectResultErrorVirtualAllocFailed;

        // Set appropriate protection values onto the new areas individually.
        {
            DWORD unusedOldProtect = 0;

            if (FALSE == VirtualProtectEx(processHandle, injectedCodeBase, kEffectiveInjectRegionSize, PAGE_EXECUTE_READ, &unusedOldProtect))
                return EInjectResult::InjectResultErrorVirtualProtectFailed;

            if (FALSE == VirtualProtectEx(processHandle, injectedDataBase, kEffectiveInjectRegionSize, PAGE_READWRITE, &unusedOldProtect))
                return EInjectResult::InjectResultErrorVirtualProtectFailed;
        }

        // Inject code and data.
        // Only mark the code buffer as requiring cleanup because both code and data buffers are from the same single allocation.
        CodeInjector injector(injectedCodeBase, injectedDataBase, true, false, processEntryPoint, kEffectiveInjectRegionSize, kEffectiveInjectRegionSize, processHandle, threadHandle);
        operationResult = injector.SetAndRun(enableDebugFeatures);

        return operationResult;
    }

    // --------

    bool ProcessInjector::PerformRequestedRemoteInjection(SRemoteProcessInjectionData* const remoteInjectionData)
    {
        EInjectResult operationResult = ProcessInjector::InjectProcess((HANDLE)remoteInjectionData->processHandle, (HANDLE)remoteInjectionData->threadHandle, remoteInjectionData->enableDebugFeatures);

        remoteInjectionData->injectionResult = (uint64_t)operationResult;
        remoteInjectionData->extendedInjectionResult = (uint64_t)GetLastError();

        CloseHandle((HANDLE)remoteInjectionData->processHandle);
        CloseHandle((HANDLE)remoteInjectionData->threadHandle);

        return true;
    }


    // -------- HELPERS ---------------------------------------------------- //
    // See "ProcessInjector.h" for documentation.

    EInjectResult ProcessInjector::GetProcessEntryPointAddress(const HANDLE processHandle, const void* const baseAddress, void** const entryPoint)
    {
        size_t numBytesRead = 0;

        // First read the DOS header to figure out the location of the NT header.
        decltype(IMAGE_DOS_HEADER::e_lfanew) ntHeadersOffset = 0;

        if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)((size_t)baseAddress + (size_t)offsetof(IMAGE_DOS_HEADER, e_lfanew)), (LPVOID)&ntHeadersOffset, sizeof(ntHeadersOffset), (SIZE_T*)&numBytesRead)) || (sizeof(ntHeadersOffset) != numBytesRead))
            return EInjectResult::InjectResultErrorReadDOSHeadersFailed;

        // Next read the NT header to figure out the location of the entry point.
        decltype(IMAGE_NT_HEADERS::OptionalHeader.AddressOfEntryPoint) entryPointOffset = 0;

        if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)((size_t)baseAddress + (size_t)ntHeadersOffset + (size_t)offsetof(IMAGE_NT_HEADERS, OptionalHeader.AddressOfEntryPoint)), (LPVOID)&entryPointOffset, sizeof(entryPointOffset), (SIZE_T*)&numBytesRead)) || (sizeof(entryPointOffset) != numBytesRead))
            return EInjectResult::InjectResultErrorReadNTHeadersFailed;

        // Compute the absolute entry point address.
        *entryPoint = (void*)((size_t)baseAddress + (size_t)entryPointOffset);

        // Success.
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult ProcessInjector::GetProcessImageBaseAddress(const HANDLE processHandle, void** const baseAddress)
    {
        // Verify that "ntdll.dll" is loaded.
        if (NULL == ntdllModuleHandle)
        {
            ntdllModuleHandle = LoadLibraryEx(L"ntdll.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

            if (NULL == ntdllModuleHandle)
                return EInjectResult::InjectResultErrorLoadNtDll;

            ntdllQueryInformationProcessProc = NULL;
        }

        // Verify that "NtQueryInformationProcess" is available.
        if (NULL == ntdllQueryInformationProcessProc)
        {
            ntdllQueryInformationProcessProc = (decltype(ntdllQueryInformationProcessProc))GetProcAddress(ntdllModuleHandle, "NtQueryInformationProcess");

            if (NULL == ntdllQueryInformationProcessProc)
                return EInjectResult::InjectResultErrorNtQueryInformationProcessUnavailable;
        }

        // Obtain the address of the process environment block (PEB) for the process, which is within the address space of the process.
        PROCESS_BASIC_INFORMATION processBasicInfo;

        if (0 != ntdllQueryInformationProcessProc(processHandle, ProcessBasicInformation, &processBasicInfo, sizeof(processBasicInfo), NULL))
            return EInjectResult::InjectResultErrorNtQueryInformationProcessFailed;

        // Read the desired information from the PEB in the process' address space.
        size_t numBytesRead = 0;

        if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)((size_t)processBasicInfo.PebBaseAddress + (size_t)offsetof(PEB, Reserved3[1])), (LPVOID)baseAddress, sizeof(baseAddress), (SIZE_T*)&numBytesRead)) || (sizeof(baseAddress) != numBytesRead))
            return EInjectResult::InjectResultErrorReadProcessPEBFailed;

        return EInjectResult::InjectResultSuccess;
    }

    // --------

    size_t ProcessInjector::GetSystemAllocationGranularity(void)
    {
        if (0 == systemAllocationGranularity)
        {
            SYSTEM_INFO systemInfo;
            GetSystemInfo(&systemInfo);

            systemAllocationGranularity = (size_t)systemInfo.dwPageSize;
        }

        return systemAllocationGranularity;
    }

    // --------

    EInjectResult ProcessInjector::HandleInjectionResult(const EInjectResult result, const bool shouldKeepSuspended, const HANDLE processHandle, const HANDLE threadHandle)
    {
        if (EInjectResult::InjectResultSuccess != result)
        {
            // If injection failed for a reason other than CreateProcess failing, kill the new process because there is no guarantee it will run correctly.
            if (EInjectResult::InjectResultErrorCreateProcess != result)
            {
                const DWORD systemErrorCode = GetLastError();
                TerminateProcess(processHandle, UINT_MAX);
                SetLastError(systemErrorCode);
            }
        }
        else
        {
            // If injection succeeded and the process was not supposed to be kept suspended, resume it.
            if (false == shouldKeepSuspended)
                ResumeThread(threadHandle);
        }

        return result;
    }

    // --------
    
    EInjectResult ProcessInjector::VerifyMatchingProcessArchitecture(const HANDLE processHandle)
    {
        USHORT machineTargetProcess = 0;
        USHORT machineCurrentProcess = 0;

        if ((FALSE == IsWow64Process2(processHandle, &machineTargetProcess, NULL)) || (FALSE == IsWow64Process2(Globals::GetCurrentProcessHandle(), &machineCurrentProcess, NULL)))
            return EInjectResult::InjectResultErrorDetermineMachineProcess;

        if (machineTargetProcess == machineCurrentProcess)
            return EInjectResult::InjectResultSuccess;
        else
            return EInjectResult::InjectResultErrorArchitectureMismatch;
    }
}
