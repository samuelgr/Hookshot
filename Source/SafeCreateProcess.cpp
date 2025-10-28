/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file SafeCreateProcess.cpp
 *   Implementation of functionality that allows Hookshot itself to create child processes safely
 *   and without interference from its own attempts to hook process creation internally.
 **************************************************************************************************/

#include "SafeCreateProcess.h"

#include <cstdint>
#include <mutex>
#include <string_view>

#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "DependencyProtect.h"
#include "Globals.h"

namespace Hookshot
{
#ifdef _WIN64

  /// Offset into the 64-bit version of the structure pointed to by `ProcessParameters` of the
  /// pointer to the environment block. Obtained by manually defining the structure and then using
  /// `offsetof`. Source:
  /// https://github.com/winsiderss/systeminformer/blob/3e9c0243cc277769cce903e15690de51fed95f7b/phnt/include/ntrtl.h#L2611
  static constexpr size_t kProcessParametersOffsetEnvironment = 128;

  /// Offset into the 64-bit version of the structure pointed to by `ProcessParameters` of the size
  /// of the environment block, in bytes. Obtained by manually defining the structure and then using
  /// `offsetof`. Source:
  /// https://github.com/winsiderss/systeminformer/blob/3e9c0243cc277769cce903e15690de51fed95f7b/phnt/include/ntrtl.h#L2611
  static constexpr size_t kProcessParametersOffsetEnvironmentSizeBytes = 1008;

#else

  /// Offset into the 32-bit version of the structure pointed to by `ProcessParameters` of the
  /// pointer to the environment block. Obtained by manually defining the structure and then using
  /// `offsetof`. Source:
  /// https://github.com/winsiderss/systeminformer/blob/3e9c0243cc277769cce903e15690de51fed95f7b/phnt/include/ntrtl.h#L2611
  static constexpr size_t kProcessParametersOffsetEnvironment = 72;

  /// Offset into the 64-bit version of the structure pointed to by `ProcessParameters` of the size
  /// of the environment block, in bytes. Obtained by manually defining the structure and then using
  /// `offsetof`. Source:
  /// https://github.com/winsiderss/systeminformer/blob/3e9c0243cc277769cce903e15690de51fed95f7b/phnt/include/ntrtl.h#L2611
  static constexpr size_t kProcessParametersOffsetEnvironmentSizeBytes = 656;

#endif

  /// Concatenates and null-terminates a string onto the end of a temporary vector object. This is
  /// intended for use when a temporary string is unsuitable, such as when constructing
  /// null-delimited lists for things like environment variable blocks.
  /// @param [in,out] vector Vector to which the string should be concatenated.
  /// @param [in] str String to concatenate.
  static void ConcatenateStringToTemporaryVector(
      Infra::TemporaryVector<wchar_t>& vector, std::wstring_view str)
  {
    for (auto c : str)
      vector.PushBack(c);
    vector.PushBack(L'\0');
  }

  /// Generates and returns a Hookshot injection bypass token as an environment variable string. The
  /// token is random and created once per process.
  /// @return Hookshot injection bypass token environment variable.
  static std::wstring_view GetHookshotInjectionBypassToken(void)
  {
    static std::wstring bypassToken;
    static std::once_flag bypassTokenFlag;

    std::call_once(
        bypassTokenFlag,
        []() -> void
        {
#ifdef _WIN64
          bypassToken = Infra::Strings::Format(
              L"HOOKSHOT_BYPASS_TOKEN=%016llx",
              static_cast<unsigned long long>(std::hash<uint64_t>()(GetTickCount64())));
#else
          bypassToken = Infra::Strings::Format(
              L"HOOKSHOT_BYPASS_TOKEN=%08x",
              static_cast<unsigned int>(std::hash<uint64_t>()(GetTickCount64())));
#endif
        });

    return bypassToken;
  }

  bool CheckForAndRemoveHookshotBypassToken(void* processCreationParameters)
  {
    wchar_t** environmentBlockPtr = reinterpret_cast<wchar_t**>(
        reinterpret_cast<size_t>(processCreationParameters) + kProcessParametersOffsetEnvironment);
    ULONG_PTR* environmentBlockSizePtr = reinterpret_cast<ULONG_PTR*>(
        reinterpret_cast<size_t>(processCreationParameters) +
        kProcessParametersOffsetEnvironmentSizeBytes);

    std::wstring_view firstEnvVar = *environmentBlockPtr;
    if (GetHookshotInjectionBypassToken() == firstEnvVar)
    {
      *environmentBlockPtr += (1 + static_cast<ULONG_PTR>(firstEnvVar.length()));
      *environmentBlockSizePtr -=
          (sizeof(wchar_t) * (1 + static_cast<ULONG_PTR>(firstEnvVar.length())));
      return true;
    }

    return false;
  }

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
      LPPROCESS_INFORMATION lpProcessInformation)
  {
    // Internal hooks are only set when the Hookshot module is injected. Otherwise there is nothing
    // to do.

    if (Globals::ELoadMethod::Injected == Globals::GetHookshotLoadMethod())
    {
      Infra::TemporaryVector<wchar_t> safeEnvironmentBlock;

      // For injection to be bypassed, the first environment variable must be the Hookshot injection
      // bypass token.
      ConcatenateStringToTemporaryVector(safeEnvironmentBlock, GetHookshotInjectionBypassToken());

      // In order to preserve the environment of the caller, the rest of the environment variables
      // need to be added. They are provided as a null-delimited list of unknown length (the unknown
      // length is why the null-delimited list tokenizer cannot be used).
      wchar_t* const allEnvStrings = GetEnvironmentStringsW();
      wchar_t* currentEnvStrings = allEnvStrings;
      while (L'\0' != currentEnvStrings[1])
      {
        std::wstring_view currentEnvString = currentEnvStrings;
        ConcatenateStringToTemporaryVector(safeEnvironmentBlock, currentEnvString);
        currentEnvStrings += (1 + currentEnvString.length());
      }
      safeEnvironmentBlock.PushBack(L'\0');
      FreeEnvironmentStringsW(allEnvStrings);

      std::wstring_view envStrings =
          std::wstring_view(safeEnvironmentBlock.Data(), safeEnvironmentBlock.Size());

      return Protected::Windows_CreateProcessW(
          lpApplicationName,
          lpCommandLine,
          lpProcessAttributes,
          lpThreadAttributes,
          bInheritHandles,
          (dwCreationFlags | CREATE_UNICODE_ENVIRONMENT),
          safeEnvironmentBlock.Data(),
          lpCurrentDirectory,
          lpStartupInfo,
          lpProcessInformation);
    }
    else
    {
      return Protected::Windows_CreateProcessW(
          lpApplicationName,
          lpCommandLine,
          lpProcessAttributes,
          lpThreadAttributes,
          bInheritHandles,
          dwCreationFlags,
          lpEnvironment,
          lpCurrentDirectory,
          lpStartupInfo,
          lpProcessInformation);
    }
  }
} // namespace Hookshot
