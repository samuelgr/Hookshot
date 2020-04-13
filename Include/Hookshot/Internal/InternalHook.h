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
 *   for external use.
 *****************************************************************************/

#pragma once

#include "HookStore.h"

#include <type_traits>


/// Declares an internal hook.  Defines a type to represent a hook for the specified function.  Parameter is the name of the function being hooked.
/// Type name is of the format "InternalHook_[function name]" and is created in whatever namespace encloses the invocation of this macro.
/// See StaticHook interface documentation for more information.
#define HOOKSHOT_INTERNAL_HOOK(func) \
    static constexpr wchar_t kInternalHookName__##func[] = _CRT_WIDE(#func); \
    using InternalHook_##func = ::Hookshot::InternalHook<kInternalHookName__##func, (void*)(&(func)), decltype(func)>

/// Implements internal hook template specialization so that function prototypes and calling conventions are automatically extracted based on the supplied function.
/// Parameters are just different syntactic representations of calling conventions, which are used to create one template specialization for calling convention.
#define HOOKSHOT_INTERNAL_HOOK_TEMPLATE(callingConvention, callingConventionInBrackets) \
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename ReturnType, typename... ArgumentTypes> class InternalHook<kOriginalFunctionName, kOriginalFunctionAddress, ReturnType callingConventionInBrackets (ArgumentTypes...)> : public InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress> \
    { \
    public: \
        static ReturnType callingConvention Hook(ArgumentTypes...); \
        static inline ReturnType callingConvention Original(ArgumentTypes... args) { return ((ReturnType(callingConvention *)(ArgumentTypes...))InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress>::GetOriginalFunction())(args...); } \
        static inline EResult SetHook(void) { return InternalHookBase<kOriginalFunctionName, kOriginalFunctionAddress>::SetHook(&Hook); } \
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
}
