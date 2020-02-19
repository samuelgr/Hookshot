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
#include <cstdlib>
#include <hookshot.h>


namespace Hookshot
{
    /// Generates and holds two types of trampoline code.
    /// The first type is used to transfer control to a hook function whenever a specified target function is invoked.
    /// The second type is used to invoke the original (i.e. unhooked) functionality of said target function.
    /// In order to be useful, the memory location where a Trampoline object is stored must have execute permission.
    /// Once a target is set, the trampoline itself becomes essentially constant.
    /// Transplanted code may be position-dependent, so this object cannot be moved or copied.
    /// Non-const methods in this class are not thread-safe and require a form of external concurrency control if accessed from multiple threads.
    class Trampoline
    {
    private:
        // -------- CONSTANTS ---------------------------------------------- //
        
        /// Total size of the trampoline, in bytes.
        /// Must be divisible by 32.
        static constexpr size_t kTrampolineSizeBytes = 64;
        
        /// Size, in bytes, of the portion of the trampoline that contains code for invoking the hook function.
        static constexpr size_t kTrampolineSizeHookFunctionBytes = kTrampolineSizeBytes / 4;

        /// Size, in bytes, of the portion of the trampoline that contains transplanted code for invoking the original function.
        static constexpr size_t kTrampolineSizeOriginalFunctionBytes = kTrampolineSizeBytes - kTrampolineSizeHookFunctionBytes;

        /// Loaded into the hook region of the trampoline at initialization time.
        /// Provides the needed preamble and allows room for the hook function address to be specified after-the-fact.
        /// When the trampoline is set, the hook function address is filled in.
        static constexpr uint8_t kHookCodeDefault[kTrampolineSizeHookFunctionBytes] = {
#ifdef HOOKSHOT64
            0x66, 0x90,                                                     // nop
            0xff, 0x25, 0x00, 0x00, 0x00, 0x00,                             // jmp QWORD PTR [rip]
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00                  // <absolute address of hook function>
#else
            0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,           // nop
            0x66, 0x90,                                                     // nop
            0xe9,                                                           // jmp rel32
            0x00, 0x00, 0x00, 0x00                                          // <rel32 address of hook function>
#endif
        };

        /// Loaded into the original function region of the trampoline at initialization time.
        /// This is the ud2 instruction, which acts as an "uninitialized poison" by causing the program to crash when executed.
        /// When the trampoline is set, this region of the code is replaced by actual transplanted code.
        static constexpr uint16_t kOriginalCodeDefault = 0x0b0f;


        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Raw trampoline code type template.
        /// Large enough to hold the entire code region, and allows different access granularities.
        /// @tparam kSizeBytes Total size in bytes of the code region.  Must be divisible by 8.
        template <size_t kSizeBytes> union UTrampolineCode
        {
            uint8_t byte[kSizeBytes / sizeof(uint8_t)];                 ///< Byte-level access
            uint16_t word[kSizeBytes / sizeof(uint16_t)];               ///< Word-level access
            uint32_t dword[kSizeBytes / sizeof(uint32_t)];              ///< Doubleword-level access
            uint64_t qword[kSizeBytes / sizeof(uint64_t)];              ///< Quadword-level access
            size_t ptr[kSizeBytes / sizeof(size_t)];                    ///< Pointer-sized access
        };
        
        /// Layout definition for the trampoline code regions.
        struct STrampolineCode
        {
            UTrampolineCode<kTrampolineSizeHookFunctionBytes> hook;             ///< Holds the trampoline code for transferring control to the hook function when the original function is invoked.
            UTrampolineCode<kTrampolineSizeOriginalFunctionBytes> original;     ///< Holds the code transplanted from the original function and used to invoke original functionality.
        };


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds the trampoline code itself.
        STrampolineCode code;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        Trampoline(void);

        /// Copy constructor. Should never be invoked.
        Trampoline(const Trampoline&) = delete;


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Retrieves and returns the address of the hook function.
        /// Valid only if this object is already set, otherwise may return a garbage value.
        /// @return Address of the hook function.
        inline TFunc GetHookFunction(void) const
        {
            return HookAddressForValue();
        }

        /// Retrieves and returns the address that, when invoked, provides the original functionality of the target function.
        /// Valid only if this object is already set, otherwise may return a garbage value.
        /// @return Address that can be invoked to execute the original functionality of the target function.
        inline TFunc GetOriginalTargetFunction(void) const
        {
            return (void*)&code.original;
        }
        
        /// Specifies if this trampoline has a target set.
        /// @return `true` if so, `false` otherwise.
        inline bool IsTargetSet(void) const
        {
            return (kOriginalCodeDefault != code.original.word[0]);
        }

        /// Sets up this trampoline so that it hooks the specified target.
        /// Transplants code as needed at the specified target address so that the code in this trampoline is executed.
        /// Provides functionality both to hook the specified target and to make the original functionality available.
        /// On failure, the target function address is unmodified.
        /// @param [in] hook Address of the hook function to which this trampoline should transfer control whenever the target is invoked.
        /// @param [in,out] target Address of the function that should be hooked by this trampoline.
        /// @return `true` if successful, `false` otherwise.
        bool SetHookForTarget(TFunc hook, void* target);


    private:
        // -------- HELPERS ------------------------------------------------ //
        
        /// Calculates the jump displacement for a relative jump instruction.
        /// @param [in] addressAfterJmpInstruction Byte address immediately following the jump instruction and its rel32 operand.
        /// @param [in] absoluteTarget Absolute address of the jump target.
        /// @return Jump displacement value.
        static inline size_t ComputeJumpDisplacement(const void* addressAfterJmpInstruction, const void* absoluteTarget)
        {
            // Formula: <absolute target address> = <instruction address after jmp> + <displacement>
            // Known values: <absolute target address> = parameter, <instruction address after jmp> = parameter
            // Value needed: <displacement> = <absolute target address> - <instruction address after jmp>
            return (size_t)absoluteTarget - (size_t)addressAfterJmpInstruction;
        }
        
        /// Computes the address of the hook function, given the value stored in this trampoline.
        /// @return Address of the hook function.
        inline TFunc HookAddressForValue(void) const
        {
#ifdef HOOKSHOT64
            // No transformation required in 64-bit mode because the address is an absolute jump target.
            return (void*)code.hook.ptr[_countof(code.hook.ptr) - 1];
#else
            // Computation is required in 32-bit mode because the value is a rel32 jump displacement.
            // Formula rel32: <absolute target address> = <instruction address after jmp> + <displacement>
            // Known values: <instruction address after jmp> = byte address directly after code.hook, <displacement> = stored value
            return (void*)((size_t)(&code.hook.ptr[_countof(code.hook.ptr)]) + code.hook.ptr[_countof(code.hook.ptr) - 1]);
#endif
        }
        
        /// Computes the value to be inserted into the trampoline's hook address field.
        /// Depending on the architecture and instruction sequence contained in #kHookCodeDefault, the address may require transformation before insertion into the trampoline.
        /// @param [in] hook Address of the hook function.
        /// @return Transformed value of the hook function address, to be inserted into the trampoline.
        inline size_t ValueForHookAddress(TFunc hook) const
        {
#ifdef HOOKSHOT64
            // No transformation required in 64-bit mode because the value is an absolute jump target address.
            return (size_t)hook;
#else
            // Computation is required in 32-bit mode because the value is a rel32 jump displacement.
            return ComputeJumpDisplacement(&code.hook.ptr[_countof(code.hook.ptr)], hook);
#endif
        }
    };
}
