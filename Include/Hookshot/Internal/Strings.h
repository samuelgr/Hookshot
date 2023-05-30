/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 **************************************************************************//**
 * @file Strings.h
 *   Declaration of common strings and functions to manipulate them.
 *****************************************************************************/

#pragma once

#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstdint>
#include <sal.h>
#include <string>
#include <string_view>


namespace Hookshot
{
    namespace Strings
    {
        // -------- COMPILE-TIME CONSTANTS --------------------------------- //
        // Can safely be used at any time, including to perform static initialization.
        // Views are guaranteed to be null-terminated.

        /// Character that occurs at the start of a command-line argument to indicate it is a file mapping handle rather than an executable name.
        inline constexpr wchar_t kCharCmdlineIndicatorFileMappingHandle = L'|';

        /// Name of the section in the injection binary that contains injection code.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
#ifdef HOOKSHOT64
        inline constexpr std::string_view kStrInjectCodeSectionName = "_CODE64";
#else
        inline constexpr std::string_view kStrInjectCodeSectionName = "_CODE32";
#endif

        /// Name of the section in the injection binary that contains injection code metadata.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
#ifdef HOOKSHOT64
        inline constexpr std::string_view kStrInjectMetaSectionName = "_META64";
#else
        inline constexpr std::string_view kStrInjectMetaSectionName = "_META32";
#endif

        // Per PE header specs, maximum string length is 8 including terminating null character.
        static_assert(kStrInjectMetaSectionName.length() < 8, "Length of PE section name is limited to 8 characters including terminating null.");

        /// Function name of the initialization procedure exported by the Hookshot library that gets injected.
#ifdef HOOKSHOT64
        inline constexpr std::string_view kStrLibraryInitializationProcName = "HookshotInjectInitialize";
#else
        inline constexpr std::string_view kStrLibraryInitializationProcName = "@HookshotInjectInitialize@0";
#endif

        /// Function name of the hook module's exported initialization routine.
#ifdef HOOKSHOT64
        inline constexpr std::string_view kStrHookLibraryInitFuncName = "HookshotMain";
#else
        inline constexpr std::string_view kStrHookLibraryInitFuncName = "@HookshotMain@4";
#endif

        /// Configuration file setting name for specifying an injected library to load.
        inline constexpr std::wstring_view kStrConfigurationSettingNameInject = L"Inject";

        /// Configuration file setting name for specifying a hook module to load.
        inline constexpr std::wstring_view kStrConfigurationSettingNameHookModule = L"HookModule";

        /// Configuration file setting name for enabling and specifying the verbosity of output to the log file.
        inline constexpr std::wstring_view kStrConfigurationSettingNameLogLevel = L"LogLevel";

        /// Configuration file setting name for specifying that Hookshot should use the configuration file to know which hook modules to load.
        inline constexpr std::wstring_view kStrConfigurationSettingNameUseConfiguredHookModules = L"UseConfiguredHookModules";


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // Not safe to access before run-time, and should not be used to perform dynamic initialization.
        // Views are guaranteed to be null-terminated.

        /// Product name.
        /// Use this to identify Hookshot in areas of user interaction.
        extern const std::wstring_view kStrProductName;

        /// Complete path and filename of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        /// For the Hookshot Launcher, this will be the executable of the launcher itself.
        extern const std::wstring_view kStrExecutableCompleteFilename;

        /// Base name of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        /// For the Hookshot Launcher, this will be the executable name of the launcher itself.
        extern const std::wstring_view kStrExecutableBaseName;

        /// Directory name of the currently-running executable, including trailing backslash if available.
        /// For Hookshot's executable form, this will be the directory containing the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        /// For the Hookshot Launcher, this will be the directory containing the launcher itself.
        extern const std::wstring_view kStrExecutableDirectoryName;

        /// Complete path and filename of the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as kStrExecutableCompleteFilename.
        /// For Hookshot's library form, this will be the name of the library.
        /// For the Hookshot Launcher, this will be the same as kStrExecutableCompleteFilename.
        extern const std::wstring_view kStrHookshotCompleteFilename;

        /// Base name for the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as kStrExecutableBaseName.
        /// For Hookshot's library form, this will be the name of the library.
        /// For the Hookshot Launcher, this will be the same as kStrExecutableBaseName.
        extern const std::wstring_view kStrHookshotBaseName;

