/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
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
    enum class ELoadMethod
    {
        Executed,                                                           ///< Executed directly. This is the default value and is applicable for the executable form of Hookshot.
        Injected,                                                           ///< Injected. An executable form of Hookshot injected this form of Hookshot into the current process.
        LibraryLoaded,                                                      ///< Loaded as a library. Some executable loaded Hookshot using a standard dynamic library loading technique.
    };

    namespace Globals
    {
        // -------- FUNCTIONS ---------------------------------------------- //

        /// Retrieves a pseudohandle to the current process.
        /// @return Current process pseudohandle.
        HANDLE GetCurrentProcessHandle(void);

        /// Retrieves the PID of the current process.
        /// @return Current process PID.
        DWORD GetCurrentProcessId(void);

        /// Retrieves the method by which this form of Hookshot was loaded.
        /// @return Method by which Hookshot was loaded.
        ELoadMethod GetHookshotLoadMethod(void);

        /// Retrieves a string representation of the method by which this form of Hookshot was loaded.
        /// @return String representation of the method by which Hookshot was loaded.
        std::wstring_view GetHookshotLoadMethodString(void);

        /// Retrieves the handle of the instance that represents the current running form of Hookshot, be it the library or the bootstrap executable.
        /// @return Instance handle for the loaded module.
        HINSTANCE GetInstanceHandle(void);

        /// Retrieves information on the current system. This includes architecture, page size, and so on.
        /// @return Reference to a read-only structure containing system information.
        const SYSTEM_INFO& GetSystemInformation(void);

        /// Sets the method by which this form of Hookshot was loaded.
        /// @param [in] loadMethod Method by which Hookshot was loadedl
        void SetHookshotLoadMethod(const ELoadMethod loadMethod);
    };
}
