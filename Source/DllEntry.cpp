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

#include "DependencyProtect.h"
#include "Globals.h"
#include "HookshotTypes.h"
#include "InjectLanding.h"
#include "LibraryInterface.h"
#include "Message.h"
#include "Strings.h"

#include <string_view>

using namespace Hookshot;


// -------- INTERNAL VARIABLES --------------------------------------------- //

/// Flag that specifies if this library was initialized.
static bool isInitialized = false;


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
/// @return Address to which to jump to continue running the injected process, or `nullptr` on failure.
extern "C" __declspec(dllexport) void* __fastcall HookshotInjectInitialize(void)
{
    if (false == isInitialized)
    {
        Globals::SetHookshotLoadMethod(EHookshotLoadMethod::Injected);
        LibraryInterface::Initialize();

        if (false == LibraryInterface::IsConfigurationDataValid())
            Message::Output(Message::ESeverity::Error, LibraryInterface::GetConfigurationErrorMessage().data());

        isInitialized = true;

        return (void*)InjectLanding;
    }
    else
    {
        Message::OutputFormatted(Message::ESeverity::Error, L"Detected an improper attempt to initialize %s by invoking %s.", Strings::kStrProductName.data(), __FUNCTIONW__);
        return nullptr;
    }
}

/// Invoked when Hookshot is loaded as a library.  See "HookshotFunctions.h" for more information.
/// @return Hookshot interface pointer, or `nullptr` on failure.
extern "C" __declspec(dllexport) IHookshot* __fastcall HookshotLibraryInitialize(void)
{
    if (false == isInitialized)
    {
        Globals::SetHookshotLoadMethod(EHookshotLoadMethod::LibraryLoaded);
        LibraryInterface::Initialize();

        if (false == LibraryInterface::IsConfigurationDataValid())
            Message::Output(Message::ESeverity::Warning, LibraryInterface::GetConfigurationErrorMessage().data());

        isInitialized = true;

        return LibraryInterface::GetHookshotInterfacePointer();
    }
    else
    {
        Message::OutputFormatted(Message::ESeverity::Warning, L"Detected an improper attempt to initialize %s by invoking %s.", Strings::kStrProductName.data(), __FUNCTIONW__);
        return nullptr;
    }
}
