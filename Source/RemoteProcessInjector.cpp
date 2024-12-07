/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file RemoteProcessInjector.cpp
 *   Implementation of requesting IPC-based process injection.
 **************************************************************************************************/

#include "RemoteProcessInjector.h"

#include <sstream>
#include <string_view>

#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "DependencyProtect.h"
#include "Strings.h"

namespace Hookshot
{
  namespace RemoteProcessInjector
  {
    EInjectResult InjectProcess(
        const HANDLE processHandle,
        const HANDLE threadHandle,
        const bool switchArchitecture,
        const bool enableDebugFeatures)
    {
      // Obtain the name of the Hookshot executable to spawn.
      // Hold both the application name and the command-line arguments, enclosing the application
      // name in quotes. At most the argument needs to represent a 64-bit integer in hexadecimal, so
      // two characters per byte, plus a space, an indicator character and a null character.
      const std::wstring_view executableFileName =
          (switchArchitecture ? Strings::kStrHookshotExecutableOtherArchitectureFilename
                              : Strings::kStrHookshotExecutableFilename);
      const size_t executableArgumentMaxCount = 3 + (2 * sizeof(uint64_t));
      const size_t executableCommandLineMaxCount =
          3 + executableFileName.length() + executableArgumentMaxCount;

      std::wstringstream executableCommandLine;
      executableCommandLine << L'\"' << executableFileName << L'\"';

      // Create an anonymous file mapping object backed by the system paging file, and ensure it can
      // be inherited by child processes. This has the effect of creating an anonymous shared memory
      // object. The resulting handle must be passed to the new instance of Hookshot that is
      // spawned.
      SECURITY_ATTRIBUTES sharedMemorySecurityAttributes;
      sharedMemorySecurityAttributes.nLength = sizeof(sharedMemorySecurityAttributes);
      sharedMemorySecurityAttributes.lpSecurityDescriptor = nullptr;
      sharedMemorySecurityAttributes.bInheritHandle = TRUE;

      HANDLE sharedMemoryHandle = Protected::Windows_CreateFileMapping(
          INVALID_HANDLE_VALUE,
          &sharedMemorySecurityAttributes,
          PAGE_READWRITE,
          0,
          sizeof(RemoteProcessInjector::SInjectRequest),
          nullptr);

      if (nullptr == sharedMemoryHandle) return EInjectResult::ErrorInterProcessCommunicationFailed;

      RemoteProcessInjector::SInjectRequest* const sharedInfo =
          reinterpret_cast<RemoteProcessInjector::SInjectRequest*>(
              Protected::Windows_MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0));

