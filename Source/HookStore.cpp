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
#include "Globals.h"
#include "HookStore.h"
#include "Message.h"
#include "TemporaryBuffer.h"
#include "X86Instruction.h"

#include <shared_mutex>


namespace Hookshot
{
    // -------- INTERNAL FUNCTIONS ------------------------------------- //

    /// Determines the base address of the memory region associated with the target function.
    /// @param [in] originalFunc Address of the function that is being hooked.
    /// @return Base address of the associated memory region, or NULL if it cannot be determined.
    static void* BaseAddressForOriginalFunc(const void* originalFunc)
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

    /// Determines if the specified hook is allowed to be set.
    /// @param [in] originalFunc Address of the function that is being hooked.
    /// @param [in] hookFunc Address of the hook function.
    /// @return `true` if hooking the specified function is allowed, `false` if not.
    static bool IsSetHookAllowed(const void* originalFunc, const void* hookFunc)
    {
        // Hooking Hookshot itself is forbidden.
        if (BaseAddressForOriginalFunc(originalFunc) == Globals::GetInstanceHandle())
            return false;

        return true;
    }

    /// Redirects the flow of execution from the specified address to the specified address.
    /// Accomplishes this task by overwriting some bytes of the source function with a jump that targets the destination address.
    /// @param [in,out] from Source function, part of which will be overwritten.
    /// @param [in] to Destination function.
    /// @return `true` on success, `false` on failure.
    static inline bool RedirectExecution(void* from, const void* to)
    {
        DWORD originalProtection = 0;
        if (0 == VirtualProtect(from, X86Instruction::kJumpInstructionLengthBytes, PAGE_EXECUTE_READWRITE, &originalProtection))
            return false;

        const bool writeJumpResult = X86Instruction::WriteJumpInstruction(from, X86Instruction::kJumpInstructionLengthBytes, to);

        DWORD unusedOriginalProtection = 0;
        const bool restoreProtectionResult = (0 != VirtualProtect(from, X86Instruction::kJumpInstructionLengthBytes, originalProtection, &unusedOriginalProtection));
        if (true == restoreProtectionResult)
            FlushInstructionCache(GetCurrentProcess(), from, (SIZE_T)X86Instruction::kJumpInstructionLengthBytes);

        return (writeJumpResult && restoreProtectionResult);
    }


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

        if (false == IsSetHookAllowed(originalFunc, hookFunc))
            return EHookshotResult::HookshotResultFailForbidden;
        
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
        void* const baseAddress = BaseAddressForOriginalFunc(originalFunc);
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
            return EHookshotResult::HookshotResultFailAllocation;

        Trampoline& trampoline = trampolineStore[allocatedIndex];
        trampoline.SetHookFunction(hookFunc);
        if (false == trampoline.SetOriginalFunction(originalFunc))
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Failed to set up a trampoline for original function at 0x%llx."), (long long)originalFunc);

            trampolineStore.Deallocate();
            return EHookshotResult::HookshotResultFailCannotSetHook;
        }

        if (false == RedirectExecution(originalFunc, trampoline.GetHookFunction()))
        {
            Message::OutputFormatted(EMessageSeverity::MessageSeverityInfo, _T("Failed to redirect execution from 0x%llx to 0x%llx."), (long long)originalFunc, (long long)trampoline.GetHookFunction());
            
            trampolineStore.Deallocate();
            return EHookshotResult::HookshotResultFailCannotSetHook;
        }

        functionToTrampoline[originalFunc] = &trampolineStore[allocatedIndex];
        functionToTrampoline[hookFunc] = &trampolineStore[allocatedIndex];

        return EHookshotResult::HookshotResultSuccess;
    }
}
