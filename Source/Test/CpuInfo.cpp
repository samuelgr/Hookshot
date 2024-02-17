/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file CpuInfo.cpp
 *   Partial implementation of CPU identification functionality.
 **************************************************************************************************/

#include "CpuInfo.h"

#include <cpuinfo_x86.h>

namespace HookshotTest
{
  CpuInfo::CpuInfo(void) : cpuInfo(cpu_features::GetX86Info()) {}

  CpuInfo& CpuInfo::GetInstance(void)
  {
    static CpuInfo cpuInfo;
    return cpuInfo;
  }
} // namespace HookshotTest
