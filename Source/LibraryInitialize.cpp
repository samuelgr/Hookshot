/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file LibraryInitialize.cpp
 *   Implementation of library initialization functions.
 *****************************************************************************/

#include "ApiWindows.h"
#include "HookStore.h"
#include "Globals.h"
#include "HookStore.h"
#include "InjectLanding.h"
#include "LibraryInitialize.h"
#include "Message.h"
#include "Strings.h"
#include "X86Instruction.h"

#include <hookshot.h>
#include <tchar.h>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "LibraryInitialize.h" for documentation.

    HookStore LibraryInitialize::hookConfig;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "LibraryInitialize.h" for documentation.

    void LibraryInitialize::CommonInitialization(void)
    {
        static bool isInitialized = false;
        
        if (false == isInitialized)
        {
            X86Instruction::Initialize();

            isInitialized = true;
        }
    }

    // --------

    bool LibraryInitialize::LoadHookModule(const TCHAR* hookModuleFileName)
    {
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_INFO_ATTEMPT_LOAD_HOOK_MODULE_FORMAT, (TCHAR*)hookModuleFileName);
        const HMODULE hookModule = LoadLibrary(hookModuleFileName);

        if (NULL == hookModule)
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_CANNOT_LOAD_HOOK_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
            return false;
        }

        const THookModuleInitProc initProc = (THookModuleInitProc)GetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName);

        if (NULL == initProc)
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_WARN_MALFORMED_HOOKSHOT_MODULE_FORMAT, GetLastError(), (TCHAR*)hookModuleFileName);
            return false;
        }

        initProc(GetHookConfigInterface());

        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_INFO_HOOK_MODULE_SUCCESS_FORMAT, (TCHAR*)hookModuleFileName);
        return true;
    }
}
