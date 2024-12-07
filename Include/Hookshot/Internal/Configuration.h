/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Configuration.h
 *   Declaration of configuration file functionality.
 **************************************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <Infra/Core/TemporaryBuffer.h>

/// Convenience wrapper around initializer list syntax for defining a configuration file section in
/// a layout object. Specify a section name followed by a series of setting name and value type
/// pairs.
#define ConfigurationFileLayoutSection(section, ...)                                               \
  {                                                                                                \
    (section),                                                                                     \
    {                                                                                              \
      __VA_ARGS__                                                                                  \
    }                                                                                              \
  }

/// Convenience wrapper around initializer list syntax for defining a setting name and value type
/// pair. Intended for use within the initializer for a configuration file section layout.
#define ConfigurationFileLayoutNameAndValueType(name, valueType)                                   \
  {                                                                                                \
    (name), (valueType)                                                                            \
  }

namespace Hookshot
{
  namespace Configuration
  {
    /// Section name for all settings that appear at global scope (i.e. outside of a section).
    inline constexpr wchar_t kSectionNameGlobal[] = L"";

    /// Enumerates possible directives that can be issued in response to a query on how to
    /// process a section or a name/value pair encountered in a configuration file.
    enum class EAction
    {
      /// Flag an error. For sections, this means the remainder of the section is skipped.
      Error,

      /// Continue processing. For sections this means the name/value pairs within will be
      /// read. For name/value pairs this means the pair will be inserted into the
      /// configuration data structure.
      Process,

      /// Skip. For sections this means to ignore all the name/value pairs within. For
      /// name/value pairs this means to do nothing.
      Skip,
    };

    /// Enumerates all supported types for configuration values.
    /// Used when checking with a subclass for guidance on whether a section/name pair is
    /// supported and, if so, how to parse the value.
    enum class EValueType
    {
      /// Combination of section and name pair is not supported.
      Error,

      /// Combination of section and name pair is supported; value is a single integer.
      Integer,

      /// Combination of section and name pair is supported; value is a single Boolean.
      Boolean,

      /// Combination of section and name pair is supported; value is a single string.
      String,

      /// Combination of section and name pair is supported; value is integer and multiple
      /// values are allowed.
      IntegerMultiValue,

      /// Combination of section and name pair is supported; value is Boolean and multiple
      /// values are allowed.
      BooleanMultiValue,

      /// Combination of section and name pair is supported; value is string and multiple
      /// values are allowed.
      StringMultiValue,
    };

    /// Underlying type used for storing integer-typed values.
    using TIntegerValue = int64_t;

    /// Underlying type used for storing Boolean-valued types.
    using TBooleanValue = bool;

    /// Underlying type used for storing string-valued types.
    using TStringValue = std::wstring;

    /// View type used for retrieving and returning integer-typed values.
    using TIntegerView = TIntegerValue;

    /// View type used for retrieving and returning integer-typed values.
    using TBooleanView = TBooleanValue;

    /// View type used for retrieving and returning string-typed values.
    using TStringView = std::wstring_view;

    /// Fourth-level object used to represent a single configuration value for a particular
    /// configuration setting.
    class Value
    {
    public:

      /// Creates an integer-typed value by copying it.
      inline Value(const TIntegerView& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::Integer), intValue(value)
      {}

      /// Creates an integer-typed value by moving it.
      inline Value(TIntegerValue&& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::Integer), intValue(std::move(value))
      {}

      /// Creates a Boolean-typed value by copying it.
      inline Value(const TBooleanView& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::Boolean), boolValue(value)
      {}

      /// Creates a Boolean-typed value by moving it.
      inline Value(TBooleanValue&& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::Boolean), boolValue(std::move(value))
      {}

      /// Creates a string-typed value by copying it.
      inline Value(const TStringView& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::String), stringValue(value)
      {}

      /// Creates a string-typed value by moving it.
      inline Value(TStringValue&& value, int lineNumber = 0)
          : lineNumber(lineNumber), type(EValueType::String), stringValue(std::move(value))
      {}

      inline Value(const Value& other) : lineNumber(other.lineNumber), type(other.type)
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

      inline Value(Value&& other) noexcept
          : lineNumber(std::move(other.lineNumber)), type(std::move(other.type))
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

