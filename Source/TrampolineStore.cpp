/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TrampolineStore.cpp
 *   Implementation of top-level data structure for trampoline objects.
 *****************************************************************************/

#include "ApiWindows.h"
#include "TrampolineStore.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    // -------- INTERNAL FUNCTIONS ------------------------------------- //

    /// Allocates a buffer suitable for holding Trampoline objects optionally using a specified base address.
    /// @param [in] baseAddress Desired base address for the buffer.
    /// @return Pointer to the allocated buffer, or `NULL` on failure.
    static inline Trampoline* AllocateTrampolineBuffer(void* baseAddress = NULL)
    {
        return (Trampoline*)VirtualAlloc(baseAddress, TrampolineStore::kTrampolineStoreSizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "TrampolineStore.h" for documentation.

    TrampolineStore::TrampolineStore(void) : count(0), trampolines(NULL)
    {
        // Nothing to do here.
    }

    // --------

    TrampolineStore::TrampolineStore(void* baseAddress) : count(0), trampolines(AllocateTrampolineBuffer(baseAddress))
    {
        // Nothing to do here.
    }

    // --------

    TrampolineStore::~TrampolineStore(void)
    {
        if (NULL != trampolines && 0 == Count())
            VirtualFree(trampolines, 0, MEM_RELEASE);
    }

    // --------
    
    TrampolineStore::TrampolineStore(TrampolineStore&& other) : count(other.count), trampolines(other.trampolines)
    {
        other.count = 0;
        other.trampolines = NULL;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "TrampolineStore.h" for documentation.

    int TrampolineStore::Allocate(void)
    {
        if ((NULL == trampolines) || (count >= kTrampolineStoreCount))
            return -1;

        new (&trampolines[count]) Trampoline();
        return count++;
    }

    // --------

    bool TrampolineStore::DeallocateIfNotSet(void)
    {
        if (trampolines[count - 1].IsTargetSet())
            return false;

        count -= 1;
        return true;
    }
}
