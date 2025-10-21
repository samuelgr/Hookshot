/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file LibraryInterface.cpp
 *   Implementation of support functionality for Hookshot's library interface.
 **************************************************************************************************/

#include "LibraryInterface.h"

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

#include <Infra/Core/Configuration.h>
#include <Infra/Core/Message.h>
#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "DependencyProtect.h"
#include "Globals.h"
#include "HookshotTypes.h"
#include "HookStore.h"
#include "InjectLanding.h"
#include "InternalHook.h"
#include "RemoteProcessInjector.h"
#include "Strings.h"
#include "X86Instruction.h"

namespace Hookshot
{
  namespace LibraryInterface
  {
    /// Function signature for the hook module initialization function.
    using THookModuleInitProc = void(__fastcall*)(IHookshot*);

    /// Complete implementation of the Hookshot interface. The parts of the implementation supplied
    /// here do not directly involve hooks or hook storage.
    class HookshotImpl : public HookStore
    {
      EResult __fastcall InjectNewSuspendedProcess(const PROCESS_INFORMATION& processInfo) override
      {
        const auto internalResult = RemoteProcessInjector::InjectProcess(
            processInfo.hProcess, processInfo.hThread, false, false);

        // TODO: This return code needs to be improved.
        return (
            (EInjectResult::Success == internalResult) ? EResult::Success : EResult::FailInternal);
      }
    };

    /// Singleton Hookshot interface implementation object.
    static HookshotImpl hookshotImpl;

    /// Determines which directory should be checked when loading hook modules. This defaults to the
    /// executable directory but can instead be configured for the Hookshot directory, in case the
    /// two are different. There is no trailing backslash on the path returned by this function.
    /// @return Name of the directory in which to look for hook modules.
    static std::wstring_view HookModuleDirectoryName(void)
    {
      static const bool loadHookModulesFromHookshotDirectory =
          Globals::GetConfigurationData()
              [Infra::Configuration::kSectionNameGlobal]
              [Strings::kStrConfigurationSettingNameLoadHookModulesFromHookshotDirectory]
                  .ValueOr(false);

      return (
          loadHookModulesFromHookshotDirectory ? Infra::ProcessInfo::GetThisModuleDirectoryName()
                                               : Infra::ProcessInfo::GetExecutableDirectoryName());
    }

    /// Obtains pointers to all of the relevant configuration settings for the currently-running
    /// executable. Currently, this function checks both the global section and the
    /// executable-specific section for configuration settings that match the specified name.
    /// @param [in] configSettingName Name of the configuration setting for which to check.
    /// @return Vector of read-only pointers to all relevant configuration setting names.
    static std::vector<const Infra::Configuration::Name*> RelevantConfigurationSettings(
        std::wstring_view configSettingName)
    {
      const auto& configData = Globals::GetConfigurationData();

      std::vector<const Infra::Configuration::Name*> relevantConfigSettings;
      relevantConfigSettings.reserve(2);

      if (configData.Contains(Infra::Configuration::kSectionNameGlobal, configSettingName))
      {
        relevantConfigSettings.push_back(
            &configData[Infra::Configuration::kSectionNameGlobal][configSettingName]);
      }

      if (configData.Contains(Infra::ProcessInfo::GetExecutableBaseName(), configSettingName))
      {
        relevantConfigSettings.push_back(
            &configData[Infra::ProcessInfo::GetExecutableBaseName()][configSettingName]);
      }

      return relevantConfigSettings;
    }

    /// Attempts to load and initialize the named hook module. Useful if hooks to be set are
    /// contained in an external hook module.
    /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
    /// @return `true` on success, `false` on failure.
    static bool LoadHookModule(std::wstring_view hookModuleFileName)
    {
      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"%s - Attempting to load hook module.",
          hookModuleFileName.data());
      const HMODULE hookModule = Protected::Windows_LoadLibraryW(hookModuleFileName.data());

