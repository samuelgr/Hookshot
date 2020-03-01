/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file StringUtilities.h
 *   Declaration of functions for manipulating Hookshot-specific strings.
 *****************************************************************************/

#pragma once

#include "Strings.h"

#include <cstddef>
#include <tchar.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- FUNCTIONS ---------------------------------------------- //

        /// Generates the expected filename for a Hookshot-related file, given the desired extension and extension length.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @param [in] extension String containing the extension to append to the Hookshot base name.
        /// @param [in] extlen Length of the supplied extension.
        /// @return `true` on success, `false` on failure.
        bool FillCompleteFilename(TCHAR* const buf, const size_t numchars, const TCHAR* const extension, const size_t extlen);
        
        /// Generates the expected filename of the file containing the injected code that is to be loaded and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        inline bool FillInjectBinaryFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectBinaryExtension, kLenInjectBinaryExtension);
        }

        /// Generates the expected filename of the dynamic-link library to be loaded by the injected process and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        inline bool FillInjectDynamicLinkLibraryFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectDynamicLinkLibraryExtension, kLenInjectDynamicLinkLibraryExtension);
        }

        /// Generates the expected filename of the executable to spawn in the event of attempting to inject a process with mismatched architecture and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        inline bool FillInjectExecutableOtherArchitectureFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectExecutableOtherArchitecture, kLenInjectExecutableOtherArchitecture);
        }

        /// Generates the expected filename of the executable-specific hook module.
        /// Executable-specific hook module name = (executable directory)\(executable name).(base name of this DLL)
        /// For example, 32-bit app C:\PathToExecutable\Executable.exe -> C:\PathToExecutable\Executable.exe.Hookshot.32.dll, using Hookshot's default filenames.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookModuleFilenameUnique(TCHAR* const buf, const size_t numchars);

        /// Generates the expected filename of the directory-common hook module.
        /// Directory-common hook module name = (executable directory)\Common.(base name of this DLL)
        /// For example, 32-bit app C:\PathToExecutable\Executable.exe -> C:\PathToExecutable\Common.Hookshot.32.dll, using Hookshot's default filenames.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookModuleFilenameCommon(TCHAR* const buf, const size_t numchars);
    };
}