      inline bool operator<(const Value& rhs) const
      {
        if (type < rhs.type)
          return true;
        else if (type > rhs.type)
          return false;

        switch (type)
        {
          case EValueType::Integer:
            return (intValue < rhs.intValue);

          case EValueType::Boolean:
            return (boolValue < rhs.boolValue);

          case EValueType::String:
            return (stringValue < rhs.stringValue);

          default:
            return false;
        }
      }

      inline bool operator==(const Value& rhs) const
      {
        if (type != rhs.type) return false;

        switch (type)
        {
          case EValueType::Integer:
            return (intValue == rhs.intValue);

          case EValueType::Boolean:
            return (boolValue == rhs.boolValue);

          case EValueType::String:
            return (stringValue == rhs.stringValue);

          default:
            return false;
        }
      }

      /// Determines whether the type of value held by this object matches the template
      /// parameter.
      /// @tparam ValueType Type of value to check against what is held by this object.
      /// @return `true` if the template parameter value matches the actual value type held by
      /// this object, `false` otherwise.
      template <typename ValueType> bool TypeIs(void) const;

      /// Extracts the value held by this object without checking for type-correctness and
      /// returns it using move semantics.
      /// @tparam ValueType Type of value to extract from this object.
      /// @return Value extracted from this object.
      template <typename ValueType> ValueType ExtractValue(void);

      /// Retrieves and returns the line number within the configuration file on which this
      /// value was located.
      /// @return Line number that corresponds to this value.
      inline int GetLineNumber(void) const
      {
        return lineNumber;
      }

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

    private:

      /// Line number within the configuration file on which this value was located.
      int lineNumber;

      /// Indicates the value type.
      EValueType type;

      /// Holds the value itself.
      union
      {
        TIntegerValue intValue;
        TBooleanValue boolValue;
        TStringValue stringValue;
      };
    };

    /// Third-level object used to represent a single configuration setting within one section
    /// of a configuration file.
    class Name
    {
    public:

      /// Alias for the underlying data structure used to store per-setting configuration
      /// values.
      using TValues = std::set<Value>;

      Name(void) = default;

      /// Allows contents to be specified directly using multiple integer literals. Intended
      /// for use by tests.
      template <
          typename IntegerType,
          std::enable_if_t<
              std::conjunction_v<
                  std::is_integral<IntegerType>,
                  std::negation<std::is_same<IntegerType, bool>>>,
              bool> = true>
      inline Name(std::initializer_list<IntegerType> values) : Name()
      {
        for (const auto& value : values)
          InsertValue(Value((TIntegerView)value));
      }

      /// Allows contents to be specified directly using a single integer literal. Intended
      /// for use by tests.
      template <
          typename IntegerType,
          std::enable_if_t<
              std::conjunction_v<
                  std::is_integral<IntegerType>,
                  std::negation<std::is_same<IntegerType, bool>>>,
              bool> = true>
      inline Name(IntegerType value) : Name({value})
      {}

      /// Allows contents to be specified directly using multiple Boolean literals. Intended
      /// for use by tests.
      template <
          typename BooleanType,
          std::enable_if_t<std::is_same_v<BooleanType, bool>, bool> = true>
      inline Name(std::initializer_list<BooleanType> values) : Name()
      {
        for (const auto& value : values)
          InsertValue(Value((TBooleanView)value));
      }

      /// Allows contents to be specified directly using a single Boolean literal. Intended
      /// for use by tests.
      template <
          typename BooleanType,
          std::enable_if_t<std::is_same_v<BooleanType, bool>, bool> = true>
      inline Name(BooleanType value) : Name({value})
      {}

      /// Allows contents to be specified directly using multiple string literals. Intended
      /// for use by tests.
      template <
          typename StringType,
          std::enable_if_t<std::negation_v<std::is_integral<StringType>>, bool> = true>
      inline Name(std::initializer_list<StringType> values) : Name()
      {
        for (const auto& value : values)
          InsertValue(Value(TStringView(value)));
      }

      /// Allows contents to be specified directly using a single string literal. Intended for
      /// use by tests.
      template <
          typename StringType,
          std::enable_if_t<std::negation_v<std::is_integral<StringType>>, bool> = true>
      inline Name(StringType value) : Name({value})
      {}

      bool operator==(const Name& rhs) const = default;

