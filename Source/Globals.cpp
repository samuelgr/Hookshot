/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Globals.cpp
 *   Implementation of accessors and mutators for global data items.
 *   Intended for miscellaneous data elements with no other suitable place.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"

using namespace Hookshot;


// -------- CLASS VARIABLES ------------------------------------------------ //
// See "Globals.h" for documentation.

HINSTANCE Globals::gInstanceHandle = NULL;


// -------- CLASS METHODS -------------------------------------------------- //
// See "Globals.h" for documentation.

HINSTANCE Globals::GetInstanceHandle(void)
{
    return gInstanceHandle;
}

// ---------

void Globals::SetInstanceHandle(HINSTANCE newInstanceHandle)
{
    gInstanceHandle = newInstanceHandle;
}
