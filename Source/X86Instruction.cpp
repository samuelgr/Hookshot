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

#include <climits>
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
    static constexpr xed_state_t kXedMachineState = {XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b};
#else
    static constexpr xed_state_t kXedMachineState = {XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b};
#endif


    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "X86Instruction.h" for documentation.

    X86Instruction::X86Instruction(void) : decodedInstruction(), address(NULL), valid(false), positionDependentMemoryReference()
    {
        // Nothing to do here.
    }


    // -------- CLASS METHODS ------------------------------------------ //
    // See "X86Instruction.h" for documentation.

    int X86Instruction::PositionDependentMemoryReference::OperandLocationFromInstruction(xed_decoded_inst_t* const decodedInstruction)
    {
        // First, look through all the operands and check for any that are relative branch displacements.
        // The target of a relative branch displacement is position-dependent.
        const unsigned int numOperands = xed_inst_noperands(xed_decoded_inst_inst(decodedInstruction));
        for (unsigned int i = 0; i < numOperands; ++i)
        {
            const xed_operand_enum_t operandName = xed_operand_name(xed_inst_operand(xed_decoded_inst_inst(decodedInstruction), i));
            switch (operandName)
            {
            case XED_OPERAND_RELBR:
                return kReferenceIsRelativeBranchDisplacement;

            default:
                break;
            }
        }

#ifdef HOOKSHOT64
        // Next, scan for any memory reference operands that use the instruction pointer register as a base.
        // Per Intel documentation, RIP-relative addressing is only supported in 64-bit code, which means that RIP is the only possible base register.
        const unsigned int numMemoryOperands = xed_decoded_inst_number_of_memory_operands(decodedInstruction);
        for (unsigned int i = 0; i < numMemoryOperands; ++i)
        {
            const xed_reg_enum_t baseRegister = xed_decoded_inst_get_base_reg(decodedInstruction, i);
            switch (baseRegister)
            {
            case XED_REG_RIP:
                return i;

            default:
                break;
            }
        }
#endif

        return kReferenceDoesNotExist;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "X86Instruction.h" for documentation.

    bool X86Instruction::CanSetMemoryDisplacementTo(const int64_t displacement) const
    {
        if (false == valid)
            return false;

        if (PositionDependentMemoryReference::kReferenceDoesNotExist == positionDependentMemoryReference.GetOperandLocation())
            return false;

        return (displacement >= GetMinMemoryDisplacement() && displacement <= GetMaxMemoryDisplacement());
    }

    // --------
    
    bool X86Instruction::DecodeInstruction(const void* const instruction, const int maxLengthBytes)
    {
        xed_decoded_inst_zero_set_mode(&decodedInstruction, &kXedMachineState);

        if (XED_ERROR_NONE != xed_decode(&decodedInstruction, (const unsigned char*)instruction, maxLengthBytes))
        {
            address = NULL;
            positionDependentMemoryReference.ClearOperandLocation();
            valid = false;
            return false;
        }

        address = instruction;
        positionDependentMemoryReference.SetFromInstruction(&decodedInstruction);
        valid = true;
        return true;
    }

    // --------

    void* X86Instruction::GetAbsoluteMemoryReferenceTarget(void) const
    {
        const int64_t displacement = GetMemoryDisplacement();
        if (INT64_MIN == displacement)
            return NULL;

        const int64_t base = (int64_t)((intptr_t)address) + (int64_t)GetLengthBytes();
        return (void*)(base + displacement);
    }

    // --------

    int X86Instruction::GetLengthBytes(void) const
    {
        if (false == valid)
            return -1;

        return (int)xed_decoded_inst_get_length(&decodedInstruction);
    }

    // --------

    int64_t X86Instruction::GetMaxMemoryDisplacement(void) const
    {
        const int64_t memoryDisplacementWidthBits = (int64_t)GetMemoryDisplacementWidthBits();
        if (0ll == memoryDisplacementWidthBits)
            return 0ll;

        return (1ll << (memoryDisplacementWidthBits - 1ll)) - 1ll;
    }

    // --------

    int64_t X86Instruction::GetMemoryDisplacement(void) const
    {
        if (false == valid)
            return INT64_MIN;

        switch (positionDependentMemoryReference.GetOperandLocation())
        {
        case PositionDependentMemoryReference::kReferenceDoesNotExist:
            return INT64_MIN;

        case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
            return (int64_t)xed_decoded_inst_get_branch_displacement(&decodedInstruction);

        default:
            return (int64_t)xed_decoded_inst_get_memory_displacement(&decodedInstruction, positionDependentMemoryReference.GetOperandLocation());
        }
    }

    // --------

    int X86Instruction::GetMemoryDisplacementWidthBits(void) const
    {
        if (false == valid)
            return 0;

        switch (positionDependentMemoryReference.GetOperandLocation())
        {
        case PositionDependentMemoryReference::kReferenceDoesNotExist:
            return 0ll;

        case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
            return (int64_t)xed_decoded_inst_get_branch_displacement_width_bits(&decodedInstruction);

        default:
            return (int64_t)xed_decoded_inst_get_memory_displacement_width_bits(&decodedInstruction, positionDependentMemoryReference.GetOperandLocation());
        }
    }

    // --------

    int64_t X86Instruction::GetMinMemoryDisplacement(void) const
    {
        const int64_t memoryDisplacementWidthBits = (int64_t)GetMemoryDisplacementWidthBits();
        if (0ll == memoryDisplacementWidthBits)
            return 0ll;

        return INT64_MIN >> (64ll - memoryDisplacementWidthBits);
    }

    // --------

    bool X86Instruction::SetMemoryDisplacement(const int64_t displacement)
    {
        if (false == CanSetMemoryDisplacementTo(displacement))
            return false;

        switch (positionDependentMemoryReference.GetOperandLocation())
        {
        case PositionDependentMemoryReference::kReferenceDoesNotExist:
            return false;

        case PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement:
            xed_decoded_inst_set_branch_displacement_bits(&decodedInstruction, (int)displacement, GetMemoryDisplacementWidthBits());
            break;

        default:
            xed_decoded_inst_set_memory_displacement_bits(&decodedInstruction, (long long)displacement, GetMemoryDisplacementWidthBits());
            break;
        }

        return (GetMemoryDisplacement() == displacement);
    }
    
    // --------

    bool X86Instruction::TransfersControlUnconditionally(void) const
    {
        if (false == valid)
            return false;

        switch (xed_decoded_inst_get_category(&decodedInstruction))
        {
        case XED_CATEGORY_RET:
        case XED_CATEGORY_UNCOND_BR:
            return true;

        default:
            return false;
        }
    }
}
