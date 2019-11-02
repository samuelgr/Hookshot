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

#include "HookStore.h"

#include <concrt.h>


namespace Hookshot
{
    // -------- CONCRETE INSTANCE METHODS ------------------------------ //
    // See "Hookshot.h" for documentation.

    EHookStatus HookStore::GetHookStatus(const THookID hook)
    {
        if ((hook < 0) || (hook >= trampolines.Count()))
            return EHookStatus::HookStatusOutOfRange;
        
        if (true == trampolines[hook].IsSet())
            return EHookStatus::HookStatusSuccess;

        if (true == trampolines[hook].IsInitialized())
            return EHookStatus::HookStatusFailure;

        return EHookStatus::HookStatusLibraryNotYetLoaded;
    }

    // --------

    TFunc HookStore::GetOriginalFunctionForHook(const THookID hook)
    {
        switch (GetHookStatus(hook))
        {
        case EHookStatus::HookStatusSuccess:
        case EHookStatus::HookStatusLibraryNotYetLoaded:
            return (TFunc)&trampolines[hook];

        default:
            return NULL;
        }
    }

    // --------

    THookID HookStore::IdentifyHook(const THookString dllName, const THookString exportFuncName)
    {
        concurrency::reader_writer_lock::scoped_lock_read(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
            return kInvalidHookID;

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() == functionHookMap)
            return kInvalidHookID;

        return functionHookMap->second;
    }

    // --------

    THookID HookStore::SetHook(const TFunc hookFunc, const THookString dllName, const THookString exportFuncName)
    {
        if (false == trampolines.IsInitialized())
            return kInvalidHookID;

        concurrency::reader_writer_lock::scoped_lock(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
        {
            mapFunctionNameToHookID.emplace_back();
            dllFunctionMap = mapDllNameToFunctionMap.emplace(dllName, mapFunctionNameToHookID.back()).first;
        }

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() != functionHookMap)
            return kInvalidHookID;

        const int newHookID = trampolines.Allocate();

        if (newHookID < 0)
            return kInvalidHookID;

        // TODO: find the address of the desired function and set the trampoline's target.
        dllFunctionMap->second.emplace(exportFuncName, (THookID)newHookID);
        return (THookID)newHookID;
    }
}
