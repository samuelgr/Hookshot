/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file ProcessInjector.h
 *   Implementation of all process creation and injection functionality.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Message.h"
#include "ProcessInjector.h"

#include <cstddef>
#include <cstdint>
#include <psapi.h>
#include <winnt.h>
#include <winternl.h>

using namespace Hookshot;


// -------- INTERNAL TYPES ------------------------------------------------- //

/// Prototype definition of the "NtQueryInformationProcess" function.
typedef NTSTATUS(WINAPI *NTQUERYINFORMATIONPROCESSPROC)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

// 32-bit (8 bytes)
// { 0x50, 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD0 } 
// 0:  50                      push   eax
// 1 : b8 00 00 00 00          mov    eax, 0x0
// 6 : ff d0                   call   eax

// 64-bit (16 bytes)
// 0:  50                      push   rax
// 1 : 90                      nop
// 2 : 48 b8 00 00 00 00 00    movabs rax, 0x0
// 9 : 00 00 00
// c : ff d0                   call   rax
// e : 90                      nop
// f : 90                      nop


// -------- INTERNAL VARIABLES --------------------------------------------- //



// -------- CLASS VARIABLES ------------------------------------------------ //
// See "ProcessInjector.h" for documentation.

HMODULE ProcessInjector::ntdllModuleHandle = NULL;

void* ProcessInjector::ntdllProcAddress = NULL;

size_t ProcessInjector::systemAllocationGranularity = 0;


// -------- CLASS METHODS -------------------------------------------------- //
// See "ProcessInjector.h" for documentation.

EInjectResult ProcessInjector::CreateInjectedProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    // This method creates processes in suspended state as part of injection functionality.
    // It will allow the new process to run, unless the caller requested a suspended process.
    const bool shouldCreateSuspended = (0 != (dwCreationFlags & CREATE_SUSPENDED)) ? true : false;

    // Attempt to create the new process in suspended state and capture information about it.
    PROCESS_INFORMATION processInfo;
    if (0 == CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, (dwCreationFlags | CREATE_SUSPENDED), lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInfo))
        return EInjectResult::InjectResultErrorCreateProcess;

    *lpProcessInformation = processInfo;

    // Attempt to inject the newly-created process and handle the result.
    return HandleInjectionResult(InjectNewlyCreatedProcess(processInfo.hProcess, processInfo.hThread), shouldCreateSuspended, processInfo.hProcess, processInfo.hThread);
}

// --------

EInjectResult ProcessInjector::CreateInjectedProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
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
    return HandleInjectionResult(InjectNewlyCreatedProcess(processInfo.hProcess, processInfo.hThread), shouldCreateSuspended, processInfo.hProcess, processInfo.hThread);
}


// -------- HELPERS -------------------------------------------------------- //
// See "ProcessInjector.h" for documentation.

EInjectResult ProcessInjector::GetProcessEntryPointAddress(const HANDLE processHandle, const void* const baseAddress, void** const entryPoint)
{
    size_t numBytesRead = 0;
    
    // First read the DOS header to figure out the location of the NT header.
    IMAGE_DOS_HEADER dosHeader;

    if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)baseAddress, (LPVOID)&dosHeader, sizeof(dosHeader), (SIZE_T*)&numBytesRead)) || (sizeof(dosHeader) != numBytesRead))
        return EInjectResult::InjectResultErrorReadDOSHeadersFailed;

    // Next read the NT header to figure out the location of the entry point.
    IMAGE_NT_HEADERS ntHeaders;
    void* const ntHeadersVA = (void*)((size_t)baseAddress + (size_t)dosHeader.e_lfanew);

    if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)ntHeadersVA, (LPVOID)&ntHeaders, sizeof(ntHeaders), (SIZE_T*)&numBytesRead)) || (sizeof(ntHeaders) != numBytesRead))
        return EInjectResult::InjectResultErrorReadNTHeadersFailed;

    // Compute the absolute entry point address.
    *entryPoint = (void*)((size_t)baseAddress + (size_t)ntHeaders.OptionalHeader.AddressOfEntryPoint);

    // Success.
    return EInjectResult::InjectResultSuccess;
}

// --------

EInjectResult ProcessInjector::GetProcessImageBaseAddress(const HANDLE processHandle, void** const baseAddress)
{
    // Verify that "ntdll.dll" is loaded.
    if (NULL == ntdllModuleHandle)
    {
        ntdllModuleHandle = LoadLibraryEx(_T("ntdll.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (NULL == ntdllModuleHandle)
            return EInjectResult::InjectResultErrorLoadNtDll;

        ntdllProcAddress = NULL;
    }

    // Verify that "NtQueryInformationProcess" is available.
    if (NULL == ntdllProcAddress)
    {
        ntdllProcAddress = GetProcAddress(ntdllModuleHandle, "NtQueryInformationProcess");

        if (NULL == ntdllProcAddress)
            return EInjectResult::InjectResultErrorNtQueryInformationProcessUnavailable;
    }

    // Obtain the address of the process environment block (PEB) for the process, which is within the address space of the process.
    PROCESS_BASIC_INFORMATION processBasicInfo;
    
    if (0 != ((NTQUERYINFORMATIONPROCESSPROC)ntdllProcAddress)(processHandle, ProcessBasicInformation, &processBasicInfo, sizeof(processBasicInfo), NULL))
        return EInjectResult::InjectResultErrorNtQueryInformationProcessFailed;

    // Read the PEB from the process' address space.
    PEB processPEB;
    size_t numBytesRead = 0;
    
    if ((FALSE == ReadProcessMemory(processHandle, (LPCVOID)processBasicInfo.PebBaseAddress, (LPVOID)&processPEB, sizeof(processPEB), (SIZE_T*)&numBytesRead)) || (sizeof(processPEB) != numBytesRead))
        return EInjectResult::InjectResultErrorReadProcessPEBFailed;
    
    // Read the base address from the PEB.
    *baseAddress = (void*)processPEB.Reserved3[1];

    // Success.
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

EInjectResult ProcessInjector::InjectNewlyCreatedProcess(const HANDLE processHandle, const HANDLE threadHandle)
{
    const size_t allocationGranularity = GetSystemAllocationGranularity();
    const size_t kEffectiveInjectRegionSize = (kInjectRegionSize < allocationGranularity) ? allocationGranularity : kInjectRegionSize;
    void* processBaseAddress = NULL;
    void* processEntryPoint = NULL;
    void* injectedCodeBase = NULL;
    void* injectedDataBase = NULL;

    EInjectResult operationResult = EInjectResult::InjectResultSuccess;

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

    // Inject code and data
    // TODO

    return operationResult;
}
