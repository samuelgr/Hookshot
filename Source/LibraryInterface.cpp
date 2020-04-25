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
#include "InternalHook.h"
#include "LibraryInterface.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"
#include "X86Instruction.h"

#include <memory>
#include <mutex>
#include <string_view>


namespace Hookshot
{
    namespace LibraryInterface
    {
        // -------- INTERNAL TYPES ----------------------------------------- //

        /// Function signature for the hook module initialization function.
        typedef void(__fastcall* THookModuleInitProc)(IHookshot*);


        // -------- INTERNAL VARIABLES ------------------------------------- //
        // See "LibraryInterface.h" for documentation.

        /// Configuration object.
        /// Holds the settings that were read from the Hookshot configuration file.
        static Configuration::Configuration configuration(std::make_unique<HookshotConfigReader>());

        /// Single hook configuration interface object.
        static HookStore hookStore;


        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Attempts to load and initialize the named hook module.
        /// Useful if hooks to be set are contained in an external hook module.
        /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
        /// @return `true` on success, `false` on failure.
        static bool LoadHookModule(std::wstring_view hookModuleFileName)
        {
            Message::OutputFormatted(Message::ESeverity::Info, L"%s - Attempting to load hook module.", hookModuleFileName.data());
            const HMODULE hookModule = Protected::Windows_LoadLibrary(hookModuleFileName.data());

            if (nullptr == hookModule)
            {
                Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to load hook module: %s", hookModuleFileName.data(), Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).c_str());
                return false;
            }

            const THookModuleInitProc initProc = (THookModuleInitProc)Protected::Windows_GetProcAddress(hookModule, Strings::kStrHookLibraryInitFuncName.data());

            if (nullptr == initProc)
            {
                Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to locate required procedure in hook module: %s", hookModuleFileName.data(), Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).c_str());
                return false;
            }

            initProc(GetHookshotInterfacePointer());

            Message::OutputFormatted(Message::ESeverity::Info, L"%s - Successfully loaded hook module.", hookModuleFileName.data());
            return true;
        }

        /// Attempts to load the specified library, which is not to be treated as a hook module.
        /// @param [in] injectOnlyLibraryFileName File name of library to load.
        /// @return `true` on success, `false` on failure.
        static bool LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName)
        {
            Message::OutputFormatted(Message::ESeverity::Info, L"%s - Attempting to load library.", injectOnlyLibraryFileName.data());
            const HMODULE hookModule = Protected::Windows_LoadLibrary(injectOnlyLibraryFileName.data());

            if (nullptr == hookModule)
            {
                Message::OutputFormatted(Message::ESeverity::Warning, L"%s - Failed to load library: %s", injectOnlyLibraryFileName.data(), Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).c_str());
                return false;
            }

