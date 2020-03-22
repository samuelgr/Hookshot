/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file DllEntry.cpp
 *   Entry points for the injected library.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "InjectLanding.h"
#include "LibraryInitialize.h"

using namespace Hookshot;


// -------- ENTRY POINT FUNCTIONS ------------------------------------------ //

/// Performs library initialization and teardown functions.
/// Invoked automatically by the operating system.
/// Refer to Windows documentation for more information.
/// @param [in] hinstDLL Instance handle for this library.
/// @param [in] fdwReason Specifies the event that caused this function to be invoked.
/// @param [in] lpvReserved Reserved.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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

/// Invoked by injection code to perform additional initialization functions, especially those not safe to perform in the main DLL entry point.
/// Success or failure of this function determines the overall success or failure of the injection process.
/// The injecting process is still waiting on this code to complete, so it should be as fast as possible to avoid undue delays.
/// @return Address to which to jump to continue running the injected process, or `NULL` on failure.
extern "C" __declspec(dllexport) void* __stdcall HookshotInjectInitialize(void)
{
    LibraryInitialize::CommonInitialization();
    return (void*)InjectLanding;
}

/// Intended to be invoked by programs that link with this library and load it directly, rather than via injection.
/// Part of the external Hookshot API.  See "Hookshot.h" for documentation.
__declspec(dllexport) IHookConfig* __stdcall HookshotLibraryInitialize(void)
{
    LibraryInitialize::CommonInitialization();
    return LibraryInitialize::GetHookConfigInterface();
}
