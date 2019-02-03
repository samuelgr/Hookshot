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

#include <cstddef>


// -------- TYPE DEFINITIONS ----------------------------------------------- //

/// Defines the structure of the data exchanged between the injecting and injected processes.
/// One instance of this structure is placed into the data region and accessed by both the injecting and injected processes.
/// A corresponding structure definition must appear in "Inject.inc" for the assembly code.
// TODO
struct SInjectData
{
    size_t sync;                                                    ///< Synchronization flag between injecting and injected processes.
    size_t unused1[(128 / sizeof(size_t)) - 1];                     ///< Padding for 128-byte alignment.
};


// -------- MACROS --------------------------------------------------------- //

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

/// Implements the first part of the syncing logic.
/// Waits until the injected process writes the expected value to the sync flag and then returns.
/// Not intended to be invoked other than by using appropriate macros.
inline bool injectSyncImplWait(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
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
inline bool injectSyncImplAdvance(size_t& syncVar1, size_t& syncVar2, const HANDLE& syncProcessHandle, size_t* const& syncFlagAddress)
{
    SIZE_T numBytes = 0;

    if ((FALSE == WriteProcessMemory(syncProcessHandle, syncFlagAddress, &syncVar2, sizeof(syncVar2), &numBytes)) || (sizeof(syncVar2) != numBytes))
        return false;

    syncVar1 += 2;
    syncVar2 += 2;

    return true;
}


// -------- EXTERNAL REFERENCES -------------------------------------------- //

// Required because names automatically get leading underscores in 32-bit mode.
#ifdef HOOKSHOT64
#define injectTrampolineStart               _injectTrampolineStart
#define injectTrampolineAddressMarker       _injectTrampolineAddressMarker
#define injectTrampolineEnd                 _injectTrampolineEnd
#define injectCodeStart                     _injectCodeStart
#define injectCodeBegin                     _injectCodeBegin
#define injectCodeEnd                       _injectCodeEnd
#endif

/// Start of the trampoline code block.
extern "C" void injectTrampolineStart(void);

/// Marker for where to place the address to which the trampoline should jump.
extern "C" void injectTrampolineAddressMarker(void);

/// End of the trampoline code block.
extern "C" void injectTrampolineEnd(void);

/// Start of the main code block.
extern "C" void injectCodeStart(void);

/// Entry point within the main code block.
extern "C" void injectCodeBegin(void);

/// End of the main code block.
extern "C" void injectCodeEnd(void);
