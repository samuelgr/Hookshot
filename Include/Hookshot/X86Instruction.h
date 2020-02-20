/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file X86Instruction.h
 *   Declaration of object interface for manipulating binary X86 instructions.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    class X86Instruction
    {
    public:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Maximum length of a single X86 instruction, in bytes.
        /// Taken from Intel documentation.
        static constexpr int kMaxInstructionLengthBytes = 15;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Determines the number of bytes that represent the instruction at the specified address, up to the specified maximum number.
        /// @param [in] instruction Address of the instruction whose length is desired.
        /// @param [in] maxLengthBytes Maximum number of bytes to read from the instruction address.
        /// @return Number of bytes required to represent the instruction, or -1 on failure (i.e. invalid instruction, length is greater than maximum, etc.).
        static int GetLengthBytes(const void* const instruction, const int maxLengthBytes = kMaxInstructionLengthBytes);

        /// Initializes the X86 instruction subsystem.  Must be called once during program initialization.
        static void Initialize(void);
    };
}
