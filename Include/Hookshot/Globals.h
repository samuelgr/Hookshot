/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
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
    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Handle of the instance that represents the running form of Xidi, be it the library or the test application.
        static HINSTANCE gInstanceHandle;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Globals(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Retrieves the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// @return Instance handle for Xidi.
        static HINSTANCE GetInstanceHandle(void);

        /// Sets the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// Intended to be called only once during initialization.
        /// @param [in] newInstanceHandle Instance handle to set.
        static void SetInstanceHandle(HINSTANCE newInstanceHandle);
    };
}
