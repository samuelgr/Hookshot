/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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
}
