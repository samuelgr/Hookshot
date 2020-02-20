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
    // -------- INTERNAL CONSTANTS ------------------------------------- //

    /// Allocation type specifier for the individual trampoline buffer spaces.
    static constexpr DWORD kTrampolineAllocationType = MEM_RESERVE | MEM_COMMIT;

    /// Allocation protection specifier for the individual trampoline buffer spaces.
    static constexpr DWORD kTrampolineAllocationProtect = PAGE_EXECUTE_READWRITE;

    
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "TrampolineStore.h" for documentation.

    TrampolineStore::TrampolineStore(void) : count(0), trampolines(NULL)
    {
        trampolines = (Trampoline*)VirtualAlloc(NULL, kTrampolineStoreSizeBytes, kTrampolineAllocationType, kTrampolineAllocationProtect);
    }

    // --------

    TrampolineStore::TrampolineStore(void* baseAddress) : count(0), trampolines(NULL)
    {
        void* const baseAddressRounded = (void*)((size_t)baseAddress & ~(kTrampolineStoreSizeBytes - 1));
        trampolines = (Trampoline*)VirtualAlloc(baseAddressRounded, kTrampolineStoreSizeBytes, kTrampolineAllocationType, kTrampolineAllocationProtect);
    }

    // --------

    TrampolineStore::~TrampolineStore(void)
    {
        if (NULL != trampolines && 0 == Count())
            VirtualFree(trampolines, 0, MEM_RELEASE);
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
