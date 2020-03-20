/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Strings.h
 *   Declaration of common constant string literals.
 *   These do not belong as resources because they are language-independent.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <tchar.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- CONSTANTS ---------------------------------------------- //

        /// Character that occurs at the start of a command-line argument to indicate it is a file mapping handle rather than an executable name.
        static constexpr TCHAR kCharCmdlineIndicatorFileMappingHandle = _T('|');

        /// Name of the section in the injection binary that contains injection code.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
#ifdef HOOKSHOT64
        static constexpr uint8_t kStrInjectCodeSectionName[] = "_CODE64";
#else
        static constexpr uint8_t kStrInjectCodeSectionName[] = "_CODE32";
#endif

        /// Length of `kStrInjectCodeSectionName` in character units, including terminating null character.
        static constexpr size_t kLenInjectCodeSectionName = _countof(kStrInjectCodeSectionName);

        /// Name of the section in the injection binary that contains injection code metadata.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
#ifdef HOOKSHOT64
        static constexpr uint8_t kStrInjectMetaSectionName[] = "_META64";
#else
        static constexpr uint8_t kStrInjectMetaSectionName[] = "_META32";
#endif

        /// Length of `kStrInjectMetaSectionName` in character units, including terminating null character.
        static constexpr size_t kLenInjectMetaSectionName = _countof(kStrInjectMetaSectionName);

        /// File extension of the dynamic-link library form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrHookshotDynamicLinkLibraryExtension[] = _T(".64.dll");
#else
        static constexpr TCHAR kStrHookshotDynamicLinkLibraryExtension[] = _T(".32.dll");
#endif

        /// Length of `kStrHookshotDynamicLinkLibraryExtension` in character units, including terminating null character.
        static constexpr size_t kLenHookshotDynamicLinkLibraryExtension = _countof(kStrHookshotDynamicLinkLibraryExtension);

        /// File extension of the executable form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrHookshotExecutableExtension[] = _T(".64.exe");
#else
        static constexpr TCHAR kStrHookshotExecutableExtension[] = _T(".32.exe");
#endif

        /// Length of `kStrHookshotExecutableExtension` in character units, including terminating null character.
        static constexpr size_t kLenHookshotExecutableExtension = _countof(kStrHookshotExecutableExtension);
        
        /// File extension of the executable form of Hookshot but targeting the opposite processor architecture.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrHookshotExecutableExtensionOtherArchitecture[] = _T(".32.exe");
#else
        static constexpr TCHAR kStrHookshotExecutableExtensionOtherArchitecture[] = _T(".64.exe");
#endif

        /// Length of `kStrHookshotExecutableOtherArchitecture` in character units, including terminating null character.
        static constexpr size_t kLenHookshotExecutableExtensionOtherArchitecture = _countof(kStrHookshotExecutableExtensionOtherArchitecture);

        /// File suffix for all hook modules.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrHookModuleSuffix[] = _T("HookModule.64.dll");
#else
        static constexpr TCHAR kStrHookModuleSuffix[] = _T("HookModule.32.dll");
#endif

        /// Length of `kStrHookModuleSuffix` in character units, including terminating null character.
        static constexpr size_t kLenHookModuleSuffix = _countof(kStrHookModuleSuffix);

        /// Function name of the initialization procedure exported by the Hookshot library that gets injected.
#ifdef HOOKSHOT64
        static constexpr char kStrLibraryInitializationProcName[] = "HookshotInjectInitialize";
#else
        static constexpr char kStrLibraryInitializationProcName[] = "_HookshotInjectInitialize@0";
#endif

        /// Length of `kStrLibraryInitializationProcName` in character units, including terminating null character.
        static constexpr size_t kLenLibraryInitializationProcName = _countof(kStrLibraryInitializationProcName);

        /// Function name of the hook module's exported initialization routine.
#ifdef HOOKSHOT64
        static constexpr char kStrHookLibraryInitFuncName[] = "HookshotMain";
#else
        static constexpr char kStrHookLibraryInitFuncName[] = "_HookshotMain@4";
#endif

        /// Length of `kStrHookLibraryInitFuncName` in character units, including terminating null character.
        static constexpr size_t kLenHookLibraryInitFuncName = _countof(kStrHookLibraryInitFuncName);
    };
}
