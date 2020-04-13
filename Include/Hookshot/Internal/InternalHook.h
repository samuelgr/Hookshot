/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file InternalHook.h
 *   Convenience wrapper types and definitions for creating hooks for
 *   Hookshot's own use.  Similar to the StaticHook interface made available
 *   for external use.  Additionally supports automatic registration so that
 *   a single call is sufficient to attempt to set all internal hooks.
 *****************************************************************************/

#pragma once

#include "HookStore.h"

#include <string_view>
#include <type_traits>


/// Declares an internal hook.  Defines a type to represent a hook for the specified function.  Parameter is the name of the function being hooked.
/// Type name is of the format "InternalHook_[function name]" and is created in whatever namespace encloses the invocation of this macro.
/// See StaticHook interface documentation for more information.
#define HOOKSHOT_INTERNAL_HOOK(func) \
    static constexpr wchar_t kInternalHookName__##func[] = _CRT_WIDE(#func); \
    static const bool kInternalHookIsRegistered__##func = ::Hookshot::RegisterInternalHook(kInternalHookName__##func, &::Hookshot::InternalHook<kInternalHookName__##func, (void*)(&(func)), decltype(func)>::SetHook); \
    using InternalHook_##func = ::Hookshot::InternalHook<kInternalHookName__##func, (void*)(&(func)), decltype(func)>

/// Implements internal hook template specialization so that function prototypes and calling conventions are automatically extracted based on the supplied function.
/// Parameters are just different syntactic representations of calling conventions, which are used to create one template specialization for calling convention.
#define HOOKSHOT_INTERNAL_HOOK_TEMPLATE(callingConvention, callingConventionInBrackets) \
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename ReturnType, typename... ArgumentTypes> class InternalHook<kOriginalFunctionName, kOriginalFunctionAddress, ReturnType callingConventionInBrackets (ArgumentTypes...)> : public InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress> \
    { \
    public: \
        static ReturnType callingConvention Hook(ArgumentTypes...); \
        static ReturnType callingConvention Original(ArgumentTypes... args) { return ((ReturnType(callingConvention *)(ArgumentTypes...))InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress>::GetOriginalFunction())(args...); } \
        static EResult SetHook(void) { return InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress>::SetHook(&Hook); } \
    }; \

namespace Hookshot
{
    /// Base class for all internal hooks.
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress> class InternalHookBase
    {
    private:
        static const void* originalFunction;

    protected:
        static inline const void* GetOriginalFunction(void)
        {
            return originalFunction;
        }

        static inline EResult SetHook(const void* hookFunc)
        {
            return HookStore::CreateHookInternal(kOriginalFunctionAddress, hookFunc, true, &originalFunction);
        }
    };
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress> const void* InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress>::originalFunction = nullptr;
    
    /// Primary template.  Specialized using #HOOKSHOT_INTERNAL_HOOK_TEMPLATE.
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename T> class InternalHook
    {
        static_assert(std::is_function<T>::value, "Supplied argument in InternalHook declaration must map to a function type.");
    };

#ifdef _WIN64
    HOOKSHOT_INTERNAL_HOOK_TEMPLATE( , );
#else
    HOOKSHOT_INTERNAL_HOOK_TEMPLATE(__cdecl, (__cdecl));
    HOOKSHOT_INTERNAL_HOOK_TEMPLATE(__fastcall, (__fastcall));
    HOOKSHOT_INTERNAL_HOOK_TEMPLATE(__stdcall, (__stdcall));
    HOOKSHOT_INTERNAL_HOOK_TEMPLATE(__vectorcall, (__vectorcall));
#endif

    /// Registers an internal hook so that it is automatically set when #SetAllInternalHooks is called.
    /// Intended to be invoked automatically by the #HOOKSHOT_INTERNAL_HOOK macro, and will fail once #SetAllInternalHooks has been called.
    /// Not concurrency-safe.
    /// @param [in] funcName Name associated with the hook to be set.
    /// @param [in] setHookFunc Address of the internal hook's `SetHook` class method.
    /// @return `true` after registration is complete.
    bool RegisterInternalHook(std::wstring_view hookName, EResult(*setHookFunc)(void));

    /// Sets all internal hooks that have been registered.
    /// Can only be called once.  Subsequent calls have no effect.
    /// Not concurrency-safe.
    void SetAllInternalHooks(void);
}
