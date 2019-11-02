/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TrampolineStore.h
 *   Declaration of top-level data structure for holding trampoline objects.
 *****************************************************************************/

#pragma once

#include "Trampoline.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    /// Manages trampoline object allocation and construction.
    /// Deallocation of individual trampoline objects is not possible.
    class TrampolineStore
    {
    private:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Amount of memory reserved for holding trampoline objects per instance of this object.
        static constexpr size_t kTrampolineStoreSizeBytes = (2 * 1024 * 1024);

        /// Maximum number of trampoline objects that can be held in this object.
        static constexpr size_t kTrampolineStoreCount = kTrampolineStoreSizeBytes / sizeof(Trampoline);


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Number of trampolines allocated.
        int count;

        /// Holds the trampoline objects themselves.
        Trampoline* trampolines;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.
        TrampolineStore(void);

        /// Default destructor.
        ~TrampolineStore(void);

        /// Copy constructor. Should never be invoked.
        TrampolineStore(const TrampolineStore&) = delete;


        // -------- OPERATORS ---------------------------------------------- //

        /// Allows access to individual Trampoline objects using array subscripting syntax, with no bounds-checking.
        /// @param [in] index Index of the trampoline to retrieve.
        /// @return Reference to the desired trampoline.
        inline Trampoline& operator[](const int index) {
            return trampolines[index];
        }

        /// Allows const access to individual Trampoline objects using array subscripting syntax, with no bounds-checking.
        /// @param [in] index Index of the trampoline to retrieve.
        /// @return Reference to the desired trampoline.
        inline const Trampoline& operator[](const int index) const {
            return trampolines[index];
        }


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Specifies if this object  is initialized properly.
        /// @return `true` if so, `false` otherwise.
        inline bool IsInitialized(void) const
        {
            return (NULL != trampolines);
        }

        /// Attempts to allocate and construct a new trampoline object.
        /// This operation is not concurrency-safe and requires external concurrency control.
        /// @return Index of the newly-allocated trampoline object, or -1 in the event of a failure.
        int Allocate(void);

        /// Retrieves the number of trampoline objects in this data structure.
        /// @return Number of trampolines allocated.
        inline int Count(void) const
        {
            return count;
        }

        /// Retrieves the number of free spaces for trampoline objects in this data structure.
        /// @return Remaining number of trampoline objects that can be allocated.
        inline int FreeCount(void) const
        {
            return (kTrampolineStoreCount - count);
        }
    };
}
