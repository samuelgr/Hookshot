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

    HMODULE ProcessInjector::ntdllModuleHandle = nullptr;

    NTSTATUS(WINAPI* ProcessInjector::ntdllQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG) = nullptr;

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

        const EInjectResult result = InjectProcess(processInfo.hProcess, processInfo.hThread, (IsDebuggerPresent() ? true : false));

        if (EInjectResult::InjectResultSuccess == result)
        {
            if (false == shouldCreateSuspended)
                ResumeThread(processInfo.hThread);
        }
        else
        {
            const DWORD systemErrorCode = GetLastError();
            TerminateProcess(processInfo.hProcess, UINT_MAX);
            SetLastError(systemErrorCode);
        }

        return result;
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

        *entryPoint = (void*)((size_t)baseAddress + (size_t)entryPointOffset);
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult ProcessInjector::GetProcessImageBaseAddress(const HANDLE processHandle, void** const baseAddress)
    {
        // This method uses the documented, but internal, Windows API function NtQueryInformationProcess and accesses an undocumented member of the PEB structure.
        // See https://docs.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess and https://docs.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb for official documentation.
        // While unlikely, if NtQueryInformationProcess or PEB are modified or made unavailable in a future version of Windows, this method might need to branch based on Windows version.

        if (nullptr == ntdllModuleHandle)
        {
            ntdllModuleHandle = LoadLibraryEx(L"ntdll.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

            if (nullptr == ntdllModuleHandle)
                return EInjectResult::InjectResultErrorLoadNtDll;

            ntdllQueryInformationProcessProc = nullptr;
        }

        if (nullptr == ntdllQueryInformationProcessProc)
        {
            ntdllQueryInformationProcessProc = (decltype(ntdllQueryInformationProcessProc))GetProcAddress(ntdllModuleHandle, "NtQueryInformationProcess");

            if (nullptr == ntdllQueryInformationProcessProc)
                return EInjectResult::InjectResultErrorNtQueryInformationProcessUnavailable;
        }

        // Obtain the address of the process environment block (PEB) for the process, which is within the address space of the process.
        PROCESS_BASIC_INFORMATION processBasicInfo;

        if (0 != ntdllQueryInformationProcessProc(processHandle, ProcessBasicInformation, &processBasicInfo, sizeof(processBasicInfo), nullptr))
            return EInjectResult::InjectResultErrorNtQueryInformationProcessFailed;

        // The field of interest in the PEB structure is ImageBaseAddress, whose offset is undocumented but has remained stable over multiple Windows generations and continues to do so.
        // See http://terminus.rewolf.pl/terminus/structures/ntdll/_PEB_combined.html for a visualization.
        // If the offset is modified in a future version of Windows, this constant might need to have a different value for different Windows versions.
#ifdef HOOKSHOT64
        constexpr size_t kOffsetPebImageBaseAddress = 16;
#else
        constexpr size_t kOffsetPebImageBaseAddress = 8;
#endif
        
        // Read the desired information from the PEB in the process' address space.
        size_t numBytesRead = 0;

        if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)((size_t)processBasicInfo.PebBaseAddress + kOffsetPebImageBaseAddress), (LPVOID)baseAddress, sizeof(baseAddress), (SIZE_T*)&numBytesRead)) || (sizeof(baseAddress) != numBytesRead))
            return EInjectResult::InjectResultErrorReadProcessPEBFailed;

        return EInjectResult::InjectResultSuccess;
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
        const size_t allocationGranularity = Globals::GetSystemInformation().dwPageSize;
        const size_t kEffectiveInjectRegionSize = (InjectInfo::kMaxInjectBinaryFileSize < allocationGranularity) ? allocationGranularity : InjectInfo::kMaxInjectBinaryFileSize;
        void* processBaseAddress = nullptr;
        void* processEntryPoint = nullptr;
        void* injectedCodeBase = nullptr;
        void* injectedDataBase = nullptr;

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
        injectedCodeBase = VirtualAllocEx(processHandle, nullptr, ((SIZE_T)kEffectiveInjectRegionSize * (SIZE_T)2), MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
        injectedDataBase = (void*)((size_t)injectedCodeBase + kEffectiveInjectRegionSize);

        if (nullptr == injectedCodeBase)
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

    EInjectResult ProcessInjector::VerifyMatchingProcessArchitecture(const HANDLE processHandle)
    {
        USHORT machineTargetProcess = 0;
        USHORT machineCurrentProcess = 0;

        if ((FALSE == IsWow64Process2(processHandle, &machineTargetProcess, nullptr)) || (FALSE == IsWow64Process2(Globals::GetCurrentProcessHandle(), &machineCurrentProcess, nullptr)))
            return EInjectResult::InjectResultErrorDetermineMachineProcess;

        if (machineTargetProcess == machineCurrentProcess)
            return EInjectResult::InjectResultSuccess;
        else
            return EInjectResult::InjectResultErrorArchitectureMismatch;
    }
}
