/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file CodeInjector.h
 *   Interface declaration for code injection, execution, and synchronization.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "InjectResult.h"

#include <cstddef>


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
    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Base address of the code region of the injected process.
        void* const baseAddressCode;

        /// Base address of the data region of the injected process.
        void* const baseAddressData;

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


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        CodeInjector(void) = delete;

        /// Initialization constructor. The only way to construct an object of this type.
        /// @param [in] baseAddressCode Base address of the code region of the injected process.
        /// @param [in] baseAddressData Base address of the data region of the injected process.
        /// @param [in] entryPoint Entry point for the injected code.
        /// @param [in] sizeCode Size of the code region, in bytes.
        /// @param [in] sizeData Size of the data region, in bytes.
        /// @param [in] injectedProcess Process handle of the injected process.
        /// @param [in] injectedProcessMainThread Main thread handle for the injected process.
        CodeInjector(void* const baseAddressCode, void* const baseAddressData, void* const entryPoint, const size_t sizeCode, const size_t sizeData, const HANDLE injectedProcess, const HANDLE injectedProcessMainThread);


        // -------- CLASS METHODS ------------------------------------------ //

        /// Computes the number of bytes needed to represent the injected code.
        /// @return Number of bytes required for the injected code.
        static size_t GetRequiredCodeSize(void);

        /// Computes the number of bytes needed for the data region within the injected process.
        /// @return Number of bytes needed for the data region.
        static size_t GetRequiredDataSize(void);

        /// Computes the number of bytes occupied by the trampoline code, which replaces the entry point code and causes execution of the injected code.
        /// @return Number of bytes occupied by the trampoline code.
        static size_t GetTrampolineCodeSize(void);


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
        EInjectResult Check(void);
        
        /// Runs the injected process once the injected code has been set.
        /// @return Indictor of the result of the operation.
        EInjectResult Run(void);

        /// Sets the injected code into the injected process, performing all required operations.
        /// @return Indictor of the result of the operation.
        EInjectResult Set(void);
    };
}
