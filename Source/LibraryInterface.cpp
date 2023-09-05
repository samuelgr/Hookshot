/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file LibraryInterface.cpp
 *   Implementation of support functionality for Hookshot's library interface.
 **************************************************************************************************/

#include "LibraryInterface.h"

#include <memory>
#include <mutex>
#include <string_view>

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
      const Configuration::ConfigurationData& configData = Globals::GetConfigurationData();
      int numHookModulesLoaded = 0;

      Message::Output(
          Message::ESeverity::Info, L"Loading hook modules specified in the configuration file.");

      if (false == configData.HasErrors())
      {
        auto hookModulesToLoad = Globals::GetConfigurationData().SectionsContaining(
            Strings::kStrConfigurationSettingNameHookModule);

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

    /// Attempts to load and initialize hook modules according to default behavior (i.e. all hook
    /// modules in the same directory as the executable).
    /// @return Number of hook modules successfully loaded.
    static int LoadDefaultHookModules(void)
    {
      int numHookModulesLoaded = 0;

      Message::Output(
          Message::ESeverity::Info,
          L"Loading all hook modules in the same directory as the executable.");

      const TemporaryString hookModuleSearchString = Strings::HookModuleFilename(L"*");
      WIN32_FIND_DATA hookModuleFileData;
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
          hookModuleFileName.Data(),
          hookModuleFileName.Capacity(),
          Strings::kStrExecutableDirectoryName.data());

      while (TRUE == moreHookModulesExist)
      {
        wcscpy_s(
            &hookModuleFileName[Strings::kStrExecutableDirectoryName.length()],
            hookModuleFileName.Capacity() - Strings::kStrExecutableDirectoryName.length(),
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

      // If a configuration file is present and valid, load the hook modules it specifies unless it
      // specifically requests the default behavior.
      if (false == configData.HasErrors())
      {
        useConfigurationFileHookModules = true;

        if (true ==
            Globals::GetConfigurationData().SectionNamePairExists(
                Configuration::kSectionNameGlobal,
                Strings::kStrConfigurationSettingNameUseConfiguredHookModules))
        {
          if (false ==
              Globals::GetConfigurationData()
                  [Configuration::kSectionNameGlobal]
                  [Strings::kStrConfigurationSettingNameUseConfiguredHookModules]
                      .FirstValue()
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

      if (false == configData.HasErrors())
      {
        auto injectOnlyLibrariesToLoad = Globals::GetConfigurationData().SectionsContaining(
            Strings::kStrConfigurationSettingNameInject);

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
  } // namespace LibraryInterface
} // namespace Hookshot
