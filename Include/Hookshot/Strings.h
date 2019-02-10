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
    struct Strings
    {
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
    };
}
