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
#include "Strings.h"
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
    if ((0 != injectData->isDebuggerAttached) && (0 == IsDebuggerPresent()))
        Message::OutputFormatted(EMessageSeverity::MessageSeverityForcedInfo, _T("Attach to \"%s\" (PID %d) to continue debugging."), Strings::kStrExecutableBaseName.data(), GetProcessId(GetCurrentProcess()));

    // First, try the executable-specific hook module filename.
    // Second, try the directory-common hook module filename.
    // If both fail, there is no hook module to load.

    if (true == LibraryInitialize::LoadHookModule(Strings::GetHookModuleFilename(Strings::kStrExecutableBaseName).c_str()))
        return;

    if (true == LibraryInitialize::LoadHookModule(Strings::GetHookModuleFilename(_T("Common")).c_str()))
        return;

    Message::Output(EMessageSeverity::MessageSeverityWarning, _T("No hook module is loaded. No hooks have been set for the current process."));
}
