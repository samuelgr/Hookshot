/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Inject.h
 *   C++ interface to injection code written in assembly.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "InjectResult.h"

#include <cstddef>


/// Initializes the state required to synchronize between injecting and injected processes using the `sync` member of SInjectData.
/// Requires the injected process handle and SInjectData base address.
#define injectSyncInit(hproc, pinjectdata)  size_t syncVar1 = 1, syncVar2 = 2; const HANDLE syncProcessHandle = hproc; size_t* const syncFlagAddress = (size_t*)((size_t)pinjectdata + offsetof(SInjectData, sync))

/// Performs the first part of synchronization.
/// Once this operation completes successfully, the injecting process is ready to continue executing but the injected process cannot advance until `injectSyncAdvance()` is invoked.
/// Returns `true` if successful, `false` if reading the sync flag from the injected process failed.
#define injectSyncWait()                    injectSyncImplWait(syncVar1, syncVar2, syncProcessHandle, syncFlagAddress)

/// Performs the second part of synchronization.
/// Returns `true` if successful, `false` if writing the sync flag from the injected process failed.
#define injectSyncAdvance()                 injectSyncImplAdvance(syncVar1, syncVar2, syncProcessHandle, syncFlagAddress)

/// Synchronizes between injecting and injected processes, essentially acting as an execution barrier in the code running in either.
/// Returns `true` if sync was successful, `false` if reading or writing the sync flag from the injected process failed.
#define injectSync()                        (injectSyncWait() && injectSyncAdvance())


namespace Hookshot
{
    /// Implements the first part of the syncing logic.
    /// Waits until the injected process writes the expected value to the sync flag and then returns.
    /// Not intended to be invoked other than by using appropriate macros.
    static inline bool injectSyncImplWait(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
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
    static inline bool injectSyncImplAdvance(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
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
    // TODO
    struct SInjectData
    {
        size_t sync;                                                    ///< Synchronization flag between injecting and injected processes.
        size_t unused1[(128 / sizeof(size_t)) - 1];                     ///< Padding for 128-byte alignment.
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
        static const size_t kMaxInjectBinaryFileSize = 4096;


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

        /// Handle to the file that corresponds to the file containing injected code.
        HANDLE injectFileHandle;

        /// Handle to the file mapping object that is created when the file that contains injected code is mapped into memory.
        HANDLE injectFileMappingHandle;

        /// Base address of the mapped injected code file once it is mapped into memory.
        void* injectFileBase;

        /// Indicator of the result of the initialization of this object.
        EInjectResult initializationResult;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        InjectInfo(void);

        /// Default destructor.
        ~InjectInfo(void);

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
