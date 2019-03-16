/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TemporaryBuffers.cpp
 *   Partial implementation of temporary buffer management functionality.
 *****************************************************************************/

#include "TemporaryBuffers.h"

#include <atomic>
#include <cstdint>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "TemporaryBuffers.h" for documentation.

    uint8_t TemporaryBuffers::buffers[kBuffersTotalNumBytes];

    std::atomic<unsigned int> TemporaryBuffers::numAllocated;


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "TemporaryBuffers.h" for documentation.

    void* TemporaryBuffers::Allocate(void)
    {
        if (GetNumAllocated() >= kBuffersCount)
            throw NULL;

        const auto indexToAllocate = numAllocated.fetch_add(1);

        if (indexToAllocate >= kBuffersCount)
            throw NULL;

        return &buffers[GetBufferSize() * indexToAllocate];
    }
    
    // --------

    void TemporaryBuffers::Free(void)
    {
        numAllocated -= 1;
    }
}
