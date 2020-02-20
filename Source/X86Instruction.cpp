/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file X86Instruction.cpp
 *   Implementation of functionality for manipulating binary X86 instructions.
 *****************************************************************************/

#include "X86Instruction.h"

#include <cstddef>
#include <cstdint>

extern "C"
{
    #include <xed/xed-interface.h>
}


namespace Hookshot
{
    // -------- CLASS METHODS ------------------------------------------ //
    // See "X86Instruction.h" for documentation.

    int X86Instruction::GetLengthBytes(const void* const instruction, const int maxLengthBytes)
    {
        return -1;
    }

    // --------

    void X86Instruction::Initialize(void)
    {
        xed_tables_init();
    }
}
