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
        concurrency::reader_writer_lock::scoped_lock_read(this->lock);

        return EHookStatus::HookStatusFailure;
    }

    // --------

    TFunc HookStore::GetOriginalFunctionForHook(const THookID hook)
    {
        concurrency::reader_writer_lock::scoped_lock_read(this->lock);

        return NULL;
    }

    // --------

    THookID HookStore::IdentifyHook(const THookString dllName, const THookString exportFuncName)
    {
        concurrency::reader_writer_lock::scoped_lock_read(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
            return -1;

        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() == functionHookMap)
            return -2;

        return functionHookMap->second;
    }

    // --------

    THookID HookStore::SetHook(const TFunc hookFunc, const THookString dllName, const THookString exportFuncName)
    {
        concurrency::reader_writer_lock::scoped_lock(this->lock);

        TDllMap::iterator dllFunctionMap = mapDllNameToFunctionMap.find(dllName);

        if (mapDllNameToFunctionMap.end() == dllFunctionMap)
        {
            mapFunctionNameToHookID.emplace_back();
            dllFunctionMap = mapDllNameToFunctionMap.emplace(dllName, mapFunctionNameToHookID.back()).first;
        }
        
        TFunctionMap::iterator functionHookMap = dllFunctionMap->second.find(exportFuncName);

        if (dllFunctionMap->second.end() != functionHookMap)
            return -1;

        dllFunctionMap->second.emplace(exportFuncName, 0);
        return 0;
    }
}
