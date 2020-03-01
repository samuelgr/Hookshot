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

        /// File extension of the binary containing injection code and metadata.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrInjectBinaryExtension[] = _T(".64.bin");
#else
        static constexpr TCHAR kStrInjectBinaryExtension[] = _T(".32.bin");
#endif

        /// Length of `kStrInjectBinaryExtension` in character units, including terminating null character.
        static constexpr size_t kLenInjectBinaryExtension = _countof(kStrInjectBinaryExtension);

        /// File extension of the dynamic-link library to be loaded by the injected process.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrInjectDynamicLinkLibraryExtension[] = _T(".64.dll");
#else
        static constexpr TCHAR kStrInjectDynamicLinkLibraryExtension[] = _T(".32.dll");
#endif

        /// Length of `kStrInjectDynamicLinkLibraryExtension` in character units, including terminating null character.
        static constexpr size_t kLenInjectDynamicLinkLibraryExtension = _countof(kStrInjectDynamicLinkLibraryExtension);

        /// File extension of the executable to spawn in the event of attempting to inject a process with mismatched architecture.
#ifdef HOOKSHOT64
        static constexpr TCHAR kStrInjectExecutableOtherArchitecture[] = _T(".32.exe");
#else
        static constexpr TCHAR kStrInjectExecutableOtherArchitecture[] = _T(".64.exe");
#endif

        /// Length of `kStrInjectExecutableOtherArchitecture` in character units, including terminating null character.
        static constexpr size_t kLenInjectExecutableOtherArchitecture = _countof(kStrInjectExecutableOtherArchitecture);

        /// Function name of the initialization procedure exported by the Hookshot library that gets injected.
#ifdef HOOKSHOT64
        static constexpr char kStrLibraryInitializationProcName[] = "DllInit";
#else
        static constexpr char kStrLibraryInitializationProcName[] = "_DllInit@0";
#endif

        /// Length of `kStrLibraryInitializationProcName` in character units, including terminating null character.
        static constexpr size_t kLenLibraryInitializationProcName = _countof(kStrLibraryInitializationProcName);

        /// Base name of the common directory-wide hook module.
        static constexpr TCHAR kStrCommonHookModuleBaseName[] = _T("Common");

        /// Length of `kStrCommonHookModuleBaseName` in character units, including terminating null character.
        static constexpr size_t kLenCommonHookModuleBaseName = _countof(kStrCommonHookModuleBaseName);

        /// Function name of the hook library's exported initialization routine.
#ifdef HOOKSHOT64
        static constexpr char kStrHookLibraryInitFuncName[] = "HookshotMain";
#else
        static constexpr char kStrHookLibraryInitFuncName[] = "_HookshotMain@4";
#endif

        /// Length of `kStrHookLibraryInitFuncName` in character units, including terminating null character.
        static constexpr size_t kLenHookLibraryInitFuncName = _countof(kStrHookLibraryInitFuncName);
    };
}
