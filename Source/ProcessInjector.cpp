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
#include "StringUtilities.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <psapi.h>
#include <winnt.h>
#include <winternl.h>


namespace Hookshot
{
    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Defines the structure of the shared memory that communicates between two instances of Hookshot.
    /// One instance fills the required input information, and the other performs the requested tasks and fills status output.
    /// To ensure safety, all values are 64-bit integers.
    struct SOtherArchitectureSharedInfo
    {
        uint64_t processHandle;                                     ///< Handle of the process to inject, as a 64-bit integer.  Must be valid for the instance of Hookshot that performs the injection.
        uint64_t threadHandle;                                      ///< Handle of the main thread in the process to inject, as a 64-bit integer.  Must be valid for the instance of Hookshot that performs the injection.
        
        uint64_t unused1[(128 / sizeof(uint64_t)) - 2];             ///< Padding for 128-byte alignment.

        uint64_t injectionResult;                                   ///< EInjectionResult value, as a 64-bit integer.  Indicates the result of the injection attempt.
        uint64_t extendedInjectionResult;                           ///< Extended injection result, as a 64-bit integer.

        uint64_t unused2[(128 / sizeof(uint64_t)) - 2];             ///< Padding for 128-byte alignment.
    };

    
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "ProcessInjector.h" for documentation.

    HMODULE ProcessInjector::ntdllModuleHandle = NULL;

    NTSTATUS(WINAPI* ProcessInjector::ntdllQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG) = NULL;

    size_t ProcessInjector::systemAllocationGranularity = 0;


    // -------- CLASS METHODS ---------------------------------------------- //
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
        EInjectResult operationResult = VerifyMatchingProcessArchitecture(processInfo.hProcess);

        switch (operationResult)
        {
        case EInjectResult::InjectResultSuccess:
            operationResult = InjectProcess(processInfo.hProcess, processInfo.hThread);
            break;

        case EInjectResult::InjectResultErrorArchitectureMismatch:
            operationResult = OtherArchitectureRequestInjection(processInfo.hProcess, processInfo.hThread);
            break;

        default:
            break;
        }

        return HandleInjectionResult(operationResult, shouldCreateSuspended, processInfo.hProcess, processInfo.hThread);
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
        EInjectResult operationResult = VerifyMatchingProcessArchitecture(processInfo.hProcess);

        switch (operationResult)
        {
        case EInjectResult::InjectResultSuccess:
            operationResult = InjectProcess(processInfo.hProcess, processInfo.hThread);
            break;

        case EInjectResult::InjectResultErrorArchitectureMismatch:
            operationResult = OtherArchitectureRequestInjection(processInfo.hProcess, processInfo.hThread);
            break;

        default:
            break;
        }

