/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file InjectLanding.cpp
 *   Partial landing code implementation.
 *   Receives control from injection code, cleans up, and runs the program.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Inject.h"
#include "LibraryInitialize.h"
#include "Message.h"
#include "StringUtilities.h"
#include "TemporaryBuffers.h"

#include <cstddef>
#include <hookshot.h>
#include <psapi.h>

using namespace Hookshot;


// -------- FUNCTIONS ------------------------------------------------------ //
// See "InjectLanding.h" for documentation.

extern "C" void APIENTRY InjectLandingCleanup(const SInjectData* const injectData)
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

extern "C" void APIENTRY InjectLandingSetHooks(void)
{
#ifdef HOOKSHOT_DEBUG
    if (FALSE == IsDebuggerPresent())
    {
        TemporaryBuffer<TCHAR> executableBaseName;
        GetModuleBaseName(GetCurrentProcess(), NULL, executableBaseName, executableBaseName.Count());
        
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_DEBUG_PID_FORMAT, (TCHAR*)executableBaseName, GetProcessId(GetCurrentProcess()));
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

    Message::OutputFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_NO_HOOK_MODULE);
}
