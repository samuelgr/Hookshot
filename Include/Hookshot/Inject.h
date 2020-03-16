/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Inject.h
 *   C++ interface to injection code written in assembly.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "Globals.h"
#include "InjectResult.h"

#include <cstddef>


/// Initializes the state required to interact with the injected process.
/// Requires the injected process handle and SInjectData base address.
#define injectInit(hproc, pinjectdata)      size_t syncVar1 = 1, syncVar2 = 2; const HANDLE injectedProcessHandle = hproc; SInjectData* const injectedProcessData = (SInjectData*)pinjectdata; size_t* const syncFlagAddress = &(injectedProcessData->sync)

/// Reads the specified SInjectData field from the injected process' data region.
/// Requires the field name and a pointer to an output variable as parameters.
#define injectDataFieldRead(field, pdest)   injectDataFieldReadImpl(injectedProcessHandle, (const void*)(&injectedProcessData->field), pdest, sizeof(injectedProcessData->field))

/// Writes the specified SInjectData field to the injected process' data region.
/// Requires the field name and a pointer to an input variable as parameters.
#define injectDataFieldWrite(field, psrc)   injectDataFieldWriteImpl(injectedProcessHandle, (void*)(&injectedProcessData->field), psrc, sizeof(injectedProcessData->field))

/// Performs the first part of synchronization.
/// Once this operation completes successfully, the injecting process is ready to continue executing but the injected process cannot advance until `injectSyncAdvance()` is invoked.
/// Returns `true` if successful, `false` if reading the sync flag from the injected process failed.
#define injectSyncWait()                    injectSyncWaitImpl(syncVar1, syncVar2, injectedProcessHandle, syncFlagAddress)

/// Performs the second part of synchronization.
/// Returns `true` if successful, `false` if writing the sync flag from the injected process failed.
#define injectSyncAdvance()                 injectSyncAdvanceImpl(syncVar1, syncVar2, injectedProcessHandle, syncFlagAddress)

/// Synchronizes between injecting and injected processes, essentially acting as an execution barrier in the code running in either.
/// Returns `true` if sync was successful, `false` if reading or writing the sync flag from the injected process failed.
#define injectSync()                        (injectSyncWait() && injectSyncAdvance())


namespace Hookshot
{
    /// Reads data from the injected process.
    /// Not intended to be invoked other than by using appropriate macros.
    static inline bool injectDataFieldReadImpl(const HANDLE& processHandle, const void* const sourceAddress, void* const destAddress, const size_t size)
    {
        SIZE_T numBytesRead = 0;
        
        if ((FALSE == ReadProcessMemory(processHandle, sourceAddress, destAddress, (SIZE_T)size, &numBytesRead)) || ((SIZE_T)size != numBytesRead))
            return false;
        
        return true;
    }

    /// Writes data to the injected process.
    /// Not intended to be invoked other than by using appropriate macros.
    static inline bool injectDataFieldWriteImpl(const HANDLE& processHandle, void* const destAddress, const void* const sourceAddress, const size_t size)
    {
        SIZE_T numBytesWritten = 0;

        if ((FALSE == WriteProcessMemory(processHandle, destAddress, sourceAddress, (SIZE_T)size, &numBytesWritten)) || ((SIZE_T)size != numBytesWritten))
            return false;

        return true;
    }
    