        /// Directory name for the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as kStrExecutableDirectoryName.
        /// For Hookshot's library form, this will be the name of the library.
        /// For the Hookshot Launcher, this will be the same as kStrExecutableDirectoryName.
        extern const std::wstring_view kStrHookshotDirectoryName;

        /// Expected filename of a Hookshot configuration file.
        /// Hookshot configuration filename = (executable directory)\(base name of this form of Hookshot).ini
        extern const std::wstring_view kStrHookshotConfigurationFilename;

        /// Expected filename for the log file.
        /// Hookshot log filename = (current user's desktop)\(base name of this form of Hookshot)_(base name of the running executable)_(process ID).log
        extern const std::wstring_view kStrHookshotLogFilename;

        /// Expected filename of the dynamic-link library form of Hookshot.
        extern const std::wstring_view kStrHookshotDynamicLinkLibraryFilename;

        /// Expected filename of the executable form of Hookshot.
        extern const std::wstring_view kStrHookshotExecutableFilename;

        /// Expected filename of the executable form of Hookshot targeting the opposite processor architecture.
        /// For example, when running in 32-bit mode, this is the name of the 64-bit executable, and vice versa.
        extern const std::wstring_view kStrHookshotExecutableOtherArchitectureFilename;


        // -------- FUNCTIONS ---------------------------------------------- //

        /// Generates and returns the application-specific authorization file name, given the full path to an executable file.
        /// Authorization files are checked for existence before Hookshot acts on a process.
        /// @param [in] Full absolute path of the application being checked for authorization.
        /// @return Application-specific authorization filename.
        std::wstring AuthorizationFilenameApplicationSpecific(std::wstring_view executablePath);

        /// Generates and returns the directory-wide authorization file name, given the full path to an executable file.
        /// Authorization files are checked for existence before Hookshot acts on a process.
        /// @param [in] Full absolute path of the application being checked for authorization.
        /// @return Directory-wide authorization filename.
        std::wstring AuthorizationFilenameDirectoryWide(std::wstring_view executablePath);

        /// Checks if one string is a suffix of another without regard for the case of each individual character.
        /// @tparam CharType Type of character in each string, either narrow or wide.
        /// @param [in] str String to be checked for a possible prefix.
        /// @param [in] maybeSuffix Candidate suffix to compare with the end of the string.
        /// @return `true` if the candidate suffix is a suffix of the specified string, `false` otherwise.
        template <typename CharType> bool EndsWithCaseInsensitive(std::basic_string_view<CharType> str, std::basic_string_view<CharType> maybeSuffix);

        /// Compares two strings without regard for the case of each individual character.
        /// @tparam CharType Type of character in each string, either narrow or wide.
        /// @param [in] strA First string in the comparison.
        /// @param [in] strB Second string in the comparison.
        /// @return `true` if the strings compare equal, `false` otherwise.
        template <typename CharType> bool EqualsCaseInsensitive(std::basic_string_view<CharType> strA, std::basic_string_view<CharType> strB);

        /// Formats a string and returns the result in a newly-allocated null-terminated temporary buffer.
        /// @param [in] format Format string, possibly with format specifiers which must be matched with the arguments that follow.
        /// @return Resulting string after all formatting is applied.
        TemporaryString FormatString(_Printf_format_string_ const wchar_t* format, ...);

        /// Generates the expected filename of a hook module of the specified name.
        /// Hook module filename = (executable directory)\(hook module name).(hook module suffix)
        /// @param [in] moduleName Hook module name to use when generating the filename.
        /// @return Hook module filename.
        std::wstring HookModuleFilename(std::wstring_view moduleName);

        /// Checks if one string is a prefix of another without regard for the case of each individual character.
        /// @tparam CharType Type of character in each string, either narrow or wide.
        /// @param [in] str String to be checked for a possible prefix.
        /// @param [in] maybePrefix Candidate prefix to compare with the beginning of the string.
        /// @return `true` if the candidate prefix is a prefix of the specified string, `false` otherwise.
        template <typename CharType> bool StartsWithCaseInsensitive(std::basic_string_view<CharType> str, std::basic_string_view<CharType> maybePrefix);

        /// Generates a string representation of a system error code.
        /// @param [in] systemErrorCode System error code for which to generate a string.
        /// @return String representation of the system error code.
        std::wstring SystemErrorCodeString(const unsigned long systemErrorCode);
    }
}
