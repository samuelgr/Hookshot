/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 **************************************************************************//**
 * @file ExeMain.cpp
 *   Entry point for the bootstrap executable.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "InjectResult.h"
#include "Message.h"
#include "ProcessInjector.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <sstream>
#include <string>

using namespace Hookshot;


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCmdShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    Hookshot::Globals::Initialize(Hookshot::Globals::ELoadMethod::Executed);

    if (2 > __argc)
    {
        Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s cannot be launched directly. An executable file must be specified as an argument.\n\nUsage: %s <command> [<arg1> <arg2>...]", Strings::kStrProductName.data(), Strings::kStrExecutableBaseName.data());
        return __LINE__;
    }

    if ((2 == __argc) && (Strings::kCharCmdlineIndicatorFileMappingHandle == __wargv[1][0]))
    {
        // A file mapping handle was specified.
        // This is a special situation, in which this program was invoked to assist with injecting an already-created process.
        // Such a situation occurs when Hookshot created a new process whose target architecture does not match (i.e. 32-bit Hookshot spawning a 64-bit program, or vice versa).
        // When this is detected, Hookshot will additionally spawn a matching version of the Hookshot executable to inject the target program.
        // Communication between both instances of Hookshot occurs by means of shared memory accessed via a file mapping object.

        if (wcslen(&__wargv[1][1]) > (2 * sizeof(size_t)))
            return __LINE__;

        // Parse the handle value.
        HANDLE sharedMemoryHandle;
        wchar_t* parseEnd;

#ifdef HOOKSHOT64
        sharedMemoryHandle = (HANDLE)wcstoull(&__wargv[1][1], &parseEnd, 16);
#else
        sharedMemoryHandle = (HANDLE)wcstoul(&__wargv[1][1], &parseEnd, 16);
#endif

        if (L'\0' != *parseEnd)
            return __LINE__;

        RemoteProcessInjector::SInjectRequest* const remoteInjectionData = (RemoteProcessInjector::SInjectRequest*)MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (nullptr == remoteInjectionData)
            return __LINE__;

        const bool remoteInjectionResult = ProcessInjector::PerformRequestedRemoteInjection(remoteInjectionData);

        UnmapViewOfFile(remoteInjectionData);
        CloseHandle(sharedMemoryHandle);

        return (false == remoteInjectionResult ? __LINE__ : 0);
    }
    else
    {
        // An executable was specified.
        // This is the normal situation, in which Hookshot is used to bootstrap an injection process.

        // First step is to combine all the command-line arguments into a single mutable string buffer, including the executable to launch.
        // Mutability is required per documentation of CreateProcessW.
        // Each individual argument must be placed in quotes (to preserve spaces within), and each quote character in the argument must be escaped.
        std::wstringstream commandLineStream;
        for (size_t argIndex = 1; argIndex < (size_t)__argc; ++argIndex)
        {
            const wchar_t* const argString = __wargv[argIndex];
            const size_t argLen = wcslen(argString);

            commandLineStream << L'\"';
            
            for (size_t i = 0; i < argLen; ++i)
            {
                if (L'\"' == argString[i])
                    commandLineStream << L'\\';

                commandLineStream << argString[i];
            }

            commandLineStream << L"\" ";
        }

        TemporaryBuffer<wchar_t> commandLine;
        if (0 != wcscpy_s(commandLine.Data(), commandLine.Capacity(), commandLineStream.str().c_str()))
        {
            Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"Specified command line exceeds the limit of %d characters.", (int)commandLine.Capacity());
            return __LINE__;
        }

        // Second step is to create and inject the new process using the assembled command line string.
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        memset((void*)&startupInfo, 0, sizeof(startupInfo));
        memset((void*)&processInfo, 0, sizeof(processInfo));

        const EInjectResult result = ProcessInjector::CreateInjectedProcess(nullptr, commandLine.Data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);

        switch (result)
        {
        case EInjectResult::Success:
            Message::OutputFormatted(Message::ESeverity::Info, L"Successfully injected %s.", __wargv[1]);
            return 0;

        case EInjectResult::ErrorCreateProcess:
            if (ERROR_ELEVATION_REQUIRED == GetLastError())
            {
                const INT_PTR executeElevatedResult = (INT_PTR)ShellExecute(nullptr, L"runas", Strings::kStrExecutableCompleteFilename.data(), commandLine.Data(), nullptr, SW_SHOWDEFAULT);
                if (executeElevatedResult > 32)
                {
                    Message::OutputFormatted(Message::ESeverity::Info, L"Re-attempting the creation and injection %s with elevation.", __wargv[1]);

                    if (IsDebuggerPresent())
                        Message::Output(Message::ESeverity::Warning, L"Debugging state is not automatically propagated across an elevation attempt. To debug Hookshot as it injects a program that requires elevation, run the debugger as an administrator.");

                    return 0;
                }
                else
                {
                    Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s\n\n%s failed to inject this executable.\n\nTarget process requires elevation (%s).", __wargv[1], Strings::kStrProductName.data(), Strings::SystemErrorCodeString((unsigned long)executeElevatedResult).AsCString());
                    return __LINE__;
                }
            }
            [[fallthrough]];

        default:
            Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s\n\n%s failed to inject this executable.\n\n%s (%s).", __wargv[1], Strings::kStrProductName.data(), InjectResultString(result).data(), Strings::SystemErrorCodeString(GetLastError()).AsCString());
            return __LINE__;
        }
    }
}
