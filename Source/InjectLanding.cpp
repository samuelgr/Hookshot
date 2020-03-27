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

#include "DependencyProtect.h"
#include "Globals.h"
#include "Inject.h"
#include "LibraryInterface.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <psapi.h>

using namespace Hookshot;


// -------- FUNCTIONS ------------------------------------------------------ //
// See "InjectLanding.h" for documentation.

extern "C" void __fastcall InjectLandingCleanup(const SInjectData* const injectData)
{
    // Before cleaning up, set aside the array of addresses to free.
    // This is needed because freeing any one of these could invalidate the injectData pointer.
    void* cleanupBaseAddress[_countof(injectData->cleanupBaseAddress)];
    memcpy((void*)cleanupBaseAddress, (void*)injectData->cleanupBaseAddress, sizeof(cleanupBaseAddress));

    for (size_t i = 0; i < _countof(cleanupBaseAddress); ++i)
    {
        if (NULL != cleanupBaseAddress[i])
            Windows::ProtectedVirtualFree(cleanupBaseAddress[i], 0, MEM_RELEASE);
    }
}

// --------

extern "C" void __fastcall InjectLandingLoadHookModules(const SInjectData* const injectData)
{
    if ((0 != injectData->enableDebugFeatures) && (0 == Windows::ProtectedIsDebuggerPresent()))
        Message::OutputFormatted(Message::ESeverity::ForcedInteractiveInfo, L"Attach to \"%s\" (PID %d) to continue debugging.", Strings::kStrExecutableBaseName.data(), Globals::GetCurrentProcessId());

    if (true == LibraryInterface::IsConfigurationDataValid())
    {
        const int numHookModulesLoaded = LibraryInterface::LoadConfiguredHookModules();
        const int numInjectOnlyLibrariesLoaded = LibraryInterface::LoadConfiguredInjectOnlyLibraries();
        Message::OutputFormatted(Message::ESeverity::Info, L"Loaded %d hook module%s and %d injection-only librar%s.", numHookModulesLoaded, (1 == numHookModulesLoaded ? L"" : L"s"), numInjectOnlyLibrariesLoaded, (1 == numInjectOnlyLibrariesLoaded ? L"y" : L"ies"));
    }
}
