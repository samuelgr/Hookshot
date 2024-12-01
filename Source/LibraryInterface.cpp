/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file LibraryInterface.cpp
 *   Implementation of support functionality for Hookshot's library interface.
 **************************************************************************************************/

#include "LibraryInterface.h"

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

#include "DependencyProtect.h"
#include "Globals.h"
#include "HookshotTypes.h"
#include "HookStore.h"
#include "InjectLanding.h"
#include "InternalHook.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"
#include "X86Instruction.h"

namespace Hookshot
{
  namespace LibraryInterface
  {
    /// Function signature for the hook module initialization function.
    using THookModuleInitProc = void(__fastcall*)(IHookshot*);

    /// Single hook configuration interface object.
    static HookStore hookStore;

    /// Determines which directory should be checked when loading hook modules. This defaults to the
    /// executable directory but can instead be configured for the Hookshot directory, in case the
    /// two are different.
    /// @return Name of the directory in which to look for hook modules.
    static std::wstring_view HookModuleDirectoryName(void)
    {
      static const bool loadHookModulesFromHookshotDirectory =
          Globals::GetConfigurationData()
              .GetFirstBooleanValue(
                  Configuration::kSectionNameGlobal,
                  Strings::kStrConfigurationSettingNameLoadHookModulesFromHookshotDirectory)
              .value_or(false);

      return (
          loadHookModulesFromHookshotDirectory ? Strings::kStrHookshotDirectoryName
                                               : Strings::kStrExecutableDirectoryName);
    }

    /// Obtains pointers to all of the relevant configuration settings for the currently-running
    /// executable. Currently, this function checks both the global section and the
    /// executable-specific section for configuration settings that match the specified name.
    /// @param [in] configSettingName Name of the configuration setting for which to check.
    /// @return Vector of read-only pointers to all relevant configuration setting names.
    static std::vector<const Configuration::Name*> RelevantConfigurationSettings(
        std::wstring_view configSettingName)
    {
      const Configuration::ConfigurationData& configData = Globals::GetConfigurationData();

      std::vector<const Configuration::Name*> relevantConfigSettings;
      relevantConfigSettings.reserve(2);

      if (false == configData.HasReadErrors())
      {
        if (configData.SectionNamePairExists(Configuration::kSectionNameGlobal, configSettingName))
        {
          relevantConfigSettings.push_back(
              &configData[Configuration::kSectionNameGlobal][configSettingName]);
        }

        if (configData.SectionNamePairExists(Strings::kStrExecutableBaseName, configSettingName))
        {
          relevantConfigSettings.push_back(
              &configData[Strings::kStrExecutableBaseName][configSettingName]);
        }
      }

      return relevantConfigSettings;
    }

