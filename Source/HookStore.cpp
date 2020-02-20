/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file HookStore.cpp
 *   Data structure implementation for holding information about hooks.
 *****************************************************************************/

#include "ApiWindows.h"
#include "HookStore.h"
#include "TemporaryBuffers.h"

#include <concrt.h>


namespace Hookshot
{
    // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //
    // See "Hookshot.h" for documentation.

    HookStore::HookStore(void) : lock(), hookMap(), trampolines()
    {
#ifdef HOOKSHOT64
        // In 64-bit mode, multiple TrampolineStore objects will be created in different parts of the address space.
        // Therefore, there is nothing to do at object construction time.
#else
        // In 32-bit mode, the entire address space can be accessed via rel32 displacements.
        // Therefore, it is sufficient just to create and use TrampolineStore objects in a centralized location.
        trampolines.emplace_back();
#endif
    }

    
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    const TFunc HookStore::GetOriginalFunctionForHook(const THookID hook)
    {
        const int trampolineStoreIndex = hook / TrampolineStore::kTrampolineStoreCount;
        const int trampolineIndex = hook % TrampolineStore::kTrampolineStoreCount;

        concurrency::reader_writer_lock::scoped_lock_read guard(this->lock);
        
        if (trampolineStoreIndex < 0 || trampolineStoreIndex >= (int)trampolines.size())
            return NULL;

        if (trampolineIndex < 0 || trampolineIndex >= trampolines[trampolineStoreIndex].Count())
            return NULL;

        return trampolines[trampolineStoreIndex][trampolineIndex].GetOriginalTargetFunction();
    }

    // --------

    THookID HookStore::IdentifyHook(const TFunc targetFunc)
    {
        if (NULL == targetFunc)
            return EHookError::HookErrorInvalidArgument;

        concurrency::reader_writer_lock::scoped_lock_read guard(this->lock);

        if (0 == hookMap.count(targetFunc))
            return EHookError::HookErrorNotFound;

        return hookMap.at(targetFunc);
    }
    
    // --------

    THookID HookStore::SetHook(const TFunc hookFunc, TFunc targetFunc)
    {
        if (NULL == hookFunc || NULL == targetFunc)
            return EHookError::HookErrorInvalidArgument;
        
        if (EHookError::HookErrorNotFound != IdentifyHook(targetFunc))
            return EHookError::HookErrorDuplicate;
        
        concurrency::reader_writer_lock::scoped_lock guard(this->lock);

#ifdef HOOKSHOT64
        // In 64-bit mode, trampolines are stored close to the target functions.
        // Therefore, it is necessary to identify the TrampolineStore object that is correct for the given target function address.
        // TODO: implement this identification process and set the value of trampolineStoreIndex appropriately.
        const size_t trampolineStoreIndex = 0;
        return EHookError::HookErrorSetFailed;
#else
        // In 32-bit mode, all trampolines are stored in a central location.
        // Therefore, it is sufficient to keep appending new TrampolineStore objects as existing ones fill up.
        if (0 == trampolines.back().FreeCount())
            trampolines.emplace_back();

        const size_t trampolineStoreIndex = trampolines.size() - 1;
#endif
        
        TrampolineStore& trampolineStore = trampolines[trampolineStoreIndex];
        if (false == trampolineStore.IsInitialized())
            return EHookError::HookErrorInitializationFailed;

        const int allocatedIndex = trampolineStore.Allocate();
        if (allocatedIndex < 0)
            return EHookError::HookErrorAllocationFailed;

        if (false == trampolineStore[allocatedIndex].SetHookForTarget(hookFunc, targetFunc))
        {
            trampolineStore.DeallocateIfNotSet();
            return EHookError::HookErrorSetFailed;
        }

        const THookID hookIdentifier = (THookID)(((trampolineStoreIndex) * TrampolineStore::kTrampolineStoreCount) + allocatedIndex);
        hookMap[targetFunc] = hookIdentifier;

        return hookIdentifier;
    }
}
