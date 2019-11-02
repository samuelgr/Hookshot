/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Trampoline.h
 *   Data structure declaration for individual trampolines.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    /// Generates and holds trampoline code used to invoke the original functionality of a hooked function.
    /// In order to be useful, the memory location where a Trampoline object is stored must have execute permission.
    class Trampoline
    {
    private:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Size of the trampoline, in bytes.
        /// Must be divisible by 16.
        static constexpr size_t kTrampolineSizeBytes = 32;

        /// Size of the trampoline, in words.
        static constexpr size_t kTrampolineSizeWords = kTrampolineSizeBytes / sizeof(uint16_t);

        /// Size of the trampoline, in doublewords.
        static constexpr size_t kTrampolineSizeDwords = kTrampolineSizeBytes / sizeof(uint32_t);

        /// Size of the trampoline, in quadwords.
        static constexpr size_t kTrampolineSizeQwords = kTrampolineSizeBytes / sizeof(uint64_t);

        /// Size of the trampoline, in pointers.
        static constexpr size_t kTrampolineSizePtrs = kTrampolineSizeBytes / sizeof(size_t);

        /// Initial value to be placed at the start of the trampoline code to indicate it is uninitialized.
        /// Consists of a "ud2" instruction followed by a two-byte "nop" instruction.
        /// The "ud2" ensures any attempt to execute will generate an error.
        static constexpr uint32_t kCodeUninitialized = 0x90660b0f;


        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Raw trampoline code type.
        /// Large enough to hold the entire code region, and allows different access granularities.
        union UTrampolineCode
        {
            uint8_t byte[kTrampolineSizeBytes];                         ///< Byte-level access
            uint16_t word[kTrampolineSizeWords];                        ///< Word-level access
            uint32_t dword[kTrampolineSizeDwords];                      ///< Doubleword-level access
            uint64_t qword[kTrampolineSizeQwords];                      ///< Quadword-level access
            size_t ptr[kTrampolineSizePtrs];                            ///< Pointer-sized access
        };


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds the trampoline code itself.
        UTrampolineCode code;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        inline Trampoline(void)
        {
            Reset();
        }


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Specifies if this trampoline is successfully set.
        /// @return `true` if so, `false` otherwise.
        inline bool IsSet(void) const
        {
            return (kCodeUninitialized != code.dword[0]);
        }

        /// Resets this trampoline to its uninitialized state.
        inline void Reset(void)
        {
            code.dword[0] = kCodeUninitialized;
        }

        /// Sets the trampoline so it effectively jumps to the specified address when executed.
        /// Reads the first instruction at the specified address and copies it into the trampoline.
        /// Appends a "jmp" instruction that targets the second instruction at the specified address.
        /// Does not modify the function at the specified address.
        /// @param [in] target Instruction address that this trampoline should target.
        /// @return `true` if successful, `false` otherwise.
        bool SetTarget(void* target);
    };
}