        return HandleInjectionResult(operationResult, shouldCreateSuspended, processInfo.hProcess, processInfo.hThread);
    }

    // --------

    bool ProcessInjector::OtherArchitecturePerformRequestedInjection(const HANDLE sharedMemoryHandle)
    {
        SOtherArchitectureSharedInfo* const sharedInfo = (SOtherArchitectureSharedInfo*)MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (NULL == sharedInfo)
            return false;

        EInjectResult operationResult = VerifyMatchingProcessArchitecture((HANDLE)sharedInfo->processHandle);
        
        // If the target process architecture matches that of this running binary, attempt to perform injection.
        // Otherwise it is an error, since this running binary was invoked to assist with target process injection.
        if (EInjectResult::InjectResultSuccess == operationResult)
            operationResult = InjectProcess((HANDLE)sharedInfo->processHandle, (HANDLE)sharedInfo->threadHandle);
        
        // Save operation results, clean up, and return.
        sharedInfo->injectionResult = (uint64_t)operationResult;
        sharedInfo->extendedInjectionResult = (uint64_t)GetLastError();
        
        CloseHandle((HANDLE)sharedInfo->processHandle);
        CloseHandle((HANDLE)sharedInfo->threadHandle);
        UnmapViewOfFile(sharedInfo);
        
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
            ntdllModuleHandle = LoadLibraryEx(_T("ntdll.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

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

    EInjectResult ProcessInjector::InjectProcess(const HANDLE processHandle, const HANDLE threadHandle)
    {
        const size_t allocationGranularity = GetSystemAllocationGranularity();
        const size_t kEffectiveInjectRegionSize = (InjectInfo::kMaxInjectBinaryFileSize < allocationGranularity) ? allocationGranularity : InjectInfo::kMaxInjectBinaryFileSize;
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

        // Inject code and data.
        // Only mark the code buffer as requiring cleanup because both code and data buffers are from the same single allocation.
        CodeInjector injector(injectedCodeBase, injectedDataBase, true, false, processEntryPoint, kEffectiveInjectRegionSize, kEffectiveInjectRegionSize, processHandle, threadHandle);
        operationResult = injector.SetAndRun();

        return operationResult;
    }

    // --------

    EInjectResult ProcessInjector::OtherArchitectureRequestInjection(const HANDLE processHandle, const HANDLE threadHandle)
    {
        // Obtain the name of the Hookshot executable to spawn.
        // Hold both the application name and the command-line arguments, enclosing the application name in quotes.
        // At most the argument needs to represent a 64-bit integer in hexadecimal, so two characters per byte, plus a space, an indicator character and a null character.
        const size_t kExecutableArgumentMaxCount = 3 + (2 * sizeof(uint64_t));
        TemporaryBuffer<TCHAR> executableCommandLine;
        executableCommandLine[0] = _T('\"');
        
        if (false == Strings::FillInjectExecutableOtherArchitectureFilename(&executableCommandLine[1], executableCommandLine.Count()))
            return EInjectResult::InjectResultErrorCannotGenerateInjectCodeFilename;

        const size_t kExecutableFileNameLength = _tcslen(executableCommandLine) + 1;
        executableCommandLine[kExecutableFileNameLength - 1] = _T('\"');
        
        // Create an anonymous file mapping object backed by the system paging file, and ensure it can be inherited by child processes.
        // This has the effect of creating an anonymous shared memory object.
        // The resulting handle must be passed to the new instance of Hookshot that is spawned.
        SECURITY_ATTRIBUTES sharedMemorySecurityAttributes;
        sharedMemorySecurityAttributes.nLength = sizeof(sharedMemorySecurityAttributes);
        sharedMemorySecurityAttributes.lpSecurityDescriptor = NULL;
        sharedMemorySecurityAttributes.bInheritHandle = TRUE;

        HANDLE sharedMemoryHandle = CreateFileMapping(INVALID_HANDLE_VALUE, &sharedMemorySecurityAttributes, PAGE_READWRITE, 0, sizeof(SOtherArchitectureSharedInfo), NULL);

        if ((NULL == sharedMemoryHandle) || (INVALID_HANDLE_VALUE == sharedMemoryHandle))
            return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;

        SOtherArchitectureSharedInfo* const sharedInfo = (SOtherArchitectureSharedInfo*)MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (NULL == sharedInfo)
            return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;

        // Generate the command-line argument to pass to the new Hookshot instance.
        // At most we need to represent a 64-bit integer in hexadecimal, so two characters per byte, plus an indicator character and a null character.
        _stprintf_s(&executableCommandLine[kExecutableFileNameLength], kExecutableArgumentMaxCount, _T(" |%llx"), (uint64_t)sharedMemoryHandle);
        
        // Create the new instance of Hookshot.
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;
        memset((void*)&startupInfo, 0, sizeof(startupInfo));
        memset((void*)&processInfo, 0, sizeof(processInfo));

        if (FALSE == CreateProcess(NULL, executableCommandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInfo))
            return EInjectResult::InjectResultErrorCreateHookshotProcessFailed;

        // Fill in the required inputs to the new instance of Hookshot.
        HANDLE duplicateProcessHandle = INVALID_HANDLE_VALUE;
        HANDLE duplicateThreadHandle = INVALID_HANDLE_VALUE;

        if ((FALSE == DuplicateHandle(GetCurrentProcess(), processHandle, processInfo.hProcess, &duplicateProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) || (FALSE == DuplicateHandle(GetCurrentProcess(), threadHandle, processInfo.hProcess, &duplicateThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)))
        {
            TerminateProcess(processInfo.hProcess, (UINT)-1);
            return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
        }
        
        sharedInfo->processHandle = (uint64_t)duplicateProcessHandle;
        sharedInfo->threadHandle = (uint64_t)duplicateThreadHandle;
        sharedInfo->injectionResult = (uint64_t)EInjectResult::InjectResultFailure;
        sharedInfo->extendedInjectionResult = 0ull;

        // Let the new instance of Hookshot run and wait for it to finish.
        ResumeThread(processInfo.hThread);
        
        if (WAIT_OBJECT_0 != WaitForSingleObject(processInfo.hProcess, INFINITE))
        {
            TerminateProcess(processInfo.hProcess, (UINT)-1);
            return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
        }

        // Obtain results from the new instance of Hookshot, clean up, and return.
        DWORD injectExitCode = 0;
        if ((FALSE == GetExitCodeProcess(processInfo.hProcess, &injectExitCode)) || (0 != injectExitCode))
            return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;

        const DWORD extendedResult = (DWORD)sharedInfo->extendedInjectionResult;
        const EInjectResult operationResult = (EInjectResult)sharedInfo->injectionResult;
        
        UnmapViewOfFile(sharedInfo);
        CloseHandle(sharedMemoryHandle);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        SetLastError(extendedResult);
        return operationResult;
    }

    // --------
    
    EInjectResult ProcessInjector::VerifyMatchingProcessArchitecture(const HANDLE processHandle)
    {
        USHORT machineTargetProcess = 0;
        USHORT machineCurrentProcess = 0;

        if ((FALSE == IsWow64Process2(processHandle, &machineTargetProcess, NULL)) || (FALSE == IsWow64Process2(GetCurrentProcess(), &machineCurrentProcess, NULL)))
            return EInjectResult::InjectResultErrorDetermineMachineProcess;

        if (machineTargetProcess == machineCurrentProcess)
            return EInjectResult::InjectResultSuccess;
        else
            return EInjectResult::InjectResultErrorArchitectureMismatch;
    }
}
