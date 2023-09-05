/***************************************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 ***********************************************************************************************//**
 * @file Empty.cpp
 *   "Empty" hook module example.
 *   This example shows an empty hook module project.  It contains the minimum required to
 *   produce a hook module that Hookshot will load successfully.
 **************************************************************************************************/

#include "Hookshot/Hookshot.h"

/// Performs Hookshot-specific initialization and must be exported from the DLL. Invoked by Hookshot
/// immediately upon hook library load (i.e. after the library loaded and `DllMain` has completed,
/// if present). Hook modules should use this opportunity to set initial hooks and perform any other
/// initialization unsafe to do in `DllMain`.
HOOKSHOT_HOOK_MODULE_ENTRY(hookshot)
{
  // Code goes here. Per the macro invocation above, variable "hookshot" is the Hookshot interface
  // object pointer.
}
