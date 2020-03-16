/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file InjectLanding.cpp
 *   Partial landing code implementation.
 *   Receives control from injection code, cleans up, and runs the program.
 *****************************************************************************/

#include "Globals.h"
#include "Inject.h"
#include "LibraryInitialize.h"
#include "Message.h"
#include "StringUtilities.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <hookshot.h>
#include <psapi.h>

using namespace Hookshot;


// -------- FUNCTIONS ------------------------------------------------------ //
// See "InjectLanding.h" for documentation.

extern "C" void __stdcall InjectLandingCleanup(const SInjectData* const injectData)
{
    // Before cleaning up, set aside the array of addresses to free.
    // This is needed because freeing any one of these could invalidate the injectData pointer.
    void* cleanupBaseAddress[_countof(injectData->cleanupBaseAddress)];
    memcpy((void*)cleanupBaseAddress, (void*)injectData->cleanupBaseAddress, sizeof(cleanupBaseAddress));

    for (size_t i = 0; i < _countof(cleanupBaseAddress); ++i)
    {
        if (NULL != cleanupBaseAddress[i])
            VirtualFree(cleanupBaseAddress[i], 0, MEM_RELEASE);
    }
}

// --------

extern "C" void __stdcall InjectLandingLoadHookModules(const SInjectData* const injectData)
{
#ifdef HOOKSHOT_DEBUG
    if (FALSE == IsDebuggerPresent())
    {
        TemporaryBuffer<TCHAR> executableBaseName;
        GetModuleBaseName(GetCurrentProcess(), NULL, executableBaseName, executableBaseName.Count());
        
        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Attach to \"%s\" (PID %d) to continue debugging."), (TCHAR*)executableBaseName, GetProcessId(GetCurrentProcess()));
    }
#endif

    // First, try the executable-specific hook module filename.
    // Second, try the directory-common hook module filename.
    // If both fail, there is no hook module to load.

    TemporaryBuffer<TCHAR> hookModuleFileName;

    if (false == Strings::FillHookModuleFilenameUnique(hookModuleFileName, hookModuleFileName.Count()))
        return;

    if (true == LibraryInitialize::LoadHookModule(hookModuleFileName))
        return;

    if (false == Strings::FillHookModuleFilenameCommon(hookModuleFileName, hookModuleFileName.Count()))
        return;

    if (true == LibraryInitialize::LoadHookModule(hookModuleFileName))
        return;

    Message::Output(EMessageSeverity::MessageSeverityWarning, _T("No hook module is loaded. No hooks have been set for the current process."));
}