      /// Extracts the first value from this configruation setting using move semantics.
      /// @tparam ValueType Expected value type of the configuration setting to extract.
      /// @return Extracted value if the value type matches the template parameter.
      template <typename ValueType> std::optional<ValueType> ExtractFirstValue(void);

      /// Extracts the first Boolean value from this configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type Boolean.
      inline std::optional<TBooleanValue> ExtractFirstBooleanValue(void)
      {
        return ExtractFirstValue<TBooleanValue>();
      }

      /// Extracts the first integer value from this configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type integer.
      inline std::optional<TIntegerValue> ExtractFirstIntegerValue(void)
      {
        return ExtractFirstValue<TIntegerValue>();
      }

      /// Extracts the first string value from this configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type string.
      inline std::optional<TStringValue> ExtractFirstStringValue(void)
      {
        return ExtractFirstValue<TStringValue>();
      }

      /// Extracts all values from this configuration setting using move semantics and returns
      /// them as a vector.
      /// @tparam ValueType Expected value type of the configuration setting to extract.
      /// @return Vector of extracted values if the value type matches the template parameter.
      template <typename ValueType> std::optional<std::vector<ValueType>> ExtractValues(void);

      /// Extracts all Boolean values from this configuration setting using move semantics and
      /// returns them as a vector.
      /// @return Vector of extracted Boolean values if the they are of type Boolean.
      inline std::optional<std::vector<TBooleanValue>> ExtractBooleanValues(void)
      {
        return ExtractValues<TBooleanValue>();
      }

      /// Extracts all integer values from this configuration setting using move semantics and
      /// returns them as a vector.
      /// @return Vector of extracted Boolean values if the they are of type integer.
      inline std::optional<std::vector<TIntegerValue>> ExtractIntegerValues(void)
      {
        return ExtractValues<TIntegerValue>();
      }

      /// Extracts all string values from this configuration setting using move semantics and
      /// returns them as a vector.
      /// @return Vector of extracted Boolean values if the they are of type string.
      inline std::optional<std::vector<TStringValue>> ExtractStringValues(void)
      {
        return ExtractValues<TStringValue>();
      }

      /// Allows read-only access to the first stored value.
      /// Useful for single-valued settings.
      /// @return First stored value.
      inline const Value& GetFirstValue(void) const
      {
        return *(values.begin());
      }

      /// Retrieves and returns the type of the stored values for this configuration setting.
      /// If this configuration setting is empty by virtue of all values being extracted then
      /// it is considered typeless, so operations that attempt to get the type return
      /// error-type.
      /// @return Type of the stored values.
      inline EValueType GetType(void) const
      {
        if (ValueCount() < 1) return EValueType::Error;

        return GetFirstValue().GetType();
      }

      /// Stores a new value for the configuration setting represented by this object by
      /// moving the input parameter. Will fail if the value already exists.
      /// @param [in] value Value to insert.
      /// @return `true` on success, `false` on failure.
      bool InsertValue(Value&& value)
      {
        return values.emplace(std::move(value)).second;
      }

      /// Determines if this configuration setting object is empty (i.e. contains no
      /// configuration data).
      /// @return `true` if so, `false` otherwise.
      inline bool IsEmpty(void) const
      {
        return values.empty();
      }

      /// Determines whether the type of value held in this configuration setting matches the
      /// template parameter. If this configuration setting is empty by virtue of all values
      /// being extracted then it is considered typeless, so this method always returns
      /// `false`.
      /// @tparam ValueType Type of value to check against what is held by this configuration
      /// setting.
      /// @return `true` if the template parameter value matches the actual value type held by
      /// this object, `false` otherwise.
      template <typename ValueType> inline bool TypeIs(void) const
      {
        if (ValueCount() < 1) return false;

        return GetFirstValue().TypeIs<ValueType>();
      }

      /// Retrieves the number of values present for the configuration setting represented by
      /// this object.
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

    private:

      /// Holds all values for each configuration setting, one element per value.
      TValues values;
    };

    /// Second-level object used to represent an entire section of a configuration file.
    class Section
    {
      /// Alias for the underlying data structure used to store per-section configuration
      /// settings.
      using TNames = std::map<std::wstring, Name, std::less<>>;

    public:

      Section(void) = default;

      /// Allows contents to be specified directly. Intended for use by tests.
      inline Section(std::initializer_list<std::pair<std::wstring, Name>> contents) : Section()
      {
        for (const auto& name : contents)
          names.emplace(name);
      }

