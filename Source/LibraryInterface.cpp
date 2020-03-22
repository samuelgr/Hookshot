/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file LibraryInterface.cpp
 *   Implementation of support functionality for Hookshot's library interface.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Configuration.h"
#include "HookshotConfigReader.h"
#include "HookStore.h"
#include "Globals.h"
#include "HookStore.h"
#include "InjectLanding.h"
#include "LibraryInterface.h"
#include "Message.h"
#include "Strings.h"
#include "X86Instruction.h"

#include <hookshot.h>
#include <memory>
#include <string_view>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "LibraryInterface.h" for documentation.

    Configuration::Configuration LibraryInterface::configuration(std::make_unique<HookshotConfigReader>());

    HookStore LibraryInterface::hookConfig;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "LibraryInterface.h" for documentation.

    void LibraryInterface::Initialize(void)
    {
        static bool isInitialized = false;
        
        if (false == isInitialized)
        {
            X86Instruction::Initialize();
            
            if (false == IsConfigurationDataValid())
                configuration.ReadConfigurationFile(Strings::kStrHookshotConfigurationFilename);

            isInitialized = true;
        }
    }

    // --------

    bool LibraryInterface::LoadHookModule(std::wstring_view hookModuleFileName)
    {
        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, L"%s - Attempting to load hook module.", hookModuleFileName.data());
        const HMODULE hookModule = LoadLibrary(hookModuleFileName.data());

        if (NULL == hookModule)
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, L"%s - Failed to load hook module (system error %d).", hookModuleFileName.data(), GetLastError());
            return false;
        }

        const THookModuleInitProc initProc = (THookModuleInitProc)GetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName.data());

        if (NULL == initProc)
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, L"%s - Failed to locate required procedure in hook module (system error %d).", hookModuleFileName.data(), GetLastError());
            return false;
        }

        initProc(GetHookConfigInterface());

        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, L"%s - Successfully loaded hook module.", hookModuleFileName.data());
        return true;
    }
}
