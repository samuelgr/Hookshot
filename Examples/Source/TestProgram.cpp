/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file TestProgram.cpp
 *   Test program used to demonstrate the example hook modules.
 *****************************************************************************/

#include <Windows.h>

 
// -------- INTERNAL VARIABLES --------------------------------------------- //

/// Message box text displayed by the test program.
static constexpr wchar_t kMessageBoxText[] = L"This is a test program.";

/// Title of the message box displayed by the test program.
static constexpr wchar_t kMessageBoxTitle[] = L"TestProgram";


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCommandShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    // This test program simply generates a message box and then exits.
    // Hook module examples use Hookshot to hook `MessageBoxW` and modify the message box that is displayed.
    // Unmodified, the message box has a single OK button and no icon.  It displays the text contained in the variables defined at the top of this file.

    MessageBoxW(nullptr, kMessageBoxText, kMessageBoxTitle, MB_OK);
    return 0;
}