      bool operator==(const Section& rhs) const = default;

      /// Allows read-only access to individual configuration settings by name, without bounds
      /// checking.
      inline const Name& operator[](std::wstring_view name) const
      {
        return names.find(name)->second;
      }

      /// Extracts the entire first configuration setting from this section using move
      /// semantics.
      /// @return Pair consisting of the configuration setting name and configuration setting
      /// object if this section is non-empty.
      std::optional<std::pair<std::wstring, Name>> ExtractFirstName(void);

      /// Extracts the first value from the specified configruation setting using move
      /// semantics. If the configuration setting is single-valued then it is destroyed.
      /// @tparam ValueType Expected value type of the configuration setting to extract.
      /// @return Extracted value if the value type matches the template parameter.
      template <typename ValueType> std::optional<ValueType> ExtractFirstValue(
          std::wstring_view name);

      /// Extracts the first Boolean value from the specified configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type Boolean.
      inline std::optional<TBooleanValue> ExtractFirstBooleanValue(std::wstring_view name)
      {
        return ExtractFirstValue<TBooleanValue>(name);
      }

      /// Extracts the first integer value from the specified configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type integer.
      inline std::optional<TIntegerValue> ExtractFirstIntegerValue(std::wstring_view name)
      {
        return ExtractFirstValue<TIntegerValue>(name);
      }

      /// Extracts the first string value from the specified configuration setting using move
      /// semantics.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Extracted value if the value is of type string.
      inline std::optional<TStringValue> ExtractFirstStringValue(std::wstring_view name)
      {
        return ExtractFirstValue<TStringValue>(name);
      }

      /// Extracts all values from the specified configuration setting using move semantics,
      /// erasing the entire setting from this section data object and returning all the
      /// values as a vector.
      /// @tparam ValueType Expected value type of the configuration setting to extract.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Vector of extracted values, if the name exists in this section and is of the
      /// correct type as identified by the template parameter.
      template <typename ValueType> std::optional<std::vector<ValueType>> ExtractValues(
          std::wstring_view name);

      /// Extracts all Boolean values from the specified configuration setting using move
      /// semantics, erasing the entire setting from this section data object and returning
      /// all the values as a vector.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Vector of extracted values, if the name was successfully located and values
      /// are of type Boolean.
      inline std::optional<std::vector<TBooleanValue>> ExtractBooleanValues(std::wstring_view name)
      {
        return ExtractValues<TBooleanValue>(name);
      }

      /// Extracts all integer values from the specified configuration setting using move
      /// semantics, erasing the entire setting from this section data object and returning
      /// all the values as a vector.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Vector of extracted values, if the name was successfully located and values
      /// are of type integer.
      inline std::optional<std::vector<TIntegerValue>> ExtractIntegerValues(std::wstring_view name)
      {
        return ExtractValues<TIntegerValue>(name);
      }

      /// Extracts all string values from the specified configuration setting using move
      /// semantics, erasing the entire setting from this section data object and returning
      /// all the values as a vector.
      /// @param [in] name Name of the configuration setting to extract.
      /// @return Vector of extracted values, if the name was successfully located and values
      /// are of type string.
      inline std::optional<std::vector<TStringValue>> ExtractStringValues(std::wstring_view name)
      {
        return ExtractValues<TStringValue>(name);
      }

      /// Convenience wrapper for quickly attempting to obtain a single Boolean-typed
      /// configuration value.
      /// @param [in] name Name of the value for which to search within this section.
      /// @return First value associated with the section and name, if it exists.
      std::optional<TBooleanView> GetFirstBooleanValue(std::wstring_view name) const;

      /// Convenience wrapper for quickly attempting to obtain a single Integer-typed
      /// configuration value.
      /// @param [in] name Name of the value for which to search within this section.
      /// @return First value associated with the section and name, if it exists.
      std::optional<TIntegerView> GetFirstIntegerValue(std::wstring_view name) const;

      /// Convenience wrapper for quickly attempting to obtain a single string-typed
      /// configuration value.
      /// @param [in] name Name of the value for which to search within this section.
      /// @return First value associated with the section and name, if it exists.
      std::optional<TStringView> GetFirstStringValue(std::wstring_view name) const;

