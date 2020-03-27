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

#include <string_view>


namespace Hookshot
{
    /// Enumerates the possible ways Hookshot can be loaded.
    enum class EHookshotLoadMethod
    {
        Executed,                                                           ///< Executed directly.  This is the default value and is applicable for the executable form of Hookshot.
        Injected,                                                           ///< Injected.  An executable form of Hookshot injected this form of Hookshot into the current process.
        LibraryLoaded,                                                      ///< Loaded as a library.  Some executable loaded Hookshot using a standard dynamic library loading technique.
    };

    /// Encapsulates all miscellaneous global data elements with no other suitable location.
    /// Not intended to be instantiated.
    class Globals
    {
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Globals(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Retrieves a pseudohandle to the current process.
        /// @return Current process pseudohandle.
        static HANDLE GetCurrentProcessHandle(void);

        /// Retrieves the PID of the current process.
        /// @return Current process PID.
        static DWORD GetCurrentProcessId(void);

        /// Retrieves the method by which this form of Hookshot was loaded.
        /// @return Method by which Hookshot was loaded.
        static EHookshotLoadMethod GetHookshotLoadMethod(void);

        /// Retrieves a string representation of the method by which this form of Hookshot was loaded.
        /// @return String representation of the method by which Hookshot was loaded.
        static std::wstring_view GetHookshotLoadMethodString(void);

        /// Retrieves the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// @return Instance handle for the loaded module.
        static HINSTANCE GetInstanceHandle(void);

        /// Sets the method by which this form of Hookshot was loaded.
        /// @param [in] loadMethod Method by which Hookshot was loadedl
        static void SetHookshotLoadMethod(const EHookshotLoadMethod loadMethod);
    };
}
