/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file CodeInjector.h
 *   Implementation of code injection, execution, and synchronization.
 *   This file is the only interface to assembly-written code.
 *   Changes to behavior of the assembly code must be reflected here too.
 *****************************************************************************/

#include "ApiWindows.h"
#include "CodeInjector.h"
#include "Inject.h"
#include "InjectResult.h"

#include <cstddef>


namespace Hookshot
{
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "CodeInjector.h" for documentation.

    CodeInjector::CodeInjector(void* const baseAddressCode, void* const baseAddressData, void* const entryPoint, const size_t sizeCode, const size_t sizeData, const HANDLE injectedProcess, const HANDLE injectedProcessMainThread) : baseAddressCode(baseAddressCode), baseAddressData(baseAddressData), entryPoint(entryPoint), sizeCode(sizeCode), sizeData(sizeData), injectedProcess(injectedProcess), injectedProcessMainThread(injectedProcessMainThread), injectInfo()
    {
        // Nothing to do here.
    }


    // -------- INSTANCE METHODS ------------------------------------------- //
    // See "CodeInjector.h" for documentation.

    EInjectResult CodeInjector::SetAndRun(void)
    {
        EInjectResult result = Check();

        if (EInjectResult::InjectResultSuccess == result)
            result = Set();

        if (EInjectResult::InjectResultSuccess == result)
            result = Run();

        if (EInjectResult::InjectResultSuccess == result)
            result = UnsetTrampoline();
        
        return result;
    }


    // -------- HELPERS ---------------------------------------------------- //
    // See "CodeInjector.h" for documentation.

    EInjectResult CodeInjector::Check(void) const
    {
        if (EInjectResult::InjectResultSuccess != injectInfo.InitializationResult())
            return injectInfo.InitializationResult();
        
        if (GetTrampolineCodeSize() > sizeof(oldCodeAtTrampoline))
            return EInjectResult::InjectResultErrorInsufficientTrampolineSpace;
        
        if (GetRequiredCodeSize() > sizeCode)
            return EInjectResult::InjectResultErrorInsufficientCodeSpace;

        if (GetRequiredDataSize() > sizeData)
            return EInjectResult::InjectResultErrorInsufficientDataSpace;

        if ((NULL == baseAddressCode) || (NULL == baseAddressData) || (NULL == entryPoint) || (INVALID_HANDLE_VALUE == injectedProcess) || (INVALID_HANDLE_VALUE == injectedProcessMainThread))
            return EInjectResult::InjectResultErrorInternalInvalidParams;
        
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    size_t CodeInjector::GetRequiredCodeSize(void) const
    {
        return (size_t)injectInfo.GetInjectCodeEnd() - (size_t)injectInfo.GetInjectCodeStart();
    }

    // --------

    size_t CodeInjector::GetRequiredDataSize(void) const
    {
        return sizeof(SInjectData);
    }

    // --------

    size_t CodeInjector::GetTrampolineCodeSize(void) const
    {
        return (size_t)injectInfo.GetInjectTrampolineEnd() - (size_t)injectInfo.GetInjectTrampolineStart();
    }

    // --------
    
    EInjectResult CodeInjector::Run(void)
    {
        injectSyncInit(injectedProcess, baseAddressData);
        
        // Allow the injected code to start running.
        if (1 != ResumeThread(injectedProcessMainThread))
            return EInjectResult::InjectResultErrorRunFailedResumeThread;

        // Synchronize with the injected code.
        if (false == injectSync())
            return EInjectResult::InjectResultErrorRunFailedSync;
        
        //
        // TODO - Implement functionality of the injected code.
        //
        
        // Wait for the injected code to reach completion and synchronize with it.
        // Once the injected code reaches this point, put the thread to sleep and then allow it to advance.
        // This way, upon waking, the thread will advance past the barrier and execute the process as normal.
        if (false == injectSyncWait())
            return EInjectResult::InjectResultErrorRunFailedSync;

        if (0 != SuspendThread(injectedProcessMainThread))
            return EInjectResult::InjectResultErrorRunFailedSuspendThread;

        if (false == injectSyncAdvance())
            return EInjectResult::InjectResultErrorRunFailedSync;

        // All injection operations completed successfully.
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult CodeInjector::Set(void)
    {
        SIZE_T numBytes = 0;

        // Back up the code currently at the trampoline's target location.
        if ((FALSE == ReadProcessMemory(injectedProcess, entryPoint, oldCodeAtTrampoline, GetTrampolineCodeSize(), &numBytes)) || (GetTrampolineCodeSize() != numBytes))
            EInjectResult::InjectResultErrorSetFailedRead;
        
        // Write the trampoline code.
        if ((FALSE == WriteProcessMemory(injectedProcess, entryPoint, injectInfo.GetInjectTrampolineStart(), GetTrampolineCodeSize(), &numBytes)) || (GetTrampolineCodeSize() != numBytes) || (FALSE == FlushInstructionCache(injectedProcess, entryPoint, GetTrampolineCodeSize())))
            EInjectResult::InjectResultErrorSetFailedWrite;

        // Place the address of the main code entry point into the trampoline code at the correct location.
        {
            void* const targetAddress = (void*)((size_t)entryPoint + ((size_t)injectInfo.GetInjectTrampolineAddressMarker() - (size_t)injectInfo.GetInjectTrampolineStart()) - sizeof(size_t));
            const size_t sourceData = (size_t)baseAddressCode + ((size_t)injectInfo.GetInjectCodeBegin() - (size_t)injectInfo.GetInjectCodeStart());
            
            if ((FALSE == WriteProcessMemory(injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytes)) || (sizeof(sourceData) != numBytes) || (FALSE == FlushInstructionCache(injectedProcess, targetAddress, sizeof(sourceData))))
                EInjectResult::InjectResultErrorSetFailedWrite;
        }

        // Write the main code.
        if ((FALSE == WriteProcessMemory(injectedProcess, baseAddressCode, injectInfo.GetInjectCodeStart(), GetRequiredCodeSize(), &numBytes)) || (GetRequiredCodeSize() != numBytes) || (FALSE == FlushInstructionCache(injectedProcess, baseAddressCode, GetRequiredCodeSize())))
            EInjectResult::InjectResultErrorSetFailedWrite;

        // Place the pointer to the data region into the correct spot in the code region.
        {
            void* const targetAddress = baseAddressCode;
            const size_t sourceData = (size_t)baseAddressData;

            if ((FALSE == WriteProcessMemory(injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytes)) || (sizeof(sourceData) != numBytes) || (FALSE == FlushInstructionCache(injectedProcess, targetAddress, sizeof(sourceData))))
                EInjectResult::InjectResultErrorSetFailedWrite;
        }

        // Initialize the data region.
        {
            SInjectData injectData;
            memset((void*)&injectData, 0, sizeof(injectData));

            if ((FALSE == WriteProcessMemory(injectedProcess, baseAddressData, &injectData, sizeof(injectData), &numBytes)) || (sizeof(injectData) != numBytes))
                EInjectResult::InjectResultErrorSetFailedWrite;
        }
        
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult CodeInjector::UnsetTrampoline(void)
    {
        SIZE_T numBytes = 0;

        if ((FALSE == WriteProcessMemory(injectedProcess, entryPoint, oldCodeAtTrampoline, GetTrampolineCodeSize(), &numBytes)) || (GetTrampolineCodeSize() != numBytes) || (FALSE == FlushInstructionCache(injectedProcess, entryPoint, GetTrampolineCodeSize())))
            return EInjectResult::InjectResultErrorUnsetFailed;
        
        return EInjectResult::InjectResultSuccess;
    }
}