      /// Stores a new value for the specified configuration setting in the section
      /// represented by this object by moving the input parameter. Will fail if the value
      /// already exists.
      /// @param [in] name Name of the configuration setting into which to insert the value.
      /// @param [in] value Value to insert.
      /// @return `true` on success, `false` on failure.
      inline bool InsertValue(std::wstring_view name, Value&& value)
      {
        auto nameIterator = names.find(name);

        if (names.end() == nameIterator)
          nameIterator = names.emplace(std::wstring(name), Name()).first;

        return nameIterator->second.InsertValue(std::move(value));
      }

      /// Determines if this configuration section data object is empty (i.e. contains no
      /// configuration data).
      /// @return `true` if so, `false` otherwise.
      inline bool IsEmpty(void) const
      {
        return names.empty();
      }

      /// Retrieves the number of configuration settings present for the section represented
      /// by this object.
      /// @return Number of configuration settings present.
      inline size_t NameCount(void) const
      {
        return names.size();
      }

      /// Determines if a configuration setting of the specified name exists in the section
      /// represented by this object.
      /// @param [in] name Name of the configuration setting to check.
      /// @return `true` if the setting exists, `false` otherwise.
      inline bool NameExists(std::wstring_view name) const
      {
        return names.contains(name);
      }

      /// Allows read-only access to all configuration settings.
      /// Useful for iterating.
      /// @return Container of all configuration settings.
      inline const TNames& Names(void) const
      {
        return names;
      }

    private:

      /// Holds configuration data within each section, one element per configuration setting.
      TNames names;
    };

    /// Top-level object used to represent all configuration data read from a configuration
    /// file.
    class ConfigurationData
    {
    public:

      /// Alias for the underlying data structure used to store top-level configuration
      /// section data.
      using TSections = std::map<std::wstring, Section, std::less<>>;

      ConfigurationData(void) = default;

      /// Allows contents to be specified directly. Intended for use by tests.
      inline ConfigurationData(std::initializer_list<std::pair<std::wstring, Section>> contents)
          : ConfigurationData()
      {
        for (const auto& section : contents)
          sections.insert(section);
      }

      /// Allows read-only access to individual sections by name, without bounds checking.
      inline const Section& operator[](std::wstring_view section) const
      {
        return sections.find(section)->second;
      }

      /// Compares data contents and presence or absence of errors only, ignoring error
      /// messages themselves.
      inline bool operator==(const ConfigurationData& rhs) const
      {
        return ((sections == rhs.sections) && (HasReadErrors() == rhs.HasReadErrors()));
      }

      /// Clears all of the stored read error messages and frees up all of the memory space
      /// they occupy. This method does not affect the return value of #HasReadErrors. Read
      /// error messages indicate a problem that occurred while the configuration file that
      /// produced this object was being read.
      inline void ClearReadErrorMessages(void)
      {
        if (readErrors.has_value()) readErrors->clear();
      }

      /// Clears all of the stored configuration data. Does not affect error messages.
      inline void Clear(void)
      {
        sections.clear();
      }

      /// Extracts the specified section from this configuration data object using move
      /// semantics. This has the additional effect of erasing it from this configuration data
      /// object.
      /// @param [in] position Iterator that corresponds to the specific section object to be
      /// extracted.
      /// @return Pair containing the extracted section name and extracted section object.
      std::pair<std::wstring, Section> ExtractSection(TSections::const_iterator position);

      /// Extracts the specified section from this configuration data object using move
      /// semantics. This has the additional effect of erasing it from this configuration data
      /// object.
      /// @param [in] section Section name to extract.
      /// @return Pair containing the extracted section name and extracted section object, if
      /// the section was successfully located.
      std::optional<std::pair<std::wstring, Section>> ExtractSection(std::wstring_view section);

      /// Convenience wrapper for quickly attempting to obtain a single Boolean-typed
      /// configuration value.
      /// @param [in] section Section name to search for the value.
      /// @param [in] name Name of the value for which to search.
      /// @return First value associated with the section and name, if it exists.
      inline std::optional<TBooleanView> GetFirstBooleanValue(
          std::wstring_view section, std::wstring_view name) const
      {
        const auto sectionIterator = sections.find(section);
        if (sections.cend() == sectionIterator) return std::nullopt;

        return sectionIterator->second.GetFirstBooleanValue(name);
      }

