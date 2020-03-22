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

    void LibraryInterface::EnableLogIfConfigured(void)
    {
        if (true == IsConfigurationDataValid())
        {
            if (true == GetConfigurationData().SectionNamePairExists(Configuration::ConfigurationData::kSectionNameGlobal, Strings::kStrConfigurationSettingNameLogLevel))
            {
                const EMessageSeverity configuredSeverity = (EMessageSeverity)(GetConfigurationData().SectionByName(Configuration::ConfigurationData::kSectionNameGlobal).NameByName(Strings::kStrConfigurationSettingNameLogLevel).FirstValue().GetIntegerValue() + EMessageSeverity::MessageSeverityForcedInteractiveBoundaryValue + 1);
                
                Message::CreateAndEnableLogFile();
                Message::SetMinimumSeverityForOutput(configuredSeverity);
            }
        }
    }

    // --------
    
    void LibraryInterface::Initialize(void)
    {
        static bool isInitialized = false;
        
        if (false == isInitialized)
        {
            X86Instruction::Initialize();
            
            if (false == IsConfigurationDataValid())
                configuration.ReadConfigurationFile(Strings::kStrHookshotConfigurationFilename);

            EnableLogIfConfigured();

            isInitialized = true;
        }
    }

    // --------

    int LibraryInterface::LoadConfiguredHookModules(void)
    {
        int numHookModulesLoaded = 0;

        if (true == IsConfigurationDataValid())
        {
            auto hookModulesToLoad = GetConfigurationData().SectionsWithName(Strings::kStrConfigurationSettingNameHookModule);

            for (auto& sectionsWithHookModules : *hookModulesToLoad)
            {
                for (auto& hookModule : sectionsWithHookModules.name.Values())
                {
                    if (true == LoadHookModule(Strings::MakeHookModuleFilename(hookModule.GetStringValue())))
                        numHookModulesLoaded += 1;
                }
            }
        }

        return numHookModulesLoaded;
    }

    // --------

    int LibraryInterface::LoadConfiguredInjectOnlyLibraries(void)
    {
        int numInjectOnlyLibrariesLoaded = 0;

        if (true == IsConfigurationDataValid())
        {
            auto injectOnlyLibrariesToLoad = GetConfigurationData().SectionsWithName(Strings::kStrConfigurationSettingNameInject);

            for (auto& sectionsWithInjectOnlyLibraries : *injectOnlyLibrariesToLoad)
            {
                for (auto& injectOnlyLibrary : sectionsWithInjectOnlyLibraries.name.Values())
                {
                    if (true == LoadInjectOnlyLibrary(injectOnlyLibrary.GetStringValue()))
                        numInjectOnlyLibrariesLoaded += 1;
                }
            }
        }

        return numInjectOnlyLibrariesLoaded;
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

    // --------

    bool LibraryInterface::LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName)
    {
        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, L"%s - Attempting to load library.", injectOnlyLibraryFileName.data());
        const HMODULE hookModule = LoadLibrary(injectOnlyLibraryFileName.data());

        if (NULL == hookModule)
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityWarning, L"%s - Failed to load library (system error %d).", injectOnlyLibraryFileName.data(), GetLastError());
            return false;
        }

        Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, L"%s - Successfully loaded library.", injectOnlyLibraryFileName.data());
        return true;
    }
}
