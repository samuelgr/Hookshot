/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file DependencyProtect.cpp
 *   Implementation of dependency protection functionality.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <intrin.h>
#include <unordered_map>


namespace Hookshot
{
    namespace Protected
    {
        // -------- INTERNAL TYPES ----------------------------------------- //

        /// Used to guard against accidentally defining multiple protected dependency pointers with the same initial address.
        /// The macro #PROTECTED_DEPENDENCY declared in this file turns multiple definitions with the same address into compiler errors.
        template <void* address> class MultipleDefinitionGuard
        {
            static bool multipleProtectedDependencyDefinitionGuard;
        };
    }


    // -------- INTERNAL VARIABLES ----------------------------------------- //

    /// Registry of all protected dependencies.
    /// Key is the current known address of the protected dependency.
    /// Value is the address of the protected dependency pointer.
    static std::unordered_map<const void*, const void* volatile*> protectedDependencies;


    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Initializes a protected dependency pointer.
    /// Returns the address passed in after registering the protected dependency in the registry.
    /// @param [in] address Initial address of the protected dependency function.
    /// @param [in] protectedDependencyPointer Address of the protected dependency function pointer.
    static inline const void* InitializeProtectedDependencyAddress(const void* address, const void* volatile* protectedDependencyPointer)
    {
        protectedDependencies[address] = protectedDependencyPointer;
        return address;
    }
}


// -------- MACROS --------------------------------------------------------- //

/// Defines and initializes a type-safe protected dependency pointer.
/// First parameter specifies the namespace path, and second the function name.
#define PROTECTED_DEPENDENCY(qualpath, nspace, func) \
    bool MultipleDefinitionGuard<(void*)&qualpath::func>::multipleProtectedDependencyDefinitionGuard = true; \
    extern const volatile decltype(&qualpath::func) nspace##_##func = (decltype(&qualpath::func))InitializeProtectedDependencyAddress(&qualpath::func, (const void* volatile*)&nspace##_##func)

/// Ensures that the protected dependency function pointers use the above macro so they are defined instead of simply declared.
#define NODECLARE_PROTECTED_DEPENDENCIES


// -------- GLOBALS -------------------------------------------------------- //
// Variables are imported from "DependencyProtect.h" and defined.

#include "DependencyProtect.h"


namespace Hookshot
{
    // -------- FUNCTIONS -------------------------------------------------- //
    // See "DependencyProtect.h" for documentation.

    void UpdateProtectedDependencyAddress(const void* oldAddress, const void* newAddress)
    {
        if (0 != protectedDependencies.count(oldAddress))
        {
            const void* volatile* const pointerToUpdate = protectedDependencies.at(oldAddress);

            protectedDependencies.erase(oldAddress);
            protectedDependencies.insert({newAddress, pointerToUpdate});

            *pointerToUpdate = newAddress;
            _mm_mfence();
        }
    }
}