      if (nullptr == hookModule)
      {
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Warning,
            L"%s - Failed to load hook module: %s",
            hookModuleFileName.data(),
            Infra::Strings::FromSystemErrorCode(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      const THookModuleInitProc initProc = (THookModuleInitProc)Protected::Windows_GetProcAddress(
          hookModule, Strings::kStrHookLibraryInitFuncName.data());

      if (nullptr == initProc)
      {
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Warning,
            L"%s - Failed to locate required procedure in hook module: %s",
            hookModuleFileName.data(),
            Infra::Strings::FromSystemErrorCode(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      initProc(GetHookshotInterfacePointer());

      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"%s - Successfully loaded hook module.",
          hookModuleFileName.data());
      return true;
    }

    /// Attempts to load the specified library, which is not to be treated as a hook module.
    /// @param [in] injectOnlyLibraryFileName File name of library to load.
    /// @return `true` on success, `false` on failure.
    static bool LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName)
    {
      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"%s - Attempting to load library.",
          injectOnlyLibraryFileName.data());
      const HMODULE hookModule = Protected::Windows_LoadLibraryW(injectOnlyLibraryFileName.data());

      if (nullptr == hookModule)
      {
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Warning,
            L"%s - Failed to load library: %s.",
            injectOnlyLibraryFileName.data(),
            Infra::Strings::FromSystemErrorCode(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"%s - Successfully loaded library.",
          injectOnlyLibraryFileName.data());
      return true;
    }

    /// Attempts to load and initialize whatever hook modules are specified in the configuration
    /// file.
    /// @return Number of hook modules successfully loaded.
    static int LoadConfiguredHookModules(void)
    {
      int numHookModulesLoaded = 0;

      Infra::Message::Output(
          Infra::Message::ESeverity::Info,
          L"Loading hook modules specified in the configuration file.");

      for (const auto& configuredHookModuleSource :
           RelevantConfigurationSettings(Strings::kStrConfigurationSettingNameHookModule))
      {
        for (auto& hookModule : configuredHookModuleSource->Values())
        {
          if (true ==
              LoadHookModule(Strings::HookModuleFilename(hookModule, HookModuleDirectoryName())))
            numHookModulesLoaded += 1;
        }
      }

      return numHookModulesLoaded;
    }

    /// Attempts to load and initialize hook modules according to default behavior (i.e. all hook
    /// modules in the same directory as the executable).
    /// @return Number of hook modules successfully loaded.
    static int LoadDefaultHookModules(void)
    {
      int numHookModulesLoaded = 0;
      const std::wstring_view hookModuleDirectory = HookModuleDirectoryName();

      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"Looking in \"%.*s\" and loading all hook modules found there.",
          static_cast<int>(hookModuleDirectory.size()),
          hookModuleDirectory.data());

      const Infra::TemporaryString hookModuleSearchString =
          Strings::HookModuleFilename(L"*", hookModuleDirectory);
      WIN32_FIND_DATA hookModuleFileData{};
      HANDLE hookModuleFind = Protected::Windows_FindFirstFileExW(
          hookModuleSearchString.AsCString(),
          FindExInfoBasic,
          &hookModuleFileData,
          FindExSearchNameMatch,
          NULL,
          0);
      BOOL moreHookModulesExist = (INVALID_HANDLE_VALUE != hookModuleFind);

      Infra::TemporaryString hookModuleFileName;

      while (TRUE == moreHookModulesExist)
      {
        hookModuleFileName.Clear();
        hookModuleFileName << hookModuleDirectory << L"\\" << hookModuleFileData.cFileName;
        if (true == LoadHookModule(hookModuleFileName.AsCString())) numHookModulesLoaded += 1;

        moreHookModulesExist =
            Protected::Windows_FindNextFileW(hookModuleFind, &hookModuleFileData);
      }

      if (INVALID_HANDLE_VALUE != hookModuleFind) Protected::Windows_FindClose(hookModuleFind);

      return numHookModulesLoaded;
    }

    IHookshot* GetHookshotInterfacePointer(void)
    {
      return &hookshotImpl;
    }

    bool Initialize(Globals::ELoadMethod loadMethod)
    {
      bool initializeResult = false;

      static std::once_flag initializeFlag;
      std::call_once(
          initializeFlag,
          [loadMethod, &initializeResult]()
          {
            Globals::Initialize(loadMethod);
            X86Instruction::Initialize();

            if (Globals::ELoadMethod::Injected == loadMethod) SetAllInternalHooks();

            initializeResult = true;
          });

      return initializeResult;
    }

    int LoadHookModules(void)
    {
      const auto& configData = Globals::GetConfigurationData();
      bool useConfigurationFileHookModules = false;

      // If a configuration file is non-empty (meaning it must be both present and valid), default
      // behavior is to load only the hook modules configured in the configuration file.
      if (false == configData.Empty())
      {
        useConfigurationFileHookModules =
            configData[Infra::Configuration::kSectionNameGlobal]
                      [Strings::kStrConfigurationSettingNameUseConfiguredHookModules]
                          .ValueOr(true);
      }

      if (true == useConfigurationFileHookModules)
        return LoadConfiguredHookModules();
      else
        return LoadDefaultHookModules();
    }

    int LoadInjectOnlyLibraries(void)
    {
      const auto& configData = Globals::GetConfigurationData();
      int numInjectOnlyLibrariesLoaded = 0;

      for (const auto& configuredInjectOnlyLibrarySource :
           RelevantConfigurationSettings(Strings::kStrConfigurationSettingNameInject))
      {
        for (auto& injectOnlyLibrary : configuredInjectOnlyLibrarySource->Values())
          if (true == LoadInjectOnlyLibrary(injectOnlyLibrary)) numInjectOnlyLibrariesLoaded += 1;
      }

      return numInjectOnlyLibrariesLoaded;
    }
  } // namespace LibraryInterface
} // namespace Hookshot
