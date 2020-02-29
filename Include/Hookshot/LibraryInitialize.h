/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file LibraryInitialize.h
 *   Declaration of library initialization functions.
 *****************************************************************************/

#pragma once

#include "HookStore.h"

#include <tchar.h>
#include <hookshot.h>


namespace Hookshot
{
    /// Encapsulates all library initialization functionality.
    /// The point of this class is to offer proper library initialization irrespective of how it was loaded (i.e. via injection or via linking and loading).
    /// All methods are class methods.
    class LibraryInitialize
    {
    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Single hook configuration interface object.
        /// It can be passed safely by reference to an arbitrary number of hook modules and other Hookshot clients during initialization.
        static HookStore hookConfig;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        LibraryInitialize(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Performs common top-level initialization operations. Idempotent.
        /// Any initialization steps that must happen irrespective of how this library was loaded should go here.
        static void CommonInitialization(void);
        
        /// Retrieves the hook configuration interface object that can be passed to Hookshot clients during initialization.
        /// @return Pointer to the hook configuration interface object.
        static inline IHookConfig* GetHookConfigInterface(void)
        {
            return &hookConfig;
        }
        
        /// Attempts to load and initialize the named hook module.
        /// Useful if hooks to be set are contained in an external hook module.
        /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
        /// @return `true` on success, `false` on failure.
        static bool LoadHookModule(const TCHAR* hookModuleFileName);
    };
}
