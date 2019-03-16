/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
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
    /// Namespace for all constant string literals.
    /// Includes some class utility methods for convenience.
    struct Strings
    {
        // -------- CONSTANTS ---------------------------------------------- //
        
        /// Character that occurs at the start of a command-line argument to indicate it is a file mapping handle rather than an executable name.
        static const TCHAR kCharCmdlineIndicatorFileMappingHandle;
        
        /// Name of the section in the injection binary that contains injection code.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
        static const uint8_t kStrInjectCodeSectionName[];

        /// Length of `kStrInjectCodeSectionName` in character units, including terminating null character.
        static const size_t kLenInjectCodeSectionName;

        /// Name of the section in the injection binary that contains injection code metadata.
        /// PE header encodes section name strings in UTF-8, so each character must directly be specified as being one byte.
        /// Per PE header specs, maximum string length is 8 including terminating null character.
        static const uint8_t kStrInjectMetaSectionName[];

        /// Length of `kStrInjectMetaSectionName` in character units, including terminating null character.
        static const size_t kLenInjectMetaSectionName;

        /// File extension of the binary containing injection code and metadata.
        static const TCHAR kStrInjectBinaryExtension[];

        /// Length of `kStrInjectBinaryExtension` in character units, including terminating null character.
        static const size_t kLenInjectBinaryExtension;

        /// File extension of the dynamic-link library to be loaded by the injected process.
        static const TCHAR kStrInjectDynamicLinkLibraryExtension[];

        /// Length of `kStrInjectDynamicLinkLibraryExtension` in character units, including terminating null character.
        static const size_t kLenInjectDynamicLinkLibraryExtension;

        /// File extension of the executable to spawn in the event of attempting to inject a process with mismatched architecture.
        static const TCHAR kStrInjectExecutableOtherArchitecture[];

        /// Length of `kStrInjectExecutableOtherArchitecture` in character units, including terminating null character.
        static const size_t kLenInjectExecutableOtherArchitecture;

        /// Function name of the initialization procedure exported by the Hookshot library that gets injected.
        static const char kStrLibraryInitializationProcName[];

        /// Length of `kStrLibraryInitializationProcName` in character units, including terminating null character.
        static const size_t kLenLibraryInitializationProcName;


        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Strings(void) = delete;

        
        // -------- CLASS METHODS ------------------------------------------ //

        /// Generates the expected filename for a Hookshot-related file, given the desired extension and extension length.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @param [in] extension String containing the extension to append to the Hookshot base name.
        /// @param [in] extlen Length of the supplied extension.
        /// @return `true` on success, `false` on failure.
        static bool FillCompleteFilename(TCHAR* const buf, const size_t numchars, const TCHAR* const extension, const size_t extlen);
        
        /// Generates the expected filename of the file containing the injected code that is to be loaded and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        static inline bool FillInjectBinaryFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectBinaryExtension, kLenInjectBinaryExtension);
        }

        /// Generates the expected filename of the dynamic-link library to be loaded by the injected process and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        static inline bool FillInjectDynamicLinkLibraryFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectDynamicLinkLibraryExtension, kLenInjectDynamicLinkLibraryExtension);
        }

        /// Generates the expected filename of the executable to spawn in the event of attempting to inject a process with mismatched architecture and places it into the specified buffer.
        /// @param [out] buf Buffer to be filled with the filename.
        /// @param [in] numchars Size of the buffer, in character units.
        /// @return `true` on success, `false` on failure.
        static inline bool FillInjectExecutableOtherArchitectureFilename(TCHAR* const buf, const size_t numchars)
        {
            return FillCompleteFilename(buf, numchars, kStrInjectExecutableOtherArchitecture, kLenInjectExecutableOtherArchitecture);
        }
    };
}
