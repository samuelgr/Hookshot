/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
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
    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Holds all registered internal hooks, and offers the ability to set them.
    /// Used to ensure internal hooks are available during dynamic initialization.
    /// Implemented as a singleton object.
    class InternalHookRegistry
    {
    public:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Flag used to indicate if internal hooks have been specified.
        bool areInternalHooksSet;

        /// Registry of all hooks that need to be set.
        std::map<std::wstring_view, EResult(*)(void)> internalHooks;


    private:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Objects cannot be constructed externally.
        InternalHookRegistry(void) : areInternalHooksSet(false), internalHooks()
        {
            // Nothing to do here.
        }

        /// Copy constructor. Should never be invoked.
        InternalHookRegistry(const InternalHookRegistry& other) = delete;


    public:
        // -------- CLASS METHODS ------------------------------------------ //

        /// Returns a reference to the singleton instance of this class.
        /// @return Reference to the singleton instance.
        static InternalHookRegistry& GetInstance(void)
        {
            static InternalHookRegistry internalHookRegistry;
            return internalHookRegistry;
        }
    };


    // -------- FUNCTIONS -------------------------------------------------- //
    // See "InternalHook.h" for documentation.

    bool RegisterInternalHook(std::wstring_view hookName, EResult(*setHookFunc)(void))
    {
        InternalHookRegistry& registry = InternalHookRegistry::GetInstance();

        if (true == registry.areInternalHooksSet)
            return false;

        registry.internalHooks[hookName] = setHookFunc;

        return true;
    }

    // --------

    void SetAllInternalHooks(void)
    {
        InternalHookRegistry& registry = InternalHookRegistry::GetInstance();

        if (true == registry.areInternalHooksSet)
            return;

        for (auto hook = registry.internalHooks.cbegin(); hook != registry.internalHooks.cend(); ++hook)
        {
            const EResult result = hook->second();

            if (SuccessfulResult(result))
                Message::OutputFormatted(Message::ESeverity::Info, L"Successfully set internal hook for %s.", hook->first.data());
            else
                Message::OutputFormatted(Message::ESeverity::Warning, L"Failed to set internal hook for %s. Hookshot features that use this hook will not work.", hook->first.data());
        }

        registry.areInternalHooksSet = true;
        registry.internalHooks.clear();
    }
}
