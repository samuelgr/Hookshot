/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file RemoteProcessInjector.cpp
 *   Implementation of requesting IPC-based process injection.
 *****************************************************************************/

#include "DependencyProtect.h"
#include "Globals.h"
#include "RemoteProcessInjector.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <sstream>
#include <string_view>


namespace Hookshot
{
    namespace RemoteProcessInjector
    {
        // -------- FUNCTIONS ---------------------------------------------- //
        // See "RemoteProcessInjector.h" for documentation.

        EInjectResult InjectProcess(const HANDLE processHandle, const HANDLE threadHandle, const bool switchArchitecture, const bool enableDebugFeatures)
        {
            // Obtain the name of the Hookshot executable to spawn.
            // Hold both the application name and the command-line arguments, enclosing the application name in quotes.
            // At most the argument needs to represent a 64-bit integer in hexadecimal, so two characters per byte, plus a space, an indicator character and a null character.
            const std::wstring_view kExecutableFileName = (switchArchitecture ? Strings::kStrHookshotExecutableOtherArchitectureFilename : Strings::kStrHookshotExecutableFilename);
            const size_t kExecutableArgumentMaxCount = 3 + (2 * sizeof(uint64_t));
            const size_t kExecutableCommandLineMaxCount = 3 + kExecutableFileName.length() + kExecutableArgumentMaxCount;

            std::wstringstream executableCommandLine;
            executableCommandLine << L'\"' << kExecutableFileName << L'\"';

            // Create an anonymous file mapping object backed by the system paging file, and ensure it can be inherited by child processes.
            // This has the effect of creating an anonymous shared memory object.
            // The resulting handle must be passed to the new instance of Hookshot that is spawned.
            SECURITY_ATTRIBUTES sharedMemorySecurityAttributes;
            sharedMemorySecurityAttributes.nLength = sizeof(sharedMemorySecurityAttributes);
            sharedMemorySecurityAttributes.lpSecurityDescriptor = nullptr;
            sharedMemorySecurityAttributes.bInheritHandle = TRUE;

            HANDLE sharedMemoryHandle = Protected::Windows_CreateFileMapping(INVALID_HANDLE_VALUE, &sharedMemorySecurityAttributes, PAGE_READWRITE, 0, sizeof(RemoteProcessInjector::SInjectRequest), nullptr);

            if (nullptr == sharedMemoryHandle)
                return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;

            RemoteProcessInjector::SInjectRequest* const sharedInfo = (RemoteProcessInjector::SInjectRequest*)Protected::Windows_MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

            if (nullptr == sharedInfo)
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
            }

            // Append the command-line argument to pass to the new Hookshot instance and convert to a mutable string, as required by CreateProcess.
            executableCommandLine << L' ' << Strings::kCharCmdlineIndicatorFileMappingHandle << std::hex << (uint64_t)sharedMemoryHandle;

            TemporaryBuffer<wchar_t> executableCommandLineMutableString;
            if (0 != wcscpy_s(executableCommandLineMutableString, executableCommandLineMutableString.Count(), executableCommandLine.str().c_str()))
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_UnmapViewOfFile(sharedInfo);
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorCannotGenerateExecutableFilename;
            }

            // Create the new instance of Hookshot.
            STARTUPINFO startupInfo;
            PROCESS_INFORMATION processInfo;
            memset((void*)&startupInfo, 0, sizeof(startupInfo));
            memset((void*)&processInfo, 0, sizeof(processInfo));

            if (FALSE == Protected::Windows_CreateProcess(nullptr, executableCommandLineMutableString, nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &startupInfo, &processInfo))
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_UnmapViewOfFile(sharedInfo);
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorCreateHookshotProcessFailed;
            }

            // Fill in the required inputs to the new instance of Hookshot.
            HANDLE duplicateProcessHandle = INVALID_HANDLE_VALUE;
            HANDLE duplicateThreadHandle = INVALID_HANDLE_VALUE;

            if ((FALSE == Protected::Windows_DuplicateHandle(Globals::GetCurrentProcessHandle(), processHandle, processInfo.hProcess, &duplicateProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) || (FALSE == Protected::Windows_DuplicateHandle(Globals::GetCurrentProcessHandle(), threadHandle, processInfo.hProcess, &duplicateThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)))
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_TerminateProcess(processInfo.hProcess, (UINT)-1);
                Protected::Windows_CloseHandle(processInfo.hProcess);
                Protected::Windows_CloseHandle(processInfo.hThread);
                Protected::Windows_UnmapViewOfFile(sharedInfo);
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
            }

            sharedInfo->processHandle = (uint64_t)duplicateProcessHandle;
            sharedInfo->threadHandle = (uint64_t)duplicateThreadHandle;
            sharedInfo->enableDebugFeatures = enableDebugFeatures;
            sharedInfo->injectionResult = (uint64_t)EInjectResult::InjectResultFailure;
            sharedInfo->extendedInjectionResult = 0ull;

            // Let the new instance of Hookshot run and wait for it to finish.
            Protected::Windows_ResumeThread(processInfo.hThread);

            if (WAIT_OBJECT_0 != Protected::Windows_WaitForSingleObject(processInfo.hProcess, 10000))
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_TerminateProcess(processInfo.hProcess, (UINT)-1);
                Protected::Windows_CloseHandle(processInfo.hProcess);
                Protected::Windows_CloseHandle(processInfo.hThread);
                Protected::Windows_UnmapViewOfFile(sharedInfo);
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
            }

            // Obtain results from the new instance of Hookshot, clean up, and return.
            DWORD injectExitCode = 0;
            if ((FALSE == Protected::Windows_GetExitCodeProcess(processInfo.hProcess, &injectExitCode)) || (0 != injectExitCode))
            {
                const DWORD extendedResult = Protected::Windows_GetLastError();
                Protected::Windows_CloseHandle(processInfo.hProcess);
                Protected::Windows_CloseHandle(processInfo.hThread);
                Protected::Windows_UnmapViewOfFile(sharedInfo);
                Protected::Windows_CloseHandle(sharedMemoryHandle);
                Protected::Windows_SetLastError(extendedResult);
                return EInjectResult::InjectResultErrorInterProcessCommunicationFailed;
            }

            const EInjectResult operationResult = (EInjectResult)sharedInfo->injectionResult;
            const DWORD extendedResult = (DWORD)sharedInfo->extendedInjectionResult;
            Protected::Windows_CloseHandle(processInfo.hProcess);
            Protected::Windows_CloseHandle(processInfo.hThread);
            Protected::Windows_UnmapViewOfFile(sharedInfo);
            Protected::Windows_CloseHandle(sharedMemoryHandle);
            Protected::Windows_SetLastError(extendedResult);
            return operationResult;
        }
    }
}
