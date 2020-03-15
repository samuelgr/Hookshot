/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file HookLibraryEntry.cpp
 *   Part of the Skeleton hook library example.
 *   Implementation of all entry-point-related functionality.
 *****************************************************************************/

#include <hookshot.h>
#include <windows.h>


// -------- ENTRY POINT FUNCTIONS ------------------------------------------ //

/// Performs Hookshot-independent initialization and teardown functions.
/// Invoked automatically by the operating system.
/// Refer to Windows documentation for more information.
/// @param [in] hinstDLL Instance handle for this library.
/// @param [in] fdwReason Specifies the event that caused this function to be invoked.
/// @param [in] lpvReserved Reserved.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
extern "C" BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    default:
        break;
    }

    return TRUE;
}


/// Performs Hookshot-specific initialization and must be exported from the DLL.
/// Invoked by Hookshot immediately upon hook library load (i.e. after the library is initialized internally and `DllMain` has completed).
/// Hook libraries should use this opportunity to set initial hooks and perform any other initialization unsafe to do in `DllMain`.
/// The instance of #IHookConfig should also be saved because it is valid throughout the execution of the current process, although Hookshot retains ownership over it.
/// @param [in] hookConfig Hookshot configuration interface, which remains at all times owned by Hookshot itself.  Refer to #IHookConfig documentation for more information.
HOOKSHOT_HOOK_LIBRARY_ENTRY(hookConfig)
{
    // Code goes here.
    // It can do anything, including interact with Hookshot via the hookConfig parameter.
}
