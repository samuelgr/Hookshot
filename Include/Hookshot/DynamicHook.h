/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file DynamicHook.h
 *   Convenience wrapper types and definitions for creating dynamic hooks.  A
 *   dynamic hook is one whose original function signature is available at
 *   compile time but whose actual address is not known until runtime.
 *   Examples include functions exported by DLLs loaded dynamically using
 *   LoadLibrary and whose addresses are therefore obtained using
 *   GetProcAddress.  Dynamic hooks require the original function address to
 *   be specified at runtime.  Nevertheless, a key advantage of using dynamic
 *   hooks, as opposed to calling Hookshot functions directly, is type safety:
 *   return type, calling convention, and argument types are extracted from
 *   the provided function prototype, and any accidental mismatches trigger
 *   compiler errors.  This file is intended to be included externally.
 *****************************************************************************/

#pragma once

#include "Hookshot.h"

#include <type_traits>


// -------- MACROS --------------------------------------------------------- //
// These provide the interface to dynamic hooks.

/// Declares a dynamic hook, given a function prototype that includes a function name.  Defines a type to represent a hook for the specified function.  Parameter is the name of the function being hooked.
/// Type name is of the format "DynamicHook_[function name]" and is created in whatever namespace encloses the invocation of this macro.
/// Relevant static members of the created type are `Hook` (the hook function, which must be implemented) and `Original` (automatically implemented to provide access to the original un-hooked functionality of the specified function).
/// To activate the dynamic hook once the original function address is known, the `SetHook` method must be invoked successfully with the original function address supplied as a parameter.
/// Function prototypes for both `Hook` and `Original` are automatically set to match that of the specified function, including calling convention.
/// To define the hook function, simply provide a funciton body for `DynamicHook_[function name]::Hook`.
/// The invocation of this macro should be placed in a location visible wherever access to the underlying type is needed.  It is safe to place in a header file that is included in multiple places.
/// Note that Hookshot might fail to create the requested hook.  Therefore, the return code from `SetHook` should be checked.
/// Once `SetHook` has been invoked successfully, further invocations have no effect and simply return #EResult::NoEffect.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION(func) \
    static constexpr wchar_t kHookName__##func[] = _CRT_WIDE(#func); \
    using DynamicHook_##func = ::Hookshot::DynamicHook<kHookName__##func, decltype(func)>

/// Equivalent to #HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION, but accepts a typed function pointer instead of a function prototype declaration.
/// A function name to associate with the dynamic hook must also be supplied.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_POINTER(name, ptr) \
    static constexpr wchar_t kHookName__##name[] = _CRT_WIDE(#name); \
    using DynamicHook_##name = ::Hookshot::DynamicHook<kHookName__##name, std::remove_pointer<decltype(ptr)>::type>;

/// Equivalent to #HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION, but accepts a manually-specified function type instead of a function prototype declaration.
/// The `typespec` parameter is syntactically the same as a type definition for a function pointer type, with or without the asterisk.
/// A function name to associate with the dynamic hook must also be supplied.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_TYPESPEC(name, typespec) \
    static constexpr wchar_t kHookName__##name[] = _CRT_WIDE(#name); \
    using DynamicHook_##name = ::Hookshot::DynamicHook<kHookName__##name, std::remove_pointer<typespec>::type>;


// -------- IMPLEMENTATION DETAILS ----------------------------------------- //
// Everything below this point is internal to the implementation of dynamic hooks.

/// Implements dynamic hook template specialization so that function prototypes and calling conventions are automatically extracted based on the supplied function.
/// Parameters are just different syntactic representations of calling conventions, which are used to create one template specialization for calling convention.
#define HOOKSHOT_DYNAMIC_HOOK_TEMPLATE(callingConvention, callingConventionInBrackets) \
    namespace Hookshot { \
        template <const wchar_t* kOriginalFunctionName, typename ReturnType, typename... ArgumentTypes> class DynamicHook<kOriginalFunctionName, ReturnType callingConventionInBrackets (ArgumentTypes...)> : public DynamicHookBase<kOriginalFunctionName> \
        { \
        public: \
            static ReturnType callingConvention Hook(ArgumentTypes...); \
            static inline ReturnType callingConvention Original(ArgumentTypes... args) { return ((ReturnType(callingConvention *)(ArgumentTypes...))DynamicHookBase<kOriginalFunctionName>::GetOriginalFunctionAddress())(args...); } \
            static inline EResult SetHook(IHookshot* const hookshot, ReturnType(callingConvention * originalFunc)(ArgumentTypes...)) { return DynamicHookBase<kOriginalFunctionName>::SetHook(hookshot, originalFunc, &Hook); } \
        }; \
    }

namespace Hookshot
{
    /// Base class for all dynamic hooks.
    /// Used to hide implementation details from external users.
    template <const wchar_t* kOriginalFunctionName> class DynamicHookBase
    {
    private:
        static const void* originalFunction;

    protected:
        static inline const void* GetOriginalFunctionAddress(void)
        {
            return originalFunction;
        }

        static inline EResult SetHook(IHookshot* const hookshot, void* originalFunc, const void* hookFunc)
        {
            if (nullptr != originalFunction)
                return EResult::NoEffect;

            const EResult result = hookshot->CreateHook(originalFunc, hookFunc);

            if (SuccessfulResult(result))
                originalFunction = hookshot->GetOriginalFunction(originalFunc);

            return result;
        }
    };
    template <const wchar_t* kOriginalFunctionName> const void* DynamicHookBase<kOriginalFunctionName>::originalFunction = nullptr;
    
    /// Primary dynamic hook template.  Specialized using #HOOKSHOT_DYNAMIC_HOOK_TEMPLATE.
    template <const wchar_t* kOriginalFunctionName, typename T> class DynamicHook
    {
        static_assert(std::is_function<T>::value);
    };
}

#ifdef _WIN64
HOOKSHOT_DYNAMIC_HOOK_TEMPLATE( , );
#else
HOOKSHOT_DYNAMIC_HOOK_TEMPLATE(__cdecl, (__cdecl));
HOOKSHOT_DYNAMIC_HOOK_TEMPLATE(__fastcall, (__fastcall));
HOOKSHOT_DYNAMIC_HOOK_TEMPLATE(__stdcall, (__stdcall));
HOOKSHOT_DYNAMIC_HOOK_TEMPLATE(__vectorcall, (__vectorcall));
#endif
