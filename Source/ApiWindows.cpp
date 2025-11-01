/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file ApiWindows.cpp
 *   Implementation of supporting functions for the Windows API.
 **************************************************************************************************/

#include "ApiWindows.h"

#include <string_view>

#include <Infra/Core/Strings.h>

namespace Hookshot
{
  void* GetWindowsApiFunctionAddress(const char* const funcName, void* const funcStaticAddress)
  {
    // List of low-level binary module handles, specified as the result of a call to LoadLibrary
    // with the name of the binary. Each is checked in sequence for the specified function, which is
    // looked up by base name.
    static const HMODULE hmodLowLevelBinaries[] = {LoadLibraryW(L"KernelBase.dll")};

    void* funcAddress = funcStaticAddress;

    for (int i = 0; (funcAddress == funcStaticAddress) && (i < _countof(hmodLowLevelBinaries)); ++i)
    {
      if (nullptr != hmodLowLevelBinaries[i])
      {
        void* const funcPossibleAddress = SafeGetProcAddress(hmodLowLevelBinaries[i], funcName);
        if (nullptr != funcPossibleAddress) funcAddress = funcPossibleAddress;
      }
    }

    return funcAddress;
  }

  FARPROC SafeGetProcAddress(HMODULE moduleHandle, LPCSTR procName)
  {
    if (nullptr == moduleHandle) return nullptr;

    const size_t moduleBaseAddress = reinterpret_cast<size_t>(moduleHandle);

    const IMAGE_DOS_HEADER* const dosHeader =
        reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBaseAddress);
    if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic) return nullptr;

    const IMAGE_NT_HEADERS* ntHeaders =
        reinterpret_cast<const IMAGE_NT_HEADERS*>(moduleBaseAddress + dosHeader->e_lfanew);
    if (IMAGE_NT_SIGNATURE != ntHeaders->Signature) return nullptr;

    const DWORD exportDirectoryStartRva =
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    const DWORD exportDirectoryEndRva = exportDirectoryStartRva +
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    const IMAGE_EXPORT_DIRECTORY* exportDirectory =
        reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(moduleBaseAddress + exportDirectoryStartRva);

    const DWORD* exportedFunctions =
        reinterpret_cast<const DWORD*>(moduleBaseAddress + exportDirectory->AddressOfFunctions);
    const DWORD* exportedNames =
        reinterpret_cast<const DWORD*>(moduleBaseAddress + exportDirectory->AddressOfNames);
    const WORD* exportedNameOrdinals =
        reinterpret_cast<const WORD*>(moduleBaseAddress + exportDirectory->AddressOfNameOrdinals);

    for (DWORD exportIndex = 0; exportIndex < exportDirectory->NumberOfNames; ++exportIndex)
    {
      const std::string_view exportName =
          reinterpret_cast<const char*>(moduleBaseAddress + exportedNames[exportIndex]);
      if (procName != exportName) continue;

      int x = 0;
      if ((exportName == "GetLastError") || (exportName == "SetLastError"))
      {
        x = 1;
      }

      const DWORD exportOrdinal = exportedNameOrdinals[exportIndex];
      const DWORD exportFinalRva = exportedFunctions[exportOrdinal];
      const size_t exportFinalAddress = moduleBaseAddress + exportFinalRva;

      const bool isInExportDirectory =
          ((exportFinalRva >= exportDirectoryStartRva) && (exportFinalRva < exportDirectoryEndRva));
      if (false == isInExportDirectory)
      {
        return reinterpret_cast<FARPROC>(exportFinalAddress);
      }
      else
      {
        const std::string_view exportForwardString =
            reinterpret_cast<const char*>(exportFinalAddress);
        const size_t exportStringSeparatorPos = exportForwardString.find_last_of('.');
        if (std::string_view::npos == exportStringSeparatorPos) return nullptr;

        Infra::TemporaryVector<char> exportForwardModuleNameBuf;
        for (size_t i = 0; i < exportStringSeparatorPos; ++i)
          exportForwardModuleNameBuf.PushBack(exportForwardString[i]);
        exportForwardModuleNameBuf.PushBack('\0');

        const char* exportForwardModuleName = exportForwardModuleNameBuf.Data();
        const char* exportForwardProcName =
            exportForwardString.data() + (1 + exportStringSeparatorPos);

        return SafeGetProcAddress(GetModuleHandleA(exportForwardModuleName), exportForwardProcName);
      }
    }

    return nullptr;
  }
} // namespace Hookshot
