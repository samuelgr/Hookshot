/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file TemporaryBuffer.cpp
 *   Partial implementation of temporary buffer management functionality.
 *****************************************************************************/

#include "TemporaryBuffer.h"

#include <cstdint>
#include <cstdlib>
#include <mutex>


namespace Hookshot
{
    // -------- INTERNAL VARIABLES ----------------------------------------- //

    /// Statically-allocated buffer space itself.
    static uint8_t staticBuffers[TemporaryBufferBase::kBuffersTotalNumBytes];

    /// List of free statically-allocated buffers.
    static uint8_t* freeBuffers[TemporaryBufferBase::kBuffersCount];

    /// Index of next valid free buffer list element.
    static int nextFreeBuffer = 0;

    /// Flag that specifies if one-time initialization needs to take place.
    static bool isInitialized = false;

    /// Mutex used to ensure concurrency control over temporary buffer allocation and deallocation.
    static std::mutex allocationMutex;

    
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "TemporaryBuffers.h" for documentation.

    TemporaryBufferBase::TemporaryBufferBase(void)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (false == isInitialized)
        {
            for (int i = 0; i < _countof(freeBuffers); ++i)
                freeBuffers[i] = &staticBuffers[TemporaryBufferBase::kBytesPerBuffer * i];
            
            nextFreeBuffer = _countof(freeBuffers) - 1;
            isInitialized = true;
        }

        if (nextFreeBuffer < 0)
        {
            buffer = new uint8_t[TemporaryBufferBase::kBytesPerBuffer];
            isHeapAllocated = true;
        }
        else
        {
            buffer = freeBuffers[nextFreeBuffer];
            nextFreeBuffer -= 1;
            isHeapAllocated = false;
        }
    }

    // --------

    TemporaryBufferBase::~TemporaryBufferBase(void)
    {
        if (true == isHeapAllocated)
        {
            delete[] buffer;
        }
        else
        {
            std::lock_guard<std::mutex> lock(allocationMutex);

            nextFreeBuffer += 1;
            freeBuffers[nextFreeBuffer] = buffer;
        }
    }
}
