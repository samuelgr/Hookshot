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
#include "HookResult.h"
#include "Inject.h"
#include "Message.h"
#include "StringUtilities.h"
#include "TemporaryBuffers.h"

#include <cstddef>
#include <hookshot.h>
#include <psapi.h>

using namespace Hookshot;


// -------- INTERNAL TYPES ------------------------------------------------- //

/// Function signature for the hook library initialization function.
typedef int(APIENTRY* THookModuleInitProc)(IHookConfig*);


// -------- INTERNAL FUNCTIONS --------------------------------------------- //

/// Attempts to load and initialize the named hook module.
/// @param [in] hookModuleFileName File name of the hook module to load and initialize.
/// @return Indicator of the result of the interaction.
static EHookResult InjectLandingLoadAndInitializeHookModule(const TCHAR* hookModuleFileName)
{
    const HMODULE hookModule = LoadLibrary(hookModuleFileName);

    if (NULL == hookModule)
    {
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_CANNOT_LOAD_HOOK_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
        return EHookResult::HookResultCannotLoadHookModule;
    }

    const THookModuleInitProc initProc = (THookModuleInitProc)GetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName);

    if (NULL == initProc)
    {
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_MALFORMED_HOOKSHOT_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
        return EHookResult::HookResultMalformedHookModule;
    }

    const int initProcResult = initProc(NULL);

    if (0 != initProcResult)
    {
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_HOOK_MODULE_FAILURE_FORMAT, initProcResult, (TCHAR*)hookModuleFileName);
        return EHookResult::HookResultInitializationFailed;
    }

    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_INFO_HOOK_MODULE_SUCCESS_FORMAT, (TCHAR*)hookModuleFileName);
    return EHookResult::HookResultSuccess;
}


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

extern "C" EHookResult APIENTRY InjectLandingSetHooks(void)
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
        return EHookResult::HookResultInsufficientMemoryFilenames;

    if (EHookResult::HookResultSuccess == InjectLandingLoadAndInitializeHookModule(hookModuleFileName))
        return EHookResult::HookResultSuccess;

    if (false == Strings::FillHookModuleFilenameCommon(hookModuleFileName, hookModuleFileName.Count()))
        return EHookResult::HookResultInsufficientMemoryFilenames;

    if (EHookResult::HookResultSuccess == InjectLandingLoadAndInitializeHookModule(hookModuleFileName))
        return EHookResult::HookResultSuccess;

    Message::OutputFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_NO_HOOK_MODULE);
    return EHookResult::HookResultFailure;
}
