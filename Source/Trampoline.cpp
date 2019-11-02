/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Trampoline.cpp
 *   Implementation of some functionality for individual trampolines.
 *****************************************************************************/

#include "Trampoline.h"

#include <cstddef>
#include <cstdint>


namespace Hookshot
{
    // -------- INSTANCE METHODS --------------------------------------- //
    // See "Trampoline.h" for documentation.

    bool Trampoline::SetTarget(void* target)
    {
        // TODO: implement this method for real.
        code.dword[0] = kCodeFailedSet;
        return false;
    }
}
