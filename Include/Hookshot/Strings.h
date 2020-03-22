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

#include "UnicodeTypes.h"

#include <cstddef>
#include <cstdint>
#include <tchar.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- COMPILE-TIME CONSTANTS --------------------------------- //
        // Can safely be used at any time, including to perform static initialization.
        
        /// Character that occurs at the start of a command-line argument to indicate it is a file mapping handle rather than an executable name.
        static constexpr TCHAR kCharCmdlineIndicatorFileMappingHandle = _T('|');

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
        static constexpr std::string_view kStrLibraryInitializationProcName = "_HookshotInjectInitialize@0";
#endif

        /// Function name of the hook module's exported initialization routine.
#ifdef HOOKSHOT64
        static constexpr std::string_view kStrHookLibraryInitFuncName = "HookshotMain";
#else
        static constexpr std::string_view kStrHookLibraryInitFuncName = "_HookshotMain@4";
#endif

        /// Configuration file setting name for specifying a hook module to load.
        static constexpr TStdStringView kStrConfigurationSettingNameHookModule = _T("HookModule");


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // Not safe to access before run-time, and should not be used to perform dynamic initialization.

        /// Base name of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const TStdStringView kStrExecutableBaseName;
        
        /// Directory name of the currently-running executable, including trailing backslash if available.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const TStdStringView kStrExecutableDirectoryName;

        /// Complete path and filename of the currently-running executable.
        /// For Hookshot's executable form, this will be the Hookshot executable.
        /// For Hookshot's library form, this will be the name of the executable that loaded it or into which it was injected.
        extern const TStdStringView kStrExecutableCompleteFilename;

        /// Expected filename of a Hookshot configuration file.
        /// Hookshot configuration filename = (executable directory)\(base name of this form of Hookshot).ini
        extern const TStdStringView kStrHookshotConfigurationFilename;

        /// Expected filename for the log file.
        /// Hookshot log filename = (current user's desktop)\(base name of this form of Hookshot)_(base name of the running executable)_(process ID).log
        extern const TStdStringView kStrHookshotLogFilename;

        /// Expected filename of the dynamic-link library form of Hookshot.
        extern const TStdStringView kStrHookshotDynamicLinkLibraryFilename;

        /// Expected filename of the executable form of Hookshot.
        extern const TStdStringView kStrHookshotExecutableFilename;

        /// Expected filename of the executable form of Hookshot targeting the opposite processor architecture.
        /// For example, when running in 32-bit mode, this is the name of the 64-bit executable, and vice versa.
        extern const TStdStringView kStrHookshotExecutableOtherArchitectureFilename;


        // -------- FUNCTIONS ---------------------------------------------- //

        /// Generates the expected filename of a hook module of the specified name.
        /// Hook module filename = (executable directory)\(hook module name).(hook module suffix)
        /// @param [in] moduleName Hook module name to use when generating the filename.
        /// @return Hook module filename.
        TStdString MakeHookModuleFilename(TStdStringView moduleName);
    }
}
