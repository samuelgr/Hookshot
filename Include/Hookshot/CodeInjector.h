/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file CodeInjector.h
 *   Interface declaration for code injection, execution, and synchronization.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "Inject.h"
#include "InjectResult.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    /// Encapsulates all knowledge of the actual code that is injected into the process.
    /// Exposes methods for injecting the code, starting the process, and synchronizing with it.
    /// Does not claim ownership over any handles used, so they must be kept valid externally throughout the duration of an instance's existance.
    /// Prior to injection, the process' main thread must be in suspended state and guaranteed to execute in the very near future the code designated as the entry point.
    /// Upon completion of all injection operations, control within the injected process will return to the designated entry point code.
    /// For newly-created processes in suspended state, the entry point can simply be the starting address of the process. Once injection completes successfully, the process will simply start normally.
    class CodeInjector
    {
    public:
        // -------- CONSTANTS ---------------------------------------------- //
        
        /// Maximum number of bytes that the trampoline code is allowed to required.
        static constexpr unsigned int kMaxTrampolineCodeBytes = 128;

        
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Base address of the code region of the injected process.
        void* const baseAddressCode;

        /// Base address of the data region of the injected process.
        void* const baseAddressData;

        /// Specifies if the buffer identified as containing code needs to be cleaned up in the injected process once it is running.
        const bool cleanupCodeBuffer;

        /// Specifies if the buffer identified as containing data needs to be cleaned up in the injected process once it is running.
        const bool cleanupDataBuffer;

        /// Entry point for the injected code.
        void* const entryPoint;

        /// Size of the code region, in bytes.
        const size_t sizeCode;

        /// Size of the data region, in bytes.
        const size_t sizeData;

        /// Process handle of the injected process.
        const HANDLE injectedProcess;

        /// Main thread handle for the injected process.
        const HANDLE injectedProcessMainThread;

        /// Container for holding the code that gets replaced by trampoline code.
        uint8_t oldCodeAtTrampoline[kMaxTrampolineCodeBytes];

        /// Utility object for providing access to all code being injected.
        const InjectInfo injectInfo;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        CodeInjector(void) = delete;

        /// Initialization constructor. The only way to construct an object of this type.
        /// @param [in] baseAddressCode Base address of the code region of the injected process.
        /// @param [in] baseAddressData Base address of the data region of the injected process.
        /// @param [in] cleanupCodeBuffer If true, the injected process will use `VirtualFree()` to free the memory starting at baseAddressCode.
        /// @param [in] cleanupDataBuffer If true, the injected process will use `VirtualFree()` to free the memory starting at baseAddressData.
        /// @param [in] entryPoint Entry point for the injected code.
        /// @param [in] sizeCode Size of the code region, in bytes.
        /// @param [in] sizeData Size of the data region, in bytes.
        /// @param [in] injectedProcess Process handle of the injected process.
        /// @param [in] injectedProcessMainThread Main thread handle for the injected process.
        CodeInjector(void* const baseAddressCode, void* const baseAddressData, const bool cleanupCodeBuffer, const bool cleanupDataBuffer, void* const entryPoint, const size_t sizeCode, const size_t sizeData, const HANDLE injectedProcess, const HANDLE injectedProcessMainThread);

        /// Copy constructor. Should never be invoked.
        CodeInjector(const CodeInjector&) = delete;


    public:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Sets the injected code into the injected process and runs it upon completion.
        /// Performs the actual operations of copying over code to the right locations and then executing it.
        /// Upon successful completion, the main thread of the injected process will be suspended and, once resumed, will return control to the entry point specified at construction time.
        /// @return Indicator of the result of the operation.
        EInjectResult SetAndRun(void);


    private:
        // -------- HELPERS ------------------------------------------------ //

        /// Validates all of the parameters specified at object creation time.
        /// Sources of possible errors include null pointers and insufficiently-sized code or data regions.
        /// @return Indictor of the result of the operation.
        EInjectResult Check(void) const;

        /// Computes the number of bytes needed to represent the injected code.
        /// @return Number of bytes required for the injected code.
        size_t GetRequiredCodeSize(void) const;

        /// Computes the number of bytes needed for the data region within the injected process.
        /// @return Number of bytes needed for the data region.
        size_t GetRequiredDataSize(void) const;

        /// Computes the number of bytes occupied by the trampoline code, which replaces the entry point code and causes execution of the injected code.
        /// @return Number of bytes occupied by the trampoline code.
        size_t GetTrampolineCodeSize(void) const;

        /// Determines the location within the injected process' address space of the GetLastError, GetProcAddress, and LoadLibraryA functions.
        /// These are required to be passed to the injected code so they may be invoked.
        /// @param [out] addrGetLastError On success, filled with the address of GetLastError.
        /// @param [out] addrGetProcAddress On success, filled with the address of GetProcAddress.
        /// @param [out] addrLoadLibraryA On success, filled with the address of LoadLibraryA. 
        /// @return `true` on success, `false` on failure.
        bool LocateFunctions(void*& addrGetLastError, void*& addrGetProcAddress, void*& addrLoadLibraryA) const;
        
        /// Runs the injected process once the injected code has been set.
        /// @return Indictor of the result of the operation.
        EInjectResult Run(void);

        /// Sets the injected code into the injected process, performing all required operations.
        /// @return Indictor of the result of the operation.
        EInjectResult Set(void);

        /// Returns the code region occupied by the trampoline to its original content.
        /// This is necessary to allow the injected process to execute as normal.
        /// @return Indicator of the result of the operation.
        EInjectResult UnsetTrampoline(void);
    };
}
