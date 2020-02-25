/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Trampoline.cpp
 *   Implementation of some functionality for individual trampolines.
 *****************************************************************************/

#include "Trampoline.h"
#include "X86Instruction.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ------------------------------------- //
    
    /// Loaded into the hook region of the trampoline at initialization time.
    /// Provides the needed preamble and allows room for the hook function address to be specified after-the-fact.
    /// When the trampoline is set, the hook function address is filled in.
    static constexpr uint8_t kHookCodeDefault[Trampoline::kTrampolineSizeHookFunctionBytes] = {
#ifdef HOOKSHOT64
            0x66, 0x90,                                                 // nop
            0xff, 0x25, 0x00, 0x00, 0x00, 0x00,                         // jmp QWORD PTR [rip]
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00              // <absolute address of hook function>
#else
            0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,       // nop
            0x66, 0x90,                                                 // nop
            0xe9,                                                       // jmp rel32
            0x00, 0x00, 0x00, 0x00                                      // <rel32 address of hook function>
#endif
    };

    /// Loaded into the original function region of the trampoline at initialization time.
    /// This is the ud2 instruction, which acts as an "uninitialized poison" by causing the program to crash when executed.
    /// When the trampoline is set, this region of the code is replaced by actual transplanted code.
    static constexpr uint16_t kOriginalCodeDefault = 0x0b0f;

    /// Preamble for writing an unconditional jump instruction as part of setting a hook.
    /// Represents the fixed opcode bytes that come before a relative 32-bit jump displacement.
    /// Primarily used for overwriting bytes in the original function, but also used to handle short relative branches in transplanted instructions.
    static constexpr uint8_t kTrampolineJumpPreamble[] = {
        0xe9                                                            // jmp rel32
    };

    /// Total size of an unconditional jump instruction written as part of setting a hook.
    /// Equal to the size of all preamble opcode bytes plus the size of a rel32 displacement.
    static constexpr size_t kTrampolineJumpSize = sizeof(kTrampolineJumpPreamble) + sizeof(uint32_t);


    // -------- INTERNAL FUNCTIONS ------------------------------------- //

    
    
    /// Places a trampoline jump operation with the specified displacement at the specified location.
    /// Supplied buffer must be large enough to hold #kTrampolineJumpSize bytes.
    /// @param [out] buf Buffer to which the trampoline jump operation should be written.
    /// @param [in] displacement 32-bit relative jump displacement.
    static void WriteTrampolineJump(uint8_t* const buf, const uint32_t displacement)
    {
        for (int i = 0; i < sizeof(kTrampolineJumpPreamble); ++i)
            buf[i] = kTrampolineJumpPreamble[i];

        *((uint32_t*)&buf[sizeof(kTrampolineJumpPreamble)]) = displacement;
    }

    
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "Trampoline.h" for documentation.

    Trampoline::Trampoline(void) : code()
    {
        for (int i = 0; i < sizeof(kHookCodeDefault); ++i)
            code.hook.byte[i] = kHookCodeDefault[i];

        code.original.word[0] = kOriginalCodeDefault;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "Trampoline.h" for documentation.
    
    bool Trampoline::IsTargetSet(void) const
    {
        return (kOriginalCodeDefault != code.original.word[0]);
    }

    // --------
    
    bool Trampoline::SetHookForTarget(const TFunc hook, TFunc target)
    {
        // TODO: implement transplanting part of this method.
        code.hook.ptr[_countof(code.hook.ptr) - 1] = ValueForHookAddress(hook);
        return true;
    }
}
