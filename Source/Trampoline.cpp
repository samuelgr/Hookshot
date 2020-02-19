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
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "Trampoline.h" for documentation.

    Trampoline::Trampoline(void) : code()
    {
        for (int i = 0; i < sizeof(kHookCodeDefault); ++i)
            code.hook.byte[i] = kHookCodeDefault[i];

        code.original.word[0] = kOriginalCodeDefault;
    }


    // -------- INSTANCE METHODS --------------------------------------- //
    // See "Trampoline.h" for documentation.
    
    bool Trampoline::SetHookForTarget(TFunc hook, void* target)
    {
        // TODO: implement transplanting part of this method.

        code.hook.ptr[_countof(code.hook.ptr) - 1] = ValueForHookAddress(hook);
        return true;
    }
}
