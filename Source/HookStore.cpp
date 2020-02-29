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
    // -------- INTERNAL FUNCTIONS ------------------------------------- //

#ifdef HOOKSHOT64
    /// Determines the base address of the memory region associated with the target function.
    /// @param [in] targetFunc Address of the target function that is being hooked.
    /// @return Base address of the associated memory region, or NULL if it cannot be determined.
    static void* BaseAddressForTargetFunc(const void* targetFunc)
    {
        // If the target function is part of a loaded module, the base address of the region is the base address of that module.
        HMODULE moduleHandle;
        if (0 != GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)targetFunc, &moduleHandle))
            return (void*)moduleHandle;
        
        // If the target function is not part of a loaded module, the base address of the region needs to be queried.
        MEMORY_BASIC_INFORMATION virtualMemoryInfo;
        if (sizeof(virtualMemoryInfo) == VirtualQuery((LPCVOID)targetFunc, &virtualMemoryInfo, sizeof(virtualMemoryInfo)))
            return virtualMemoryInfo.AllocationBase;

        // At this point the base address cannot be determined.
        return NULL;
    }
#endif


    // -------- CLASS VARIABLES ---------------------------------------- //
    // See "HookStore.h" for documentation.

    concurrency::reader_writer_lock HookStore::lock;

    std::unordered_map<const void*, THookID> HookStore::hookMap;

    std::vector<TrampolineStore> HookStore::trampolines;

#ifdef HOOKSHOT64
    std::unordered_map<void*, int> HookStore::trampolineStoreMap;
#endif

    
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    const void* HookStore::GetOriginalFunctionForHook(THookID hook)
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

    THookID HookStore::IdentifyHook(const void* targetFunc)
    {
        if (NULL == targetFunc)
            return EHookError::HookErrorInvalidArgument;

        concurrency::reader_writer_lock::scoped_lock_read guard(this->lock);

        if (0 == hookMap.count(targetFunc))
            return EHookError::HookErrorNotFound;

        return hookMap.at(targetFunc);
    }
    
    // --------

    THookID HookStore::SetHook(const void* hookFunc, void* targetFunc)
    {
        if (NULL == hookFunc || NULL == targetFunc)
            return EHookError::HookErrorInvalidArgument;
        
        if (EHookError::HookErrorNotFound != IdentifyHook(targetFunc))
            return EHookError::HookErrorDuplicate;
        
        concurrency::reader_writer_lock::scoped_lock guard(this->lock);

#ifdef HOOKSHOT64
        // In 64-bit mode, trampolines are stored close to the target functions.
        // Therefore, it is necessary to identify the TrampolineStore object that is correct for the given target function address.
        // Because only one TrampolineStore object exists per base address, the number of allowed hooks per base address is limited.
        void* const baseAddress = BaseAddressForTargetFunc(targetFunc);
        if (NULL == baseAddress)
            return EHookError::HookErrorInitializationFailed;

        // If this is the first target function for the specified base address, attempt to place a TrampolineStore buffer.
        // Do this by repeatedly moving backward in memory from the base address by the size of the TrampolineStore buffer until either too many attempts were made or a location is identified.
        if (0 == trampolineStoreMap.count(baseAddress))
        {
            size_t proposedTrampolineStoreAddress = (size_t)baseAddress - TrampolineStore::kTrampolineStoreSizeBytes;

            for (int i = 0; i < 100; ++i)
            {
                TrampolineStore newTrampolineStore((void*)proposedTrampolineStoreAddress);
                if (true == newTrampolineStore.IsInitialized())
                {
                    trampolineStoreMap[baseAddress] = (int)trampolines.size();
                    trampolines.push_back(std::move(newTrampolineStore));
                    break;
                }
                
                proposedTrampolineStoreAddress -= TrampolineStore::kTrampolineStoreSizeBytes;
            }
        }

        if (0 == trampolineStoreMap.count(baseAddress))
            return EHookError::HookErrorAllocationFailed;

        const size_t trampolineStoreIndex = trampolineStoreMap.at(baseAddress);
#else
        // In 32-bit mode, all trampolines are stored in a central location.
        // Therefore, it is sufficient to keep appending new TrampolineStore objects as existing ones fill up.
        if (0 == trampolines.size())
            trampolines.emplace_back();

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
