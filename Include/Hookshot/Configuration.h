/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Configuration.h
 *   Public interface for interacting with configuraiton files and data.
 *   Intended to be included by external users.
 *****************************************************************************/

#pragma once

#include "UnicodeTypes.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <tchar.h>


namespace Configuration
{
    // -------- TYPE DEFINITIONS ------------------------------------------- //

    /// Enumerates all supported actions for configuration sections.
    /// Used when checking with a subclass for guidance on what to do when a particular named section is encountered.
    enum class ESectionAction
    {
        Error,                                                          ///< Section name is not supported.
        Read,                                                           ///< Section name is supported and interesting, so the section will be read.
        Skip,                                                           ///< Section name is supported but uninteresting, so the whole section should be skipped.
    };

    /// Enumerates all supported types for configuration values.
    /// Used when checking with a subclass for guidance on whether a section/name pair is supported and, if so, how to parse the value.
    enum class EValueType
    {
        Error,                                                          ///< Combination of section and name pair is not supported.
        Integer,                                                        ///< Combination of section and name pair is supported; value is an integer.
        Boolean,                                                        ///< Combination of section and name pair is supported; value is a Boolean.
        String,                                                         ///< Combination of section and name pair is supported; value is a string.
    };

    /// Underlying type used for storing integer-typed values.
    typedef int64_t TIntegerValue;

    /// Underlying type used for storing Boolean-valued types.
    typedef bool TBooleanValue;

    /// Underlying type used for storing string-valued types.
    typedef TStdString TStringValue;
    
    /// Fourth-level object used to represent a single configuration value for a particular configuration setting.
    class Value
    {
    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// View type used for retrieving and returning integer-typed values.
        typedef TIntegerValue TIntegerView;

        /// View type used for retrieving and returning integer-typed values.
        typedef TBooleanValue TBooleanView;

        /// View type used for retrieving and returning string-typed values.
        typedef TStdStringView TStringView;


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Indicates the value type.
        EValueType type;

        /// Holds the value itself.
        union
        {
            TIntegerValue intValue;
            TBooleanValue boolValue;
            TStringValue stringValue;
        };


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Initialization constructor. Creates an integer-typed value.
        inline Value(const TIntegerValue& value) : type(EValueType::Integer), intValue(value)
        {
            // Nothing to do here.
        }

        /// Initialization constructor. Creates a Boolean-typed value.
        inline Value(const TBooleanValue& value) : type(EValueType::Boolean), boolValue(value)
        {
            // Nothing to do here.
        }

        /// Initialization constructor. Creates a string-typed value.
        inline Value(const TStringValue& value) : type(EValueType::String), stringValue(value)
        {
            // Nothing to do here.
        }

        /// Default destructor.
        inline ~Value(void)
        {
            switch (type)
            {
            case EValueType::Integer:
                intValue.~TIntegerValue();
                break;

            case EValueType::Boolean:
                boolValue.~TBooleanValue();
                break;

            case EValueType::String:
                stringValue.~TStringValue();
                break;

            default:
                break;
            }
        }

        /// Copy constructor.
        inline Value(const Value& other) : type(other.type)
        {
            switch (other.type)
            {
            case EValueType::Integer:
                new (&intValue) TIntegerValue(other.intValue);
                break;

            case EValueType::Boolean:
                new (&boolValue) TBooleanValue(other.boolValue);
                break;

            case EValueType::String:
                new (&stringValue) TStringValue(other.stringValue);
                break;

            default:
                break;
            }
        }

        /// Move constructor.
        inline Value(Value&& other) : type(std::move(other.type))
        {
            switch (other.type)
            {
            case EValueType::Integer:
                new (&intValue) TIntegerValue(std::move(other.intValue));
                break;

            case EValueType::Boolean:
                new (&boolValue) TBooleanValue(std::move(other.boolValue));
                break;

            case EValueType::String:
                new (&stringValue) TStringValue(std::move(other.stringValue));
                break;

            default:
                break;
            }
        }


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Retrieves and returns the type of the stored value.
        /// @return Type of the stored value.
        inline EValueType GetType(void) const
        {
            return type;
        }

