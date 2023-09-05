/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file ProcessInjector.cpp
 *   Implementation of all process creation and injection functionality.
 **************************************************************************************************/

#include "ProcessInjector.h"

#include <winnt.h>
#include <winternl.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <vector>

#include "ApiWindowsShell.h"
#include "CodeInjector.h"
#include "Globals.h"
#include "Inject.h"
#include "InjectResult.h"
#include "Message.h"
#include "RemoteProcessInjector.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

namespace Hookshot
{
  namespace ProcessInjector
  {
    /// System allocation granularity. Captured once and re-used as needed.
    static size_t systemAllocationGranularity = 0;

    /// Advances the specified process' loader progress until it is ready to begin executing.
    /// It is assumed and required that the specified process be newly-created and suspended.
    /// @param [in] processHandle Handle to the process to be advanced.
    /// @return Indicator of the result of the oepration.
    static EInjectResult AdvanceProcess(const HANDLE processHandle)
    {
      // This function attaches to the specified process as if debugging it and waits for it to
      // trigger the breakpoint that indicates the initial load process has completed. Windows will
      // generate an exception breakpoint once the loader finishes initializing the process but
      // before it runs. At that point the process is still suspended. The main thread will not
      // execute until `ResumeThread` is invoked on it.

      if (0 == DebugActiveProcess(GetProcessId(processHandle)))
        return EInjectResult::ErrorAdvanceProcessFailed;

      DEBUG_EVENT debugEvent{};
      ZeroMemory(&debugEvent, sizeof(debugEvent));

      while (debugEvent.dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
      {
        if (0 == WaitForDebugEvent(&debugEvent, 1000))
          return EInjectResult::ErrorAdvanceProcessFailed;

        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
      }

      DebugActiveProcessStop(GetProcessId(processHandle));
      return EInjectResult::Success;
    }

    /// Attempts to read NT optional headers from a loaded module in a different process.
    /// @param [in] processHandle Handle to the process that contains the module for which NT
    /// optional headers are desired.
    /// @param [in] baseAddress Base image address of the loaded module, in the address space of the
    /// specified process, for which NT optional headers are desired.
    /// @param [out] optionalHeader Buffer to be filled with the optional header data, if the
    /// operation is successful.
    /// @return Indicator of the result of the operation.
    static EInjectResult FillNtOptionalHeader(
        const HANDLE processHandle,
        const void* const baseAddress,
        IMAGE_OPTIONAL_HEADER* optionalHeader)
    {
      size_t numBytesRead = 0;
      decltype(IMAGE_DOS_HEADER::e_lfanew) ntHeadersOffset = 0;

      if ((FALSE ==
           ReadProcessMemory(
               processHandle,
               reinterpret_cast<LPCVOID>(
                   reinterpret_cast<size_t>(baseAddress) +
                   static_cast<size_t>(offsetof(IMAGE_DOS_HEADER, e_lfanew))),
               reinterpret_cast<LPVOID>(&ntHeadersOffset),
               sizeof(ntHeadersOffset),
               reinterpret_cast<SIZE_T*>(&numBytesRead))) ||
          (sizeof(ntHeadersOffset) != numBytesRead))
        return EInjectResult::ErrorReadDOSHeadersFailed;

      if ((FALSE ==
           ReadProcessMemory(
               processHandle,
               reinterpret_cast<LPCVOID>(
                   reinterpret_cast<size_t>(baseAddress) + static_cast<size_t>(ntHeadersOffset) +
                   static_cast<size_t>(offsetof(IMAGE_NT_HEADERS, OptionalHeader))),
               reinterpret_cast<LPVOID>(optionalHeader),
               sizeof(IMAGE_OPTIONAL_HEADER),
               reinterpret_cast<SIZE_T*>(&numBytesRead))) ||
          (sizeof(IMAGE_OPTIONAL_HEADER) != numBytesRead))
        return EInjectResult::ErrorReadNTHeadersFailed;

      return EInjectResult::Success;
    }

    /// Attempts to retrieve the handle for the `ntdll.dll` module which should be loaded in this
    /// process and all created processes, even if suspended.
    /// @return Handle of `ntdll.dll`, or `nullptr` in the event of a failure to locate it.
    static HMODULE GetNtDllModule(void)
    {
      static const HMODULE ntdllModuleHandle =
          LoadLibraryEx(L"ntdll.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
      return ntdllModuleHandle;
    }

    /// Retrieves a handle to a module loaded in another process.
    /// Similar to `GetModuleHandle` but not limited to the currently running process.
    /// @param [in] processHandle Handle to the process for which a module handle is requested.
    /// @param [in] moduleName Name of the module for which a handle is requested.
    /// @return Handle of the specified module in the specified process, or `nullptr` if the
    /// operation failed.
    static HMODULE GetRemoteModuleHandle(HANDLE processHandle, std::wstring_view moduleName)
    {
      TemporaryBuffer<HMODULE> loadedModules;
      DWORD numLoadedModules = 0;

      if (FALSE ==
          EnumProcessModules(
              processHandle,
              loadedModules.Data(),
              loadedModules.CapacityBytes(),
              &numLoadedModules))
        return nullptr;

      numLoadedModules /= sizeof(HMODULE);

      TemporaryString loadedModuleName;
      for (DWORD modidx = 0; modidx < numLoadedModules; ++modidx)
      {
        const HMODULE loadedModule = loadedModules[modidx];

        loadedModuleName.UnsafeSetSize(GetModuleFileNameEx(
            processHandle, loadedModule, loadedModuleName.Data(), loadedModuleName.Capacity()));
        if (true == loadedModuleName.Empty()) return nullptr;

        if (true ==
            Strings::EndsWithCaseInsensitive<wchar_t>(loadedModuleName.AsStringView(), moduleName))
          return loadedModule;
      }

      SetLastError(ERROR_MOD_NOT_FOUND);
      return nullptr;
    }

    /// Retrieves a pointer to an exported procedure in another process.
    /// Similar to `GetProcAddress` but not limited to the currently running process.
    /// The returned pointer is in the address space of the other process.
    /// @param [in] processHandle Handle to the process for which a procedure pointer is requested.
    /// @param [in] moduleHandle Handle to the module, which must be loaded in the specified
    /// process, that is to be searched for an exported procedure.
    /// @param [in] procName Name of the exported procedure for which a pointer is requested.
    /// @return Handle of the specified module in the specified process, or `nullptr` if the
    /// operation failed.
    static FARPROC
        GetRemoteProcAddress(HANDLE processHandle, HMODULE moduleHandle, std::string_view procName)
    {
      size_t moduleExportTableRelativeBaseAddress = 0;
      std::vector<uint8_t> moduleExportTable;

      do
      {
        IMAGE_OPTIONAL_HEADER optionalHeader;
        EInjectResult operationResult =
            FillNtOptionalHeader(processHandle, moduleHandle, &optionalHeader);
        if (EInjectResult::Success != operationResult) return nullptr;

        // Index 0 in the data directory table contains export table information.
        // See
        // https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
        // for more information. Read the entire export address table, including all function names
        // and pointers, into a vector. Keeping the relative base address allows relative virtual
        // addresses in the export table directory to be converted to vector byte indices.

        moduleExportTableRelativeBaseAddress = optionalHeader.DataDirectory[0].VirtualAddress;
        moduleExportTable = std::vector<uint8_t>(optionalHeader.DataDirectory[0].Size, 0);

        size_t numBytesRead = 0;
        if ((FALSE ==
             ReadProcessMemory(
                 processHandle,
                 reinterpret_cast<LPCVOID>(
                     reinterpret_cast<size_t>(moduleHandle) + moduleExportTableRelativeBaseAddress),
                 reinterpret_cast<LPVOID>(moduleExportTable.data()),
                 static_cast<SIZE_T>(moduleExportTable.size()),
                 reinterpret_cast<SIZE_T*>(&numBytesRead))) ||
            (moduleExportTable.size() != numBytesRead))
          return nullptr;
      }
      while (false);

      // The entire export address table is stored in a vector, and the very beginning of it is a
      // directory. From the directory we obtain pointers to three arrays: (1) An array of export
      // function names (each element being a 4-byte relative virtual address of the beginning of
      // the name string) (2) An array of name ordinals (each element being a 2-byte index into the
      // array below) (3) An array of export function relative virtual addresses (each element being
      // a 4-byte relative virtual address of the starting location of the function)
      const IMAGE_EXPORT_DIRECTORY* moduleExportDirectory =
          reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(moduleExportTable.data());
      const DWORD* exportFunctionNameArray = reinterpret_cast<DWORD*>(
          reinterpret_cast<size_t>(moduleExportTable.data()) +
          static_cast<size_t>(moduleExportDirectory->AddressOfNames) -
          moduleExportTableRelativeBaseAddress);
      const WORD* exportFunctionNameOrdinalArray = reinterpret_cast<WORD*>(
          reinterpret_cast<size_t>(moduleExportTable.data()) +
          static_cast<size_t>(moduleExportDirectory->AddressOfNameOrdinals) -
          moduleExportTableRelativeBaseAddress);
      const DWORD* exportFunctionAddressArray = reinterpret_cast<DWORD*>(
          reinterpret_cast<size_t>(moduleExportTable.data()) +
          static_cast<size_t>(moduleExportDirectory->AddressOfFunctions) -
          moduleExportTableRelativeBaseAddress);

      // For each name, we need to compare it with the requested procedure name.
      // If it is a match, its index in the array of function names is used as an index into the
      // ordinal array, which in turn gives an index into the function address array.
      std::optional<WORD> requestedProcOrdinal = std::nullopt;

      for (unsigned int procIndex = 0; (procIndex < moduleExportDirectory->NumberOfNames) &&
           (false == requestedProcOrdinal.has_value());
           ++procIndex)
      {
        std::string_view exportFunctionName = reinterpret_cast<const char*>(
            reinterpret_cast<size_t>(moduleExportTable.data()) +
            static_cast<size_t>(exportFunctionNameArray[procIndex]) -
            moduleExportTableRelativeBaseAddress);

        if (exportFunctionName == procName)
          requestedProcOrdinal = exportFunctionNameOrdinalArray[procIndex];
      }

      if (false == requestedProcOrdinal.has_value())
      {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return nullptr;
      }

      return reinterpret_cast<FARPROC>(
          reinterpret_cast<size_t>(moduleHandle) +
          static_cast<size_t>(exportFunctionAddressArray[requestedProcOrdinal.value()]));
    }

    /// Attempts to determine the address of the entry point of the given process by locating the
    /// CLR entry point. This alternative mechanism is required for those processes that are managed
    /// and hence use a different loading process. In these special cases the entry point is
    /// actually the address of the `_CorExeMain` function exported by `mscoree.dll` rather than the
    /// entry point contained in the process' own header. See
    /// https://learn.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/corexemain-function
    /// for more information.
    /// @param [in] processHandle Handle to the process for which information is requested.
    /// @param [in] processEnvironmentBlock Read-only reference to the process environment block
    /// which has already been read from the process of interest.
    /// @param [out] entryPoint Address of the pointer that receives the entry point address.
    /// @return Indicator of the result of the operation.
    static EInjectResult
        GetClrEntryPointAddress(const HANDLE processHandle, void** const entryPoint)
    {
      const HMODULE clrModuleHandle = GetRemoteModuleHandle(processHandle, L"mscoree.dll");
      if (nullptr == clrModuleHandle) return EInjectResult::ErrorGetModuleHandleClrLibraryFailed;

      void* clrEntryPoint = reinterpret_cast<void*>(
          GetRemoteProcAddress(processHandle, clrModuleHandle, "_CorExeMain"));
      if (nullptr == clrEntryPoint) return EInjectResult::ErrorGetProcAddressClrEntryPointFailed;

      *entryPoint = clrEntryPoint;
      return EInjectResult::Success;
    }

    /// Attempts to determine the address of the entry point of the given process.
    /// All addresses used by this method are in the virtual address space of the target process.
    /// @param [in] processHandle Handle to the process for which information is requested.
    /// @param [in] baseAddress Base address of the process' executable image.
    /// @param [out] entryPoint Address of the pointer that receives the entry point address.
    /// @return Indicator of the result of the operation.
    static EInjectResult GetProcessEntryPointAddress(
        const HANDLE processHandle, const void* const baseAddress, void** const entryPoint)
    {
      IMAGE_OPTIONAL_HEADER optionalHeader;
      EInjectResult operationResult =
          FillNtOptionalHeader(processHandle, baseAddress, &optionalHeader);
      if (EInjectResult::Success != operationResult) return operationResult;

      // Index 14 in the data directory table contains CLR metadata.
      // Its presence indicates that the executable is managed and therefore the load process is
      // different. See
      // https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
      // for more information.
      if (0 != optionalHeader.DataDirectory[14].Size)
      {
        Message::Output(
            Message::ESeverity::Info,
            L"Process appears to be managed by the CLR. Using the CLR library's entry point address.");
        return GetClrEntryPointAddress(processHandle, entryPoint);
      }
      else
      {
        Message::Output(
            Message::ESeverity::Info,
            L"Process appears to be unmanaged by the CLR. Using the executable's own entry point address.");
        *entryPoint = reinterpret_cast<void*>(
            reinterpret_cast<size_t>(baseAddress) +
            static_cast<size_t>(optionalHeader.AddressOfEntryPoint));
        return EInjectResult::Success;
      }
    }

    /// Attempts to read the process environment block from the specified process.
    /// @param [in] processHandle Handle to the process for which information is requested.
    /// @param [out] processEnvironmentBlock Pointer to a buffer to be filled with the contents of
    /// the process environment block from the requested process.
    /// @return Indicator of the result of the operation.
    static EInjectResult
        GetProcessEnvironmentBlock(const HANDLE processHandle, PEB* processEnvironmentBlock)
    {
      // This function uses the documented, but internal, Windows API function
      // NtQueryInformationProcess. See
      // https://docs.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess
      // for official documentation. While unlikely, if NtQueryInformationProcess is modified or
      // made unavailable in a future version of Windows, this method might need to branch based on
      // Windows version.

      static const NTSTATUS(WINAPI * ntdllQueryInformationProcessProc)(
          HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG) =
          ((nullptr == GetNtDllModule())
               ? nullptr
               : (decltype(ntdllQueryInformationProcessProc))GetProcAddress(
                     GetNtDllModule(), "NtQueryInformationProcess"));

      if (nullptr == GetNtDllModule()) return EInjectResult::ErrorLoadNtDll;
      if (nullptr == ntdllQueryInformationProcessProc)
        return EInjectResult::ErrorNtQueryInformationProcessUnavailable;

      // Obtain the address of the process environment block (PEB) for the process, which is within
      // the address space of the process.
      PROCESS_BASIC_INFORMATION processBasicInfo;

      if (0 !=
          ntdllQueryInformationProcessProc(
              processHandle,
              ProcessBasicInformation,
              &processBasicInfo,
              sizeof(processBasicInfo),
              nullptr))
        return EInjectResult::ErrorNtQueryInformationProcessFailed;

      // Read the entire PEB from the process' address space.
      size_t numBytesRead = 0;
      if ((FALSE ==
           ReadProcessMemory(
               processHandle,
               reinterpret_cast<LPCVOID>(processBasicInfo.PebBaseAddress),
               reinterpret_cast<LPVOID>(processEnvironmentBlock),
               sizeof(PEB),
               reinterpret_cast<SIZE_T*>(&numBytesRead))) ||
          (sizeof(PEB) != numBytesRead))
        return EInjectResult::ErrorReadProcessPEBFailed;

      return EInjectResult::Success;
    }

    /// Attempts to determine the base address of the primary executable image for the given
    /// process.
    /// @param [in] processEnvironmentBlock Read-only reference to the process environment block
    /// which has already been read from the process of interest.
    /// @param [out] baseAddress Address of the pointer that receives the image base address in the
    /// virtual address space of the given process.
    /// @return Indicator of the result of the operation.
    static EInjectResult
        GetProcessImageBaseAddress(const PEB& processEnvironmentBlock, void** const baseAddress)
    {
      // The field of interest in the PEB structure is ImageBaseAddress, whose offset is
      // undocumented but has remained stable over multiple Windows generations and continues to do
      // so. See http://terminus.rewolf.pl/terminus/structures/ntdll/_PEB_combined.html for a
      // visualization. If the offset is modified in a future version of Windows, this constant
      // might need to have a different value for different Windows versions.

#ifdef HOOKSHOT64
      constexpr size_t kByteOffsetPebImageBaseAddress = 16;
#else
      constexpr size_t kByteOffsetPebImageBaseAddress = 8;
#endif

      *baseAddress = *(reinterpret_cast<void**>(
          reinterpret_cast<size_t>(&processEnvironmentBlock) + kByteOffsetPebImageBaseAddress));
      return EInjectResult::Success;
    }

    /// Verifies that Hookshot was given explicit authorization from the end user to inject the
    /// specified process.
    /// @param [in] processHandle Handle to the process to check.
    /// @return Indicator of the result of the operation.
    static EInjectResult VerifyAuthorizedToInjectProcess(const HANDLE processHandle)
    {
      TemporaryString processExecutablePath;
      DWORD processExecutablePathLength = processExecutablePath.Capacity();

      if (0 ==
          QueryFullProcessImageName(
              processHandle, 0, processExecutablePath.Data(), &processExecutablePathLength))
        return EInjectResult::ErrorCannotDetermineAuthorization;
      processExecutablePath.UnsafeSetSize((unsigned int)processExecutablePathLength);

      TemporaryString authorizationFileName =
          Strings::AuthorizationFilenameApplicationSpecific(processExecutablePath.Data());
      if (FALSE == PathFileExists(authorizationFileName.AsCString()))
      {
        Message::OutputFormatted(
            Message::ESeverity::Warning,
            L"Authorization not granted, cannot open application-specific file %s.",
            authorizationFileName.AsCString());

        authorizationFileName =
            Strings::AuthorizationFilenameDirectoryWide(processExecutablePath.Data());
        if (FALSE == PathFileExists(authorizationFileName.AsCString()))
        {
          Message::OutputFormatted(
              Message::ESeverity::Warning,
              L"Authorization not granted, cannot open directory-wide file %s.",
              authorizationFileName.AsCString());
          return EInjectResult::ErrorNotAuthorized;
        }
      }

      Message::OutputFormatted(
          Message::ESeverity::Info,
          L"Authorization granted by presence of file %s.",
          authorizationFileName.AsCString());
      return EInjectResult::Success;
    }

    /// Verifies that the architecture of the target process matches the architecture of this
    /// running binary.
    /// @param [in] processHandle Handle to the process to check.
    /// @return Indicator of the result of the operation.
    static EInjectResult VerifyMatchingProcessArchitecture(const HANDLE processHandle)
    {
      USHORT machineTargetProcess = 0;
      USHORT machineCurrentProcess = 0;

      if ((FALSE == IsWow64Process2(processHandle, &machineTargetProcess, nullptr)) ||
          (FALSE ==
           IsWow64Process2(Globals::GetCurrentProcessHandle(), &machineCurrentProcess, nullptr)))
        return EInjectResult::ErrorDetermineMachineProcess;

      if (machineTargetProcess == machineCurrentProcess)
        return EInjectResult::Success;
      else
        return EInjectResult::ErrorArchitectureMismatch;
    }

    /// Attempts to inject a process with Hookshot code.
    /// @param [in] processHandle Handle to the process to inject.
    /// @param [in] threadHandle Handle to the main thread of the process to inject.
    /// @param [in] enableDebugFeatures If `true`, signals to the injected process that a debugger
    /// is present, so certain debug features should be enabled.
    /// @return Indicator of the result of the operation.
    static EInjectResult InjectProcess(
        const HANDLE processHandle, const HANDLE threadHandle, const bool enableDebugFeatures)
    {
      // Verify that Hookshot is authorized to act on the process.
      EInjectResult operationResult = VerifyAuthorizedToInjectProcess(processHandle);

      // Make sure the architectures match between this process and the process being injected.
      if (EInjectResult::Success == operationResult)
        operationResult = VerifyMatchingProcessArchitecture(processHandle);

      switch (operationResult)
      {
        case EInjectResult::Success:
          break;

        case EInjectResult::ErrorArchitectureMismatch:
          return RemoteProcessInjector::InjectProcess(
              processHandle, threadHandle, true, enableDebugFeatures);

        default:
          return operationResult;
      }

      const size_t allocationGranularity = Globals::GetSystemInformation().dwPageSize;
      const size_t effectiveInjectRegionSize =
          (InjectInfo::kMaxInjectBinaryFileSize < allocationGranularity)
          ? allocationGranularity
          : InjectInfo::kMaxInjectBinaryFileSize;
      void* processBaseAddress = nullptr;
      void* processEntryPoint = nullptr;
      void* injectedCodeBase = nullptr;
      void* injectedDataBase = nullptr;

      // Advance the process so that the loader thread finishes loading any modules needed.
      operationResult = AdvanceProcess(processHandle);
      if (EInjectResult::Success != operationResult) return operationResult;

      // Attempt to obtain the process environment block for the new process.
      PEB processEnvironmentBlock;
      operationResult = GetProcessEnvironmentBlock(processHandle, &processEnvironmentBlock);
      if (EInjectResult::Success != operationResult) return operationResult;

      // Attempt to obtain the base address of the executable image of the new process.
      operationResult = GetProcessImageBaseAddress(processEnvironmentBlock, &processBaseAddress);
      if (EInjectResult::Success != operationResult) return operationResult;

      // Attempt to obtain the entry point address of the new process.
      operationResult =
          GetProcessEntryPointAddress(processHandle, processBaseAddress, &processEntryPoint);
      if (EInjectResult::Success != operationResult) return operationResult;

      // Allocate code and data areas in the target process.
      // Code first, then data.
      injectedCodeBase = VirtualAllocEx(
          processHandle,
          nullptr,
          (static_cast<SIZE_T>(effectiveInjectRegionSize) * 2),
          MEM_RESERVE | MEM_COMMIT,
          PAGE_NOACCESS);
      injectedDataBase = reinterpret_cast<void*>(
          reinterpret_cast<size_t>(injectedCodeBase) + effectiveInjectRegionSize);

      if (nullptr == injectedCodeBase) return EInjectResult::ErrorVirtualAllocFailed;

      // Set appropriate protection values onto the new areas individually.
      {
        DWORD unusedOldProtect = 0;

        if (FALSE ==
            VirtualProtectEx(
                processHandle,
                injectedCodeBase,
                effectiveInjectRegionSize,
                PAGE_EXECUTE_READ,
                &unusedOldProtect))
          return EInjectResult::ErrorVirtualProtectFailed;

        if (FALSE ==
            VirtualProtectEx(
                processHandle,
                injectedDataBase,
                effectiveInjectRegionSize,
                PAGE_READWRITE,
                &unusedOldProtect))
          return EInjectResult::ErrorVirtualProtectFailed;
      }

      // Inject code and data.
      // Only mark the code buffer as requiring cleanup because both code and data buffers are from
      // the same single allocation.
      CodeInjector injector(
          injectedCodeBase,
          injectedDataBase,
          true,
          false,
          processEntryPoint,
          effectiveInjectRegionSize,
          effectiveInjectRegionSize,
          processHandle,
          threadHandle);
      operationResult = injector.SetAndRun(enableDebugFeatures);

      return operationResult;
    }

    EInjectResult CreateInjectedProcess(
        LPCWSTR lpApplicationName,
        LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation)
    {
      // This method creates processes in suspended state as part of injection functionality.
      // It will allow the new process to run, unless the caller requested a suspended process.
      const bool shouldCreateSuspended = (0 != (dwCreationFlags & CREATE_SUSPENDED)) ? true : false;

      // Attempt to create the new process in suspended state and capture information about it.
      PROCESS_INFORMATION processInfo = *lpProcessInformation;
      if (0 ==
          CreateProcessW(
              lpApplicationName,
              lpCommandLine,
              lpProcessAttributes,
              lpThreadAttributes,
              bInheritHandles,
              (dwCreationFlags | CREATE_SUSPENDED),
              lpEnvironment,
              lpCurrentDirectory,
              lpStartupInfo,
              &processInfo))
        return EInjectResult::ErrorCreateProcess;

      *lpProcessInformation = processInfo;

      const EInjectResult result = InjectProcess(
          processInfo.hProcess, processInfo.hThread, (IsDebuggerPresent() ? true : false));

      if (EInjectResult::Success == result)
      {
        if (false == shouldCreateSuspended) ResumeThread(processInfo.hThread);
      }
      else
      {
        const DWORD systemErrorCode = GetLastError();
        TerminateProcess(processInfo.hProcess, static_cast<UINT>(-1));
        SetLastError(systemErrorCode);
      }

      return result;
    }

    bool PerformRequestedRemoteInjection(
        RemoteProcessInjector::SInjectRequest* const remoteInjectionData)
    {
      EInjectResult operationResult = InjectProcess(
          reinterpret_cast<HANDLE>(remoteInjectionData->processHandle),
          reinterpret_cast<HANDLE>(remoteInjectionData->threadHandle),
          remoteInjectionData->enableDebugFeatures);

      remoteInjectionData->injectionResult = static_cast<uint64_t>(operationResult);
      remoteInjectionData->extendedInjectionResult = static_cast<uint64_t>(GetLastError());

      CloseHandle(reinterpret_cast<HANDLE>(remoteInjectionData->processHandle));
      CloseHandle(reinterpret_cast<HANDLE>(remoteInjectionData->threadHandle));

      return true;
    }
  } // namespace ProcessInjector
} // namespace Hookshot
