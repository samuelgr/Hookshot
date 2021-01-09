/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
 **************************************************************************//**
 * @file StaticHook.cpp
 *   "StaticHook" hook module example.
 *   This example shows how to use the static hook types and definitions to
 *   create a hook more easily and safely than using Hookshot functions
 *   directly.  The address of `MessageBoxW` is available at link time, so a
 *   static hook can be used.
 *****************************************************************************/

#include "Hookshot/StaticHook.h"

#include <Windows.h>


// Declaration of a static hook that targets the function MessageBoxW.
// If this project spanned multiple source files, this declaration could be placed in a header file if multiple source files needed access to the type that it defines.
// A static hook works because MessageBoxW is declared in a header file (via Windows.h), and its address is visible to the linker at link time.
HOOKSHOT_STATIC_HOOK(MessageBoxW);


/// Hook function for `MessageBoxW`.
/// This function contains the code that executes anytime the `MessageBoxW` function is called by any module in the current process, once Hookshot has successfully created a hook for `MessageBoxW` as is requested in the hook module entry point below.
/// Class name and method name both follow the static hook naming convention (see "StaticHook.h"), and the return and parameter types are all defined by the static hook type definition.  Any deviations would result in a compiler error.
/// Even though not directly specified, the calling convention of the hook function below is also automatically defined by the static hook type definition.  Attempting to override it here would result in a compiler error.
int StaticHook_MessageBoxW::Hook(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // The pointer for accessing original (i.e. un-hooked) MessageBoxW functionality, which Hookshot provides, is automatically saved by the static hook.
    // Per static hook conventions (see "StaticHook.h"), such functionality is exposed via the static hook's Original method.
    // Method signature is derived from the static hook declaration, so any incorrect parameter types would result in a compiler error.
    // For the purposes of this example, the test program's message box is modified by overriding the text and the title and adding an error icon.
    return Original(hWnd, L"MODIFIED USING A STATIC HOOK.", L"StaticHook Example", MB_ICONERROR);
}


/// Hook module entry point.
/// See "HookshotFunctions.h" for documentation.
HOOKSHOT_HOOK_MODULE_ENTRY(hookshot)
{
    // Request that Hookshot hook MessageBoxW using the static hook defined above.
    // The function pointer that Hookshot provides to access the original functionality of MessageBoxW is saved automatically by the static hook.
    const Hookshot::EResult result = StaticHook_MessageBoxW::SetHook(hookshot);

    // If successful, the test program's message box should be modified as described above.
    // If not, an error message is displayed, in addition to the original unmodified message box that the test program would otherwise display.
    if (!Hookshot::SuccessfulResult(result))
    {
        // Since MessageBoxW was not successfully hooked, this call will proceed unmodified.
        // Showing a message box during a hook module's entry point is not generally a good idea, but in this specific scenario it is useful for the purposes of demonstrating how Hookshot works.
        MessageBoxW(nullptr, L"Failed to hook MessageBoxW", L"StaticHook Example", MB_ICONERROR);
    }
}
