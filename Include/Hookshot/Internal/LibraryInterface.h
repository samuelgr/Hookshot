/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file LibraryInterface.h
 *   Declaration of support functionality for Hookshot's library interface.
 *****************************************************************************/

#pragma once

#include "Configuration.h"
#include "HookshotTypes.h"

#include <string_view>


namespace Hookshot
{
    /// Encapsulates all supporting functionality for Hookshot's library interface.
    /// The point of these functions is to offer consistent interaction with outisde code irrespective of how the Hookshot library was loaded (i.e. via injection or via linking and loading).
    namespace LibraryInterface
    {
        // -------- FUNCTIONS ---------------------------------------------- //

        /// Determines if the configuration file exists, irrespective of whether or not it is correctly formed.
        /// @return `true` if the configuration file exists, `false` if not.
        bool DoesConfigurationFileExist(void);

        /// Enables the log, if it is configured in the configuration file.
        void EnableLogIfConfigured(void);
        
        /// Retrieves the Hookshot configuration data object.
        /// Only useful if #IsConfigurationDataValid returns `true`.
        const Configuration::ConfigurationData& GetConfigurationData(void);

        /// Retrieves a string containing a message that describes the error encountered while attempting to read the Hookshot configuration file.
        /// Only useful if #IsConfigurationDataValid returns `false`.
        /// @return String containing the configuration file read error message.
        std::wstring_view GetConfigurationErrorMessage(void);

        /// Retrieves the Hookshot interface object pointer that can be passed to external clients.
        /// @return Hook interface object pointer.
        IHookshot* GetHookshotInterfacePointer(void);

        /// Performs common top-level initialization operations. Idempotent.
        /// Any initialization steps that must happen irrespective of how this library was loaded should go here.
        /// Will fail if the initialization attempt is inappropriate, duplicate, and so on.
        /// @param [in] loadMethod Hookshot library load method.
        /// @return `true` if successful, `false` otherwise.
        bool Initialize(const ELoadMethod loadMethod);

        /// Determines if the configuration data object contains valid data (i.e. the configuration file was read and parsed successfully).
        /// @return `true` if it contains valid data, `false` if not.
        bool IsConfigurationDataValid(void);

        /// Attempts to load and initialize all applicable hook modules.
        /// @return Number of hook modules successfully loaded.
        int LoadHookModules(void);

        /// Attempts to load and initialize all applicable inject-only libraries.
        /// @return Number of inject-only libraries successfully loaded.
        int LoadInjectOnlyLibraries(void);
    }
}
