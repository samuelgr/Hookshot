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
#include "HookStore.h"
#include "UnicodeTypes.h"

#include <tchar.h>
#include <hookshot.h>


namespace Hookshot
{
    /// Encapsulates all supporting functionality for Hookshot's library interface.
    /// The point of this class is to offer consistent interaction with outisde code irrespective of how the Hookshot library was loaded (i.e. via injection or via linking and loading).
    /// All methods are class methods.
    class LibraryInterface
    {
    private:
        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Configuration object.
        /// Holds the settings that were read from the Hookshot configuration file.
        static Configuration::Configuration configuration;
        
        /// Single hook configuration interface object.
        /// It can be passed safely by reference to an arbitrary number of hook modules and other Hookshot clients during initialization.
        static HookStore hookConfig;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        LibraryInterface(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Retrieves the Hookshot configuration data object.
        /// Only useful if #IsConfigurationDataValid returns `true`.
        static inline const Configuration::ConfigurationData& GetConfigurationData(void)
        {
            return configuration.GetData();
        }

        /// Retrieves a string containing a message that describes the error encountered while attempting to read the Hookshot configuration file.
        /// Only useful if #IsConfigurationDataValid returns `false`.
        /// @return String containing the configuration file read error message.
        static inline TStdStringView GetConfigurationErrorMessage(void)
        {
            return configuration.GetReadErrorMessage();
        }

        /// Retrieves the hook configuration interface object that can be passed to Hookshot clients during initialization.
        /// @return Pointer to the hook configuration interface object.
        static inline IHookConfig* GetHookConfigInterface(void)
        {
            return &hookConfig;
        }

        /// Performs common top-level initialization operations. Idempotent.
        /// Any initialization steps that must happen irrespective of how this library was loaded should go here.
        static void Initialize(void);

        /// Determines if the configuration data object contains valid data (i.e. the configuration file was read and parsed successfully).
        /// @return `true` if it contains valid data, `false` if not.
        static inline bool IsConfigurationDataValid(void)
        {
            return configuration.IsDataValid();
        }
        
        /// Attempts to load and initialize the named hook module.
        /// Useful if hooks to be set are contained in an external hook module.
        /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
        /// @return `true` on success, `false` on failure.
        static bool LoadHookModule(TStdStringView hookModuleFileName);
    };
}
