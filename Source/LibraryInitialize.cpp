/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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
        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Attempting to loaded hook module %s."), (TCHAR*)hookModuleFileName);
        const HMODULE hookModule = LoadLibrary(hookModuleFileName);

        if (NULL == hookModule)
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, _T("System Error %d - Failed to load hook module %s."), GetLastError(), (TCHAR*)hookModuleFileName);
            return false;
        }

        const THookModuleInitProc initProc = (THookModuleInitProc)GetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName);

        if (NULL == initProc)
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, _T("System Error %d - Failed to locate required procedure in hook module %s."), GetLastError(), (TCHAR*)hookModuleFileName);
            return false;
        }

        initProc(GetHookConfigInterface());

        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Successfully loaded hook module %s."), (TCHAR*)hookModuleFileName);
        return true;
    }
}