    /// Implements the first part of the syncing logic.
    /// Waits until the injected process writes the expected value to the sync flag and then returns.
    /// Not intended to be invoked other than by using appropriate macros.
    static inline bool injectSyncWaitImpl(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
    {
        size_t syncFlagValue = 0;
        SIZE_T numBytes = 0;

        while (syncFlagValue != syncVar1)
        {
            if ((FALSE == ReadProcessMemory(syncProcessHandle, syncFlagAddress, &syncFlagValue, sizeof(syncFlagValue), &numBytes)) || (sizeof(syncFlagValue) != numBytes))
                return false;
        }

        return true;
    }

    /// Implements the second part of the syncing logic.
    /// Writes the value to the sync flag for which the injected process is currently waiting.
    /// Not intended to be invoked other than by using appropriate macros.
    static inline bool injectSyncAdvanceImpl(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
    {
        SIZE_T numBytes = 0;

        if ((FALSE == WriteProcessMemory(syncProcessHandle, syncFlagAddress, &syncVar2, sizeof(syncVar2), &numBytes)) || (sizeof(syncVar2) != numBytes))
            return false;

        syncVar1 += 2;
        syncVar2 += 2;

        return true;
    }
    
    /// Defines the structure of the data exchanged between the injecting and injected processes.
    /// One instance of this structure is placed into the data region and accessed by both the injecting and injected processes.
    /// A corresponding structure definition must appear in "Inject.inc" for the assembly code.
    struct SInjectData
    {
        size_t sync;                                                    ///< Synchronization flag between injecting and injected processes.
        size_t unused1[(128 / sizeof(size_t)) - 1];                     ///< Padding for 128-byte alignment.

        uint32_t injectionResultCodeSuccess;                            ///< Result code to use to indicate success.
        uint32_t injectionResultCodeLoadLibraryFailed;                  ///< Result code to use to indicate that a LoadLibrary operation failed.  Set by the injecting process.
        uint32_t injectionResultCodeGetProcAddressFailed;               ///< Result code to use to indicate that a GetProcAddress operation failed.  Set by the injecting process.
        uint32_t injectionResultCodeInitializationFailed;               ///< Result code to use to indicate that the loaded library failed to initialize.
        uint32_t unused3[(128 / sizeof(uint32_t)) - 4];                 ///< Padding for 128-byte alignment.
        
        uint32_t injectionResult;                                       ///< Result of the injection operation.  Written by the injected process.
        uint32_t extendedInjectionResult;                               ///< Extended result of the injection operation.  Written by the injected process.
        uint32_t unused4[(128 / sizeof(uint32_t)) - 2];                 ///< Padding for 128-byte alignment.
        
        const void* funcGetLastError;                                   ///< Address of the GetLastError function in the injected process.
        const void* funcGetProcAddress;                                 ///< Address of the GetProcAddress function in the injected process.
        const void* funcLoadLibraryA;                                   ///< Address of the LoadLibraryA function in the injected process.
        const char* strLibraryName;                                     ///< Address of the first character of the string that represents the library name to load.
        const char* strProcName;                                        ///< Address of the first character of the string that represents the procedure name within the loaded library to invoke.
        void* cleanupBaseAddress[5];                                    ///< Base addresses of buffers that should be freed once the injection code loads and passes control to the Hookshot library.
        size_t unused5[(128 / sizeof(size_t)) - 10];                    ///< Padding for 128-byte alignment.
    };

    /// Utility class for managing information about the structure of the assembly-written injected code.
    /// Injected code is dynamically loaded into this process at runtime and then copied to the injected process.
    /// Pointers contained within this class are expressed in this process' address space and correspond to the code that has been loaded.
    /// Requires knowledge of the symbols exported by the modules that contain the code.
    class InjectInfo
    {
    public:
        // -------- CONSTANTS ---------------------------------------------- //
        
        /// Maximum size, in bytes, of the binary files that are loaded.
        static constexpr size_t kMaxInjectBinaryFileSize = 4096;


    private:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Start of the trampoline code block.
        void* injectTrampolineStart;

        /// Marker for where to place the address to which the trampoline should jump.
        /// The assembly code reserves space for this address, and the injecting process must fill it in dynamically.
        void* injectTrampolineAddressMarker;

        /// End of the trampoline code block.
        void* injectTrampolineEnd;

        /// Start of the main code block.
        void* injectCodeStart;

        /// Entry point within the main code block.
        /// This is the address that should be placed at the trampoline address marker after it is converted into a pointer in the injected process' address space.
        void* injectCodeBegin;

        /// End of the main code block.
        void* injectCodeEnd;

        /// Indicator of the result of the initialization of this object.
        EInjectResult initializationResult;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        InjectInfo(void);

        /// Copy constructor. Should never be invoked.
        InjectInfo(const InjectInfo&) = delete;


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectTrampolineStart(void) const
        {
            return injectTrampolineStart;
        }

        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectTrampolineAddressMarker(void) const
        {
            return injectTrampolineAddressMarker;
        }
        
        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectTrampolineEnd(void) const
        {
            return injectTrampolineEnd;
        }

        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectCodeStart(void) const
        {
            return injectCodeStart;
        }

        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectCodeBegin(void) const
        {
            return injectCodeBegin;
        }

        /// Provides read-only access to the correspondingly-named instance variable.
        /// @return Value of the corresponding instance variable.
        inline void* GetInjectCodeEnd(void) const
        {
            return injectCodeEnd;
        }

        /// Specifies the result of attempting to initialize this object.
        /// If not successful, it should be destroyed without any other methods called.
        /// @return Indicator of the result of initialization.
        inline EInjectResult InitializationResult(void) const
        {
            return initializationResult;
        }
    };
}
