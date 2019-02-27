/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Strings.cpp
 *   Definitions of common constant string literals.
 *   These do not belong as resources because they are language-independent.
 *****************************************************************************/

#include "Globals.h"
#include "Strings.h"

#include <cstdint>
#include <cstdlib>
#include <tchar.h>


namespace Hookshot
{
    // -------- CONSTANTS -------------------------------------------------- //
    // See "Strings.h" for documentation.

    const TCHAR Strings::kCharCmdlineIndicatorFileMappingHandle = _T('|');

#ifdef HOOKSHOT64
    const uint8_t Strings::kStrInjectCodeSectionName[] = "_CODE64";
#else
    const uint8_t Strings::kStrInjectCodeSectionName[] = "_CODE32";
#endif
    const size_t Strings::kLenInjectCodeSectionName = _countof(kStrInjectCodeSectionName);

#ifdef HOOKSHOT64
    const uint8_t Strings::kStrInjectMetaSectionName[] = "_META64";
#else
    const uint8_t Strings::kStrInjectMetaSectionName[] = "_META32";
#endif
    const size_t Strings::kLenInjectMetaSectionName = _countof(kStrInjectMetaSectionName);

#ifdef HOOKSHOT64
    const TCHAR Strings::kStrInjectBinaryExtension[] = _T(".64.bin");
#else
    const TCHAR Strings::kStrInjectBinaryExtension[] = _T(".32.bin");
#endif
    const size_t Strings::kLenInjectBinaryExtension = _countof(kStrInjectBinaryExtension);

#ifdef HOOKSHOT64
    const TCHAR Strings::kStrInjectDynamicLinkLibraryExtension[] = _T(".64.dll");
#else
    const TCHAR Strings::kStrInjectDynamicLinkLibraryExtension[] = _T(".32.dll");
#endif

    const size_t Strings::kLenInjectDynamicLinkLibraryExtension = _countof(kStrInjectDynamicLinkLibraryExtension);

#ifdef HOOKSHOT64
    const TCHAR Strings::kStrInjectExecutableOtherArchitecture[] = _T(".32.exe");
#else
    const TCHAR Strings::kStrInjectExecutableOtherArchitecture[] = _T(".64.exe");
#endif

    const size_t Strings::kLenInjectExecutableOtherArchitecture = _countof(kStrInjectExecutableOtherArchitecture);

#ifdef HOOKSHOT64
    const char Strings::kStrLibraryInitializationProcName[] = "DllInit";
#else
    const char Strings::kStrLibraryInitializationProcName[] = "_DllInit@0";
#endif

    const size_t Strings::kLenLibraryInitializationProcName = _countof(kStrLibraryInitializationProcName);


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Strings.h" for documentation.
    
    bool Strings::FillCompleteFilename(TCHAR* const buf, const size_t numchars, const TCHAR* const extension, const size_t extlen)
    {
        const size_t lengthBasePath = Globals::FillHookshotModuleBasePath(buf, numchars);

        if (0 == lengthBasePath)
            return false;

        if ((lengthBasePath + extlen) >= numchars)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return false;
        }

        _tcscpy_s(&buf[lengthBasePath], extlen, extension);

        return true;
    }
}
