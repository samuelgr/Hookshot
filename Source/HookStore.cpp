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

    TFunc HookStore::GetOriginalFunctionForHook(const THookID hook)
    {
        if (hook < trampolines.Count())
            return (TFunc)&trampolines[hook];

        return NULL;
    }

    // --------

    THookID HookStore::IdentifyHook(const THookString dllName, const THookString exportFuncName)
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

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
            return EHookError::HookErrorNotFound;

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() == functionHookMap)
            return EHookError::HookErrorNotFound;

        return functionHookMap->second;
    }

    // --------

    THookID HookStore::SetHook(const TFunc hookFunc, const THookString& dllName, const THookString& exportFuncName)
    {
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
        // In case of compilation in wide-character mode a conversion is needed because GetProcAddress does not support wchar_t.
        TemporaryBuffer<char> exportFuncNameA;
        if (0 != wcstombs_s(NULL, exportFuncNameA, exportFuncNameA.Size(), exportFuncName.c_str(), exportFuncName.length()))
            return EHookError::HookErrorResolveFunctionName;

        TFunc exportFuncAddress = (TFunc)GetProcAddress(dllHandle, exportFuncNameA);
#else
        TFunc exportFuncAddress = (TFunc)GetProcAddress(dllHandle, exportFuncName.c_str());
#endif

        if (NULL == exportFuncAddress)
            return EHookError::HookErrorFunctionNotExported;

        // Grab the lock and try to insert the new hook.
        concurrency::reader_writer_lock::scoped_lock(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllAbsoluteName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
        {
            mapFunctionNameToHookID.emplace_back();
            dllFunctionMap = mapDllNameToFunctionMap.emplace(dllAbsoluteName, mapFunctionNameToHookID.back()).first;
        }

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        // TODO: what if trampoline ops fail, does that mean no re-attempts because the mapping entry is already present?
        if (dllFunctionMap->second.end() != functionHookMap)
            return EHookError::HookErrorDuplicate;

        const int newHookID = trampolines.Allocate();

        if (newHookID < 0)
            return EHookError::HookErrorAllocationFailed;

        // TODO: set the trampoline's target.
        dllFunctionMap->second.emplace(exportFuncName, (THookID)newHookID);
        return (THookID)newHookID;
    }


    // -------- HELPERS ------------------------------------------------ //
    // See "Hookshot.h" for documentation.

    bool HookStore::ResolveLoadedModuleName(HMODULE dllHandle, THookString& dllAbsoluteName)
    {
        TemporaryBuffer<TCHAR> dllAbsoluteNameBuffer;
        DWORD outputNumChars = GetModuleFileName(dllHandle, dllAbsoluteNameBuffer, (DWORD)dllAbsoluteNameBuffer.Count());

        if ((((DWORD)dllAbsoluteNameBuffer.Count() == outputNumChars) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) || (0 == outputNumChars))
            return false;

        return true;
    }
}