      if (nullptr == sharedInfo)
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);
        return EInjectResult::ErrorInterProcessCommunicationFailed;
      }

      // Append the command-line argument to pass to the new Hookshot instance and convert to a
      // mutable string, as required by CreateProcess.
      executableCommandLine << L' ' << Strings::kCharCmdlineIndicatorFileMappingHandle << std::hex
                            << reinterpret_cast<uint64_t>(sharedMemoryHandle);

      Infra::TemporaryBuffer<wchar_t> executableCommandLineMutableString;
      if (0 !=
          wcscpy_s(
              executableCommandLineMutableString.Data(),
              executableCommandLineMutableString.Capacity(),
              executableCommandLine.str().c_str()))
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_UnmapViewOfFile(sharedInfo);
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);
        return EInjectResult::ErrorCannotGenerateExecutableFilename;
      }

      // Create the new instance of Hookshot.
      STARTUPINFO startupInfo;
      PROCESS_INFORMATION processInfo;
      memset(reinterpret_cast<void*>(&startupInfo), 0, sizeof(startupInfo));
      memset(reinterpret_cast<void*>(&processInfo), 0, sizeof(processInfo));

      if (FALSE ==
          Protected::Windows_CreateProcess(
              nullptr,
              executableCommandLineMutableString.Data(),
              nullptr,
              nullptr,
              TRUE,
              CREATE_SUSPENDED,
              nullptr,
              nullptr,
              &startupInfo,
              &processInfo))
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_UnmapViewOfFile(sharedInfo);
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);

        if (true == switchArchitecture)
          return EInjectResult::ErrorCreateHookshotOtherArchitectureProcessFailed;
        else
          return EInjectResult::ErrorCreateHookshotProcessFailed;
      }

      // Fill in the required inputs to the new instance of Hookshot.
      HANDLE duplicateProcessHandle = INVALID_HANDLE_VALUE;
      HANDLE duplicateThreadHandle = INVALID_HANDLE_VALUE;

      if ((FALSE ==
           Protected::Windows_DuplicateHandle(
               Infra::ProcessInfo::GetCurrentProcessHandle(),
               processHandle,
               processInfo.hProcess,
               &duplicateProcessHandle,
               0,
               FALSE,
               DUPLICATE_SAME_ACCESS)) ||
          (FALSE ==
           Protected::Windows_DuplicateHandle(
               Infra::ProcessInfo::GetCurrentProcessHandle(),
               threadHandle,
               processInfo.hProcess,
               &duplicateThreadHandle,
               0,
               FALSE,
               DUPLICATE_SAME_ACCESS)))
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_TerminateProcess(processInfo.hProcess, (UINT)-1);
        Protected::Windows_CloseHandle(processInfo.hProcess);
        Protected::Windows_CloseHandle(processInfo.hThread);
        Protected::Windows_UnmapViewOfFile(sharedInfo);
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);
        return EInjectResult::ErrorInterProcessCommunicationFailed;
      }

      sharedInfo->processHandle = reinterpret_cast<uint64_t>(duplicateProcessHandle);
      sharedInfo->threadHandle = reinterpret_cast<uint64_t>(duplicateThreadHandle);
      sharedInfo->enableDebugFeatures = enableDebugFeatures;
      sharedInfo->injectionResult = static_cast<uint64_t>(EInjectResult::Failure);
      sharedInfo->extendedInjectionResult = 0ull;

      // Let the new instance of Hookshot run and wait for it to finish.
      Protected::Windows_ResumeThread(processInfo.hThread);

      if (WAIT_OBJECT_0 != Protected::Windows_WaitForSingleObject(processInfo.hProcess, 10000))
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_TerminateProcess(processInfo.hProcess, ~(0u));
        Protected::Windows_CloseHandle(processInfo.hProcess);
        Protected::Windows_CloseHandle(processInfo.hThread);
        Protected::Windows_UnmapViewOfFile(sharedInfo);
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);
        return EInjectResult::ErrorInterProcessCommunicationFailed;
      }

      // Obtain results from the new instance of Hookshot, clean up, and return.
      DWORD injectExitCode = 0;
      if ((FALSE == Protected::Windows_GetExitCodeProcess(processInfo.hProcess, &injectExitCode)) ||
          (0 != injectExitCode))
      {
        const DWORD extendedResult = Protected::Windows_GetLastError();
        Protected::Windows_CloseHandle(processInfo.hProcess);
        Protected::Windows_CloseHandle(processInfo.hThread);
        Protected::Windows_UnmapViewOfFile(sharedInfo);
        Protected::Windows_CloseHandle(sharedMemoryHandle);
        Protected::Windows_SetLastError(extendedResult);
        return EInjectResult::ErrorInterProcessCommunicationFailed;
      }

      EInjectResult operationResult = static_cast<EInjectResult>(sharedInfo->injectionResult);
      DWORD extendedResult = static_cast<DWORD>(sharedInfo->extendedInjectionResult);
      Protected::Windows_CloseHandle(processInfo.hProcess);
      Protected::Windows_CloseHandle(processInfo.hThread);
      Protected::Windows_UnmapViewOfFile(sharedInfo);
      Protected::Windows_CloseHandle(sharedMemoryHandle);
      Protected::Windows_SetLastError(extendedResult);

      // Some errors are architecture-specific as a way of helping the user understand the issue.
      if (true == switchArchitecture)
      {
        switch (operationResult)
        {
          case EInjectResult::ErrorCannotLoadLibrary:
            operationResult = EInjectResult::ErrorCannotLoadLibraryOtherArchitecture;
            break;

          default:
            break;
        }
      }

      return operationResult;
    }
  } // namespace RemoteProcessInjector
} // namespace Hookshot
