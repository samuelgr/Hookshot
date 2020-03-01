/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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
        static constexpr unsigned int kBuffersTotalNumBytes = 8 * 1024 * 1024;

        /// Specifies the number of temporary buffers to create.
        static constexpr unsigned int kBuffersCount = 32;


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
        static inline void* Allocate(void)
        {
            if (GetNumAllocated() >= kBuffersCount)
                throw NULL;

            const auto indexToAllocate = numAllocated.fetch_add(1);

            if (indexToAllocate >= kBuffersCount)
                throw NULL;

            return &buffers[GetBufferSize() * indexToAllocate];
        }

        /// Frees the most-recently-allocated buffer.
        static inline void Free(void)
        {
            numAllocated -= 1;
        }

        /// Retrieves the size of each allocated buffer.
        /// @return Size, in bytes, of each allocated buffer.
        constexpr static inline unsigned int GetBufferSize(void)
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


        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
        
        /// Default constructor.
        inline TemporaryBuffer(void) : buffer((T*)TemporaryBuffers::Allocate())
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
        /// Enables objects of this type to be used as if they were pointers to the underlying type.
        operator T*(void) const
        {
            return buffer;
        }


        // -------- INSTANCE METHODS --------------------------------------- //
        
        /// Retrieves the size of the buffer space, in number of elements of type T.
        /// @return Size of the buffer, in T-sized elements.
        constexpr inline unsigned int Count(void) const
        {
            return Size() / sizeof(T);
        }
        
        /// Retrieves the size of the buffer space, in bytes.
        /// @return Size of the buffer, in bytes.
        constexpr inline unsigned int Size(void) const
        {
            return TemporaryBuffers::GetBufferSize();
        }
    };
}
