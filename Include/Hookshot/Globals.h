/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Globals.h
 *   Declaration of a namespace for storing and retrieving global data.
 *   Intended for miscellaneous data elements with no other suitable place.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"


namespace Hookshot
{
    /// Encapsulates all miscellaneous global data elements with no other suitable location.
    /// Not intended to be instantiated.
    class Globals
    {
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Globals(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //
        
        /// Retrieves the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// @return Instance handle for the loaded module.
        static HINSTANCE GetInstanceHandle(void);
    };
}
