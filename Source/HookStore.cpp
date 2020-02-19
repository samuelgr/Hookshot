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
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    TFunc HookStore::GetOriginalFunctionForHook(const THookID hook) const
    {
        if (hook < trampolines.Count())
            return (TFunc)&trampolines[hook];

        return NULL;
    }

    // --------

    THookID HookStore::IdentifyHook(const THookString& dllName, const THookString& exportFuncName)
    {
        // Verify the DLL is loaded and retrieve its absolute path.
        // DLL names could be provided as base name only and full path, but only full paths are stored to avoid aliasing.
        HMODULE dllHandle = GetModuleHandle(dllName.c_str());
        if (NULL == dllHandle)
            return EHookError::HookErrorLibraryNotLoaded;

        THookString dllAbsoluteName;
        if (false == ResolveLoadedModuleName(dllHandle, dllAbsoluteName))
            return EHookError::HookErrorResolveModuleName;

        // Grab the lock as a reader and perform the lookup.
        concurrency::reader_writer_lock::scoped_lock_read(this->lock);

        TDllMap::const_iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
            return EHookError::HookErrorNotFound;

        TFunctionMap::const_iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() == functionHookMap)
            return EHookError::HookErrorNotFound;

        return functionHookMap->second;
    }

    // --------

    THookID HookStore::SetHook(const TFunc hookFunc, const THookString& dllName, const THookString& exportFuncName)
    {
        if (NULL == hookFunc)
            return EHookError::HookErrorInvalidArgument;

        if (false == trampolines.IsInitialized())
            return EHookError::HookErrorInitializationFailed;

        // Verify the DLL is loaded and retrieve its absolute path.
        // DLL names could be provided as base name only and full path, but only full paths are stored to avoid aliasing.
        HMODULE dllHandle = GetModuleHandle(dllName.c_str());
        if (NULL == dllHandle)
            return EHookError::HookErrorLibraryNotLoaded;
        
        THookString dllAbsoluteName;
        if (false == ResolveLoadedModuleName(dllHandle, dllAbsoluteName))
            return EHookError::HookErrorResolveModuleName;

        // Retrieve the address of the exported function, which is used as the trampoline target.
        // This also verifies that the requested DLL exports the requested function.
#ifdef UNICODE
        // In case of compilation in wide-character mode a conversion is needed because GetProcAddress does not support wide characters.
        TemporaryBuffer<char> exportFuncNameStr;
        if (0 != wcstombs_s(NULL, exportFuncNameStr, exportFuncNameStr.Size(), exportFuncName.c_str(), exportFuncName.length()))
            return EHookError::HookErrorResolveFunctionName;
#else
        const char* const exportFuncNameStr = exportFuncName.c_str();
#endif
        void* exportFuncAddress = (void*)GetProcAddress(dllHandle, exportFuncNameStr);
        if (NULL == exportFuncAddress)
            return EHookError::HookErrorFunctionNotExported;

        // Grab the lock as a writer and try to insert the new hook.
        concurrency::reader_writer_lock::scoped_lock(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllAbsoluteName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
        {
            mapFunctionNameToHookID.emplace_back();
            dllFunctionMap = mapDllNameToFunctionMap.emplace(dllAbsoluteName, mapFunctionNameToHookID.back()).first;
        }

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);
        THookID hookID;

        // Check for duplicates.
        // If a mapping table entry exists but the trampoline is unset, it is safe to set it.
        // If a mapping table entry exists and the trampoline is already set, fail because it cannot be changed.
        // If no mapping table entry exists, one is created.
        if (dllFunctionMap->second.end() != functionHookMap)
        {
            hookID = functionHookMap->second;

            if (trampolines[hookID].IsTargetSet())
                return EHookError::HookErrorDuplicate;
        }
        else
        {
            hookID = (THookID)trampolines.Allocate();

            if (hookID < 0)
                return EHookError::HookErrorAllocationFailed;

            dllFunctionMap->second.emplace(exportFuncName, hookID);
        }

        if (false == trampolines[hookID].SetHookForTarget(hookFunc, exportFuncAddress))
            return EHookError::HookErrorSetFailed;

        return hookID;
    }


    // -------- HELPERS ------------------------------------------------ //
    // See "Hookshot.h" for documentation.

    bool HookStore::ResolveLoadedModuleName(HMODULE dllHandle, THookString& dllAbsoluteName)
    {
        TemporaryBuffer<TCHAR> dllAbsoluteNameBuffer;
        DWORD outputNumChars = GetModuleFileName(dllHandle, dllAbsoluteNameBuffer, (DWORD)dllAbsoluteNameBuffer.Count());

        if ((((DWORD)dllAbsoluteNameBuffer.Count() == outputNumChars) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) || (0 == outputNumChars))
            return false;

        dllAbsoluteName = dllAbsoluteNameBuffer;

        return true;
    }
}
