/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file Strings.cpp
 *   Implementation of functions for manipulating Hookshot-specific strings.
 **************************************************************************************************/

#include "Strings.h"

#include <intrin.h>
#include <sal.h>

#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <mutex>
#include <string>
#include <string_view>

#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "DependencyProtect.h"

namespace Hookshot
{
  namespace Strings
  {
    /// File extension of the dynamic-link library form of Hookshot.
#ifdef _WIN64
    static constexpr std::wstring_view kStrHookshotDynamicLinkLibraryExtension = L".64.dll";
#else
    static constexpr std::wstring_view kStrHookshotDynamicLinkLibraryExtension = L".32.dll";
#endif

    /// File extension of the executable form of Hookshot.
#ifdef _WIN64
    static constexpr std::wstring_view kStrHookshotExecutableExtension = L".64.exe";
#else
    static constexpr std::wstring_view kStrHookshotExecutableExtension = L".32.exe";
#endif

    /// File extension of the executable form of Hookshot but targeting the opposite processor
    /// architecture.
#ifdef _WIN64
    static constexpr std::wstring_view kStrHookshotExecutableOtherArchitectureExtension =
        L".32.exe";
#else
    static constexpr std::wstring_view kStrHookshotExecutableOtherArchitectureExtension =
        L".64.exe";
#endif

    /// File extension for a Hookshot configuration file.
    static constexpr std::wstring_view kStrHookshotConfigurationFileExtension = L".ini";

    /// File extension for a Hookshot log file.
    static constexpr std::wstring_view kStrHookshotLogFileExtension = L".log";

    /// File extension for all hook modules.
#ifdef _WIN64
    static constexpr std::wstring_view kStrHookModuleExtension = L".HookModule.64.dll";
#else
    static constexpr std::wstring_view kStrHookModuleExtension = L".HookModule.32.dll";
#endif

    /// File extension for all files whose presence would be checked to determine if Hookshot is
    /// authorized to act on a process.
    static constexpr std::wstring_view kStrAuthorizationFileExtension = L".hookshot";

    std::wstring_view GetHookshotDynamicLinkLibraryFilename(void)
    {
      static std::wstring initString;
      static std::once_flag initFlag;

      std::call_once(
          initFlag,
          []() -> void
          {
            std::wstring_view pieces[] = {
                Infra::ProcessInfo::GetThisModuleDirectoryName(),
                L"\\",
                Infra::ProcessInfo::GetProductName(),
                kStrHookshotDynamicLinkLibraryExtension};

            size_t totalLength = 0;
            for (int i = 0; i < _countof(pieces); ++i)
              totalLength += pieces[i].length();

            initString.reserve(1 + totalLength);

            for (int i = 0; i < _countof(pieces); ++i)
              initString.append(pieces[i]);
          });

      return initString;
    }

    std::wstring_view GetHookshotExecutableFilename(void)
    {
      static std::wstring initString;
      static std::once_flag initFlag;

      std::call_once(
          initFlag,
          []() -> void
          {
            std::wstring_view pieces[] = {
                Infra::ProcessInfo::GetThisModuleDirectoryName(),
                L"\\",
                Infra::ProcessInfo::GetProductName(),
                kStrHookshotExecutableExtension};

            size_t totalLength = 0;
            for (int i = 0; i < _countof(pieces); ++i)
              totalLength += pieces[i].length();

            initString.reserve(1 + totalLength);

            for (int i = 0; i < _countof(pieces); ++i)
              initString.append(pieces[i]);
          });

      return initString;
    }

    std::wstring_view GetHookshotExecutableOtherArchitectureFilename(void)
    {
      static std::wstring initString;
      static std::once_flag initFlag;

      std::call_once(
          initFlag,
          []() -> void
          {
            std::wstring_view pieces[] = {
                Infra::ProcessInfo::GetThisModuleDirectoryName(),
                L"\\",
                Infra::ProcessInfo::GetProductName(),
                kStrHookshotExecutableOtherArchitectureExtension};

            size_t totalLength = 0;
            for (int i = 0; i < _countof(pieces); ++i)
              totalLength += pieces[i].length();

            initString.reserve(1 + totalLength);

            for (int i = 0; i < _countof(pieces); ++i)
              initString.append(pieces[i]);
          });

      return initString;
    }

    Infra::TemporaryString AuthorizationFilenameApplicationSpecific(
        std::wstring_view executablePath)
    {
      Infra::TemporaryString authorizationFilename;

      authorizationFilename += executablePath;
      authorizationFilename += kStrAuthorizationFileExtension;

      return authorizationFilename;
    }

    Infra::TemporaryString AuthorizationFilenameDirectoryWide(std::wstring_view executablePath)
    {
      Infra::TemporaryString authorizationFilename;

      const size_t lastBackslashPos = executablePath.find_last_of(L"\\");
      if (std::wstring_view::npos == lastBackslashPos)
      {
        authorizationFilename += kStrAuthorizationFileExtension;
      }
      else
      {
        executablePath.remove_suffix(executablePath.length() - lastBackslashPos - 1);
        authorizationFilename << executablePath << kStrAuthorizationFileExtension;
      }

      return authorizationFilename;
    }

    Infra::TemporaryString HookModuleFilename(
        std::wstring_view moduleName, std::wstring_view directoryName)
    {
      Infra::TemporaryString hookModuleFilename;
      hookModuleFilename << directoryName;
      if (false == directoryName.ends_with(L"\\"))
      {
        hookModuleFilename << L"\\";
      }
      hookModuleFilename << moduleName << kStrHookModuleExtension;

      return hookModuleFilename;
    }
  } // namespace Strings
} // namespace Hookshot