            Message::OutputFormatted(Message::ESeverity::Info, L"%s - Successfully loaded library.", injectOnlyLibraryFileName.data());
            return true;
        }

        /// Attempts to load and initialize whatever hook modules are specified in the configuration file.
        /// @return Number of hook modules successfully loaded.
        static int LoadConfiguredHookModules(void)
        {
            int numHookModulesLoaded = 0;

            Message::Output(Message::ESeverity::Info, L"Loading hook modules specified in the configuration file.");

            if (true == IsConfigurationDataValid())
            {
                auto hookModulesToLoad = GetConfigurationData().SectionsContaining(Strings::kStrConfigurationSettingNameHookModule);

                for (auto& sectionsWithHookModules : *hookModulesToLoad)
                {
                    for (auto& hookModule : sectionsWithHookModules.name.Values())
                    {
                        if (true == LoadHookModule(Strings::HookModuleFilename(hookModule.GetStringValue())))
                            numHookModulesLoaded += 1;
                    }
                }
            }

            return numHookModulesLoaded;
        }

        /// Attempts to load and initialize hook modules according to default behavior (i.e. all hook modules in the same directory as the executable).
        /// @return Number of hook modules successfully loaded.
        static int LoadDefaultHookModules(void)
        {
            int numHookModulesLoaded = 0;

            Message::Output(Message::ESeverity::Info, L"Loading all hook modules in the same directory as the executable.");

            const std::wstring hookModuleSearchString = Strings::HookModuleFilename(L"*");
            WIN32_FIND_DATA hookModuleFileData;
            HANDLE hookModuleFind = Protected::Windows_FindFirstFileEx(hookModuleSearchString.c_str(), FindExInfoBasic, &hookModuleFileData, FindExSearchNameMatch, NULL, 0);
            BOOL moreHookModulesExist = (INVALID_HANDLE_VALUE != hookModuleFind);

            TemporaryBuffer<wchar_t> hookModuleFileName;
            wcscpy_s(hookModuleFileName, hookModuleFileName.Count(), Strings::kStrExecutableDirectoryName.data());

            while (TRUE == moreHookModulesExist)
            {
                wcscpy_s(&hookModuleFileName[Strings::kStrExecutableDirectoryName.length()], hookModuleFileName.Count() - Strings::kStrExecutableDirectoryName.length(), hookModuleFileData.cFileName);

                if (true == LoadHookModule(&hookModuleFileName[0]))
                    numHookModulesLoaded += 1;

                moreHookModulesExist = Protected::Windows_FindNextFile(hookModuleFind, &hookModuleFileData);
            }

            if (INVALID_HANDLE_VALUE != hookModuleFind)
                Protected::Windows_FindClose(hookModuleFind);

            return numHookModulesLoaded;
        }


        // -------- FUNCTIONS ---------------------------------------------- //
        // See "LibraryInterface.h" for documentation.

        bool DoesConfigurationFileExist(void)
        {
            return (Configuration::EFileReadResult::FileNotFound != configuration.GetFileReadResult());
        }

        // --------

        void EnableLogIfConfigured(void)
        {
            if (true == IsConfigurationDataValid())
            {
                if (true == GetConfigurationData().SectionNamePairExists(Configuration::kSectionNameGlobal, Strings::kStrConfigurationSettingNameLogLevel))
                {
                    const int64_t requestedSeverity = GetConfigurationData()[Configuration::kSectionNameGlobal][Strings::kStrConfigurationSettingNameLogLevel].FirstValue().GetIntegerValue();

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

        const Configuration::ConfigurationData& GetConfigurationData(void)
        {
            return configuration.GetData();
        }

        // --------

        std::wstring_view GetConfigurationErrorMessage(void)
        {
            return configuration.GetReadErrorMessage();
        }

        // --------

        IHookshot* GetHookshotInterfacePointer(void)
        {
            return &hookStore;
        }

        // --------

        bool Initialize(const ELoadMethod loadMethod)
        {
            static volatile bool isInitialized = false;

            if (false == isInitialized)
            {
                static std::mutex initializeMutex;
                std::lock_guard<std::mutex> lock(initializeMutex);

                if (false == isInitialized)
                {
                    Globals::SetHookshotLoadMethod(loadMethod);
                    X86Instruction::Initialize();

                    configuration.ReadConfigurationFile(Strings::kStrHookshotConfigurationFilename);

                    if (true == DoesConfigurationFileExist())
                    {
                        if (false == IsConfigurationDataValid())
                            Message::Output(Message::ESeverity::Error, GetConfigurationErrorMessage().data());
                    }
                    else
                    {
                        Message::Output(Message::ESeverity::Warning, GetConfigurationErrorMessage().data());
                    }

                    EnableLogIfConfigured();

                    if (ELoadMethod::Injected == loadMethod)
                        SetAllInternalHooks();

                    isInitialized = true;
                    return true;
                }
            }

            return false;
        }

        // --------

        bool IsConfigurationDataValid(void)
        {
            return (Configuration::EFileReadResult::Success == configuration.GetFileReadResult());
        }

        // --------

        int LoadHookModules(void)
        {
            bool useConfigurationFileHookModules = false;

            if (true == DoesConfigurationFileExist())
            {
                // If a configuration file is present, load the hook modules it specifies unless it specifically requests the default behavior.
                useConfigurationFileHookModules = true;

                if (true == IsConfigurationDataValid())
                {
                    if (true == GetConfigurationData().SectionNamePairExists(Configuration::kSectionNameGlobal, Strings::kStrConfigurationSettingNameUseConfiguredHookModules))
                    {
                        if (false == GetConfigurationData()[Configuration::kSectionNameGlobal][Strings::kStrConfigurationSettingNameUseConfiguredHookModules].FirstValue().GetBooleanValue())
                        {
                            useConfigurationFileHookModules = false;
                        }
                    }
                }
            }

            if (true == useConfigurationFileHookModules)
                return LoadConfiguredHookModules();
            else
                return LoadDefaultHookModules();
        }

        // --------

        int LoadInjectOnlyLibraries(void)
        {
            int numInjectOnlyLibrariesLoaded = 0;

            if (true == IsConfigurationDataValid())
            {
                auto injectOnlyLibrariesToLoad = GetConfigurationData().SectionsContaining(Strings::kStrConfigurationSettingNameInject);

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
    }
}