      /// Convenience wrapper for quickly attempting to obtain a single Integer-typed
      /// configuration value.
      /// @param [in] section Section name to search for the value.
      /// @param [in] name Name of the value for which to search.
      /// @return First value associated with the section and name, if it exists.
      inline std::optional<TIntegerView> GetFirstIntegerValue(
          std::wstring_view section, std::wstring_view name) const
      {
        const auto sectionIterator = sections.find(section);
        if (sections.cend() == sectionIterator) return std::nullopt;

        return sectionIterator->second.GetFirstIntegerValue(name);
      }

      /// Convenience wrapper for quickly attempting to obtain a single string-typed
      /// configuration value.
      /// @param [in] section Section name to search for the value.
      /// @param [in] name Name of the value for which to search.
      /// @return First value associated with the section and name, if it exists.
      inline std::optional<TStringView> GetFirstStringValue(
          std::wstring_view section, std::wstring_view name) const
      {
        const auto sectionIterator = sections.find(section);
        if (sections.cend() == sectionIterator) return std::nullopt;

        return sectionIterator->second.GetFirstStringValue(name);
      }

      /// Retrieves and returns the error messages that arose during the configuration file
      /// read attempt that produced this object. Does not check that read error messages
      /// actually exist.
      /// @return Error messages from last configuration file read attempt.
      inline const std::vector<std::wstring>& GetReadErrorMessages(void) const
      {
        return *readErrors;
      }

      /// Specifies whether or not any errors arose during the configuration file read attempt
      /// that produced this object. More details on any errors that arose are available by
      /// examining the error messages, unless they have already been cleared
      /// @return `true` if so, `false` if not.
      inline bool HasReadErrors(void) const
      {
        return readErrors.has_value();
      }

      /// Stores a new value for the specified configuration setting in the specified section
      /// by moving the input parameter. Will fail if the value already exists.
      /// @param [in] section Section into which to insert the configuration setting.
      /// @param [in] name Name of the configuration setting into which to insert the value.
      /// @param [in] value Value to insert.
      /// @return `true` on success, `false` on failure.
      bool InsertValue(std::wstring_view section, std::wstring_view name, Value&& value)
      {
        auto sectionIterator = sections.find(section);

        if (sections.end() == sectionIterator)
          sectionIterator = sections.emplace(section, Section()).first;

        return sectionIterator->second.InsertValue(name, std::move(value));
      }

      /// Inserts an error message into the list of error messages.
      /// Each such error message is a semantically-rich description of an error that occurred
      /// during the configuration file read attempt that fills this object.
      inline void InsertReadErrorMessage(std::wstring_view errorMessage)
      {
        if (false == readErrors.has_value()) readErrors.emplace();

        readErrors->emplace_back(errorMessage);
      }

      /// Determines if this configuration data object is empty (i.e. contains no
      /// configuration data).
      /// @return `true` if so, `false` otherwise.
      inline bool IsEmpty(void) const
      {
        return sections.empty();
      }

      /// Retrieves the number of sections present in the configuration represented by this
      /// object.
      /// @return Number of configuration settings present.
      inline size_t SectionCount(void) const
      {
        return sections.size();
      }

      /// Determines if a section of the specified name exists in the configuration
      /// represented by this object.
      /// @param [in] section Section name to check.
      /// @return `true` if the setting exists, `false` otherwise.
      inline bool SectionExists(std::wstring_view section) const
      {
        return sections.contains(section);
      }

      /// Determines if a configuration setting of the specified name exists in the specified
      /// section.
      /// @param [in] section Section name to check.
      /// @param [in] name Name of the configuration setting to check.
      /// @return `true` if the setting exists, `false` otherwise.
      inline bool SectionNamePairExists(std::wstring_view section, std::wstring_view name) const
      {
        auto sectionIterator = sections.find(section);
        if (sections.end() == sectionIterator) return false;

        return sectionIterator->second.NameExists(name);
      }

      /// Allows read-only access to all sections.
      /// Useful for iterating.
      /// @return Container of all sections.
      inline const TSections& Sections(void) const
      {
        return sections;
      }

      /// Converts the entire contents of this object into a configuration file string.
      /// @return String that contains a configuration file representation of the data held by
      /// this object.
      Infra::TemporaryString ToConfigurationFileString(void) const;

    private:

      /// Holds configuration data at the level of entire sections, one element per section.
      TSections sections;

      /// Holds the error messages that describes any errors that occurred during
      /// configuration file read.
      std::optional<std::vector<std::wstring>> readErrors;
    };

