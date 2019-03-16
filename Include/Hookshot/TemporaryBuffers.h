/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TemporaryBuffers.h
 *   Partial implementation of temporary buffer management functionality.
 *****************************************************************************/

#pragma once

#include <atomic>
#include <cstdint>


namespace Hookshot
{
    /// Manages a global set of temporary buffers.
    /// These can be used for any purpose and are intended to replace large stack-allocated or heap-allocated buffers.
    /// Instead, memory is allocated statically at load-time and divided up as needed to various parts of the application.
    /// From the allocated memory pool, individual buffers are allocated and deallocated like a stack.
    /// Do not invoke methods from this class directly.  Instead, stack-allocate objects using the template class below it.
    /// All methods are class methods.
    class TemporaryBuffers
    {
    public:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Specifies the total size of all temporary buffers, in bytes.
        static const unsigned int kBuffersTotalNumBytes = 8 * 1024 * 1024;

        /// Specifies the number of temporary buffers to create.
        static const unsigned int kBuffersCount = 32;


    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// The buffer space itself.
        static uint8_t buffers[kBuffersTotalNumBytes];

        /// Number of buffers currently allocated.
        static std::atomic<unsigned int> numAllocated;

        
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        TemporaryBuffers(void) = delete;

        
        // -------- CLASS METHODS ------------------------------------------ //

        /// Attempts to allocate a new buffer.
        /// On failure, throws `NULL`.
        /// @return Pointer to the new temporary buffer.
        static void* Allocate(void);

        /// Frees the most-recently-allocated buffer.
        static void Free(void);

        /// Retrieves the size of each allocated buffer.
        /// @return Size, in bytes, of each allocated buffer.
        static inline unsigned int GetBufferSize(void)
        {
            return (kBuffersTotalNumBytes / kBuffersCount);
        }
        
        /// Retrieves the current number of allocated buffers.
        /// @return Number of allocated buffers.
        static inline unsigned int GetNumAllocated(void)
        {
            return numAllocated;
        }

        
        // -------- FRIENDS ------------------------------------------------ //

        template <typename T> friend struct TemporaryBuffer;
    };

    /// Manages a single temporary buffer.
    /// Interfaces with the global temporary buffer management class.
    /// To use temporary buffers, stack-allocate objects of this type.
    template <typename T> struct TemporaryBuffer
    {
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Pointer to the buffer space.
        T* const buffer;

        /// Size of the buffer space, in bytes.
        const unsigned int size;

        /// Size of the buffer space, in T-sized elements.
        const unsigned int count;

        
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
        
        /// Default constructor.
        inline TemporaryBuffer(void) : buffer((T*)TemporaryBuffers::Allocate()), size(TemporaryBuffers::GetBufferSize()), count(TemporaryBuffers::GetBufferSize() / sizeof(T))
        {
            // Nothing to do here.
        }
        
        /// Default destructor.
        inline ~TemporaryBuffer(void)
        {
            TemporaryBuffers::Free();
        }

        /// Copy constructor. Should never be invoked.
        TemporaryBuffer(const TemporaryBuffer<T>&) = delete;


        // -------- OPERATORS ---------------------------------------------- //

        /// Allows implicit conversion of a temporary buffer to the buffer pointer itself.
        /// Enables drop-in replacement for functions that accept a buffer pointer.
        operator T*(void) const
        {
            return buffer;
        }
    };
}