    /// Attempts to load and initialize the named hook module. Useful if hooks to be set are
    /// contained in an external hook module.
    /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
    /// @return `true` on success, `false` on failure.
    static bool LoadHookModule(std::wstring_view hookModuleFileName)
    {
      Message::OutputFormatted(
          Message::ESeverity::Info,
          L"%s - Attempting to load hook module.",
          hookModuleFileName.data());
      const HMODULE hookModule = Protected::Windows_LoadLibrary(hookModuleFileName.data());

      if (nullptr == hookModule)
      {
        Message::OutputFormatted(
            Message::ESeverity::Warning,
            L"%s - Failed to load hook module: %s.",
            hookModuleFileName.data(),
            Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      const THookModuleInitProc initProc = (THookModuleInitProc)Protected::Windows_GetProcAddress(
          hookModule, Strings::kStrHookLibraryInitFuncName.data());

      if (nullptr == initProc)
      {
        Message::OutputFormatted(
            Message::ESeverity::Warning,
            L"%s - Failed to locate required procedure in hook module: %s.",
            hookModuleFileName.data(),
            Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      initProc(GetHookshotInterfacePointer());

      Message::OutputFormatted(
          Message::ESeverity::Info,
          L"%s - Successfully loaded hook module.",
          hookModuleFileName.data());
      return true;
    }

    /// Attempts to load the specified library, which is not to be treated as a hook module.
    /// @param [in] injectOnlyLibraryFileName File name of library to load.
    /// @return `true` on success, `false` on failure.
    static bool LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName)
    {
      Message::OutputFormatted(
          Message::ESeverity::Info,
          L"%s - Attempting to load library.",
          injectOnlyLibraryFileName.data());
      const HMODULE hookModule = Protected::Windows_LoadLibrary(injectOnlyLibraryFileName.data());

      if (nullptr == hookModule)
      {
        Message::OutputFormatted(
            Message::ESeverity::Warning,
            L"%s - Failed to load library: %s.",
            injectOnlyLibraryFileName.data(),
            Strings::SystemErrorCodeString(Protected::Windows_GetLastError()).AsCString());
        return false;
      }

      Message::OutputFormatted(
          Message::ESeverity::Info,
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

      Message::Output(
          Message::ESeverity::Info, L"Loading hook modules specified in the configuration file.");

      for (const auto& configuredHookModuleSource :
           RelevantConfigurationSettings(Strings::kStrConfigurationSettingNameHookModule))
      {
        for (auto& hookModule : configuredHookModuleSource->Values())
        {
          if (true ==
              LoadHookModule(Strings::HookModuleFilename(
                  hookModule.GetStringValue(), HookModuleDirectoryName())))
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

      Message::OutputFormatted(
          Message::ESeverity::Info,
          L"Looking in \"%.*s\" and loading all hook modules found there.",
          static_cast<int>(hookModuleDirectory.size()),
          hookModuleDirectory.data());

      const TemporaryString hookModuleSearchString =
          Strings::HookModuleFilename(L"*", hookModuleDirectory);
      WIN32_FIND_DATA hookModuleFileData{};
      HANDLE hookModuleFind = Protected::Windows_FindFirstFileEx(
          hookModuleSearchString.AsCString(),
          FindExInfoBasic,
          &hookModuleFileData,
          FindExSearchNameMatch,
          NULL,
          0);
      BOOL moreHookModulesExist = (INVALID_HANDLE_VALUE != hookModuleFind);

      TemporaryBuffer<wchar_t> hookModuleFileName;
      wcscpy_s(
          hookModuleFileName.Data(), hookModuleFileName.Capacity(), hookModuleDirectory.data());

      while (TRUE == moreHookModulesExist)
      {
        wcscpy_s(
            &hookModuleFileName[hookModuleDirectory.length()],
            hookModuleFileName.Capacity() - hookModuleDirectory.length(),
            hookModuleFileData.cFileName);

        if (true == LoadHookModule(&hookModuleFileName[0])) numHookModulesLoaded += 1;

        moreHookModulesExist = Protected::Windows_FindNextFile(hookModuleFind, &hookModuleFileData);
      }

      if (INVALID_HANDLE_VALUE != hookModuleFind) Protected::Windows_FindClose(hookModuleFind);

      return numHookModulesLoaded;
    }

    IHookshot* GetHookshotInterfacePointer(void)
    {
      return &hookStore;
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
      const Configuration::ConfigurationData& configData = Globals::GetConfigurationData();
      bool useConfigurationFileHookModules = false;

      // If a configuration file is present, non-empty, and valid, load the hook modules it
      // specifies unless it specifically requests the default behavior.
      if ((false == configData.HasReadErrors()) && (false == configData.IsEmpty()))
      {
        useConfigurationFileHookModules = true;

        if (true ==
            configData.SectionNamePairExists(
                Configuration::kSectionNameGlobal,
                Strings::kStrConfigurationSettingNameUseConfiguredHookModules))
        {
          if (false ==
              configData[Configuration::kSectionNameGlobal]
                        [Strings::kStrConfigurationSettingNameUseConfiguredHookModules]
                            .GetFirstValue()
                            .GetBooleanValue())
          {
            useConfigurationFileHookModules = false;
          }
        }
      }

      if (true == useConfigurationFileHookModules)
        return LoadConfiguredHookModules();
      else
        return LoadDefaultHookModules();
    }

    int LoadInjectOnlyLibraries(void)
    {
      const Configuration::ConfigurationData& configData = Globals::GetConfigurationData();
      int numInjectOnlyLibrariesLoaded = 0;

      for (const auto& configuredInjectOnlyLibrarySource :
           RelevantConfigurationSettings(Strings::kStrConfigurationSettingNameInject))
      {
        for (auto& injectOnlyLibrary : configuredInjectOnlyLibrarySource->Values())
        {
          if (true == LoadInjectOnlyLibrary(injectOnlyLibrary.GetStringValue()))
            numInjectOnlyLibrariesLoaded += 1;
        }
      }

      return numInjectOnlyLibrariesLoaded;
    }
  } // namespace LibraryInterface
} // namespace Hookshot