    /// Interface for reading and parsing INI-formatted configuration files.
    /// Name-and-value pairs (of the format "name = value") are namespaced by sections (of the
    /// format "[section name]"). Provides basic configuration file reading and parsing
    /// functionality, but leaves managing and error-checking configuration values to
    /// subclasses.
    class ConfigurationFileReader
    {
    public:

      virtual ~ConfigurationFileReader(void) = default;

      /// Reads and parses a configuration file, storing the settings in the supplied
      /// configuration object. Intended to be invoked externally. Subclasses should not
      /// override this method.
      /// @param [in] configFileName Name of the configuration file to read.
      /// @param [in] mustExist Indicates that it is an error for the configuration file not
      /// to exist. This requirement is unusual, so the default behavior is not to require it.
      /// @return Configuration data object filled based on the contents of the configuration
      /// file.
      ConfigurationData ReadConfigurationFile(
          std::wstring_view configFileName, bool mustExist = false);

      /// Reads and parses a configuration file held in memory, storing the settings in the
      /// supplied configuration object. Intended to be invoked externally, primarily by
      /// tests. Subclasses should not override this method.
      /// @param [in] configFileName Name of the configuration file to read.
      /// @return Configuration data object filled based on the contents of the configuration
      /// file.
      ConfigurationData ReadInMemoryConfigurationFile(std::wstring_view configBuffer);

    protected:

      /// Invoked at the beginning of a configuration file read operation.
      /// Subclasses are given the opportunity to initialize or reset any stored state, as
      /// needed. Overriding this method is optional, as a default implementation exists that
      /// does nothing.
      virtual void BeginRead(void);

      /// Invoked at the end of a configuration file read operation.
      /// Subclasses are given the opportunity to initialize or reset any stored state, as
      /// needed. Overriding this method is optional, as a default implementation exists that
      /// does nothing.
      virtual void EndRead(void);

      /// Sets a semantically-rich error message to be presented to the user in response to a
      /// subclass returning an error when asked what action to take. If a subclass does not
      /// set a semantically-rich error message then the default error message is used
      /// instead. Intended to be invoked optionally by subclasses during any method calls
      /// that return #EAction but only when #EAction::Error is being returned.
      /// @param [in] errorMessage String that is consumed to provide a semantically-rich
      /// error message.
      inline void SetLastErrorMessage(std::wstring&& errorMessage)
      {
        lastErrorMessage = std::move(errorMessage);
      }

      /// Convenience wrapper that sets a semantically-rich error message using a string view.
      /// @param [in] errorMessage String view that is copied to provide a semantically-rich
      /// error message.
      inline void SetLastErrorMessage(std::wstring_view errorMessage)
      {
        SetLastErrorMessage(std::wstring(errorMessage));
      }

      /// Convenience wrapper that sets a semantically-rich error message using a
      /// null-terminated C-style string.
      /// @param [in] errorMessage Temporary buffer containing a null-terminated string that
      /// is copied to provide a semantically-rich error message.
      inline void SetLastErrorMessage(const wchar_t* errorMessage)
      {
        SetLastErrorMessage(std::wstring(errorMessage));
      }

      /// Specifies the action to take when a given section is encountered in a configuration
      /// file (i.e. the names that typically appear in [square brackets] and separate the
      /// configuration file into namespaces). Invoked while reading from a configuration
      /// file. Subclasses must override this method. They are allowed to process the section
      /// name however they see fit and indicate to the caller what action to take.
      /// @param [in] section Name of the section, as read from the configuration file.
      /// @return Action to take with the section.
      virtual EAction ActionForSection(std::wstring_view section) = 0;

      /// Invoked to allow the subclass to process the specified integer-typed configuration
      /// setting, identified by enclosing section name and by configuration setting name.
      /// Subclasses are allowed to process the value however they see fit and indicate to the
      /// caller what action to take. Any values passed as read-only views are backed by
      /// temporary memory that will be discarded upon method return. Subclasses should copy
      /// values that need to be preserved outside of the configuration data structure.
      /// @param [in] section Name of the enclosing section, as read from the configuration
      /// file.
      /// @param [in] name Name of the configuration setting, as read from the configuration
      /// file.
      /// @param [in] value View of the value of the configuration setting, as read and parsed
      /// from the configuration file.
      /// @return Action to take with the name/value pair.
      virtual EAction ActionForValue(
          std::wstring_view section, std::wstring_view name, TIntegerView value) = 0;

