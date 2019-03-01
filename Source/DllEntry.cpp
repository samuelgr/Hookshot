/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file DllEntry.cpp
 *   Entry points for the injected library.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "InjectLanding.h"

using namespace Hookshot;


// -------- ENTRY POINT FUNCTIONS ------------------------------------------ //

/// Performs library initialization and teardown functions.
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
        Globals::SetInstanceHandle(hinstDLL);
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
extern "C" __declspec(dllexport) void* APIENTRY DllInit(void)
{
    // Any additional initialization would take place here.
    // Currently there is nothing to do, so just allow the injection to proceed.
    return (void*)InjectLanding;
}
