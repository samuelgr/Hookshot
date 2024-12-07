/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file CodeInjector.h
 *   Implementation of code injection, execution, and synchronization. This file is the only
 *   interface to assembly-written code. Changes to behavior of the assembly code must be reflected
 *   here too.
 **************************************************************************************************/

#include "CodeInjector.h"

#include <cstddef>

#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "Inject.h"
#include "InjectResult.h"
#include "Strings.h"

namespace Hookshot
{
  CodeInjector::CodeInjector(
      void* const baseAddressCode,
      void* const baseAddressData,
      const bool cleanupCodeBuffer,
      const bool cleanupDataBuffer,
      void* const entryPoint,
      const size_t sizeCode,
      const size_t sizeData,
      const HANDLE injectedProcess,
      const HANDLE injectedProcessMainThread)
      : baseAddressCode(baseAddressCode),
        baseAddressData(baseAddressData),
        cleanupCodeBuffer(cleanupCodeBuffer),
        cleanupDataBuffer(cleanupDataBuffer),
        entryPoint(entryPoint),
        sizeCode(sizeCode),
        sizeData(sizeData),
        injectedProcess(injectedProcess),
        injectedProcessMainThread(injectedProcessMainThread),
        oldCodeAtTrampoline(),
        injectInfo()
  {}

  EInjectResult CodeInjector::SetAndRun(const bool enableDebugFeatures)
  {
    EInjectResult result = Check();

    if (EInjectResult::Success == result) result = Set(enableDebugFeatures);

    if (EInjectResult::Success == result) result = Run();

    if (EInjectResult::Success == result) result = UnsetTrampoline();

    return result;
  }

  EInjectResult CodeInjector::Check(void) const
  {
    if (EInjectResult::Success != injectInfo.InitializationResult())
      return injectInfo.InitializationResult();

    if (GetTrampolineCodeSize() > sizeof(oldCodeAtTrampoline))
      return EInjectResult::ErrorInsufficientTrampolineSpace;

    if (GetRequiredCodeSize() > sizeCode) return EInjectResult::ErrorInsufficientCodeSpace;

    if (GetRequiredDataSize() > sizeData) return EInjectResult::ErrorInsufficientDataSpace;

    if ((nullptr == baseAddressCode) || (nullptr == baseAddressData) || (nullptr == entryPoint) ||
        (INVALID_HANDLE_VALUE == injectedProcess) ||
        (INVALID_HANDLE_VALUE == injectedProcessMainThread))
      return EInjectResult::ErrorInternalInvalidParams;

    return EInjectResult::Success;
  }

  size_t CodeInjector::GetRequiredCodeSize(void) const
  {
    return reinterpret_cast<size_t>(injectInfo.GetInjectCodeEnd()) -
        reinterpret_cast<size_t>(injectInfo.GetInjectCodeStart());
  }

  size_t CodeInjector::GetRequiredDataSize(void) const
  {
    return sizeof(SInjectData);
  }

  size_t CodeInjector::GetTrampolineCodeSize(void) const
  {
    return reinterpret_cast<size_t>(injectInfo.GetInjectTrampolineEnd()) -
        reinterpret_cast<size_t>(injectInfo.GetInjectTrampolineStart());
  }

  bool CodeInjector::LocateFunctions(
      void*& addrGetLastError, void*& addrGetProcAddress, void*& addrLoadLibraryA) const
  {
    HMODULE moduleGetLastError = nullptr;
    HMODULE moduleGetProcAddress = nullptr;
    HMODULE moduleLoadLibraryA = nullptr;

    Infra::TemporaryString moduleFilenameGetLastError;
    Infra::TemporaryString moduleFilenameGetProcAddress;
    Infra::TemporaryString moduleFilenameLoadLibraryA;
    MODULEINFO moduleInfo;

    // Get module handles for the desired functions in the current process.
    // The same DLL will export them in the target process, but the base address might not be the
    // same.
    if (FALSE ==
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(GetLastError),
            &moduleGetLastError))
      return false;

