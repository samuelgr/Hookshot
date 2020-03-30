/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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

using namespace Hookshot;


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCommandShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    Globals::SetHookshotLoadMethod(EHookshotLoadMethod::Executed);

    if (2 > __argc)
    {
        Message::OutputFormatted(Message::ESeverity::Error, L"Usage: %s <command> [<arg1> <arg2>...]", Strings::kStrExecutableBaseName.data());
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

        SRemoteProcessInjectionData* const remoteInjectionData = (SRemoteProcessInjectionData*)MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
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
        TemporaryBuffer<wchar_t> commandLine;
        size_t commandLinePos = 0;

        for (size_t argIndex = 1; argIndex < __argc; ++argIndex)
        {
            const wchar_t* const argString = __wargv[argIndex];
            const size_t argLen = wcslen(argString);
            size_t argNumQuoteChars = 0;

            for (size_t i = 0; i < argLen; ++i)
            {
                if (L'\"' == argString[i])
                    argNumQuoteChars += 1;
            }

            // Compute the number of characters needed to represent the argument.
            // Each quote character requires an additional character for the escape backslash.
            // There must also be room for a whitespace separator, a quote character at the start, a quote character at the end, and possibly a terminal null character.
            const size_t argNumCharsNeeded = argLen + argNumQuoteChars + 4;

            if ((commandLinePos + argNumCharsNeeded) >= commandLine.Count())
            {
                Message::OutputFormatted(Message::ESeverity::Error, L"Specified command line exceeds the limit of %d characters.", (int)commandLine.Count());
                return __LINE__;
            }

            // Copy one character at a time from the command-line argument to the accumulating string.
            // Enclose the whole thing in quotes, add a whitespace separator at the end, and add a backslash in front of any quote characters found in the argument string.
            commandLine[commandLinePos++] = L'\"';

            for (size_t i = 0; i < argLen; ++i)
            {
                if (L'\"' == argString[i])
                    commandLine[commandLinePos++] = L'\\';

                commandLine[commandLinePos++] = argString[i];
            }

            commandLine[commandLinePos++] = L'\"';
            commandLine[commandLinePos++] = L' ';
        }

        commandLine[commandLinePos] = L'\0';

        // Second step is to create and inject the new process using the assembled command line string.
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        memset((void*)&startupInfo, 0, sizeof(startupInfo));
        memset((void*)&processInfo, 0, sizeof(processInfo));
        
        const EInjectResult result = ProcessInjector::CreateInjectedProcess(nullptr, commandLine, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);

        switch (result)
        {
        case EInjectResult::InjectResultSuccess:
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            Message::OutputFormatted(Message::ESeverity::Info, L"Successfully injected %s.", __wargv[1]);
            return 0;

        default:
            Message::OutputFormatted(Message::ESeverity::Error, L"EInjectResult %d.%d - Failed to inject %s.", (int)result, (int)GetLastError(), __wargv[1]);
            return __LINE__;
        }
    }
}
