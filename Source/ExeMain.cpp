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

using namespace Hookshot;


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `NULL`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCommandShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR lpCmdLine, int nCmdShow)
{
    if (2 != __argc)
    {
        Message::Output(EMessageSeverity::MessageSeverityError, _T("This program requires exactly one argument that specifies command-line of the executable to run and inject."));
        return __LINE__;
    }

    if (Strings::kCharCmdlineIndicatorFileMappingHandle == __targv[1][0])
    {
        // A file mapping handle was specified.
        // This is a special situation, in which this program was invoked to assist with injecting an already-created process.
        // Such a situation occurs when Hookshot created a new process whose target architecture does not match (i.e. 32-bit Hookshot spawning a 64-bit program, or vice versa).
        // When this is detected, Hookshot will additionally spawn a matching version of the Hookshot executable to inject the target program.
        // Communication between both instances of Hookshot occurs by means of shared memory accessed via a file mapping object.

        if (_tcslen(&__targv[1][1]) > (2 * sizeof(size_t)))
            return __LINE__;

        // Parse the handle value.
        HANDLE sharedMemoryHandle;
        TCHAR* parseEnd;

#ifdef HOOKSHOT64
        sharedMemoryHandle = (HANDLE)_tcstoull(&__targv[1][1], &parseEnd, 16);
#else
        sharedMemoryHandle = (HANDLE)_tcstoul(&__targv[1][1], &parseEnd, 16);
#endif  

        if (_T('\0') != *parseEnd)
            return __LINE__;

        SRemoteProcessInjectionData* const remoteInjectionData = (SRemoteProcessInjectionData*)MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (NULL == remoteInjectionData)
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
        
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        memset((void*)&startupInfo, 0, sizeof(startupInfo));
        memset((void*)&processInfo, 0, sizeof(processInfo));

        const EInjectResult result = ProcessInjector::CreateInjectedProcess(NULL, __targv[1], NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);

        switch (result)
        {
        case EInjectResult::InjectResultSuccess:
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Successfully injected %s."), __targv[1]);
            return 0;

        default:
            Message::OutputFormatted(EMessageSeverity::MessageSeverityError, _T("EInjectResult %d.%d - Failed to inject %s."), (int)result, (int)GetLastError(), __targv[1]);
            return __LINE__;
        }
    }
}
