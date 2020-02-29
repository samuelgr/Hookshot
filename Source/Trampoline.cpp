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

#include "ApiWindows.h"
#include "Message.h"
#include "TemporaryBuffers.h"
#include "Trampoline.h"
#include "X86Instruction.h"

#include <cstddef>
#include <cstdint>
#include <vector>


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ------------------------------------- //
    
    /// Loaded into the hook region of the trampoline at initialization time.
    /// Provides the needed preamble and allows room for the hook function address to be specified after-the-fact.
    /// When the trampoline is set, the hook function address is filled in after the code contained here.
    static constexpr uint8_t kHookCodePreamble[] = {
#ifdef HOOKSHOT64
            0x66, 0x90,                                                 // nop
            0xff, 0x25, 0x00, 0x00, 0x00, 0x00,                         // jmp QWORD PTR [rip]
#else
            0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,       // nop
            0x66, 0x90,                                                 // nop
            0xe9,                                                       // jmp rel32
#endif
    };
    
    // Used to verify that the amount of preamble code is just right.
    // If too much, a pointer to the hook function will not fit into the allowed space of the hook code region.  To fix, increase the size of the hook region of the trampoline or decrease the size of the code.
    // If too little, the code will not execute correctly because there will be a gap between the code and the pointer.  To fix, pad the hook code preamble with nop instructions.
    static_assert(!(sizeof(kHookCodePreamble) + sizeof(void*) > Trampoline::kTrampolineSizeHookFunctionBytes), "Hook code preamble is too big.  Either increase the size of the hook region of the trampoline or decrease the size of the preamble code.");
    static_assert(!(sizeof(kHookCodePreamble) + sizeof(void*) < Trampoline::kTrampolineSizeHookFunctionBytes), "Hook code preamble is too small.  Pad with nop instructions to increase the size.");

    /// Loaded into the original function region of the trampoline at initialization time.
    /// This is the ud2 instruction, which acts as an "uninitialized poison" by causing the program to crash when executed.
    /// When the trampoline is set, this region of the code is replaced by actual transplanted code.
    static constexpr uint16_t kOriginalCodeDefault = 0x0b0f;

    
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "Trampoline.h" for documentation.

    Trampoline::Trampoline(void) : code()
    {
        for (int i = 0; i < sizeof(kHookCodePreamble); ++i)
            code.hook.byte[i] = kHookCodePreamble[i];

        code.original.word[0] = kOriginalCodeDefault;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "Trampoline.h" for documentation.
    
    bool Trampoline::IsTargetSet(void) const
    {
        return (kOriginalCodeDefault != code.original.word[0]);
    }

    // --------
    
    bool Trampoline::SetHookForTarget(const void* hook, void* target)
    {
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_TRAMPOLINE_SET_HOOK_START_FORMAT, (long long)hook, (long long)target, (long long)&code.hook, (long long)&code.original);
        
        // Sanity check.  Make sure the target is not too far away from this trampoline.
        if (false == X86Instruction::CanWriteJumpInstruction(target, &code.hook))
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityWarning, IDS_HOOKSHOT_TRAMPOLINE_SET_FAILED_TOO_FAR_FORMAT, (long long)target);
            return false;
        }
        
        // Easy part. Make the hook part of the trampoline actually target the hook function.
        // This is done by placing the address (or displacement) of the hook function into the hook part of the trampoline at the last pointer-sized element.
        code.hook.ptr[_countof(code.hook.ptr) - 1] = ValueForHookAddress(hook);
        
        // Hard part. Transplanting code from the location of the original function into the original function part of the trampoline.  This is done in several sub-parts, and more details will be provided while executing each sub-part.
        // First, read and decode instructions until either enough bytes worth of instructions are decoded to hold an unconditional jump or a terminal instruction is hit.  If the latter happens strictly before the former, then setting this hook failed due to there not being enough bytes of function to transplant.
        // Second, iterate through all the decoded instructions and check for position-dependent memory references.  Update the displacements as needed for any such decoded instructions.  If that is not possible for even one instruction, then setting this hook failed.
        // Third, encode the decoded instructions into this trampoline's original function region.  If needed (i.e. the last of the decoded instructions is non-terminal), append an unconditional jump instruction to the correct address within the original function.  This will be completed at the same time as the second sub-part.
        // Fourth and final, overwrite the beginning of the original function with an unconditional jump to the hook region of this trampoline.  This is where it is important other threads not be trying to execute code in the original function.

        // First sub-part.
        uint8_t* const originalFunctionBytes = (uint8_t*)target;
        constexpr int numOriginalFunctionBytesNeeded = X86Instruction::kJumpInstructionLengthBytes;
        int numOriginalFunctionBytes = 0;
        int instructionIndex = 0;
        std::vector<X86Instruction> originalInstructions;

        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_START_FORMAT, (long long)target, numOriginalFunctionBytesNeeded);

        while (numOriginalFunctionBytes < numOriginalFunctionBytesNeeded)
        {
            X86Instruction& decodedInstruction = originalInstructions.emplace_back();
            decodedInstruction.DecodeInstruction(&originalFunctionBytes[numOriginalFunctionBytes]);
            
            if (false == decodedInstruction.IsValid())
            {
                Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_INSTRUCTION_INVALID_FORMAT, instructionIndex);
                return false;
            }

            if (Message::WillOutputMessageOfSeverity(EMessageSeverity::MessageSeverityDebug))
            {
                TemporaryBuffer<TCHAR> disassembly;
                if (true == decodedInstruction.PrintDisassembly(disassembly, disassembly.Count()))
                    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_INSTRUCTION_FORMAT, instructionIndex, decodedInstruction.GetLengthBytes(), &disassembly[0]);
                else
                    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_INSTRUCTION_NODISASSEMBLE_FORMAT, instructionIndex, decodedInstruction.GetLengthBytes());

                if (decodedInstruction.IsTerminal())
                    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_INSTRUCTION_IS_TERMINAL_FORMAT, instructionIndex);
            }
            
            numOriginalFunctionBytes += decodedInstruction.GetLengthBytes();
            instructionIndex += 1;

            if (decodedInstruction.IsTerminal())
                break;
        }

        if (numOriginalFunctionBytes < numOriginalFunctionBytesNeeded)
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_TOTAL_BYTES_INSUFFICIENT_FORMAT, numOriginalFunctionBytes, numOriginalFunctionBytesNeeded);
            return false;
        }
        else
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_DECODE_TOTAL_BYTES_SUFFICIENT_FORMAT, numOriginalFunctionBytes, numOriginalFunctionBytesNeeded);

        // Second and third sub-parts.
        int numTrampolineBytesWritten = 0;
        int numExtraTrampolineBytesUsed = 0;

        for (int i = 0; i < (int)originalInstructions.size(); ++i)
        {
            const int numTrampolineBytesLeft = sizeof(code.original) - numTrampolineBytesWritten - numExtraTrampolineBytesUsed;
            void* const nextTrampolineAddressToWrite = &code.original.byte[numTrampolineBytesWritten];
            
            // Second sub-part. Handle any position-dependent memory references.
            if (originalInstructions[i].HasPositionDependentMemoryReference())
            {
                const int64_t originalDisplacement = originalInstructions[i].GetMemoryDisplacement();
                Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_HAS_POSITION_DEPENDENT_MEMORY_REFERENCE_FORMAT, i, (long long)originalDisplacement);
                
                // If the displacement is so small that it refers to another instruction that is also being transplanted, then there is no need to modify it.
                // Note the need to check both forwards (positive) and backwards (negative) displacement directions.
                const int64_t minForwardDisplacementNeedingModification = (int64_t)(numOriginalFunctionBytes - (numTrampolineBytesWritten + originalInstructions[i].GetLengthBytes()));
                const int64_t minBackwardDisplacementNotNeedingMofification = (int64_t)(-1 * (numTrampolineBytesWritten + originalInstructions[i].GetLengthBytes()));

                if (originalDisplacement >= minForwardDisplacementNeedingModification || originalDisplacement < minBackwardDisplacementNotNeedingMofification)
                {
                    // There are no changes to the length of the instruction, so the change to the displacement value is just the difference in its new and original locations in memory.
                    const intptr_t newDisplacementValue = ((intptr_t)originalInstructions[i].GetAddress() - (intptr_t)nextTrampolineAddressToWrite) + (intptr_t)originalDisplacement;
                    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_DISPLACEMENT_NEW_VALUE_FORMAT, i, (long long)originalInstructions[i].GetAddress(), (long long)nextTrampolineAddressToWrite, (long long)newDisplacementValue);

#ifdef HOOKSHOT64
                    // In 64-bit mode, it is necessary to verify that the new displacement value falls within the range of possible rel32 values.
                    // In 32-bit mode, this is not a problem because the entire address space is accessible via rel32.
                    if ((newDisplacementValue > (intptr_t)INT32_MAX) || (newDisplacementValue < (intptr_t)INT32_MIN))
                    {
                        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_DISPLACEMENT_OUT_OF_RANGE_FORMAT, i);
                        return false;
                    }
