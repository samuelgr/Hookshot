/*****************************************************************************
* Hookshot
*   General-purpose library for hooking API calls in spawned processes.
*****************************************************************************
* Authored by Samuel Grossman
* Copyright (c) 2019
*************************************************************************//**
* @file ExeMain.cpp
*   Entry point for the bootstrap executable.
*****************************************************************************/

#include <tchar.h>
#include <Windows.h>


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `NULL`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCommandShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return `TRUE` if this function successfully initialized or uninitialized this library, `FALSE` otherwise.
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR lpCmdLine, int nCmdShow)
{
    return 0;
}