        /// Retrieves and returns an immutable reference to the stored value as an integer.
        /// Does not ensure the type of value is actually integer.
        /// @return Stored value.
        inline const TIntegerView GetIntegerValue(void) const
        {
            return intValue;
        }

        /// Retrieves and returns an immutable reference to the stored value as a Boolean.
        /// Does not ensure the type of value is actually Boolean.
        /// @return Stored value.
        inline const TBooleanView GetBooleanValue(void) const
        {
            return boolValue;
        }

        /// Retrieves and returns an immutable reference to the stored value as a string.
        /// Does not ensure the type of value is actually string.
        /// @return Stored value.
        inline const TStringView GetStringValue(void) const
        {
            return stringValue;
        }
        
    };

    /// Third-level object used to represent a single configuration setting within one section of a configuration file.
    class Name
    {
    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Alias for the underlying data structure used to store per-setting configuration values.
        typedef std::deque<Value> TValues;


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds all values for each configuration setting, one element per value.
        TValues values;


    public:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Stores a new value for the configuration setting represented by this object.
        /// @tparam ValueType Type of value to insert.
        /// @param [in] value Value to insert.
        template <typename ValueType> void Insert(const ValueType& value)
        {
            values.emplace_back(value);
        }
        
        /// Allows read-only access to individual values by index, with bounds-checking.
        /// @param [in] index Index of the value to retrieve.
        /// @return Reference to the desired value.
        inline const Value& Value(const int index) const
        {
            return values.at(index);
        }

        /// Retrieves the number of values present for the configuration setting represented by this object.
        /// @return Number of values present.
        inline size_t ValueCount(void) const
        {
            return values.size();
        }

        /// Allows read-only access to all values.
        /// Useful for iterating.
        /// @return Container of all values.
        inline const TValues& Values(void) const
        {
            return values;
        }
    };
    
    /// Second-level object used to represent an entire section of a configuration file.
    class Section
    {
    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Alias for the underlying data structure used to store per-section configuration settings.
        typedef std::map<TStdString, Name, std::less<>> TNames;


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds configuration data within each section, one element per configuration setting.
        TNames names;


    public:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Stores a new value for the specified configuration setting in the section represented by this object.
        /// @tparam ValueType Type of value to insert.
        /// @param [in] name Name of the configuration setting into which to insert the value.
        /// @param [in] value Value to insert.
        template <typename ValueType> void Insert(TStdStringView name, const ValueType& value)
        {
            auto nameIterator = names.find(name);

            if (names.end() == nameIterator)
            {
                names.emplace(name, ::Configuration::Name());
                nameIterator = names.find(name);
            }

            nameIterator->second.Insert(value);
        }

        /// Allows read-only access to individual configuration settings by name, with bounds checking.
        /// @param [in] name Name of the configuration setting to retrieve.
        /// @return Reference to the desired configuration setting.
        inline const Name& Name(TStdStringView name) const
        {
            return names.find(name)->second;
        }
        
        /// Retrieves the number of configuration settings present for the section represented by this object.
        /// @return Number of configuration settings present.
        inline size_t NameCount(void) const
        {
            return names.size();
        }
        
        /// Determines if a configuration setting of the specified name exists in the section represented by this object.
        /// @param [in] name Name of the configuration setting to check.
        /// @return `true` if the setting exists, `false` otherwise.
        inline bool NameExists(TStdStringView name) const
        {
            return (0 != names.count(name));
        }

        /// Allows read-only access to all configuration settings.
        /// Useful for iterating.
        /// @return Container of all configuration settings.
        inline const TNames& Names(void) const
        {
            return names;
        }
    };

    /// Top-level object used to represent all configuration data read from a configuration file.
    class Configuration
    {
    private:
        // -------- TYPE DEFINITIONS --------------------------------------- //

