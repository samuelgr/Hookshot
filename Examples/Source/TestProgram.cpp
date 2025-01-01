/***************************************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file TestProgram.cpp
 *   Test program used to demonstrate the example hook modules.
 **************************************************************************************************/

#include <Windows.h>

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCommandShow Flag that specifies how the main application window should be shown.
/// Not applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
  // This test program simply generates a message box and then exits. Hook module examples use
  // Hookshot to hook `MessageBoxW` and modify the message box that is displayed. Unmodified, the
  // message box displays the text shown below and has a single OK button and no icon.
  MessageBoxW(nullptr, L"This is a test program.", L"TestProgram", MB_OK);
  return 0;
}
