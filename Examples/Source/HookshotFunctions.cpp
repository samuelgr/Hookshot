/***************************************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file HookshotFunctions.cpp
 *   "HookshotFunctions" hook module example.
 *   This example shows how to use Hookshot functions to create a hook.
 **************************************************************************************************/

#include <Windows.h>

#include "Hookshot/Hookshot.h"

/// Function pointer, which will hold the address that Hookshot will provide that can be invoked to
/// get the original (i.e. un-hooked) version of `MessageBoxW`. Pointer type, including calling
/// convention, needs to be specified manually and be identical to that of `MessageBoxW`. In this
/// case, the correct type for the function pointer can be obtained by making use of the
/// `MessageBoxW` function prototype declaration available via the `Windows.h` header file.
static decltype(&MessageBoxW) originalMessageBoxW = nullptr;

/// Hook function for `MessageBoxW`.
/// This function contains the code that executes anytime the `MessageBoxW` function is called by
/// any module in the current process, once Hookshot has successfully created a hook for
/// `MessageBoxW` as is requested in the hook module entry point below. Function name can be
/// arbitrary.  However, its prototype, including calling convention, needs to be specified manually
/// and be identical to that of `MessageBoxW`.
static int __stdcall HookMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
  // For the purposes of this example, the test program's message box is modified by overriding the
  // text and the title and adding an information icon.
  return originalMessageBoxW(
      hWnd,
      L"Modified using Hookshot functions.",
      L"HookshotFunctions Example",
      MB_ICONINFORMATION);
}

/// Hook module entry point. See "HookshotFunctions.h" for documentation.
HOOKSHOT_HOOK_MODULE_ENTRY(hookshot)
{
  // Request that Hookshot hook MessageBoxW using the hook function HookMessageBoxW defined above.
  const Hookshot::EResult result = hookshot->CreateHook(MessageBoxW, HookMessageBoxW);

  // If successful, the test program's message box should be modified as described in
  // HookMessageBoxW.  Hookshot's pointer to the original functionality of MessageBoxW must be saved
  // so that HookMessageBoxW can access it. If not, an error message is displayed, in addition to
  // the original unmodified message box that the test program would otherwise display.
  if (Hookshot::SuccessfulResult(result))
  {
    // Passing a parameter of HookMessageBoxW would also work here.  Hookshot identifies hooks both
    // by original function address and by hook function address.
    originalMessageBoxW = (decltype(originalMessageBoxW))hookshot->GetOriginalFunction(MessageBoxW);
  }
  else
  {
    // Since MessageBoxW was not successfully hooked, this call will proceed unmodified. Showing a
    // message box during a hook module's entry point is not generally a good idea, but in this
    // specific scenario it is useful for the purposes of demonstrating how Hookshot works.
    MessageBoxW(nullptr, L"Failed to hook MessageBoxW", L"HookshotFunctions Example", MB_ICONERROR);
  }
}
