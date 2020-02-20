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
    // -------- INTERNAL CONSTANTS ------------------------------------- //

    /// Captures the state of the current machine, based on its operating mode (i.e. 32-bit or 64-bit).
    /// This is a constant that corresponds to the currently-executing process mode.
#ifdef HOOKSHOT64
    static const xed_state_t kXedMachineState = {XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b};
#else
    static const xed_state_t kXedMachineState = {XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b};
#endif

    
    // -------- CLASS METHODS ------------------------------------------ //
    // See "X86Instruction.h" for documentation.

    int X86Instruction::GetLengthBytes(const void* const instruction, const int maxLengthBytes)
    {
        xed_decoded_inst_t decoded_instruction;

        for (int numBytes = 0; (numBytes <= maxLengthBytes) && (numBytes <= kMaxInstructionLengthBytes); ++numBytes)
        {
            xed_decoded_inst_zero_set_mode(&decoded_instruction, &kXedMachineState);
            
            if (XED_ERROR_NONE == xed_ild_decode(&decoded_instruction, (const unsigned char*)instruction, numBytes))
                return (int)xed_decoded_inst_get_length(&decoded_instruction);
        }
        
        return -1;
    }

    // --------

    void X86Instruction::Initialize(void)
    {
        xed_tables_init();
    }
}
