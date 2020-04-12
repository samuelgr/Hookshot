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
#include "HookStore.h"

#include <string_view>


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
        static HookStore hookStore;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        LibraryInterface(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //

        /// Determines if the configuration file exists, irrespective of whether or not it is correctly formed.
        /// @return `true` if the configuration file exists, `false` if not.
        static inline bool DoesConfigurationFileExist(void)
        {
            return (Configuration::EFileReadResult::FileNotFound != configuration.GetFileReadResult());
        }

        /// Enables the log, if it is configured in the configuration file.
        static void EnableLogIfConfigured(void);
        
        /// Retrieves the Hookshot configuration data object.
        /// Only useful if #IsConfigurationDataValid returns `true`.
        static inline const Configuration::ConfigurationData& GetConfigurationData(void)
        {
            return configuration.GetData();
        }

        /// Retrieves a string containing a message that describes the error encountered while attempting to read the Hookshot configuration file.
        /// Only useful if #IsConfigurationDataValid returns `false`.
        /// @return String containing the configuration file read error message.
        static inline std::wstring_view GetConfigurationErrorMessage(void)
        {
            return configuration.GetReadErrorMessage();
        }

        /// Retrieves the Hookshot interface object pointer that can be passed to external clients.
        /// @return Hook interface object pointer.
        static inline IHookshot* GetHookshotInterfacePointer(void)
        {
            return &hookStore;
        }

        /// Performs common top-level initialization operations. Idempotent.
        /// Any initialization steps that must happen irrespective of how this library was loaded should go here.
        static void Initialize(void);

        /// Determines if the configuration data object contains valid data (i.e. the configuration file was read and parsed successfully).
        /// @return `true` if it contains valid data, `false` if not.
        static inline bool IsConfigurationDataValid(void)
        {
            return (Configuration::EFileReadResult::Success == configuration.GetFileReadResult());
        }

        /// Attempts to load and initialize the hook modules named in the Hookshot configuration file.
        /// @return Number of hook modules successfully loaded.
        static int LoadConfiguredHookModules(void);

        /// Attempts to load and initialize the inject-only libraries named in the Hookshot configuration file.
        /// @return Number of inject-only libraries successfully loaded.
        static int LoadConfiguredInjectOnlyLibraries(void);

        /// Attempts to load and initialize the named hook module.
        /// Useful if hooks to be set are contained in an external hook module.
        /// @param [in] hookModuleFileName File name of the hook module to load and initialize.
        /// @return `true` on success, `false` on failure.
        static bool LoadHookModule(std::wstring_view hookModuleFileName);

        /// Attempts to load the specified library, which is not to be treated as a hook module.
        /// @param [in] injectOnlyLibraryFileName File name of library to load.
        /// @return `true` on success, `false` on failure.
        static bool LoadInjectOnlyLibrary(std::wstring_view injectOnlyLibraryFileName);
    };
}
