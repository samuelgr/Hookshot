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

    size_t Globals::FillModuleBasePath(TCHAR* buf, const size_t numchars)
    {
        const DWORD length = GetModuleFileName(GetInstanceHandle(), buf, (DWORD)numchars);

        if (0 == length || (numchars == length && ERROR_INSUFFICIENT_BUFFER == GetLastError()))
            return 0;

        // Find the index of the last '.' character in the returned path.
        // If it does not exist, then there is no extension so nothing more to do.
        TCHAR* const lastDot = _tcsrchr(buf, _T('.'));

        if (NULL == lastDot)
            return length;
        
        // Find the index of the last '\' character in the returned path.
        // There may be a '.' character somewhere in the directory name, so truncating blindly would be incorrect.
        TCHAR* const lastBackslash = _tcsrchr(buf, _T('\\'));

        // If the last '\' is further than the last '.' then there is also no extension present so nothing more to do.
        if ((size_t)lastBackslash > (size_t)lastDot)
            return length;
        
        // Otherwise, truncate the string at the '.' and reduce the effective length.
        const size_t truncatedLength = ((size_t)lastDot - (size_t)buf) / sizeof(buf[0]);
        buf[truncatedLength] = _T('\0');
        
        return truncatedLength;
    }

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