        /// Alias for the underlying data structure used to store top-level configuration section data.
        typedef std::map<TStdString, Section, std::less<>> TSections;


        /// Holds an individual section and name pair.
        /// Used when responding to queries for all settings of a given name across all sections.
        struct SSectionNamePair
        {
            TStdStringView section;                                     ///< Name of the section that holds the identified configuration setting.
            const Name& name;                                           ///< Reference to the object that holds all values for the identified configuration setting.

            /// Initialization constructor. Initializes both references.
            inline SSectionNamePair(TStdStringView section, const Name& name) : section(section), name(name)
            {
                // Nothing to do here.
            }
        };

        /// Alias for the data structure used to respond to queries for all settings of a given name across all sections.
        typedef std::list<SSectionNamePair> TSectionNamePairList;


        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Holds configuration data at the level of entire sections, one element per section.
        TSections sections;


    public:
        // -------- INSTANCE METHODS --------------------------------------- //

        /// Stores a new value for the specified configuration setting in the specified section.
        /// @tparam ValueType Type of value to insert.
        /// @param [in] section Section into which to insert the configuration setting.
        /// @param [in] name Name of the configuration setting into which to insert the value.
        /// @param [in] value Value to insert.
        template <typename ValueType> void Insert(TStdStringView section, TStdStringView name, const ValueType& value)
        {
            auto sectionIterator = sections.find(section);
            
            if (sections.end() == sectionIterator)
            {
                sections.emplace(section, ::Configuration::Section());
                sectionIterator = sections.find(section);
            }
            
            sectionIterator->second.Insert(name, value);
        }

        /// Allows read-only access to individual sections by name, with bounds checking.
        /// @param [in] section Name of the section to retrieve.
        /// @return Reference to the desired section.
        inline const Section& Section(TStdStringView section) const
        {
            return sections.find(section)->second;
        }
        
        /// Retrieves the number of sections present in the configuration represented by this object.
        /// @return Number of configuration settings present.
        inline size_t SectionCount(void) const
        {
            return sections.size();
        }
        
        /// Determines if a section of the specified name exists in the configuration represented by this object.
        /// @param [in] section Section name to check.
        /// @return `true` if the setting exists, `false` otherwise.
        inline bool SectionExists(TStdStringView section) const
        {
            return (0 != sections.count(section));
        }
        
        /// Allows read-only access to all sections.
        /// Useful for iterating.
        /// @return Container of all sections.
        inline const TSections& Sections(void) const
        {
            return sections;
        }

        /// Searches all sections in the configuration for settings identified by the specified name.
        /// For each, identifies both the section (by name) and the configuration setting (by the object that holds its values).
        /// Places all such pairs into a container and returns the container.
        /// If there are no matches, returns an empty container.
        /// @param [in] name Name of the configuration setting for which to search.
        /// @return Container holding the results.
        inline std::unique_ptr<TSectionNamePairList> SectionsWithName(TStdStringView name) const
        {
            std::unique_ptr<TSectionNamePairList> sectionsWithName = std::make_unique<TSectionNamePairList>();

            for (auto& section : sections)
            {
                if (section.second.NameExists(name))
                    sectionsWithName->emplace_back(section.first, section.second.Name(name));
            }

            return sectionsWithName;
        }
    };

    /// Interface for reading and parsing INI-formatted configuration files.
    /// Name-and-value pairs (of the format "name = value") are namespaced by sections (of the format "[section name]").
    /// Provides basic configuration file reading and parsing functionality, but leaves managing and error-checking configuration values to subclasses.
    class ConfigurationFileReaderBase
    {
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default destructor.
        virtual ~ConfigurationFileReaderBase(void) = default;


        // -------- INSTANCE METHODS --------------------------------------- //

