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

#include "Strings.h"

#include <cstdint>
#include <cstdlib>
#include <tchar.h>


namespace Hookshot
{
    // -------- CONSTANTS -------------------------------------------------- //
    // See "Strings.h" for documentation.

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
}
