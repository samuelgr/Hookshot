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

#include "ApiWindows.h"
#include "Globals.h"
#include "Strings.h"
#include "TemporaryBuffers.h"

#include <cstddef>
#include <cstdlib>
#include <psapi.h>


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

        // --------

        bool FillHookModuleFilenameUnique(TCHAR* const buf, const size_t numchars)
        {
            TemporaryBuffer<TCHAR> thisModuleName;
            GetModuleBaseName(GetCurrentProcess(), Globals::GetInstanceHandle(), thisModuleName, thisModuleName.Count());

            GetModuleFileName(NULL, buf, (DWORD)numchars);
            
            if (0 != _tcscat_s(buf, numchars, _T(".")))
                return false;

            if (0 != _tcscat_s(buf, numchars, thisModuleName))
                return false;
            
            return true;
        }

        // --------

        bool FillHookModuleFilenameCommon(TCHAR* const buf, const size_t numchars)
        {
            TemporaryBuffer<TCHAR> thisModuleName;
            GetModuleBaseName(GetCurrentProcess(), Globals::GetInstanceHandle(), thisModuleName, thisModuleName.Count());

            GetModuleFileName(NULL, buf, (DWORD)numchars);
            
            {
                TCHAR* const lastBackslash = _tcsrchr(buf, _T('\\'));

                if (NULL == lastBackslash)
                    buf[0] = _T('\0');
                else
                    lastBackslash[1] = _T('\0');
            }

            if (0 != _tcscat_s(buf, numchars, kStrCommonHookModuleBaseName))
                return false;

            if (0 != _tcscat_s(buf, numchars, _T(".")))
                return false;

            if (0 != _tcscat_s(buf, numchars, thisModuleName))
                return false;
            
            return true;
        }
    }
}