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
#include "LibraryInterface.h"
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
    if ((0 != injectData->enableDebugFeatures) && (0 == IsDebuggerPresent()))
        Message::OutputFormatted(EMessageSeverity::MessageSeverityForcedInfo, L"Attach to \"%s\" (PID %d) to continue debugging.", Strings::kStrExecutableBaseName.data(), GetProcessId(GetCurrentProcess()));

    if (true == LibraryInterface::IsConfigurationDataValid())
    {
        auto hookModulesToLoad = LibraryInterface::GetConfigurationData().SectionsWithName(Strings::kStrConfigurationSettingNameHookModule);
        int numHookModulesLoaded = 0;
        
        for (auto& sectionsWithHookModules : *hookModulesToLoad)
        {
            for (auto& hookModule : sectionsWithHookModules.name.Values())
            {
                if (Configuration::EValueType::String != hookModule.GetType())
                {
                    Message::Output(EMessageSeverity::MessageSeverityError, L"Internal error while loading hook modules.");
                    return;
                }

                if (true == LibraryInterface::LoadHookModule(Strings::MakeHookModuleFilename(hookModule.GetStringValue())))
                    numHookModulesLoaded += 1;
            }
        }

        if (0 == numHookModulesLoaded)
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, L"No hook modules are loaded; no hooks have been set for process \"%s\" (PID %d).", Strings::kStrExecutableBaseName.data(), GetProcessId(GetCurrentProcess()));
    }
    else
    {
        Message::Output(EMessageSeverity::MessageSeverityError, LibraryInterface::GetConfigurationErrorMessage().data());
    }
}
