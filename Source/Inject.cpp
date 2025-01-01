/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file Inject.cpp
 *   Utility implementations for interfacing with injection code.
 **************************************************************************************************/

#include "Inject.h"

#include <cstddef>
#include <cstring>

#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "InjectResult.h"
#include "Strings.h"

namespace Hookshot
{
  /// Magic value that identifies the metadata section of a loaded binary file.
  static constexpr DWORD kInjectionMetaMagicValue = 0x51525354;

  /// Defines the structure of the metadata section in a loaded binary file.
  struct SInjectMeta
  {
    /// Magic value. Present in all versions of Hookshot.
    DWORD magic;
    /// Version number. Currently not used and must be 0. Present in all versions of Hookshot.
    DWORD version;

    // Fields that may change from one version to the next. These correspond to labels in the
    // assembly-written code. Each specifies an offset within the code section of the
    // correspondingly-named label.
    DWORD offsetInjectTrampolineStart;
    DWORD offsetInjectTrampolineAddressMarker;
    DWORD offsetInjectTrampolineEnd;
    DWORD offsetInjectCodeStart;
    DWORD offsetInjectCodeBegin;
    DWORD offsetInjectCodeEnd;
  };

  /// Obtains access to the binary data that contains the injection code.
  /// @param [out] baseAddress On success, filled with the base address of the injection code.
  /// @param [out] sizeBytes On success, filled with the size in bytes of the injection code.
  /// @return `true` on success, `false` on failure.
  static bool LoadInjectCodeBinary(void** baseAddress, size_t* sizeBytes)
  {
    static void* injectCodeBinaryBaseAddress = nullptr;
    static size_t injectCodeBinarySizeBytes = 0;

    if (nullptr == injectCodeBinaryBaseAddress)
    {
      const HRSRC resourceInfoBlock = FindResource(
          Infra::ProcessInfo::GetThisModuleInstanceHandle(),
          MAKEINTRESOURCE(IDR_HOOKSHOT_INJECT_CODE),
          RT_RCDATA);
      if (nullptr == resourceInfoBlock) return false;

      const HGLOBAL resourceHandle =
          LoadResource(Infra::ProcessInfo::GetThisModuleInstanceHandle(), resourceInfoBlock);
      if (nullptr == resourceHandle) return false;

      void* const resourceBaseAddress = LockResource(resourceHandle);
      if (nullptr == resourceBaseAddress) return false;

      size_t resourceSizeBytes = static_cast<size_t>(
          SizeofResource(Infra::ProcessInfo::GetThisModuleInstanceHandle(), resourceInfoBlock));
      if (0 == resourceSizeBytes) return false;

      injectCodeBinaryBaseAddress = resourceBaseAddress;
      injectCodeBinarySizeBytes = resourceSizeBytes;
    }

    *baseAddress = injectCodeBinaryBaseAddress;
    *sizeBytes = injectCodeBinarySizeBytes;
    return true;
  }

  InjectInfo::InjectInfo(void)
      : injectTrampolineStart(nullptr),
        injectTrampolineAddressMarker(nullptr),
        injectTrampolineEnd(nullptr),
        injectCodeStart(nullptr),
        injectCodeBegin(nullptr),
        injectCodeEnd(nullptr),
        initializationResult(EInjectResult::Failure)
  {
    void* injectBinaryBase = nullptr;
    size_t injectBinarySizeBytes = 0;

    if (false == LoadInjectCodeBinary(&injectBinaryBase, &injectBinarySizeBytes))
    {
      initializationResult = EInjectResult::ErrorCannotLoadInjectCode;
      return;
    }

    if (kMaxInjectBinaryFileSize < injectBinarySizeBytes)
    {
      initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
      return;
    }

    // Parse the injection binary and fill pointer values.
    {
      // Verify a valid DOS header.
      const IMAGE_DOS_HEADER* const dosHeader = static_cast<IMAGE_DOS_HEADER*>(injectBinaryBase);

      if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic)
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

      // Verify a valid NT header and perform simple sanity checks.
      const IMAGE_NT_HEADERS* const ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(
          reinterpret_cast<size_t>(dosHeader) + static_cast<size_t>(dosHeader->e_lfanew));

      if (IMAGE_NT_SIGNATURE != ntHeader->Signature)
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

#ifdef _WIN64
      if (IMAGE_FILE_MACHINE_AMD64 != ntHeader->FileHeader.Machine)
#else
      if (IMAGE_FILE_MACHINE_I386 != ntHeader->FileHeader.Machine)
#endif
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

      // Look through the section headers for the required code and metadata sections.
      void* sectionCode = nullptr;
      SInjectMeta* sectionMeta = nullptr;

      {
        const IMAGE_SECTION_HEADER* const sectionHeader =
            reinterpret_cast<const IMAGE_SECTION_HEADER*>(&ntHeader[1]);

        // For each section found, check if its name matches one of the required section.
        // Since each such section should only appear once, also verify uniqueness.
        for (WORD secidx = 0; secidx < ntHeader->FileHeader.NumberOfSections; ++secidx)
        {
          if (Strings::kStrInjectCodeSectionName ==
              reinterpret_cast<const char*>(sectionHeader[secidx].Name))
          {
            if (nullptr != sectionCode)
            {
              initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
              return;
            }

            sectionCode = reinterpret_cast<void*>(
                reinterpret_cast<size_t>(injectBinaryBase) +
                static_cast<size_t>(sectionHeader[secidx].PointerToRawData));
          }
          else if (
              Strings::kStrInjectMetaSectionName ==
              reinterpret_cast<const char*>(sectionHeader[secidx].Name))
          {
            if (nullptr != sectionMeta)
            {
              initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
              return;
            }

            sectionMeta = reinterpret_cast<SInjectMeta*>(
                reinterpret_cast<size_t>(injectBinaryBase) +
                static_cast<size_t>(sectionHeader[secidx].PointerToRawData));
          }
        }
      }

      // Verify that both sections exist.
      if ((nullptr == sectionCode) || (nullptr == sectionMeta))
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

      // Check the validity and version-correctness of the metadata section.
      if (kInjectionMetaMagicValue != sectionMeta->magic)
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

      if (0 != sectionMeta->version)
      {
        initializationResult = EInjectResult::ErrorMalformedInjectCodeFile;
        return;
      }

      // Fill in pointers using the information obtained.
      injectTrampolineStart = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectTrampolineStart));
      injectTrampolineAddressMarker = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectTrampolineAddressMarker));
      injectTrampolineEnd = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectTrampolineEnd));
      injectCodeStart = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectCodeStart));
      injectCodeBegin = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectCodeBegin));
      injectCodeEnd = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(sectionCode) +
          static_cast<size_t>(sectionMeta->offsetInjectCodeEnd));

      // All operations completed successfully.
      initializationResult = EInjectResult::Success;
    }
  }
} // namespace Hookshot
