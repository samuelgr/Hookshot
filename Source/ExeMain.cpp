/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file ExeMain.cpp
 *   Entry point for the bootstrap executable.
 **************************************************************************************************/

#include <sstream>
#include <string>

#include <Infra/Core/Message.h>
#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "Globals.h"
#include "InjectResult.h"
#include "ProcessInjector.h"
#include "Strings.h"

using namespace Hookshot;

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCmdShow Flag that specifies how the main application window should be shown. Not
/// applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
  Hookshot::Globals::Initialize(Hookshot::Globals::ELoadMethod::Executed);

  if (2 > __argc)
  {
    Infra::Message::OutputFormatted(
        Infra::Message::ESeverity::ForcedInteractiveError,
        L"%.*s cannot be launched directly. An executable file must be specified as an argument.\n\nUsage: %.*s <command> [<arg1> <arg2>...]",
        static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
        Infra::ProcessInfo::GetProductName().data(),
        static_cast<int>(Infra::ProcessInfo::GetExecutableBaseName().length()),
        Infra::ProcessInfo::GetExecutableBaseName().data());
    return __LINE__;
  }

  if ((2 == __argc) && (Strings::kCharCmdlineIndicatorFileMappingHandle == __wargv[1][0]))
  {
    // A file mapping handle was specified.
    // This is a special situation, in which this program was invoked to assist with injecting an
    // already-created process. Such a situation occurs when Hookshot created a new process whose
    // target architecture does not match (i.e. 32-bit Hookshot spawning a 64-bit program, or vice
    // versa). When this is detected, Hookshot will additionally spawn a matching version of the
    // Hookshot executable to inject the target program. Communication between both instances of
    // Hookshot occurs by means of shared memory accessed via a file mapping object.

    if (wcslen(&__wargv[1][1]) > (2 * sizeof(size_t))) return __LINE__;

    // Parse the handle value.
    HANDLE sharedMemoryHandle;
    wchar_t* parseEnd;

#ifdef _WIN64
    sharedMemoryHandle = reinterpret_cast<HANDLE>(wcstoull(&__wargv[1][1], &parseEnd, 16));
#else
    sharedMemoryHandle = reinterpret_cast<HANDLE>(wcstoul(&__wargv[1][1], &parseEnd, 16));
#endif

    if (L'\0' != *parseEnd) return __LINE__;

    RemoteProcessInjector::SInjectRequest* const remoteInjectionData =
        reinterpret_cast<RemoteProcessInjector::SInjectRequest*>(
            MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (nullptr == remoteInjectionData) return __LINE__;

    const bool remoteInjectionResult =
        ProcessInjector::PerformRequestedRemoteInjection(remoteInjectionData);

    UnmapViewOfFile(remoteInjectionData);
    CloseHandle(sharedMemoryHandle);

    return (false == remoteInjectionResult ? __LINE__ : 0);
  }
  else
  {
    // An executable was specified.
    // This is the normal situation, in which Hookshot is used to bootstrap an injection process.

    // First step is to combine all the command-line arguments into a single mutable string buffer,
    // including the executable to launch. Mutability is required per documentation of
    // CreateProcessW. Each individual argument must be placed in quotes (to preserve spaces
    // within), and each quote character in the argument must be escaped.
    std::wstringstream commandLineStream;
    for (size_t argIndex = 1; argIndex < (size_t)__argc; ++argIndex)
    {
      const wchar_t* const argString = __wargv[argIndex];
      const size_t argLen = wcslen(argString);

      commandLineStream << L'\"';

      for (size_t i = 0; i < argLen; ++i)
      {
        if (L'\"' == argString[i]) commandLineStream << L'\\';

        commandLineStream << argString[i];
      }

      commandLineStream << L"\" ";
    }

    Infra::TemporaryBuffer<wchar_t> commandLine;
    if (0 != wcscpy_s(commandLine.Data(), commandLine.Capacity(), commandLineStream.str().c_str()))
    {
      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::ForcedInteractiveError,
          L"Specified command line exceeds the limit of %u characters.",
          commandLine.Capacity());
      return __LINE__;
    }

    // Second step is to create and inject the new process using the assembled command line string.
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    memset(reinterpret_cast<void*>(&startupInfo), 0, sizeof(startupInfo));
    memset(reinterpret_cast<void*>(&processInfo), 0, sizeof(processInfo));

    const EInjectResult result = ProcessInjector::CreateInjectedProcess(
        nullptr,
        commandLine.Data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    switch (result)
    {
      case EInjectResult::Success:
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Info, L"Successfully injected %s.", __wargv[1]);
        return 0;

      case EInjectResult::ErrorCreateProcess:
        if (ERROR_ELEVATION_REQUIRED == GetLastError())
        {
          const INT_PTR executeElevatedResult = reinterpret_cast<INT_PTR>(ShellExecute(
              nullptr,
              L"runas",
              Infra::ProcessInfo::GetExecutableCompleteFilename().data(),
              commandLine.Data(),
              nullptr,
              SW_SHOWDEFAULT));
          if (executeElevatedResult > 32)
          {
            Infra::Message::OutputFormatted(
                Infra::Message::ESeverity::Info,
                L"Re-attempting the creation and injection %s with elevation.",
                __wargv[1]);

            if (IsDebuggerPresent())
              Infra::Message::Output(
                  Infra::Message::ESeverity::Warning,
                  L"Debugging state is not automatically propagated across an elevation attempt. To debug Hookshot as it injects a program that requires elevation, run the debugger as an administrator.");

            return 0;
          }
          else
          {
            Infra::Message::OutputFormatted(
                Infra::Message::ESeverity::ForcedInteractiveError,
                L"%s\n\n%.*s failed to inject this executable.\n\nTarget process requires elevation (%s).",
                __wargv[1],
                static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
                Infra::ProcessInfo::GetProductName().data(),
                Infra::Strings::FromSystemErrorCode((unsigned long)executeElevatedResult)
                    .AsCString());
            return __LINE__;
          }
        }
        [[fallthrough]];

      default:
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::ForcedInteractiveError,
            L"%s\n\n%.*s failed to inject this executable.\n\n%s (%s)",
            __wargv[1],
            static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
            Infra::ProcessInfo::GetProductName().data(),
            InjectResultString(result).data(),
            Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());
        return __LINE__;
    }
  }
}
