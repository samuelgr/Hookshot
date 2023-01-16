/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 **************************************************************************//**
 * @file DynamicHook.cpp
 *   "DynamicHook" hook module example.
 *   This example shows how to use the dynamic hook types and definitions to
 *   create a hook more easily and safely than using Hookshot functions
 *   directly.  For the purpose of this example, the address of `MessageBoxW`
 *   is supplied at runtime.
 *****************************************************************************/

#include "Hookshot/DynamicHook.h"

#include <Windows.h>


// Declaration of a dynamic hook that targets the function MessageBoxW.
// If this project spanned multiple source files, this declaration could be placed in a header file if multiple source files needed access to the type that it defines.
// There are three options for how to make this declaration.
// Use this preprocessor symbol to select which option to use.
#define DYNAMIC_HOOK_EXAMPLE_OPTION 1


#if DYNAMIC_HOOK_EXAMPLE_OPTION == 1

// Option 1: from a function prototype.
// MessageBoxW has a prototype declared in a header file, so the dynamic hook can figure out the required return type, parameter types, and calling convention from the declaration.
// This option is recommended where possible.
HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION(MessageBoxW);

#elif DYNAMIC_HOOK_EXAMPLE_OPTION == 2

// Option 2: from a function pointer.
// The type associated with the function pointer is used to figure out the required return type, parameter types, and calling convention.
// Since function pointers do not contain function name information the way function prototype declarations do, a name must be supplied manually as the first parameter to the macro.
// This is a useful option if a correctly-typed function pointer already exists somewhere else in the code so that the hook type can be consistent with the function pointer type.
// For the purposes of this example, the pointer type is specified manually and immediately before its use in the dynamic hook declaration.
static int(__stdcall * const functionPointerOfTypeMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT) = nullptr;
HOOKSHOT_DYNAMIC_HOOK_FROM_POINTER(MessageBoxFunction, functionPointerOfTypeMessageBoxW);

#elif DYNAMIC_HOOK_EXAMPLE_OPTION == 3

// Option 3: from a manual type specification.
// With this option, the function return type, parameter types, and calling convention of the function to hook are all specified manually and must exactly match those of the function being hooked.
// However, once specified, these types and conventions are enforced whenever the resulting dynamic hook class is used.
// A name must also be specified as the first parameter to the macro.
HOOKSHOT_DYNAMIC_HOOK_FROM_TYPESPEC(MessageBoxFunction, int(__stdcall)(HWND, LPCWSTR, LPCWSTR, UINT));

#else

#error "DYNAMIC_HOOK_EXAMPLE_OPTION must be equal to 1, 2, or 3."

#endif

/// Hook function for `MessageBoxW`.
/// This function contains the code that executes anytime the `MessageBoxW` function is called by any module in the current process, once Hookshot has successfully created a hook for `MessageBoxW` as is requested in the hook module entry point below.
/// Class name and method name both follow the dynamic hook naming convention (see "DynamicHook.h"), and the return and parameter types are all defined by the dynamic hook type definition.  Any deviations would result in a compiler error.
/// Even though not directly specified, the calling convention of the hook function below is also automatically defined by the dynamic hook type definition.  Attempting to override it here would result in a compiler error.
#if DYNAMIC_HOOK_EXAMPLE_OPTION == 1

// The "MessageBoxW" part of the class name matches the function name in the dynamic hook declaration.
int DynamicHook_MessageBoxW::Hook(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)

#else

// The "MessageBoxFunction" part of the class name matches the name parameter provided to the dynamic hook declaration.
int DynamicHook_MessageBoxFunction::Hook(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)

#endif
{
    // The pointer for accessing original (i.e. un-hooked) MessageBoxW functionality, which Hookshot provides, is automatically saved by the dynamic hook.
    // Per dynamic hook conventions (see "DynamicHook.h"), such functionality is exposed via the dynamic hook's Original method.
    // Method signature is derived from the dynamic hook declaration, so any incorrect parameter types would result in a compiler error.
    // For the purposes of this example, the test program's message box is modified by overriding the text and the title and adding a warning icon.
    return Original(hWnd, L"MODIFIED using a DYNAMIC HOOK!!!", L"DynamicHook Example", MB_ICONWARNING);
}


/// Hook module entry point.
/// See "HookshotFunctions.h" for documentation.
HOOKSHOT_HOOK_MODULE_ENTRY(hookshot)
{
    // Figure out the address of MessageBoxW.  How this is done does not matter to Hookshot.
    // Since it is known that the test program calls MessageBoxW, it is also known that the DLL containing MessageBoxW (user32.dll) is already loaded.
    HMODULE user32ModuleHandle = GetModuleHandle(L"user32.dll");
    if (nullptr == user32ModuleHandle)
    {
        MessageBoxW(nullptr, L"Failed to obtain a handle to user32.dll.", L"DynamicHook Example", MB_ICONERROR);
        return;
    }

    void* messageBoxProcAddress = GetProcAddress(user32ModuleHandle, "MessageBoxW");
    if (nullptr == messageBoxProcAddress)
    {
        MessageBoxW(nullptr, L"Failed to locate MessageBoxW in user32.dll.", L"DynamicHook Example", MB_ICONERROR);
        return;
    }

    // Request that Hookshot hook MessageBoxW using the dynamic hook defined above.
    // The function pointer that Hookshot provides to access the original functionality of MessageBoxW is saved automatically by the dynamic hook.
    // Unlike with static hooks, dynamic hooks require the address of the function to hook to be specified along with the request to create the hook.
    // There is no type checking on the address provided, as often these addresses are obtained from functions like GetProcAddress that return typeless pointers.
#if DYNAMIC_HOOK_EXAMPLE_OPTION == 1
    const Hookshot::EResult result = DynamicHook_MessageBoxW::SetHook(hookshot, messageBoxProcAddress);
#else
    const Hookshot::EResult result = DynamicHook_MessageBoxFunction::SetHook(hookshot, messageBoxProcAddress);
#endif

    // If successful, the test program's message box should be modified as described above.
    // If not, an error message is displayed, in addition to the original unmodified message box that the test program would otherwise display.
    if (!Hookshot::SuccessfulResult(result))
    {
        // Since MessageBoxW was not successfully hooked, this call will proceed unmodified.
        // Showing a message box during a hook module's entry point is not generally a good idea, but in this specific scenario it is useful for the purposes of demonstrating how Hookshot works.
        MessageBoxW(nullptr, L"Failed to hook MessageBoxW", L"DynamicHook Example", MB_ICONERROR);
    }
}
