/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file ChildProcessInjector.cpp
 *   Implementation of internal hooks for injecting child processes.
 **************************************************************************************************/

#include <Infra/Core/Message.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "DependencyProtect.h"
#include "Globals.h"
#include "InjectResult.h"
#include "InternalHook.h"
#include "RemoteProcessInjector.h"
#include "SafeCreateProcess.h"

/// Prototype for the undocumented low-level process creation system call. This is what Hookshot
/// will hook to avoid missing any attempts at process creation.
/// Source: https://captmeelo.com/redteam/maldev/2022/05/10/ntcreateuserprocess.html
NTSTATUS NTAPI NtCreateUserProcess(
    PHANDLE ProcessHandle,
    PHANDLE ThreadHandle,
    ACCESS_MASK ProcessDesiredAccess,
    ACCESS_MASK ThreadDesiredAccess,
    POBJECT_ATTRIBUTES ProcessObjectAttributes,
    POBJECT_ATTRIBUTES ThreadObjectAttributes,
    ULONG ProcessFlags,
    ULONG ThreadFlags,
    void* ProcessParameters, // RTL_USER_PROCESS_PARAMETERS
    void* CreateInfo,        // PPS_CREATE_INFO
    void* AttributeList      // PPS_ATTRIBUTE_LIST
);

/// Thread flag for indicating that the main thread in a new process should be created in a
/// suspended state.
/// Source: https://captmeelo.com/redteam/maldev/2022/05/10/ntcreateuserprocess.html
inline constexpr ULONG kThreadCreateFlagsCreateSuspended = 0x00000001;

namespace Hookshot
{
  HOOKSHOT_INTERNAL_HOOK(NtCreateUserProcess);

  /// Injects a newly-created child process with HookshotDll. Outputs a message indicating the
  /// result of the attempted injection.
  /// @param [in] processHandle Handle to the process to inject.
  /// @param [in] threadHandle Handle to the main thread of the process to inject.
  static void InjectChildProcess(const HANDLE processHandle, const HANDLE threadHandle)
  {
    Infra::TemporaryBuffer<wchar_t> childProcessExecutable;
    DWORD childProcessExecutableLength = childProcessExecutable.Capacity();

    if (0 ==
        Protected::Windows_QueryFullProcessImageNameW(
            processHandle, 0, childProcessExecutable.Data(), &childProcessExecutableLength))
      childProcessExecutableLength = 0;

    const EInjectResult result = RemoteProcessInjector::InjectProcess(
        processHandle, threadHandle, false, Protected::Windows_IsDebuggerPresent());

    if (EInjectResult::Success == result)
      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"Successfully injected child process %s.",
          (0 == childProcessExecutableLength ? L"(error determining executable file name)"
                                             : &childProcessExecutable[0]));
    else
      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Warning,
          L"%s - Failed to inject child process: %s: %s",
          (0 == childProcessExecutableLength ? L"(error determining executable file name)"
                                             : &childProcessExecutable[0]),
          InjectResultString(result).data(),
          Infra::Strings::FromSystemErrorCode(Protected::Windows_GetLastError()).AsCString());
  }

  void* InternalHook_NtCreateUserProcess::OriginalFunctionAddress(void)
  {
    auto result = GetProcAddress(Globals::GetNtDllModule(), "NtCreateUserProcess");
    return result;
  }

  NTSTATUS
  InternalHook_NtCreateUserProcess::Hook(
      PHANDLE ProcessHandle,
      PHANDLE ThreadHandle,
      ACCESS_MASK ProcessDesiredAccess,
      ACCESS_MASK ThreadDesiredAccess,
      POBJECT_ATTRIBUTES ProcessObjectAttributes,
      POBJECT_ATTRIBUTES ThreadObjectAttributes,
      ULONG ProcessFlags,
      ULONG ThreadFlags,
      void* ProcessParameters,
      void* CreateInfo,
      void* AttributeList)
  {
    constexpr ULONG kThreadCreateFlagsCreateSuspended = 0x00000001;
    const bool shouldCreateSuspended =
        (0 != (ThreadFlags & kThreadCreateFlagsCreateSuspended)) ? true : false;

    const NTSTATUS createProcessResult = Original(
        ProcessHandle,
        ThreadHandle,
        ProcessDesiredAccess,
        ThreadDesiredAccess,
        ProcessObjectAttributes,
        ThreadObjectAttributes,
        ProcessFlags,
        (ThreadFlags | kThreadCreateFlagsCreateSuspended),
        ProcessParameters,
        CreateInfo,
        AttributeList);

    if (ERROR_SUCCESS == createProcessResult)
    {
      if (false == CheckForAndRemoveHookshotBypassToken(ProcessParameters))
      {
        InjectChildProcess(*ProcessHandle, *ThreadHandle);
      }
    }

    if (false == shouldCreateSuspended) Protected::Windows_ResumeThread(*ThreadHandle);

    return createProcessResult;
  }
} // namespace Hookshot
