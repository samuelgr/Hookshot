/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Strings.cpp
 *   Implementation of functions for manipulating Hookshot-specific strings.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstdlib>
#include <psapi.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Fills the directory name of the currently-running executable (not necessarily Hookshot), including trailing backslash.
        /// If there is no backslash contained in the path retrieved for said executable, sets the buffer to the empty string.
        /// @param [out] buf Buffer to be filled with the directory name.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        static bool FillExecutableDirectoryName(TCHAR* const buf, const size_t numchars)
        {
            if ((GetModuleFileName(NULL, buf, (DWORD)numchars) == numchars) && (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
                return false;

            TCHAR* const lastBackslash = _tcsrchr(buf, _T('\\'));

            if (NULL == lastBackslash)
                buf[0] = _T('\0');
            else
                lastBackslash[1] = _T('\0');

            return true;
        }

        /// Fills the specified buffer with the fully-qualified path of the current running form of Hookshot, minus the extension.
        /// This is useful for determining the correct path of the next file or module to load.
        /// @param [in,out] buf Buffer to be filled.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return Number of characters written to the buffer (not including the terminal `NULL` character, which is always written), or 0 in the event of an error.
        static size_t FillHookshotBasePath(TCHAR* const buf, const size_t numchars)
        {
            const DWORD length = GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)numchars);

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
        
        /// Generates the expected filename for a Hookshot-related file, given the desired extension and extension length.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @param [in] extension String containing the extension to append to the Hookshot base name.
        /// @param [in] extlen Length of the supplied extension.
        /// @return `true` on success, `false` on failure.
        static bool FillHookshotFilename(TCHAR* const buf, const size_t numchars, const TCHAR* const extension, const size_t extlen)
        {
            const size_t lengthBasePath = FillHookshotBasePath(buf, numchars);

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


        
        // -------- FUNCTIONS ---------------------------------------------- //
        // See "Strings.h" for documentation.

        bool FillHookModuleFilename(const TCHAR* const moduleName, TCHAR* const buf, const size_t numchars)
        {
            if (false == FillExecutableDirectoryName(buf, numchars))
                return false;

            if (0 != _tcscat_s(buf, numchars, moduleName))
                return false;

            if (0 != _tcscat_s(buf, numchars, kStrHookModuleExtension))
                return false;

            return true;
        }

        // --------
        
        bool FillHookModuleFilenameUnique(TCHAR* const buf, const size_t numchars)
        {
            GetModuleFileName(NULL, buf, (DWORD)numchars);

            if (0 != _tcscat_s(buf, numchars, kStrHookModuleExtension))
                return false;
            
            return true;
        }

        // --------

        bool FillHookModuleFilenameCommon(TCHAR* const buf, const size_t numchars)
        {
            if (false == FillExecutableDirectoryName(buf, numchars))
                return false;

            if (0 != _tcscat_s(buf, numchars, _T("Common")))
                return false;

            if (0 != _tcscat_s(buf, numchars, kStrHookModuleExtension))
                return false;
            
            return true;
        }

        // --------

        bool FillHookshotConfigurationFilename(TCHAR* const buf, const size_t numchars)
        {
            if (false == FillExecutableDirectoryName(buf, numchars))
                return false;

            TemporaryBuffer<TCHAR> hookshotBasePath;
            if (0 == FillHookshotBasePath(hookshotBasePath, hookshotBasePath.Count()))
                return false;

            TCHAR* hookshotBaseName = _tcsrchr(hookshotBasePath, _T('\\'));
            if (NULL == hookshotBaseName)
                hookshotBaseName = hookshotBasePath;
            else
                hookshotBaseName += 1;

            if (0 != _tcscat_s(buf, numchars, hookshotBaseName))
                return false;

            if (0 != _tcscat_s(buf, numchars, kStrHookshotConfigurationFileExtension))
                return false;

            return true;
        }

        // --------

        bool FillHookshotDynamicLinkLibraryFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillHookshotFilename(buf, numchars, kStrHookshotDynamicLinkLibraryExtension, kLenHookshotDynamicLinkLibraryExtension);
        }

        // --------

        bool FillHookshotExecutableFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillHookshotFilename(buf, numchars, kStrHookshotExecutableExtension, kLenHookshotExecutableExtension);
        }

        // --------

        bool FillHookshotExecutableOtherArchitectureFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillHookshotFilename(buf, numchars, kStrHookshotExecutableExtensionOtherArchitecture, kLenHookshotExecutableExtensionOtherArchitecture);
        }
    }
}
