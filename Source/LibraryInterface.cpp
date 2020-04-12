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

#include "DependencyProtect.h"
#include "Configuration.h"
#include "Globals.h"
#include "HookshotConfigReader.h"
#include "HookshotTypes.h"
#include "HookStore.h"
#include "InjectLanding.h"
#include "LibraryInterface.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"
#include "X86Instruction.h"

#include <memory>
#include <string_view>


namespace Hookshot
{
    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Function signature for the hook module initialization function.
    typedef void(__fastcall* THookModuleInitProc)(IHookshot*);


    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "LibraryInterface.h" for documentation.

    Configuration::Configuration LibraryInterface::configuration(std::make_unique<HookshotConfigReader>());

    HookStore LibraryInterface::hookStore;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "LibraryInterface.h" for documentation.

    void LibraryInterface::EnableLogIfConfigured(void)
    {
        if (true == IsConfigurationDataValid())
        {
            if (true == GetConfigurationData().SectionNamePairExists(Configuration::ConfigurationData::kSectionNameGlobal, Strings::kStrConfigurationSettingNameLogLevel))
            {
                const int64_t requestedSeverity = GetConfigurationData().SectionByName(Configuration::ConfigurationData::kSectionNameGlobal).NameByName(Strings::kStrConfigurationSettingNameLogLevel).FirstValue().GetIntegerValue();

                // Offset the requested severity by subtracting 1 from it so that 0 = disabled, 1 = error, 2 = warning, etc.
                if (requestedSeverity > 0)
                {
                    const Message::ESeverity configureSeverity = (Message::ESeverity)(requestedSeverity - 1);

                    Message::CreateAndEnableLogFile();
                    Message::SetMinimumSeverityForOutput(configureSeverity);
                }
            }
        }
    }

    // --------
    
    void LibraryInterface::Initialize(void)
    {
        X86Instruction::Initialize();

        if (false == IsConfigurationDataValid())
            configuration.ReadConfigurationFile(Strings::kStrHookshotConfigurationFilename);

        EnableLogIfConfigured();
    }

    // --------

    int LibraryInterface::LoadConfiguredHookModules(void)
    {
        int numHookModulesLoaded = 0;

        if (true == DoesConfigurationFileExist())
        {
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
        }
        else
        {
            std::wstring hookModuleSearchString = Strings::MakeHookModuleFilename(L"*");
            
            WIN32_FIND_DATA hookModuleFileData;
            HANDLE hookModuleFind = Windows::ProtectedFindFirstFileEx(hookModuleSearchString.c_str(), FindExInfoBasic, &hookModuleFileData, FindExSearchNameMatch, NULL, 0);
            BOOL moreHookModulesExist = (INVALID_HANDLE_VALUE != hookModuleFind);

            TemporaryBuffer<wchar_t> hookModuleFileName;
            wcscpy_s(hookModuleFileName, hookModuleFileName.Count(), Strings::kStrExecutableDirectoryName.data());

            while (TRUE == moreHookModulesExist)
            {
                wcscpy_s(&hookModuleFileName[Strings::kStrExecutableDirectoryName.length()], hookModuleFileName.Count() - Strings::kStrExecutableDirectoryName.length(), hookModuleFileData.cFileName);

                if (true == LoadHookModule(&hookModuleFileName[0]))
                    numHookModulesLoaded += 1;

                moreHookModulesExist = Windows::ProtectedFindNextFile(hookModuleFind, &hookModuleFileData);
            }

            if (INVALID_HANDLE_VALUE != hookModuleFind)
                Windows::ProtectedFindClose(hookModuleFind);
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
        Message::OutputFormatted(Message::ESeverity::Info, L"%s - Attempting to load hook module.", hookModuleFileName.data());
        const HMODULE hookModule = Windows::ProtectedLoadLibrary(hookModuleFileName.data());

        if (nullptr == hookModule)
        {
            Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to load hook module (system error %d).", hookModuleFileName.data(), Windows::ProtectedGetLastError());
            return false;
        }

        const THookModuleInitProc initProc = (THookModuleInitProc)Windows::ProtectedGetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName.data());

        if (nullptr == initProc)
        {
            Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to locate required procedure in hook module (system error %d).", hookModuleFileName.data(), Windows::ProtectedGetLastError());
            return false;
        }

        initProc(GetHookshotInterfacePointer());

        Message::OutputFormatted(Message::ESeverity::Info, L"%s - Successfully loaded hook module.", hookModuleFileName.data());
        return true;
    }

    // --------

    bool LibraryInterface::LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName)
    {
        Message::OutputFormatted(Message::ESeverity::Info, L"%s - Attempting to load library.", injectOnlyLibraryFileName.data());
        const HMODULE hookModule = Windows::ProtectedLoadLibrary(injectOnlyLibraryFileName.data());

        if (nullptr == hookModule)
        {
            Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to load library (system error %d).", injectOnlyLibraryFileName.data(), Windows::ProtectedGetLastError());
            return false;
        }

        Message::OutputFormatted(Message::ESeverity::Info, L"%s - Successfully loaded library.", injectOnlyLibraryFileName.data());
        return true;
    }
}
