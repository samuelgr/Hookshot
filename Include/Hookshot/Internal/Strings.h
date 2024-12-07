/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Strings.h
 *   Declaration of common strings and functions to manipulate them.
 **************************************************************************************************/

#pragma once

#include <sal.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <Infra/Core/TemporaryBuffer.h>

namespace Hookshot
{
  namespace Strings
  {
    /// Character that occurs at the start of a command-line argument to indicate it is a file
    /// mapping handle rather than an executable name.
    inline constexpr wchar_t kCharCmdlineIndicatorFileMappingHandle = L'|';

    /// Name of the section in the injection binary that contains injection code.
    /// PE header encodes section name strings in UTF-8, so each character must directly be
    /// specified as being one byte. Per PE header specs, maximum string length is 8 including
    /// terminating null character.
#ifdef _WIN64
    inline constexpr std::string_view kStrInjectCodeSectionName = "_CODE64";
#else
    inline constexpr std::string_view kStrInjectCodeSectionName = "_CODE32";
#endif

    /// Name of the section in the injection binary that contains injection code metadata.
    /// PE header encodes section name strings in UTF-8, so each character must directly be
    /// specified as being one byte.
#ifdef _WIN64
    inline constexpr std::string_view kStrInjectMetaSectionName = "_META64";
#else
    inline constexpr std::string_view kStrInjectMetaSectionName = "_META32";
#endif

    // Per PE header specs, maximum string length is 8 including terminating null character.
    static_assert(
        kStrInjectMetaSectionName.length() < 8,
        "Length of PE section name is limited to 8 characters including terminating null.");

    /// Function name of the initialization procedure exported by the Hookshot library that gets
    /// injected.
#ifdef _WIN64
    inline constexpr std::string_view kStrLibraryInitializationProcName =
        "HookshotInjectInitialize";
#else
    inline constexpr std::string_view kStrLibraryInitializationProcName =
        "@HookshotInjectInitialize@0";
#endif

    /// Function name of the hook module's exported initialization routine.
#ifdef _WIN64
    inline constexpr std::string_view kStrHookLibraryInitFuncName = "HookshotMain";
#else
    inline constexpr std::string_view kStrHookLibraryInitFuncName = "@HookshotMain@4";
#endif

    /// Configuration file setting name for specifying an injected library to load.
    inline constexpr std::wstring_view kStrConfigurationSettingNameInject = L"Inject";

    /// Configuration file setting name for specifying a hook module to load.
    inline constexpr std::wstring_view kStrConfigurationSettingNameHookModule = L"HookModule";

    /// Configuration file setting name for enabling and specifying the verbosity of output to the
    /// log file.
    inline constexpr std::wstring_view kStrConfigurationSettingNameLogLevel = L"LogLevel";

    /// Configuration file setting name for specifying that Hookshot should use the configuration
    /// file to know which hook modules to load.
    inline constexpr std::wstring_view kStrConfigurationSettingNameUseConfiguredHookModules =
        L"UseConfiguredHookModules";

    /// Configuration file setting for specifying that Hookshot should look for hook modules in its
    /// own directory instead of in the executable's directory.
    inline constexpr std::wstring_view
        kStrConfigurationSettingNameLoadHookModulesFromHookshotDirectory =
            L"LoadHookModulesFromHookshotDirectory";

    /// Expected filename of a Hookshot configuration file.
    /// Hookshot configuration filename = (executable directory)\(base name of this form of
    /// Hookshot).ini
    std::wstring_view GetHookshotConfigurationFilename(void);

    /// Expected filename for the log file.
    /// Hookshot log filename = (current user's desktop)\(base name of this form of Hookshot)_(base
    /// name of the running executable)_(process ID).log
    std::wstring_view GetHookshotLogFilename(void);

    /// Expected filename of the dynamic-link library form of Hookshot.
    std::wstring_view GetHookshotDynamicLinkLibraryFilename(void);

    /// Expected filename of the executable form of Hookshot.
    std::wstring_view GetHookshotExecutableFilename(void);

    /// Expected filename of the executable form of Hookshot targeting the opposite processor
    /// architecture. For example, when running in 32-bit mode, this is the name of the 64-bit
    /// executable, and vice versa.
    std::wstring_view GetHookshotExecutableOtherArchitectureFilename(void);

    /// Generates and returns the application-specific authorization file name, given the full path
    /// to an executable file. Authorization files are checked for existence before Hookshot acts on
    /// a process.
    /// @param [in] Full absolute path of the application being checked for authorization.
    /// @return Application-specific authorization filename.
    Infra::TemporaryString AuthorizationFilenameApplicationSpecific(
        std::wstring_view executablePath);

    /// Generates and returns the directory-wide authorization file name, given the full path to an
    /// executable file. Authorization files are checked for existence before Hookshot acts on a
    /// process.
    /// @param [in] Full absolute path of the application being checked for authorization.
    /// @return Directory-wide authorization filename.
    Infra::TemporaryString AuthorizationFilenameDirectoryWide(std::wstring_view executablePath);

    /// Generates the expected filename of a hook module of the specified name.
    /// Hook module filename = (directory name)\(hook module name).(hook module suffix)
    /// @param [in] moduleName Hook module name to use when generating the filename.
    /// @param [in] directoryName Directory name to use when generating the filename.
    /// @return Resulting hook module filename.
    Infra::TemporaryString HookModuleFilename(
        std::wstring_view moduleName, std::wstring_view directoryName);
  } // namespace Strings
} // namespace Hookshot
