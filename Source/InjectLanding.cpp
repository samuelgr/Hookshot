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
#include "Strings.h"
#include "TemporaryBuffers.h"

#include <cstddef>
#include <psapi.h>

using namespace Hookshot;


// -------- FUNCTIONS ------------------------------------------------------ //
// See "InjectLanding.h" for documentation.

extern "C" void APIENTRY InjectLandingCleanup(const SInjectData* const injectData)
{
    // Copy the array of addresses to free.
    // This is needed because freeing any one of these could invalidate the injectData pointer.
    void* cleanupBaseAddress[_countof(injectData->cleanupBaseAddress)];
    memcpy((void*)cleanupBaseAddress, (void*)injectData->cleanupBaseAddress, sizeof(cleanupBaseAddress));
    
    // Free all requested buffers.
    for (size_t i = 0; i < _countof(cleanupBaseAddress); ++i)
    {
        if (NULL != cleanupBaseAddress[i])
            VirtualFree(cleanupBaseAddress[i], 0, MEM_RELEASE);
    }
}

// --------

extern "C" void APIENTRY InjectLandingHandleSetHooksResult(EHookResult result)
{
    if (EHookResult::HookResultSuccess != result)
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_FAILED_SET_HOOKS_FORMAT, result, GetLastError());
}

// --------

extern "C" EHookResult APIENTRY InjectLandingSetHooks(void)
{
#ifdef HOOKSHOT_DEBUG
    // For debugging purposes emit a message indicating the current process ID to facilitate attaching a debugger.
    if (FALSE == IsDebuggerPresent())
    {
        TemporaryBuffer<TCHAR> executableBaseName;
        GetModuleBaseName(GetCurrentProcess(), NULL, executableBaseName, executableBaseName.Count());
        
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_DEBUG_PID_FORMAT, (TCHAR*)executableBaseName, GetProcessId(GetCurrentProcess()));
    }
#endif

    // Obtain the module base name for this library.
    // This will be appended to both base names that need to be tried to figure out 
    TemporaryBuffer<TCHAR> hookModuleExtension;
    GetModuleBaseName(GetCurrentProcess(), Globals::GetInstanceHandle(), hookModuleExtension, hookModuleExtension.Count());
    
    // All attempts will be looking for files in the same directory as the executable.
    TemporaryBuffer<TCHAR> hookModuleFileName;
    GetModuleFileName(NULL, hookModuleFileName, hookModuleFileName.Count());

    // First, try the name of the executable plus the name of this module.
    // This is in case executable-specific hooks exist.
    if (0 != _tcscat_s(hookModuleFileName, hookModuleFileName.Count(), _T(".")))
        return EHookResult::HookResultInsufficientMemoryFilenames;
    
    if (0 != _tcscat_s(hookModuleFileName, hookModuleFileName.Count(), hookModuleExtension))
        return EHookResult::HookResultInsufficientMemoryFilenames;
    
    // Attempt to load the module.
    {
        const HMODULE hookModule = LoadLibrary(hookModuleFileName);

        if (NULL == hookModule)
        {
            const DWORD lastError = GetLastError();
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_CANNOT_LOAD_HOOK_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
            SetLastError(lastError);
        }
        else
        {
            // Module loaded successfully.
            // Do other stuff here.
            return EHookResult::HookResultSuccess;
        }
    }

    // Second, try a common file in the same directory as the executable.
    //
    {
        TCHAR* const lastBackslash = _tcsrchr(hookModuleFileName, _T('\\'));

        if (NULL == lastBackslash)
            hookModuleFileName[0] = _T('\0');
        else
            lastBackslash[1] = _T('\0');
    }

    if (0 != _tcscat_s(hookModuleFileName, hookModuleFileName.Count(), Strings::kStrCommonHookModuleBaseName))
        return EHookResult::HookResultInsufficientMemoryFilenames;

    if (0 != _tcscat_s(hookModuleFileName, hookModuleFileName.Count(), _T(".")))
        return EHookResult::HookResultInsufficientMemoryFilenames;

    if (0 != _tcscat_s(hookModuleFileName, hookModuleFileName.Count(), hookModuleExtension))
        return EHookResult::HookResultInsufficientMemoryFilenames;

    // Attempt to load the module.
    {
        const HMODULE hookModule = LoadLibrary(hookModuleFileName);

        if (NULL == hookModule)
        {
            const DWORD lastError = GetLastError();
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_CANNOT_LOAD_HOOK_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
            SetLastError(lastError);
        }
        else
        {
            // Module loaded successfully.
            // Do other stuff here.
            return EHookResult::HookResultSuccess;
        }
    }

    // At this point, both attempts have failed.
    // No hooks file could be loaded.
    return EHookResult::HookResultCannotLoadHookModule;
}
