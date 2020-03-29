/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file TrampolineStore.cpp
 *   Implementation of top-level data structure for trampoline objects.
 *****************************************************************************/

#include "DependencyProtect.h"
#include "TrampolineStore.h"


namespace Hookshot
{
    // -------- INTERNAL FUNCTIONS ------------------------------------- //

    /// Allocates a buffer suitable for holding Trampoline objects optionally using a specified base address.
    /// @param [in] baseAddress Desired base address for the buffer.
    /// @return Pointer to the allocated buffer, or `nullptr` on failure.
    static inline Trampoline* AllocateTrampolineBuffer(void* baseAddress = nullptr)
    {
        return (Trampoline*)Windows::ProtectedVirtualAlloc(baseAddress, TrampolineStore::kTrampolineStoreSizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "TrampolineStore.h" for documentation.

    TrampolineStore::TrampolineStore(void) : count(0), trampolines(AllocateTrampolineBuffer())
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
        if (nullptr != trampolines && 0 == Count())
            Windows::ProtectedVirtualFree(trampolines, 0, MEM_RELEASE);
    }

    // --------
    
    TrampolineStore::TrampolineStore(TrampolineStore&& other) : count(other.count), trampolines(other.trampolines)
    {
        other.count = 0;
        other.trampolines = nullptr;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "TrampolineStore.h" for documentation.

    int TrampolineStore::Allocate(void)
    {
        if ((nullptr == trampolines) || (count >= kTrampolineStoreCount))
            return -1;

        new (&trampolines[count]) Trampoline();
        return count++;
    }

    // --------

    void TrampolineStore::Deallocate(void)
    {
        count -= 1;
    }
}
