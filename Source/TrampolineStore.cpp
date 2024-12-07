/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file TrampolineStore.cpp
 *   Implementation of top-level data structure for trampoline objects.
 **************************************************************************************************/

#include "TrampolineStore.h"

#include <Infra/Core/ProcessInfo.h>

#include "DependencyProtect.h"

namespace Hookshot
{
  const int TrampolineStore::kTrampolineStoreSizeBytes =
      Infra::ProcessInfo::GetSystemInformation().dwPageSize;
  const int TrampolineStore::kTrampolineStoreCount = kTrampolineStoreSizeBytes / sizeof(Trampoline);

  /// Allocates a buffer suitable for holding Trampoline objects optionally using a specified base
  /// address.
  /// @param [in] baseAddress Desired base address for the buffer.
  /// @return Pointer to the allocated buffer, or `nullptr` on failure.
  static inline Trampoline* AllocateTrampolineBuffer(void* baseAddress = nullptr)
  {
    return reinterpret_cast<Trampoline*>(Protected::Windows_VirtualAlloc(
        baseAddress,
        TrampolineStore::kTrampolineStoreSizeBytes,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_EXECUTE_READWRITE));
  }

  TrampolineStore::TrampolineStore(void) : count(0), trampolines(AllocateTrampolineBuffer()) {}

  TrampolineStore::TrampolineStore(void* baseAddress)
      : count(0), trampolines(AllocateTrampolineBuffer(baseAddress))
  {}

  TrampolineStore::~TrampolineStore(void)
  {
    if (nullptr != trampolines && 0 == Count())
      Protected::Windows_VirtualFree(trampolines, 0, MEM_RELEASE);
  }

  TrampolineStore::TrampolineStore(TrampolineStore&& other) noexcept
      : count(other.count), trampolines(other.trampolines)
  {
    other.count = 0;
    other.trampolines = nullptr;
  }

  int TrampolineStore::Allocate(void)
  {
    if ((nullptr == trampolines) || (count >= kTrampolineStoreCount)) return -1;

    new (&trampolines[count]) Trampoline();
    return count++;
  }

  void TrampolineStore::Deallocate(void)
  {
    count -= 1;
  }
} // namespace Hookshot
