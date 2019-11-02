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
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "TrampolineStore.h" for documentation.

    TrampolineStore::TrampolineStore(void) : count(0), trampolines(NULL)
    {
        trampolines = (Trampoline*)VirtualAlloc(NULL, kTrampolineStoreSizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    // --------

    TrampolineStore::~TrampolineStore(void)
    {
        if (NULL != trampolines)
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
}