    if (FALSE ==
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(GetProcAddress),
            &moduleGetProcAddress))
      return false;

    if (FALSE ==
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(LoadLibraryA),
            &moduleLoadLibraryA))
      return false;

    // Compute the relative addresses of each desired function with respect to the base address of
    // its associated DLL.
    size_t offsetGetLastError = static_cast<size_t>(-1);
    size_t offsetGetProcAddress = static_cast<size_t>(-1);
    size_t offsetLoadLibraryA = static_cast<size_t>(-1);

    if (FALSE ==
        GetModuleInformation(
            Infra::ProcessInfo::GetCurrentProcessHandle(),
            moduleGetLastError,
            &moduleInfo,
            sizeof(moduleInfo)))
      return false;

    offsetGetLastError =
        reinterpret_cast<size_t>(GetLastError) - reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll);

    if ((moduleGetProcAddress != moduleGetLastError) &&
        (FALSE ==
         GetModuleInformation(
             Infra::ProcessInfo::GetCurrentProcessHandle(),
             moduleGetProcAddress,
             &moduleInfo,
             sizeof(moduleInfo))))
      return false;

    offsetGetProcAddress =
        reinterpret_cast<size_t>(GetProcAddress) - reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll);

    if ((moduleLoadLibraryA != moduleGetProcAddress) &&
        (FALSE ==
         GetModuleInformation(
             Infra::ProcessInfo::GetCurrentProcessHandle(),
             moduleGetProcAddress,
             &moduleInfo,
             sizeof(moduleInfo))))
      return false;

    offsetLoadLibraryA =
        reinterpret_cast<size_t>(LoadLibraryA) - reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll);

    // Compute the full path names for each module that offers the required functions.
    moduleFilenameGetLastError.UnsafeSetSize(GetModuleFileName(
        moduleGetLastError,
        moduleFilenameGetLastError.Data(),
        moduleFilenameGetLastError.Capacity()));
    if (true == moduleFilenameGetLastError.Empty()) return false;

    moduleFilenameGetProcAddress.UnsafeSetSize(GetModuleFileName(
        moduleGetProcAddress,
        moduleFilenameGetProcAddress.Data(),
        moduleFilenameGetProcAddress.Capacity()));
    if (true == moduleFilenameGetProcAddress.Empty()) return false;

    moduleFilenameLoadLibraryA.UnsafeSetSize(GetModuleFileName(
        moduleLoadLibraryA,
        moduleFilenameLoadLibraryA.Data(),
        moduleFilenameLoadLibraryA.Capacity()));
    if (true == moduleFilenameLoadLibraryA.Empty()) return false;

    // Enumerate all of the modules in the target process.
    // This approach is necessary because GetModuleHandle(Ex) cannot act on processes other than the
    // calling process.
    Infra::TemporaryBuffer<HMODULE> loadedModules;
    DWORD numLoadedModules = 0;

    if (FALSE ==
        EnumProcessModules(
            injectedProcess,
            loadedModules.Data(),
            loadedModules.CapacityBytes(),
            &numLoadedModules))
      return false;

    numLoadedModules /= sizeof(HMODULE);

    // For each loaded module, see if its full name matches one of the desired modules and, if so,
    // compute the address of the desired function.
    addrGetLastError = nullptr;
    addrGetProcAddress = nullptr;
    addrLoadLibraryA = nullptr;

    for (DWORD modidx = 0; (modidx < numLoadedModules) &&
         ((nullptr == addrGetLastError) || (nullptr == addrGetProcAddress) ||
          (nullptr == addrLoadLibraryA));
         ++modidx)
    {
      const HMODULE loadedModule = loadedModules[modidx];
      Infra::TemporaryString loadedModuleName;

      loadedModuleName.UnsafeSetSize(GetModuleFileNameEx(
          injectedProcess, loadedModule, loadedModuleName.Data(), loadedModuleName.Capacity()));
      if (true == loadedModuleName.Empty()) return false;

      if (nullptr == addrGetLastError)
      {
        if (true ==
            Infra::Strings::EqualsCaseInsensitive(
                moduleFilenameGetLastError.AsStringView(), loadedModuleName.AsStringView()))
        {
          if (FALSE ==
              GetModuleInformation(injectedProcess, loadedModule, &moduleInfo, sizeof(moduleInfo)))
            return false;

          addrGetLastError = reinterpret_cast<void*>(
              reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll) + offsetGetLastError);
        }
      }

      if (nullptr == addrGetProcAddress)
      {
        if (true ==
            Infra::Strings::EqualsCaseInsensitive(
                moduleFilenameGetProcAddress.AsStringView(), loadedModuleName.AsStringView()))
        {
          if (FALSE ==
              GetModuleInformation(injectedProcess, loadedModule, &moduleInfo, sizeof(moduleInfo)))
            return false;

          addrGetProcAddress = reinterpret_cast<void*>(
              reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll) + offsetGetProcAddress);
        }
      }

      if (nullptr == addrLoadLibraryA)
      {
        if (true ==
            Infra::Strings::EqualsCaseInsensitive(
                moduleFilenameLoadLibraryA.AsStringView(), loadedModuleName.AsStringView()))
        {
          if (FALSE ==
              GetModuleInformation(injectedProcess, loadedModule, &moduleInfo, sizeof(moduleInfo)))
            return false;

          addrLoadLibraryA = reinterpret_cast<void*>(
              reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll) + offsetLoadLibraryA);
        }
      }
    }

    return (
        (nullptr != addrGetLastError) && (nullptr != addrGetProcAddress) &&
        (nullptr != addrLoadLibraryA));
  }

  EInjectResult CodeInjector::Run(void)
  {
    injectInit(injectedProcess, baseAddressData);

    // Allow the injected code to start running.
    if (1 != ResumeThread(injectedProcessMainThread))
      return EInjectResult::ErrorRunFailedResumeThread;

    // Synchronize with the injected code.
    if (false == injectSync()) return EInjectResult::ErrorRunFailedSync;

    // Fill in some values that the injected process needs to perform required operations.
    {
      void* addrGetLastError;
      void* addrGetProcAddress;
      void* addrLoadLibraryA;

      if (true != LocateFunctions(addrGetLastError, addrGetProcAddress, addrLoadLibraryA))
        return EInjectResult::ErrorCannotLocateRequiredFunctions;

      if (false == injectDataFieldWrite(funcGetLastError, &addrGetLastError))
        return EInjectResult::ErrorCannotWriteRequiredFunctionLocations;

      if (false == injectDataFieldWrite(funcGetProcAddress, &addrGetProcAddress))
        return EInjectResult::ErrorCannotWriteRequiredFunctionLocations;

      if (false == injectDataFieldWrite(funcLoadLibraryA, &addrLoadLibraryA))
        return EInjectResult::ErrorCannotWriteRequiredFunctionLocations;
    }

    // Synchronize with the injected code.
    if (false == injectSync()) return EInjectResult::ErrorRunFailedSync;

    // Wait for the injected code to reach completion and synchronize with it.
    // Once the injected code reaches this point, put the thread to sleep and then allow it to
    // advance. This way, upon waking, the thread will advance past the barrier and execute the
    // process as normal.
    if (false == injectSyncWait()) return EInjectResult::ErrorRunFailedSync;

    if (0 != SuspendThread(injectedProcessMainThread))
      return EInjectResult::ErrorRunFailedSuspendThread;

    if (false == injectSyncAdvance()) return EInjectResult::ErrorRunFailedSync;

    // Read from the injected process to determine the result of the injection attempt.
    uint32_t injectionResult;
    uint32_t extendedInjectionResult;

    if (false == injectDataFieldRead(injectionResult, &injectionResult))
      return EInjectResult::ErrorCannotReadStatus;

    if (false == injectDataFieldRead(extendedInjectionResult, &extendedInjectionResult))
      return EInjectResult::ErrorCannotReadStatus;

    SetLastError(static_cast<DWORD>(extendedInjectionResult));
    return static_cast<EInjectResult>(injectionResult);
  }

  EInjectResult CodeInjector::Set(const bool enableDebugFeatures)
  {
    SIZE_T numBytes = 0;

    // Back up the code currently at the trampoline's target location.
    if ((FALSE ==
         ReadProcessMemory(
             injectedProcess,
             entryPoint,
             reinterpret_cast<LPVOID>(oldCodeAtTrampoline.data()),
             GetTrampolineCodeSize(),
             &numBytes)) ||
        (GetTrampolineCodeSize() != numBytes))
      EInjectResult::ErrorSetFailedRead;

    // Write the trampoline code.
    if ((FALSE ==
         WriteProcessMemory(
             injectedProcess,
             entryPoint,
             injectInfo.GetInjectTrampolineStart(),
             GetTrampolineCodeSize(),
             &numBytes)) ||
        (GetTrampolineCodeSize() != numBytes) ||
        (FALSE == FlushInstructionCache(injectedProcess, entryPoint, GetTrampolineCodeSize())))
      EInjectResult::ErrorSetFailedWrite;

    // Place the address of the main code entry point into the trampoline code at the correct
    // location.
    {
      void* const targetAddress = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(entryPoint) +
          (reinterpret_cast<size_t>(injectInfo.GetInjectTrampolineAddressMarker()) -
           reinterpret_cast<size_t>(injectInfo.GetInjectTrampolineStart())) -
          sizeof(size_t));
      const size_t sourceData = reinterpret_cast<size_t>(baseAddressCode) +
          (reinterpret_cast<size_t>(injectInfo.GetInjectCodeBegin()) -
           reinterpret_cast<size_t>(injectInfo.GetInjectCodeStart()));

      if ((FALSE ==
           WriteProcessMemory(
               injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytes)) ||
          (sizeof(sourceData) != numBytes) ||
          (FALSE == FlushInstructionCache(injectedProcess, targetAddress, sizeof(sourceData))))
        EInjectResult::ErrorSetFailedWrite;
    }

    // Write the main code.
    if ((FALSE ==
         WriteProcessMemory(
             injectedProcess,
             baseAddressCode,
             injectInfo.GetInjectCodeStart(),
             GetRequiredCodeSize(),
             &numBytes)) ||
        (GetRequiredCodeSize() != numBytes) ||
        (FALSE == FlushInstructionCache(injectedProcess, baseAddressCode, GetRequiredCodeSize())))
      EInjectResult::ErrorSetFailedWrite;

    // Place the pointer to the data region into the correct spot in the code region.
    {
      void* const targetAddress = baseAddressCode;
      const size_t sourceData = reinterpret_cast<size_t>(baseAddressData);

      if ((FALSE ==
           WriteProcessMemory(
               injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytes)) ||
          (sizeof(sourceData) != numBytes) ||
          (FALSE == FlushInstructionCache(injectedProcess, targetAddress, sizeof(sourceData))))
        EInjectResult::ErrorSetFailedWrite;
    }

    // Initialize the data region.
    {
      SInjectData injectData;
      char injectDataStrings[InjectInfo::kMaxInjectBinaryFileSize - sizeof(injectData)];

      memset(&injectData, 0, sizeof(injectData));
      memset(&injectDataStrings, 0, sizeof(injectDataStrings));

      injectData.enableDebugFeatures = (true == enableDebugFeatures ? 1 : 0);
      injectData.injectionResultCodeSuccess = static_cast<uint32_t>(EInjectResult::Success);
      injectData.injectionResultCodeLoadLibraryFailed =
          static_cast<uint32_t>(EInjectResult::ErrorCannotLoadLibrary);
      injectData.injectionResultCodeGetProcAddressFailed =
          static_cast<uint32_t>(EInjectResult::ErrorMalformedLibrary);
      injectData.injectionResultCodeInitializationFailed =
          static_cast<uint32_t>(EInjectResult::ErrorLibraryInitFailed);
      injectData.injectionResult = static_cast<uint32_t>(EInjectResult::Failure);

      strcpy_s(injectDataStrings, Strings::kStrLibraryInitializationProcName.data());

      if (0 !=
          wcstombs_s(
              nullptr,
              &injectDataStrings[Strings::kStrLibraryInitializationProcName.length() + 1],
              sizeof(injectDataStrings) -
                  (Strings::kStrLibraryInitializationProcName.length() + 1) - 1,
              Strings::GetHookshotDynamicLinkLibraryFilename().data(),
              sizeof(injectDataStrings) -
                  (Strings::kStrLibraryInitializationProcName.length() + 1) - 2))
        return EInjectResult::ErrorCannotGenerateLibraryFilename;

      injectData.strLibraryName = reinterpret_cast<const char*>(
          reinterpret_cast<size_t>(baseAddressData) + sizeof(injectData) +
          (Strings::kStrLibraryInitializationProcName.length() + 1));
      injectData.strProcName = reinterpret_cast<const char*>(
          reinterpret_cast<size_t>(baseAddressData) + sizeof(injectData));

      // Figure out which addresses need to be cleaned up.
      // These buffers will be freed once injection is complete.
      {
        unsigned int cleanupIndex = 0;

        if (true == cleanupCodeBuffer)
          injectData.cleanupBaseAddress[cleanupIndex++] = baseAddressCode;

        if (true == cleanupDataBuffer)
          injectData.cleanupBaseAddress[cleanupIndex++] = baseAddressData;
      }

      if ((FALSE ==
           WriteProcessMemory(
               injectedProcess, baseAddressData, &injectData, sizeof(injectData), &numBytes)) ||
          (sizeof(injectData) != numBytes))
        EInjectResult::ErrorSetFailedWrite;

      if ((FALSE ==
           WriteProcessMemory(
               injectedProcess,
               reinterpret_cast<void*>(
                   reinterpret_cast<size_t>(baseAddressData) + sizeof(injectData)),
               injectDataStrings,
               sizeof(injectDataStrings),
               &numBytes)) ||
          (sizeof(injectDataStrings) != numBytes))
        EInjectResult::ErrorSetFailedWrite;
    }

    return EInjectResult::Success;
  }

  EInjectResult CodeInjector::UnsetTrampoline(void)
  {
    SIZE_T numBytes = 0;

    if ((FALSE ==
         WriteProcessMemory(
             injectedProcess,
             entryPoint,
             reinterpret_cast<LPCVOID>(oldCodeAtTrampoline.data()),
             GetTrampolineCodeSize(),
             &numBytes)) ||
        (GetTrampolineCodeSize() != numBytes) ||
        (FALSE == FlushInstructionCache(injectedProcess, entryPoint, GetTrampolineCodeSize())))
      return EInjectResult::ErrorUnsetFailed;

    return EInjectResult::Success;
  }
} // namespace Hookshot
