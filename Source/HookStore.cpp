/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookStore.cpp
 *   Data structure implementation for holding information about hooks.
 *****************************************************************************/

#include "ApiWindows.h"
#include "HookStore.h"
#include "TemporaryBuffer.h"
#include "X86Instruction.h"

#include <shared_mutex>


namespace Hookshot
{
    // -------- INTERNAL FUNCTIONS ------------------------------------- //

#ifdef HOOKSHOT64
    /// Determines the base address of the memory region associated with the target function.
    /// @param [in] originalFunc Address of the target function that is being hooked.
    /// @return Base address of the associated memory region, or NULL if it cannot be determined.
    static void* BaseAddressFororiginalFunc(const void* originalFunc)
    {
        // If the target function is part of a loaded module, the base address of the region is the base address of that module.
        HMODULE moduleHandle;
        if (0 != GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)originalFunc, &moduleHandle))
            return (void*)moduleHandle;
        
        // If the target function is not part of a loaded module, the base address of the region needs to be queried.
        MEMORY_BASIC_INFORMATION virtualMemoryInfo;
        if (sizeof(virtualMemoryInfo) == VirtualQuery((LPCVOID)originalFunc, &virtualMemoryInfo, sizeof(virtualMemoryInfo)))
            return virtualMemoryInfo.AllocationBase;

        // At this point the base address cannot be determined.
        return NULL;
    }
#endif


    // -------- CLASS VARIABLES ---------------------------------------- //
    // See "HookStore.h" for documentation.

    std::shared_mutex HookStore::hookStoreMutex;

    std::unordered_map<const void*, Trampoline*> HookStore::functionToTrampoline;

    std::vector<TrampolineStore> HookStore::trampolines;

#ifdef HOOKSHOT64
    std::unordered_map<void*, int> HookStore::trampolineStoreMap;
#endif

    
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    const void* HookStore::GetOriginalFunction(void* func)
    {
        std::shared_lock<std::shared_mutex> lock(hookStoreMutex);

        if (0 == functionToTrampoline.count(func))
            return NULL;

        return functionToTrampoline.at(func)->GetOriginalFunction();
    }
    
    EHookshotResult HookStore::SetHook(void* originalFunc, const void* hookFunc)
    {
        if (NULL == originalFunc || NULL == hookFunc)
            return EHookshotResult::HookshotResultFailInvalidArgument;

        if (((intptr_t)hookFunc >= (intptr_t)originalFunc) && ((intptr_t)hookFunc < (intptr_t)originalFunc + X86Instruction::kJumpInstructionLengthBytes))
            return EHookshotResult::HookshotResultFailInvalidArgument;
        
        // At this point, data structures will be consulted, so it is necessary to take a lock.
        std::unique_lock<std::shared_mutex> lock(hookStoreMutex);

        // Check for duplicates.
        // If Hookshot has already set a hook that touches either the specified original or hook function, that is an error.
        if (0 != functionToTrampoline.count(originalFunc) || 0 != functionToTrampoline.count(hookFunc))
            return EHookshotResult::HookshotResultFailDuplicate;

#ifdef HOOKSHOT64
        // In 64-bit mode, trampolines are stored close to the target functions.
        // Therefore, it is necessary to identify the TrampolineStore object that is correct for the given target function address.
        // Because only one TrampolineStore object exists per base address, the number of allowed hooks per base address is limited.
        void* const baseAddress = BaseAddressFororiginalFunc(originalFunc);
        if (NULL == baseAddress)
            return EHookshotResult::HookshotResultFailInternal;

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
            return EHookshotResult::HookshotResultFailAllocation;

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
            return EHookshotResult::HookshotResultFailInternal;

        const int allocatedIndex = trampolineStore.Allocate();
        if (allocatedIndex < 0)
            return EHookshotResult::HookshotResultFailInternal;

        if (false == trampolineStore[allocatedIndex].SetHookForTarget(hookFunc, originalFunc))
        {
            trampolineStore.DeallocateIfNotSet();
            return EHookshotResult::HookshotResultFailCannotSetHook;
        }

        functionToTrampoline[originalFunc] = &trampolineStore[allocatedIndex];
        functionToTrampoline[hookFunc] = &trampolineStore[allocatedIndex];

        return EHookshotResult::HookshotResultSuccess;
    }
}
