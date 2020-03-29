/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Strings.h
 *   Declaration of common strings and functions to manipulate them.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>


namespace Hookshot
{
    namespace Strings
    {
        // -------- COMPILE-TIME CONSTANTS --------------------------------- //
        // Can safely be used at any time, including to perform static initialization.
        
        /// Character that occurs at the start of a command-line argument to indicate it is a file mapping handle rather than an executable name.
        static constexpr wchar_t kCharCmdlineIndicatorFileMappingHandle = L'|';

        /// Name of the section in the injection binary that contains injection code.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
#ifdef HOOKSHOT64
        static constexpr std::string_view kStrInjectCodeSectionName = "_CODE64";
#else
        static constexpr std::string_view kStrInjectCodeSectionName = "_CODE32";
#endif

        /// Name of the section in the injection binary that contains injection code metadata.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
#ifdef HOOKSHOT64
        static constexpr std::string_view kStrInjectMetaSectionName = "_META64";
#else
        static constexpr std::string_view kStrInjectMetaSectionName = "_META32";
#endif
        
        // Per PE header specs, maximum string length is 8 including terminating null character.
        static_assert(kStrInjectMetaSectionName.length() < 8, "Length of PE section name is limited to 8 characters including terminating null.");

        /// Function name of the initialization procedure exported by the Hookshot library that gets injected.
#ifdef HOOKSHOT64
        static constexpr std::string_view kStrLibraryInitializationProcName = "HookshotInjectInitialize";
#else
        static constexpr std::string_view kStrLibraryInitializationProcName = "@HookshotInjectInitialize@0";
#endif

        /// Function name of the hook module's exported initialization routine.
#ifdef HOOKSHOT64
        static constexpr std::string_view kStrHookLibraryInitFuncName = "HookshotMain";
#else
        static constexpr std::string_view kStrHookLibraryInitFuncName = "@HookshotMain@4";
#endif

        /// Configuration file setting name for specifying a library to load only but not initialize.
        /// Useful when Hookshot is used only to perform injection.
        static constexpr std::wstring_view kStrConfigurationSettingNameInject = L"Inject";
        
        /// Configuration file setting name for specifying a hook module to load and initialize as a hook module.
        /// Useful when Hookshot is both performing injection and setting hooks.
        static constexpr std::wstring_view kStrConfigurationSettingNameHookModule = L"HookModule";

        /// Configuration file setting name for enabling and specifying the verbosity of output to the log file.
        static constexpr std::wstring_view kStrConfigurationSettingNameLogLevel = L"LogLevel";


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // Not safe to access before run-time, and should not be used to perform dynamic initialization.

        /// Product name.
        /// Use this to identify Hookshot in areas of user interaction.
        extern const std::wstring_view kStrProductName;
        
        /// Base name of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const std::wstring_view kStrExecutableBaseName;
        
        /// Directory name of the currently-running executable, including trailing backslash if available.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const std::wstring_view kStrExecutableDirectoryName;

        /// Complete path and filename of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const std::wstring_view kStrExecutableCompleteFilename;

        /// Base name for the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as #kStrExecutableBaseName.
        /// For Hookshot's library form, this will be the name of the library.
        extern const std::wstring_view kStrHookshotBaseName;

        /// Directory name for the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as #kStrExecutableDirectoryName.
        /// For Hookshot's library form, this will be the name of the library.
        extern const std::wstring_view kStrHookshotDirectoryName;

        /// Complete path and filename of the currently-running form of Hookshot.
        /// For Hookshot's executable form, this will be the same as #kStrExecutableCompleteFilename.
        /// For Hookshot's library form, this will be the name of the library.
        extern const std::wstring_view kStrHookshotCompleteFilename;
        
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

        /// Generates the expected filename of a hook module of the specified name.
        /// Hook module filename = (executable directory)\(hook module name).(hook module suffix)
        /// @param [in] moduleName Hook module name to use when generating the filename.
        /// @return Hook module filename.
        std::wstring MakeHookModuleFilename(std::wstring_view moduleName);
    }
}
