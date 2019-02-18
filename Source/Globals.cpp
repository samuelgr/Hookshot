/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Globals.cpp
 *   Implementation of accessors and mutators for global data items.
 *   Intended for miscellaneous data elements with no other suitable place.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"

#include <cstring>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "Globals.h" for documentation.

    HINSTANCE Globals::gInstanceHandle = NULL;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Globals.h" for documentation.

    size_t Globals::FillHookshotModuleBasePath(TCHAR* const buf, const size_t numchars)
    {
        const DWORD length = GetModuleFileName(GetInstanceHandle(), buf, (DWORD)numchars);

        if (0 == length || (numchars == length && ERROR_INSUFFICIENT_BUFFER == GetLastError()))
            return 0;

        // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
        // Therefore, look for the last two dot characters and truncate them.
        TCHAR* const lastDot = _tcsrchr(buf, _T('.'));

        if (NULL == lastDot)
            return 0;

        *lastDot = _T('\0');

        TCHAR* const secondLastDot = _tcsrchr(buf, _T('.'));

        if (NULL == secondLastDot)
            return 0;

        *secondLastDot = _T('\0');

        return ((size_t)secondLastDot - (size_t)buf) / sizeof(buf[0]);
    }

    // ---------

    HINSTANCE Globals::GetInstanceHandle(void)
    {
        return gInstanceHandle;
    }

    // ---------

    void Globals::SetInstanceHandle(HINSTANCE newInstanceHandle)
    {
        gInstanceHandle = newInstanceHandle;
    }
}