#endif

                    // TODO: if the new displacement is too wide to fit into the original instruction, then handle it.

                    if (false == originalInstructions[i].SetMemoryDisplacement((int64_t)newDisplacementValue))
                    {
                        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_DISPLACEMENT_FAILED_TO_SET_FORMAT, i);
                        return false;
                    }
                }
                else
                    Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_DISPLACEMENT_SMALL_ENOUGH_FORMAT, i);
            }

            // Third sub-part. Re-encode the instruction.
            int numEncodedBytes = originalInstructions[i].EncodeInstruction(nextTrampolineAddressToWrite, numTrampolineBytesLeft);

            if (originalInstructions[i].GetLengthBytes() != numEncodedBytes)
            {
                Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_FAILED_FORMAT, i, (long long)nextTrampolineAddressToWrite);
                code.original.word[0] = kOriginalCodeDefault;
                return false;
            }

            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_SUCCESSFUL_FORMAT, i, numEncodedBytes, (long long)nextTrampolineAddressToWrite);
            numTrampolineBytesWritten += numEncodedBytes;
        }

        // If the last transplanted instruction is terminal, then the entirety of the original function was transplanted, so there is no need to jump to an address in the original function.
        // Otherwise, there are more instructions left in the original function, so make sure to jump to them after executing the trasnplanted instructions.
        if (false == originalInstructions.back().IsTerminal())
        {
            const int numTrampolineBytesLeft = sizeof(code.original) - numTrampolineBytesWritten - numExtraTrampolineBytesUsed;
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_ENCODE_FINAL_NONTERMINAL_JUMP_FORMAT, (long long)&originalFunctionBytes[numOriginalFunctionBytes], numTrampolineBytesLeft);
            
            if (false == X86Instruction::WriteJumpInstruction(&code.original.byte[numTrampolineBytesWritten], numTrampolineBytesLeft, &originalFunctionBytes[numOriginalFunctionBytes]))
            {
                code.original.word[0] = kOriginalCodeDefault;
                return false;
            }
        }

        FlushInstructionCache(GetCurrentProcess(), &code, sizeof(code));

        
        // Fourth sub-part.  Overwriting the original function might require playing with virtual memory permissions.
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_LAST_STEP_ATTEMPT_OVERWRITE_FORMAT, (long long)target, X86Instruction::kJumpInstructionLengthBytes, (long long)&code.hook, numOriginalFunctionBytes - X86Instruction::kJumpInstructionLengthBytes);

        DWORD originalProtection = 0;
        if (0 == VirtualProtect(&originalFunctionBytes[0], numOriginalFunctionBytes, PAGE_EXECUTE_READWRITE, &originalProtection))
        {
            code.original.word[0] = kOriginalCodeDefault;
            return false;
        }

        const bool writeJumpResult = X86Instruction::WriteJumpInstruction(&originalFunctionBytes[0], X86Instruction::kJumpInstructionLengthBytes, &code.hook);
        if (true == writeJumpResult && numOriginalFunctionBytes > X86Instruction::kJumpInstructionLengthBytes)
            X86Instruction::FillWithNop(&originalFunctionBytes[X86Instruction::kJumpInstructionLengthBytes], numOriginalFunctionBytes - X86Instruction::kJumpInstructionLengthBytes);

        DWORD unusedOriginalProtection = 0;
        const bool restoreProtectionResult = (0 != VirtualProtect(&originalFunctionBytes[0], numOriginalFunctionBytes, originalProtection, &unusedOriginalProtection));
        if (true == restoreProtectionResult)
            FlushInstructionCache(GetCurrentProcess(), &originalFunctionBytes[0], (SIZE_T)numOriginalFunctionBytes);

        if (writeJumpResult && restoreProtectionResult)
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_TRAMPOLINE_SET_HOOK_COMPLETE_FORMAT, (long long)hook, (long long)target);
            return true;
        }
        else
        {
            Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityDebug, IDS_HOOKSHOT_TRAMPOLINE_SET_HOOK_FAILED_FORMAT, (long long)hook, (long long)target);
            code.original.word[0] = kOriginalCodeDefault;
            return false;
        }
    }
}