      /// Invoked to allow the subclass to process the specified Boolean-typed configuration
      /// setting, identified by enclosing section name and by configuration setting name.
      /// Subclasses are allowed to process the value however they see fit and indicate to the
      /// caller what action to take. Any values passed as read-only views are backed by
      /// temporary memory that will be discarded upon method return. Subclasses should copy
      /// values that need to be preserved outside of the configuration data structure.
      /// @param [in] section Name of the enclosing section, as read from the configuration
      /// file.
      /// @param [in] name Name of the configuration setting, as read from the configuration
      /// file.
      /// @param [in] value View of the value of the configuration setting, as read and parsed
      /// from the configuration file.
      /// @return Action to take with the name/value pair.
      virtual EAction ActionForValue(
          std::wstring_view section, std::wstring_view name, TBooleanView value) = 0;

      /// Invoked to allow the subclass to process specified string-typed configuration
      /// setting, identified by enclosing section name and by configuration setting name.
      /// Subclasses are allowed to process the value however they see fit and indicate to the
      /// caller what action to take. Any values passed as read-only views are backed by
      /// temporary memory that will be discarded upon method return. Subclasses should copy
      /// values that need to be preserved outside of the configuration data structure.
      /// @param [in] section Name of the enclosing section, as read from the configuration
      /// file.
      /// @param [in] name Name of the configuration setting, as read from the configuration
      /// file.
      /// @param [in] value View of the value of the configuration setting, as read and parsed
      /// from the configuration file.
      /// @return Action to take with the name/value pair.
      virtual EAction ActionForValue(
          std::wstring_view section, std::wstring_view name, TStringView value) = 0;

      /// Specifies the type of the value for the given configuration setting.
      /// In lines that are of the form "name = value" parameters identify both the enclosing
      /// section and the name part. Subclasses should override this method. For example, if
      /// the value is expected to be an integer, subclasses should indicate this so the value
      /// is parsed and submitted correctly.
      /// @param [in] section Name of the enclosing section, as read from the configuration
      /// file.
      /// @param [in] name Name of the configuration setting (the left part of the example
      /// line given above).
      /// @return Type to associate with the value (the right part of the example line given
      /// above), which can be an error if the particular configuration setting is not
      /// supported.
      virtual EValueType TypeForValue(std::wstring_view section, std::wstring_view name) = 0;

    private:

      /// Used internally to retrieve and reset a semantically-rich error message if it
      /// exists.
      /// @return Last error message.
      inline std::wstring GetLastErrorMessage(void)
      {
        return std::move(lastErrorMessage);
      }

      /// Used internally to determine if a semantically-rich error message exists.
      /// @return `true` if an error message has been set, `false` otherwise.
      inline bool HasLastErrorMessage(void) const
      {
        return !(lastErrorMessage.empty());
      }

      /// Internal implementation of reading and parsing configuration files from any source.
      /// @tparam ReadHandleType Handle that implements the functions required to read one
      /// line at a time from an input source.
      /// @param [in] readHandle Mutable reference to a handle that controls the reading
      /// process.
      /// @param [in] configSourceName Name associated with the source of the configuration
      /// file data that can be used to identify it in logs and error messages.
      template <typename ReadHandleType> ConfigurationData ReadConfiguration(
          ReadHandleType& readHandle, std::wstring_view configSourceName);

      /// Holds a semantically-rich error message to be presented to the user whenever there
      /// is an error processing a configuration value. Used for temporary internal state
      /// only. These messages are subsequently stored in the configuration data object into
      /// which a configuration file is being read.
      std::wstring lastErrorMessage;
    };

    /// Type alias for a suggested format for storing the supported layout of a section within a
    /// configuration file. Useful for pre-determining what is allowed to appear within one
    /// section of a configuration file.
    using TConfigurationFileSectionLayout = std::map<std::wstring_view, EValueType, std::less<>>;

    /// Type alias for a suggested format for storing the supported layout of a configuration
    /// file. Useful for pre-determining what is allowed to appear within a configuration file.
    using TConfigurationFileLayout =
        std::map<std::wstring_view, TConfigurationFileSectionLayout, std::less<>>;
  } // namespace Configuration
} // namespace Hookshot
