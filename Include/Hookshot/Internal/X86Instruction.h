/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 **************************************************************************//**
 * @file X86Instruction.h
 *   Declaration of object interface for manipulating binary X86 instructions.
 *****************************************************************************/

#pragma once

extern "C" {
#include "xed/xed-interface.h"
}

#include <climits>
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

        /// Preamble for writing an unconditional jump instruction as in #WriteJumpInstruction.
        /// Represents the fixed opcode bytes that come before a relative 32-bit jump displacement.
        static constexpr uint8_t kJumpInstructionPreamble[] = {
            0xe9                                                            // jmp rel32
        };

        /// Length of an unconditional jump instruction, in bytes, as written by #WriteJumpInstruction.
        /// Equal to the length of the binary preamble plus the size of a 32-bit displacement.
        static constexpr int kJumpInstructionLengthBytes = sizeof(kJumpInstructionPreamble) + sizeof(uint32_t);

        /// Value used to indicate an invalid memory displacement.
        static constexpr int64_t kInvalidMemoryDisplacement = INT64_MIN;


    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Holds information on a position-dependent memory reference that might be present in an X86 instruction.
        class PositionDependentMemoryReference
        {
        public:
            // -------- CONSTANTS ------------------------------------------ //

            /// Value that specifies that an instruction does not contain a position-dependent memory reference.
            static constexpr int kReferenceDoesNotExist = INT_MIN;

            /// Value that specifies that an instruction's position-dependent memory reference is a branch displacement.
            static constexpr int kReferenceIsRelativeBranchDisplacement = -1;


        private:
            // -------- INSTANCE VARIABLES --------------------------------- //

            /// Holds operand information on the instruction's position-dependent memory reference.
            /// Either a constant from above or the actual index of a memory operand.
            int operandLocation;


        public:
            // -------- CONSTRUCTION AND DESTRUCTION ----------------------- //

            /// Default constructor.
            inline PositionDependentMemoryReference(void) : operandLocation(kReferenceDoesNotExist)
            {
                // Nothing to do here.
            }

            /// Initialization constructor.
            /// Allows one-line initialization from a decoded instruction.
            /// @param [in] decodedInstruction Instruction in which to search for a position-dependent memory reference operand.
            inline PositionDependentMemoryReference(xed_decoded_inst_t* decodedInstruction) : operandLocation(OperandLocationFromInstruction(decodedInstruction))
            {
                // Nothing to do here.
            }


            // -------- CLASS METHODS -------------------------------------- //

            /// Searches through the specified instruction for a position-dependent memory reference and returns a value accordingly.
            /// @param [in] decodedInstruction Decoded instruction in which to search.
            /// @return Memory operand index, or one of the class-level constants defined previously.
            static int OperandLocationFromInstruction(xed_decoded_inst_t* decodedInstruction);


            // -------- INSTANCE METHODS ----------------------------------- //

            /// Clears out any operand location information stored.
            inline void ClearOperandLocation(void)
            {
                operandLocation = kReferenceDoesNotExist;
            }

            /// Retrieves the operand location, if any, of a position-dependent memory reference.
            /// @return Memory operand index, or one of the class-level constants defined previously.
            inline int GetOperandLocation(void) const
            {
                return operandLocation;
            }

            /// Searches through the specified instruction for a position-dependent memory reference and updates this object accordingly.
            /// @param [in] decodedInstruction Decoded instruction in which to search.
            inline void SetFromInstruction(xed_decoded_inst_t* decodedInstruction)
            {
                operandLocation = OperandLocationFromInstruction(decodedInstruction);
            }
        };

        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds the represented instruction in decoded form.
        xed_decoded_inst_t decodedInstruction;

        /// Holds the original address of the represented instruction.
        /// Used to convert IP-relative operands to absolute addresses.
        void* address;

        /// Specifies if the represented instruction is valid.
        bool valid;

        /// Holds the initial byte of the instruction if it might be a REX prefix.
        uint8_t possibleRexPrefix;

        /// Holds information on any position-dependent memory accesses performed by this instruction, if any.
        PositionDependentMemoryReference positionDependentMemoryReference;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        X86Instruction(void);


        // -------- CLASS METHODS ------------------------------------------ //

        /// Initializes the X86 instruction subsystem. Must be called once during program initialization.
        static inline void Initialize(void)
        {
            xed_tables_init();
        }

        /// Determines if a jump instruction can be assembled from the specified location to the specified location.
        /// @param [in] from Proposed address of the jump instruction itself.
        /// @param [in] to Proposed target address of the jump instruction.
        /// @return `true` if possible, `false` if not.
        static bool CanWriteJumpInstruction(const void* const from, const void* const to);

        /// Fills the specified buffer with nop instructions.
        /// @param [out] buf Buffer to which nop instructions should be written.
        /// @param [in] numBytes Size of the buffer to fill, in bytes.
        static void FillWithNop(void* const buf, const size_t numBytes);

        /// Places a jump instruction with the specified displacement at the specified location.
        /// Supplied buffer must be large enough to hold #kJumpInstructionLengthBytes bytes.
        /// @param [out] where Buffer to which the trampoline jump operation should be written.
        /// @param [in] whereSizeBytes Number of bytes available for writing the jump instruction.
        /// @param [in] to Target address of the jump instruction.
        /// @return `true` on success, `false` on failure due to the jump target being too far for a rel32 jump instruction.
        static bool WriteJumpInstruction(void* const where, const int whereSizeBytes, const void* const to);


        // -------- INSTANCE METHODS --------------------------------------- //

        /// If this instruction contains a position-dependent memory reference, determines if it is possible to set the displacement to the specified value.
        /// @param [in] displacement Desired displacement value to check.
        /// @return `true` if so, `false` if not or if either this instruction is invalid or no such memory reference exists.
        bool CanSetMemoryDisplacementTo(const int64_t displacement) const;

        /// Attempts to decode the instruction at the specified address, reading up to a maximum of the specified number of bytes.
        /// On success, this object captures the decoded representation of the requested instruction.
        /// @param [in] instruction Address of the instruction to decode.
        /// @param [in] maxLengthBytes Maximum number of bytes to read from the instruction address.
        /// @return `true` on success, `false` on failure.
        bool DecodeInstruction(void* const instruction, const int maxLengthBytes = kMaxInstructionLengthBytes);

        /// Attempts to encode this instruction to the specified address, overwriting a number of bytes equal to the length of this instruction.
        /// @param [out] buf Destination buffer.
        /// @param [in] maxLengthBytes Maximum allowed encoding length, in bytes.
        /// @return Number of bytes written on success, 0 on failure.
        int EncodeInstruction(void* const buf, const int maxLengthBytes = kMaxInstructionLengthBytes) const;

        /// If this instruction contains a position-dependent memory reference, computes and returns the absolute target address of said reference.
        /// @return Absolute target address, or `nullptr` if either this instruction is invalid or no such memory reference exists.
        void* GetAbsoluteMemoryReferenceTarget(void) const;

        /// Retrieves and returns the original location in memory of this instruction.
        /// @return Original address of this instruction, or `nullptr` if it is invalid.
        inline void* GetAddress(void) const
        {
            return address;
        }

        /// Retrieves and returns the number of bytes that represent the instruction in its encoded form.
        /// @return Number of bytes, or -1 if the instruction is invalid.
        int GetLengthBytes(void) const;

        /// If this instruction contains a position-dependent memory reference, computes and returns the maximum allowed displacement that can be incorporated into this instruction.
        /// @return Maximum displacement value, or #kInvalidMemoryDisplacement if either this instruction is invalid or no such memory reference exists.
        int64_t GetMaxMemoryDisplacement(void) const;

        /// If this instruction contains a position-dependent memory reference, determines the value of the displacement.
        /// @return Actual displacement value, or #kInvalidMemoryDisplacement if either this instruction is invalid or no such memory reference exists.
        int64_t GetMemoryDisplacement(void) const;

        /// If this instruction contains a position-dependent memory reference, determines the width in bits of the binary representation of the displacement value.
        /// @return Displacement value width in bits, or 0 if either this instruction is invalid or no such memory reference exists.
        int GetMemoryDisplacementWidthBits(void) const;

        /// If this instruction contains a position-dependent memory reference, computes and returns the minimum allowed displacement that can be incorporated into this instruction.
        /// @return Maximum displacement value, or #kInvalidMemoryDisplacement if either this instruction is invalid or no such memory reference exists.
        int64_t GetMinMemoryDisplacement(void) const;

        /// Determines if this instruction makes a memory reference whose address depends on this instruction's position.
        /// A memory reference for which this method returns `true` uses the value of the instruction pointer in computing the effective address.
        /// This could be because the instruction uses a relative branch displacement or, in 64-bit mode, RIP-relative addressing.
        /// @return `true` if so, `false` otherwise or if this instruction is invalid.
        inline bool HasPositionDependentMemoryReference(void)
        {
            return (PositionDependentMemoryReference::kReferenceDoesNotExist != positionDependentMemoryReference.GetOperandLocation());
        }

        /// Determines if this instruction makes a memory reference, in the form of a relative branch displacement, whose target depends on this instruction's position.
        /// A memory reference for which this method returns `true` uses the value of the instruction pointer in computing the target of a possible jump.
        /// An instruction for which this method returns `false` but #HasPositionDependentMemoryReference returns `true` uses its position-dependent memory reference to access instruction operand data.
        /// @return `true` if so, `false` otherwise or if this instruction is invalid.
        inline bool HasRelativeBranchDisplacement(void)
        {
            return (PositionDependentMemoryReference::kReferenceIsRelativeBranchDisplacement == positionDependentMemoryReference.GetOperandLocation());
        }

        /// Determines if this instruction is recognized as one that is often used for padding purposes and likely part of a padding buffer.
        /// Only a few single-byte instructions currently satisfy this criterion.
        /// @return `true` if so, `false` otherwise or if this instruction is invalid.
        bool IsPadding(void) const;

        /// If this instruction is one that is often used for padding purposes, check if this instruction is part of a padding buffer with at least the specified number of bytes after it.
        /// In order for this function to return `true` the this instruction must be recognized as a valid padding instruction and be followed by at least 3 identical instructions.
        /// @param [in] numBytes Number of bytes to check. Assumed to be the value 4 if it is less than 4.
        /// @return `true` if so, `false` otherwise or if this instruction is invalid.
        bool IsPaddingWithLengthAtLeast(int numBytes) const;

        /// Determines if this instruction marks the end of a control flow (i.e. end of a function, unconditional jump to someplace else, etc.)
        /// @return `true` if so, `false` otherwise or if this instruction is invalid.
        bool IsTerminal(void) const;

        /// Specifies whether the instruction represented by this object is valid.
        /// If not, no other methods to obtain information about the instruction will produce valid results.
        inline bool IsValid(void) const
        {
            return valid;
        }

        /// Attempts to print the disassembled instruction to the specified buffer.
        /// @param [out] buf Buffer to fill.
        /// @param [in] numChars Size of the buffer, in character units.
        bool PrintDisassembly(wchar_t* const buf, const size_t numChars) const;

        /// If this instruction contains a position-dependent memory reference, attempts to update the displacement value to the specified value.
        /// @param [in] displacement Displacement value to set.
        /// @return `true` on success, `false` on failure.
        bool SetMemoryDisplacement(const int64_t displacement);
    };
}
