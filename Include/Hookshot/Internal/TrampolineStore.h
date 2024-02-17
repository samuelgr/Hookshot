/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file TrampolineStore.h
 *   Declaration of top-level data structure for holding trampoline objects.
 **************************************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include "Trampoline.h"

namespace Hookshot
{
  /// Manages trampoline object allocation and construction. On creation, allocates buffer space for
  /// trampoline objects that, once filled with at least one trampoline, cannot be destroyed. This
  /// is because trampoline objects cannot be destroyed or reset once created and set, and the
  /// buffer space that stores them must live as long as they do. Methods are not concurrency-safe
  /// and require some external form of concurrency control.
  class TrampolineStore
  {
  public:

    /// Amount of memory reserved for holding trampoline objects per instance of this object.
    static const int kTrampolineStoreSizeBytes;

    /// Maximum number of trampoline objects that can be held in this object.
    static const int kTrampolineStoreCount;

    TrampolineStore(void);

    ~TrampolineStore(void);

    /// Allows modifying the location of the buffer that stores trampoline objects. Base address is
    /// rounded down to the nearest multiple of #kTrampolineStoreSizeBytes.
    TrampolineStore(void* baseAddress);

    TrampolineStore(const TrampolineStore&) = delete;

    TrampolineStore(TrampolineStore&& other) noexcept;

    inline Trampoline& operator[](const int index)
    {
      return trampolines[index];
    }

    inline const Trampoline& operator[](const int index) const
    {
      return trampolines[index];
    }

    /// Specifies if this object  is initialized properly.
    /// @return `true` if so, `false` otherwise.
    inline bool IsInitialized(void) const
    {
      return (nullptr != trampolines);
    }

    /// Attempts to allocate and construct a new trampoline object.
    /// @return Index of the newly-allocated trampoline object, or -1 in the event of a failure.
    int Allocate(void);

    /// Deallocates the most recently-allocated trampoline object.
    void Deallocate(void);

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

  private:

    /// Number of trampolines allocated.
    int count;

    /// Holds the trampoline objects themselves.
    Trampoline* trampolines;
  };
} // namespace Hookshot
