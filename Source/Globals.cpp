/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Globals.cpp
 *   Implementation of accessors and mutators for global data items.
 *   Intended for miscellaneous data elements with no other suitable place.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"

#include <cstring>


namespace Hookshot
{
    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "Globals.h" for documentation.

    HINSTANCE Globals::gInstanceHandle = NULL;
}
