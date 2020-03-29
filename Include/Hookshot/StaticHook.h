/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file StaticHook.h
 *   Convenience wrapper types and definitions for creating static hooks.  A
 *   static hook is one whose original function address is available at
 *   compile or link time.  Examples include API functions declared in header
 *   files and defined in libraries against which a hook module links.
 *   Windows API functions, exported by system-supplied libraries, can
 *   generally be hooked this way.  A key advantage of using static hooks, as
 *   opposed to calling Hookshot functions directly, is type safety: return
 *   type, calling convention, and argument types are automatically detected
 *   from the declaration of the function being hooked, and any accidental
 *   mismatches trigger compiler errors.  This file is intended to be
 *   included externally.
 *****************************************************************************/

#pragma once

#include "Hookshot.h"

#include <string>
#include <type_traits>


// -------- MACROS --------------------------------------------------------- //
// These provide the interface to static hooks.

/// Declares a static hook.  Defines a type to represent a hook for the specified function.  Parameter is the name of the function being hooked.
/// Type name is of the format "StaticHook_[function name]" and is created in whatever namespace encloses the invocation of this macro.
/// Relevant static members of the created type are `Hook` (the hook function, which must be implemented) and `Original` (automatically implemented to provide access to the original un-hooked functionality of the specified function).
/// Function prototypes for both `Hook` and `Original` are automatically set to match that of the specified function, including calling convention.
/// To define the hook function, simply provide a funciton body for `StaticHook_[function name]::Hook`.
/// The invocation of this macro should be placed in a location visible wherever access to the underlying type is needed.  It is safe to place in a header file that is included in multiple places.
/// If Hookshot fails to hook the function as requested by a static hook, the process is terminated with an error message, which includes information on which specific static hook operation failed.
/// Therefore, it is safe to assume that all requested static hooks were reported as being created successfully by Hookshot.
#define HOOKSHOT_DECLARE_STATIC_HOOK(func) \
    static constexpr wchar_t kHookName__##func[] = _CRT_WIDE(#func); \
    using StaticHook_##func = ::Hookshot::StaticHook<kHookName__##func, (void*)(&(func)), decltype(func)>

/// Explicitly instantiates a static hook.  Parameter is the name of the function being hooked.
/// A single invocation in one source file is required to ensure the static hook is set properly and automatically.
/// It is an error to invoke this macro from multiple source files.  For that reason, it should not be invoked in a header file.  Ideally the invocation would be placed near the hook function definition.
#define HOOKSHOT_INSTANTIATE_STATIC_HOOK(func) \
    template class ::Hookshot::StaticHook<kHookName__##func, (void*)(&(func)), decltype(func)>

/// Convenience macro for both declaring and instantiating a static hook.
/// Can be used in a source file when a hook type does not need to be visible anywhere else.
#define HOOKSHOT_DECLARE_AND_INSTANTIATE_STATIC_HOOK(func) \
    HOOKSHOT_DECLARE_STATIC_HOOK(func); \
    HOOKSHOT_INSTANTIATE_STATIC_HOOK(func)


// -------- IMPLEMENTATION DETAILS ----------------------------------------- //
// Everything below this point is internal to the implementation of static hooks.

/// Implements static hook template specialization so that function prototypes and calling conventions are automatically extracted based on the supplied function.
/// Parameters are just different syntactic representations of calling conventions, which are used to create one template specialization for calling convention.
#define HOOKSHOT_STATIC_HOOK_TEMPLATE(callingConvention, callingConventionInBrackets) \
    namespace Hookshot { \
        template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename ReturnType, typename... ArgumentTypes> class StaticHook<kOriginalFunctionName, kOriginalFunctionAddress, ReturnType callingConventionInBrackets (ArgumentTypes...)> \
        { \
        private: \
            static ReturnType(callingConvention * const originalFunction)(ArgumentTypes...); \
        public: \
            static ReturnType callingConvention Hook(ArgumentTypes...); \
            static inline ReturnType callingConvention Original(ArgumentTypes... args) { return originalFunction(args...); } \
        }; \
        template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename ReturnType, typename... ArgumentTypes> ReturnType(callingConvention * const StaticHook<kOriginalFunctionName, kOriginalFunctionAddress, ReturnType callingConventionInBrackets (ArgumentTypes...)>::originalFunction)(ArgumentTypes...) = (CreateHookOrDie(kOriginalFunctionAddress, &Hook, (std::wstring(L"StaticHook failed for function ") + std::wstring(kOriginalFunctionName) + std::wstring(L"."))), (ReturnType(callingConvention *)(ArgumentTypes...))GetOriginalFunction(kOriginalFunctionAddress)); \
    }

namespace Hookshot
{
    /// Primary static hook template.  Specialized using #HOOKSHOT_STATIC_HOOK_TEMPLATE.
    template <const wchar_t* kOriginalFunctionName, void* const kOriginalFunctionAddress, typename T> class StaticHook
    {
        static_assert(std::is_function<T>::value);
    };
}

#ifdef _WIN64

// No need to specify a calling convention in 64-bit mode because only a single x64 calling convention exists.
HOOKSHOT_STATIC_HOOK_TEMPLATE( , );

#else

// One template specialization exists for each calling convention, and the appropriate calling convention that matches the function prototype is selected automatically.
HOOKSHOT_STATIC_HOOK_TEMPLATE(__cdecl, (__cdecl));
HOOKSHOT_STATIC_HOOK_TEMPLATE(__fastcall, (__fastcall));
HOOKSHOT_STATIC_HOOK_TEMPLATE(__stdcall, (__stdcall));
HOOKSHOT_STATIC_HOOK_TEMPLATE(__vectorcall, (__vectorcall));

#endif
