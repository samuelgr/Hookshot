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

#include "DependencyProtect.h"
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
        if (0 != Windows::ProtectedGetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)originalFunc, &moduleHandle))
            return (void*)moduleHandle;
        
        // If the target function is not part of a loaded module, the base address of the region needs to be queried.
        MEMORY_BASIC_INFORMATION virtualMemoryInfo;
        if (sizeof(virtualMemoryInfo) == Windows::ProtectedVirtualQuery((LPCVOID)originalFunc, &virtualMemoryInfo, sizeof(virtualMemoryInfo)))
            return virtualMemoryInfo.AllocationBase;

        // At this point the base address cannot be determined.
        return NULL;
    }

    /// Checks the specified hook for validity and safety.
    /// @param [in] originalFunc Address of the function that is being hooked.
    /// @param [in] hookFunc Address of the hook function.
    /// @return `true` if hooking the specified function is allowed, `false` if not.
    static bool IsHookSpecValid(const void* originalFunc, const void* hookFunc)
    {
        // Simple null pointer check.
        if (NULL == originalFunc || NULL == hookFunc)
            return false;

        // Verify that the hook function is not located within the region of the original function that is guaranteed to be transplanted.
        if (((intptr_t)hookFunc >= (intptr_t)originalFunc) && ((intptr_t)hookFunc < (intptr_t)originalFunc + X86Instruction::kJumpInstructionLengthBytes))
            return false;
        
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
        if (0 == Windows::ProtectedVirtualProtect(from, X86Instruction::kJumpInstructionLengthBytes, PAGE_EXECUTE_READWRITE, &originalProtection))
            return false;

        const bool writeJumpResult = X86Instruction::WriteJumpInstruction(from, X86Instruction::kJumpInstructionLengthBytes, to);

        DWORD unusedOriginalProtection = 0;
        const bool restoreProtectionResult = (0 != Windows::ProtectedVirtualProtect(from, X86Instruction::kJumpInstructionLengthBytes, originalProtection, &unusedOriginalProtection));
        if (true == restoreProtectionResult)
            Windows::ProtectedFlushInstructionCache(Globals::GetCurrentProcessHandle(), from, (SIZE_T)X86Instruction::kJumpInstructionLengthBytes);

        return (writeJumpResult && restoreProtectionResult);
    }


    // -------- CLASS VARIABLES ---------------------------------------- //
    // See "HookStore.h" for documentation.

    std::shared_mutex HookStore::hookStoreMutex;

    std::unordered_map<const void*, Trampoline*> HookStore::functionToTrampoline;

    std::unordered_map<Trampoline*, const void*> HookStore::trampolineToOriginalFunction;

    std::vector<TrampolineStore> HookStore::trampolines;

#ifdef HOOKSHOT64
    std::unordered_map<void*, int> HookStore::trampolineStoreMap;
#endif

    
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    EResult HookStore::CreateHook(void* originalFunc, const void* hookFunc)
    {
        if (false == IsHookSpecValid(originalFunc, hookFunc))
            return EResult::HookshotResultFailInvalidArgument;

        std::unique_lock<std::shared_mutex> lock(hookStoreMutex);

        // Check for duplicates.
        // If Hookshot has already set a hook that touches either the specified original or hook function, that is an error.
        if (0 != functionToTrampoline.count(originalFunc) || 0 != functionToTrampoline.count(hookFunc))
            return EResult::HookshotResultFailDuplicate;

#ifdef HOOKSHOT64
        // In 64-bit mode, trampolines are stored close to the target functions.
        // Therefore, it is necessary to identify the TrampolineStore object that is correct for the given target function address.
        // Because only one TrampolineStore object exists per base address, the number of allowed hooks per base address is limited.
        void* const baseAddress = BaseAddressForOriginalFunc(originalFunc);
        if (NULL == baseAddress)
            return EResult::HookshotResultFailInternal;

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
            return EResult::HookshotResultFailAllocation;

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
            return EResult::HookshotResultFailInternal;

        const int allocatedIndex = trampolineStore.Allocate();
        if (allocatedIndex < 0)
            return EResult::HookshotResultFailAllocation;

        Trampoline& trampoline = trampolineStore[allocatedIndex];
        trampoline.SetHookFunction(hookFunc);
        if (false == trampoline.SetOriginalFunction(originalFunc))
        {
            Message::OutputFormatted(Message::ESeverity::Info, L"Failed to set up a trampoline for original function at 0x%llx.", (long long)originalFunc);

            trampolineStore.Deallocate();
            return EResult::HookshotResultFailCannotSetHook;
        }

        UpdateProtectedDependencyAddress(originalFunc, trampoline.GetOriginalFunction());

        if (false == RedirectExecution(originalFunc, trampoline.GetHookFunction()))
        {
            Message::OutputFormatted(Message::ESeverity::Info, L"Failed to redirect execution from 0x%llx to 0x%llx.", (long long)originalFunc, (long long)trampoline.GetHookFunction());

            trampolineStore.Deallocate();
            return EResult::HookshotResultFailCannotSetHook;
        }

        functionToTrampoline[originalFunc] = &trampolineStore[allocatedIndex];
        functionToTrampoline[hookFunc] = &trampolineStore[allocatedIndex];
        trampolineToOriginalFunction[&trampoline] = originalFunc;

        return EResult::HookshotResultSuccess;
    }

    // --------

    EResult HookStore::DisableHookFunction(const void* originalOrHookFunc)
    {
        return ReplaceHookFunction(originalOrHookFunc, GetOriginalFunction(originalOrHookFunc));
    }

    // --------
    
    const void* HookStore::GetOriginalFunction(const void* originalOrHookFunc)
    {
        std::shared_lock<std::shared_mutex> lock(hookStoreMutex);

        if (0 == functionToTrampoline.count(originalOrHookFunc))
            return NULL;

        return functionToTrampoline.at(originalOrHookFunc)->GetOriginalFunction();
    }

    // --------

    EResult HookStore::ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc)
    {
        std::unique_lock<std::shared_mutex> lock(hookStoreMutex);

        // If this fails, the specified hook does not exist.
        if (0 == functionToTrampoline.count(originalOrHookFunc))
            return EResult::HookshotResultFailNotFound;

        Trampoline* const trampoline = functionToTrampoline.at(originalOrHookFunc);

        // If this fails, internal data structures are inconsistent.
        if (0 == trampolineToOriginalFunction.count(trampoline))
            return EResult::HookshotResultFailInternal;

        const void* const originalFunc = trampolineToOriginalFunction.at(trampoline);
        const void* const oldHookFunc = trampoline->GetHookTrampolineTarget();
        if (oldHookFunc == newHookFunc)
            return EResult::HookshotResultNoEffect;

        // If this fails, internal data structures are inconsistent.
        if (0 == functionToTrampoline.count(originalFunc) || 0 == functionToTrampoline.count(oldHookFunc))
            return EResult::HookshotResultFailInternal;

        // If this fails, the replacement hook function is already involved in a different hook.
        if (0 != functionToTrampoline.count(newHookFunc))
            return EResult::HookshotResultFailDuplicate;

        // If this fails, the specified hook cannot be set.
        if (false == IsHookSpecValid(originalFunc, newHookFunc))
            return EResult::HookshotResultFailInvalidArgument;

        trampoline->SetHookFunction(newHookFunc);
        functionToTrampoline.erase(oldHookFunc);
        functionToTrampoline[newHookFunc] = trampoline;

        return EResult::HookshotResultSuccess;
    }
}
