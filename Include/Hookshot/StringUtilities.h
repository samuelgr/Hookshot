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

        /// Generates the expected filename of a hook module of the specified name.
        /// Hook module filename = (executable directory)\(hook module name).(hook module suffix)
        /// @param [in] moduleName Hook module name to use when generating the filename.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookModuleFilename(const TCHAR* const moduleName, TCHAR* const buf, const size_t numchars);

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
        
        /// Generates the expected filename of the dynamic-link library form of Hookshot and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookshotDynamicLinkLibraryFilename(TCHAR* const buf, const size_t numchars);

        /// Generates the expected filename of the executable form of Hookshot and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookshotExecutableFilename(TCHAR* const buf, const size_t numchars);

        /// Generates the expected filename of the executable form of Hookshot targeting the opposite processor architecture and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        bool FillHookshotExecutableOtherArchitectureFilename(TCHAR* const buf, const size_t numchars);
    };
}
