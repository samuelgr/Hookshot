/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file InjectLanding.cpp
 *   Partial landing code implementation. Receives control from injection code, cleans up, and
 *   runs the program.
 **************************************************************************************************/

#include <cstddef>

#include <Infra/Core/Message.h>
#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "DependencyProtect.h"
#include "Inject.h"
#include "InternalHook.h"
#include "LibraryInterface.h"
#include "Strings.h"

using namespace Hookshot;

extern "C" void __fastcall InjectLandingCleanup(const SInjectData* const injectData)
{
  // Before cleaning up, set aside the array of addresses to free. This is needed because freeing
  // any one of these could invalidate the injectData pointer.
  void* cleanupBaseAddress[_countof(injectData->cleanupBaseAddress)];
  memcpy(cleanupBaseAddress, injectData->cleanupBaseAddress, sizeof(cleanupBaseAddress));

  for (size_t i = 0; i < _countof(cleanupBaseAddress); ++i)
  {
    if (nullptr != cleanupBaseAddress[i])
      Protected::Windows_VirtualFree(cleanupBaseAddress[i], 0, MEM_RELEASE);
  }
}

extern "C" void __fastcall InjectLandingLoadHookModules(const SInjectData* const injectData)
{
  if ((0 != injectData->enableDebugFeatures) && (0 == Protected::Windows_IsDebuggerPresent()))
    Infra::Message::OutputFormatted(
        Infra::Message::ESeverity::ForcedInteractiveInfo,
        L"Attach to \"%.*s\" (PID %d) to continue debugging.",
        static_cast<int>(Infra::ProcessInfo::GetExecutableBaseName().length()),
        Infra::ProcessInfo::GetExecutableBaseName().data(),
        Infra::ProcessInfo::GetCurrentProcessId());

  SetAllInternalHooks();

  const int numHookModulesLoaded = LibraryInterface::LoadHookModules();
  const int numInjectOnlyLibrariesLoaded = LibraryInterface::LoadInjectOnlyLibraries();

  Infra::Message::OutputFormatted(
      Infra::Message::ESeverity::Info,
      L"Loaded %d hook module%s and %d injection-only librar%s.",
      numHookModulesLoaded,
      (1 == numHookModulesLoaded ? L"" : L"s"),
      numInjectOnlyLibrariesLoaded,
      (1 == numInjectOnlyLibrariesLoaded ? L"y" : L"ies"));
}
