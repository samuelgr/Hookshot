/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file ExeMain.cpp
 *   Entry point for the bootstrap executable.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Message.h"
#include "ProcessInjector.h"

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
    Globals::SetInstanceHandle(hInstance);
    
    if (2 != __argc)
    {
        Message::OutputFromResource(EMessageSeverity::MessageSeverityError, IDS_HOOKSHOT_EXE_ERROR_USAGE);
        return __LINE__;
    }

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
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_SUCCESS_INJECT_FORMAT, __targv[1]);
        break;

    default:
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityError, IDS_HOOKSHOT_ERROR_INJECT_FAILED_FORMAT, (int)result, (int)GetLastError(), __targv[1]);
        break;
    }

    return (int)result;
}
