/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file CpuInfo.h
 *   Declaration of an interface for identifying the current CPU.
 **************************************************************************************************/

#pragma once

#include "cpuinfo_x86.h"

namespace HookshotTest
{
  /// Obtains and holds CPU feature information. Implemented as a singleton object.
  class CpuInfo
  {
  public:

    /// Retrieves and returns the current CPU's vendor string.
    /// @return Vendor string.
    static inline const char* VendorString(void)
    {
      return GetInstance().VendorStringInternal();
    }

    /// Retrieves and returns the current CPU's feature flag information.
    /// @return Feature flag information.
    static inline const cpu_features::X86Features& FeatureFlags(void)
    {
      return GetInstance().FeatureFlagsInternal();
    }

    /// Specifies if the processor is currently in 64-bit "long" mode.
    /// @return `true` if so, `false` otherwise.
    static inline constexpr bool Is64BitLongModeEnabled(void)
    {
#ifdef HOOKSHOT64
      return true;
#else
      return false;
#endif
    }

  private:

    CpuInfo(void);

    CpuInfo(const CpuInfo&) = delete;

    /// Returns a reference to the singleton instance of this class.
    /// Not intended to be invoked externally.
    /// @return Reference to the singleton instance.
    static CpuInfo& GetInstance(void);

    /// Internal implementation of vendor string retrieval.
    /// @return Vendor string.
    inline const char* VendorStringInternal(void) const
    {
      return cpuInfo.vendor;
    }

    /// Internal implementaion of feature flag retrieval.
    /// @return Feature flag information.
    inline const cpu_features::X86Features& FeatureFlagsInternal(void) const
    {
      return cpuInfo.features;
    }

    /// CPU feature information.
    const cpu_features::X86Info cpuInfo;
  };
} // namespace HookshotTest
