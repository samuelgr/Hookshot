/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file InternalHook.cpp
 *   Implementation of internal hook registration and creation functionality.
 *****************************************************************************/

#include "ApiWindows.h"
#include "InternalHook.h"
#include "Message.h"

#include <map>
#include <string_view>


namespace Hookshot
{
    // -------- INTERNAL VARIABLES ----------------------------------------- //

    /// Flag used to indicate if internal hooks have been specified.
    static bool areInternalHooksSet = false;

    /// Registry of all hooks that need to be set.
    static std::map<std::wstring_view, EResult(*)(void)> hookRegistry;


    // -------- FUNCTIONS -------------------------------------------------- //
    // See "InternalHook.h" for documentation.
    
    bool RegisterInternalHook(std::wstring_view hookName, EResult(*setHookFunc)(void))
    {
        if (true == areInternalHooksSet)
            return false;

        hookRegistry[hookName] = setHookFunc;

        return true;
    }

    // --------

    void SetAllInternalHooks(void)
    {
        if (true == areInternalHooksSet)
            return;

        for (auto hook = hookRegistry.cbegin(); hook != hookRegistry.cend(); ++hook)
        {
            const EResult result = hook->second();

            if (SuccessfulResult(result))
                Message::OutputFormatted(Message::ESeverity::Info, L"Successfully set internal hook for %s.", hook->first.data());
            else
                Message::OutputFormatted(Message::ESeverity::Warning, L"Failed to set internal hook for %s.  Hookshot features that use this hook will not work.", hook->first.data());
        }

        areInternalHooksSet = true;
        hookRegistry.clear();
    }
}
