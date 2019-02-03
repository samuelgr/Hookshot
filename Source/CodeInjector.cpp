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

    CodeInjector::CodeInjector(void* const baseAddressCode, void* const baseAddressData, void* const entryPoint, const size_t sizeCode, const size_t sizeData, const HANDLE injectedProcess, const HANDLE injectedProcessMainThread) : baseAddressCode(baseAddressCode), baseAddressData(baseAddressData), entryPoint(entryPoint), sizeCode(sizeCode), sizeData(sizeData), injectedProcess(injectedProcess), injectedProcessMainThread(injectedProcessMainThread)
    {
        // Nothing to do here.
    }


    // -------- CLASS METHODS ------------------------------------------ //
    // See "CodeInjector.h" for documentation.

    size_t CodeInjector::GetRequiredCodeSize(void)
    {
        return (size_t)&injectCodeEnd - (size_t)&injectCodeStart;
    }

    // --------

    size_t CodeInjector::GetRequiredDataSize(void)
    {
        return (size_t)0;
    }

    size_t CodeInjector::GetTrampolineCodeSize(void)
    {
        return (size_t)&injectTrampolineEnd - (size_t)&injectTrampolineStart;
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
        
        return result;
    }


    // -------- HELPERS ---------------------------------------------------- //
    // See "CodeInjector.h" for documentation.

    EInjectResult CodeInjector::Check(void)
    {
        if (GetRequiredCodeSize() > sizeCode)
            return EInjectResult::InjectResultErrorInsufficientCodeSpace;

        if (GetRequiredDataSize() > sizeData)
            return EInjectResult::InjectResultErrorInsufficientDataSpace;

        if ((NULL == baseAddressCode) || (NULL == baseAddressData) || (NULL == entryPoint) || (INVALID_HANDLE_VALUE == injectedProcess) || (INVALID_HANDLE_VALUE == injectedProcessMainThread))
            return EInjectResult::InjectResultErrorInternalInvalidParams;
        
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult CodeInjector::Run(void)
    {
        // TODO
        // Also remember to save old code in Set and restore it here in Run
        ResumeThread(injectedProcessMainThread);
        
        return EInjectResult::InjectResultSuccess;
    }

    // --------

    EInjectResult CodeInjector::Set(void)
    {
        SIZE_T numBytesWritten = 0;

        // Write the trampoline code.
        if ((FALSE == WriteProcessMemory(injectedProcess, entryPoint, &injectTrampolineStart, GetTrampolineCodeSize(), &numBytesWritten)) || (GetTrampolineCodeSize() != numBytesWritten))
            EInjectResult::InjectResultErrorSetFailedWriteProcessMemory;

        // Place the address of the main code entry point into the trampoline code at the correct location.
        {
            void* const targetAddress = (void*)((size_t)entryPoint + ((size_t)&injectTrampolineAddressMarker - (size_t)&injectTrampolineStart) - sizeof(size_t));
            const size_t sourceData = (size_t)baseAddressCode + ((size_t)&injectCodeBegin - (size_t)&injectCodeStart);
            
            if ((FALSE == WriteProcessMemory(injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytesWritten)) || (sizeof(sourceData) != numBytesWritten))
                EInjectResult::InjectResultErrorSetFailedWriteProcessMemory;
        }

        // Write the main code.
        if ((FALSE == WriteProcessMemory(injectedProcess, baseAddressCode, &injectCodeStart, GetRequiredCodeSize(), &numBytesWritten)) || (GetRequiredCodeSize() != numBytesWritten))
            EInjectResult::InjectResultErrorSetFailedWriteProcessMemory;

        // Place the pointer to the data region into the correct spot in the code region.
        {
            void* const targetAddress = baseAddressCode;
            const size_t sourceData = (size_t)baseAddressData;

            if ((FALSE == WriteProcessMemory(injectedProcess, targetAddress, &sourceData, sizeof(sourceData), &numBytesWritten)) || (sizeof(sourceData) != numBytesWritten))
                EInjectResult::InjectResultErrorSetFailedWriteProcessMemory;
        }
        
        return EInjectResult::InjectResultSuccess;
    }
}
