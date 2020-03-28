/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Skeleton.cpp
 *   Skeleton hook module example, showing an empty hook module project.
 *****************************************************************************/

#include "Hookshot\Hookshot.h"


/// Performs Hookshot-specific initialization and must be exported from the DLL.
/// Invoked by Hookshot immediately upon hook library load (i.e. after the library is initialized internally and `DllMain` has completed, if present).
/// Hook modules should use this opportunity to set initial hooks and perform any other initialization unsafe to do in `DllMain`.
HOOKSHOT_HOOK_MODULE_ENTRY
{
    // Code goes here.
}
