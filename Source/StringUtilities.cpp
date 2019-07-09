/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file StringUtilities.cpp
 *   Implementation of functions for manipulating Hookshot-specific strings.
 *****************************************************************************/

#include "Globals.h"

#include <cstddef>
#include <cstdlib>
#include <tchar.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- FUNCTIONS ---------------------------------------------- //
        // See "StringUtilities.h" for documentation.
        
        bool FillCompleteFilename(TCHAR* const buf, const size_t numchars, const TCHAR* const extension, const size_t extlen)
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
}
