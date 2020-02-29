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

        /// Handle of the instance that represents the running form of Hookshot.
        static HINSTANCE gInstanceHandle;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Globals(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Fills the specified buffer with the fully-qualified path of the current running form of Hookshot, minus the extension.
        /// This is useful for determining the correct path of the next module to load.
        /// Internally uses `GetModuleFilename()` so check MSDN for documentation on error values, which are simply passed unchanged back.
        /// @param [in,out] buf Buffer to be filled.
        /// @param [in] numchars Size of the buffer, measured in characters.
        /// @return Number of characters written to the buffer (not including the terminal `NULL` character, which is always written), or 0 in the event of an error.
        static size_t FillHookshotModuleBasePath(TCHAR* const buf, const size_t numchars);
        
        /// Retrieves the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// @return Instance handle for the loaded module.
        static inline HINSTANCE GetInstanceHandle(void)
        {
            return gInstanceHandle;
        }

        /// Sets the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// Intended to be called only once during initialization.
        /// @param [in] newInstanceHandle Instance handle to set.
        static inline void SetInstanceHandle(HINSTANCE newInstanceHandle)
        {
            gInstanceHandle = newInstanceHandle;
        }
    };
}
