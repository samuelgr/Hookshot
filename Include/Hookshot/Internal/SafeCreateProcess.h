/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file SafeCreateProcess.h
 *   Declaration of functionality that allows Hookshot itself to create child processes safely and
 *   without interference from its own attempts to hook process creation internally.
 **************************************************************************************************/

#pragma once

#include "ApiWindows.h"

namespace Hookshot
{
  /// Examines the supplied process creation parameters (parameter to `NtCreateUserProcess`) and
  /// looks at the environment block to see if it contains a valid Hookshot injection bypass token.
  /// If it does, adjusts the environment block to remove it.
  /// @param [in,out] processCreationParameters Process creation parameters, which were originally
  /// supplied to `NtCreateUserProcess`.
  /// @return `true` if a valid injection bypass token was found, `false` otherwise.
  bool CheckForAndRemoveHookshotBypassToken(void* processCreationParameters);

  /// Safely creates a process without interference from Hookshot's own attempts to hook process
  /// creation internally. Acts as a drop-in replacement for the `CreateProcessW` Windows API
  /// function.
  BOOL SafeCreateProcess(
      LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine,
      LPSECURITY_ATTRIBUTES lpProcessAttributes,
      LPSECURITY_ATTRIBUTES lpThreadAttributes,
      BOOL bInheritHandles,
      DWORD dwCreationFlags,
      LPVOID lpEnvironment,
      LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo,
      LPPROCESS_INFORMATION lpProcessInformation);
} // namespace Hookshot