        /// Reads and parses a configuration file, storing the settings in a newly-allocated #Configuration object that is owned by the caller.
        /// Intended to be invoked externally. Subclasses should not override this method.
        /// @param [in] configFileName Name of the configuration file to read.
        /// @return Pointer to the object that holds the settings, or `NULL` on failure.
        std::unique_ptr<Configuration> ReadConfigurationFile(const TCHAR* const configFileName);


    protected:
        // -------- ABSTRACT INSTANCE METHODS ------------------------------ //

        /// Specifies the action to take when a given section is encountered in a configuration file.
        /// These are the names that typically appear in [square brackets].
        /// Invoked while reading from a configuration file.
        /// Subclasses should override this method.
        /// For example, if the particular section name is not within the list of supported configuration namespaces, subclasses can flag an error.
        /// @param [in] section Name of the section, as read from the configuration file.
        /// @return Action to take with the section.
        virtual ESectionAction ActionForSection(TStdStringView section) = 0;

        /// Invoked to allow the subclass to error-check the specified integer-typed configuration setting, identified by enclosing section name and by configuration setting name.
        /// @param [in] section Name of the enclosing section, as read from the configuration file.
        /// @param [in] name Name of the configuration setting, as read from the configuration file.
        /// @param [in] value Value of the configuration setting, as read and parsed from the configuration file.
        /// @return `true` if the submitted value was acceptable (according to whatever arbitrary characteristics the subclass wishes), `false` otherwise.
        virtual bool CheckValue(TStdStringView section, TStdStringView name, const TIntegerValue& value) = 0;

        /// Invoked to allow the subclass to error-check the specified Boolean-typed configuration setting, identified by enclosing section name and by configuration setting name.
        /// @param [in] section Name of the enclosing section, as read from the configuration file.
        /// @param [in] name Name of the configuration setting, as read from the configuration file.
        /// @param [in] value Value of the configuration setting, as read and parsed from the configuration file.
        /// @return `true` if the submitted value was acceptable (according to whatever arbitrary characteristics the subclass wishes), `false` otherwise.
        virtual bool CheckValue(TStdStringView section, TStdStringView name, const TBooleanValue& value) = 0;

        /// Invoked to allow the subclass to error-check specified string-typed configuration setting, identified by enclosing section name and by configuration setting name.
        /// @param [in] section Name of the enclosing section, as read from the configuration file.
        /// @param [in] name Name of the configuration setting, as read from the configuration file.
        /// @param [in] value Value of the configuration setting, as read and parsed from the configuration file.
        /// @return `true` if the submitted value was acceptable (according to whatever arbitrary characteristics the subclass wishes), `false` otherwise.
        virtual bool CheckValue(TStdStringView section, TStdStringView name, const TStringValue& value) = 0;

        /// Specifies the type of the value for the given configuration setting.
        /// In lines that are of the form "name = value" parameters identify both the enclosing section and the name part.
        /// Subclasses should override this method.
        /// For example, if the value is expected to be an integer, subclasses should indicate this so the value is parsed and submitted correctly.
        /// @param [in] section Name of the enclosing section, as read from the configuration file.
        /// @param [in] name Name of the configuration setting (the left part of the example line given above).
        /// @return Type to associate with the value (the right part of the example line given above), which can be an error if the particular configuration setting is not supported.
        virtual EValueType TypeForValue(TStdStringView section, TStdStringView name) = 0;
    };

    /// Reference implementation of a configuration object that parses all values as strings and supports multiple values for each configuration setting.
    /// If desired, subclasses can provide more structure to configuration files by overriding methods to customize value types and reject anything that should not appear in a configuration file.
    class ConfigurationFileReader : public ConfigurationFileReaderBase
    {
    protected:
        // -------- CONCRETE INSTANCE METHODS ------------------------------ //
        // See above for documentation.

        ESectionAction ActionForSection(TStdStringView section) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const TIntegerValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const TBooleanValue& value) override;
        bool CheckValue(TStdStringView section, TStdStringView name, const TStringValue& value) override;
        EValueType TypeForValue(TStdStringView section, TStdStringView name) override;
    };
}
