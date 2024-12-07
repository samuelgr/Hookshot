/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Configuration.cpp
 *   Implementation of configuration file functionality.
 **************************************************************************************************/

#include "Configuration.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Infra/Core/TemporaryBuffer.h>

#include "Strings.h"

namespace Hookshot
{
  namespace Configuration
  {
    /// Enumerates all possible classifications of configuration file lines.
    /// Used during parsing to classify each line encountered.
    enum class ELineClassification
    {
      /// Line could not be parsed.
      Error,

      /// Line should be ignored, either because it is just whitespace or because it is a
      /// comment.
      Ignore,

      /// Line begins a section, whose name appears in square brackets.
      Section,

      /// Line is a value within the current section and so should be parsed.
      Value,
    };

    /// Wrapper around a standard file handle.
    /// Attempts to open the specified file on construction and close it on destruction.
    struct FileHandle
    {
      FILE* fileHandle = nullptr;

      inline FileHandle(const wchar_t* filename, const wchar_t* mode)
      {
        _wfopen_s(&fileHandle, filename, mode);
      }

      FileHandle(const FileHandle& other) = delete;

      inline ~FileHandle(void)
      {
        if (nullptr != fileHandle) fclose(fileHandle);
      }

      inline operator FILE*(void) const
      {
        return fileHandle;
      }

      inline FILE** operator&(void)
      {
        return &fileHandle;
      }

      inline bool IsEndOfInput(void)
      {
        return feof(fileHandle);
      }

      inline bool IsError(void)
      {
        return ferror(fileHandle);
      }
    };

    /// Wrapper that emulates a file handle but for in-memory buffers.
    struct MemoryBufferHandle
    {
      std::wstring_view remainingBuffer;

      inline MemoryBufferHandle(std::wstring_view configBuffer) : remainingBuffer(configBuffer) {}

      inline bool IsEndOfInput(void)
      {
        return remainingBuffer.empty();
      }

      inline bool IsError(void)
      {
        return false;
      }
    };

    /// Tests if the supplied character is allowed as a configuration setting name (the part
    /// before the '=' sign in the configuration file).
    /// @param [in] charToTest Character to test.
    /// @return `true` if so, `false` if not.
    static bool IsAllowedNameCharacter(const wchar_t charToTest)
    {
      switch (charToTest)
      {
        case L'.':
        case L'-':
        case L'_':
          return true;

        default:
          return (iswalnum(charToTest) ? true : false);
      }
    }

    /// Tests if the supplied character is allowed as a section name (appears between square
    /// brackets in the configuration file).
    /// @param [in] charToTest Character to test.
    /// @return `true` if so, `false` if not.
    static bool IsAllowedSectionCharacter(const wchar_t charToTest)
    {
      switch (charToTest)
      {
        case L',':
        case L'.':
        case L';':
        case L':':
        case L'\'':
        case L'\\':
        case L'{':
        case L'}':
        case L'-':
        case L'_':
        case L' ':
        case L'+':
        case L'=':
        case L'!':
        case L'@':
        case L'#':
        case L'$':
        case L'%':
        case L'^':
        case L'&':
        case L'(':
        case L')':
          return true;

        default:
          return (iswalnum(charToTest) ? true : false);
      }
    }

    /// Tests if the supplied character is allowed as a configuration setting value (the part
    /// after the '=' sign in the configuration file).
    /// @param [in] charToTest Character to test.
    /// @return `true` if so, `false` if not.
    static bool IsAllowedValueCharacter(const wchar_t charToTest)
    {
      switch (charToTest)
      {
        case L',':
        case L'.':
        case L';':
        case L':':
        case L'\'':
        case L'\\':
        case L'{':
        case L'[':
        case L'}':
        case L']':
        case L'-':
        case L'_':
        case L' ':
        case L'+':
        case L'=':
        case L'!':
        case L'@':
        case L'#':
        case L'$':
        case L'%':
        case L'^':
        case L'&':
        case L'(':
        case L')':
        case L'*':
        case L'?':
          return true;

        default:
          return (iswalnum(charToTest) ? true : false);
      }
    }

    /// Classifies the provided configuration file line and returns a value indicating the
    /// result.
    /// @param [in] buf Buffer containing the configuration file line.
    /// @param [in] length Number of characters in the buffer.
    /// @return Configuration line classification.
    static ELineClassification ClassifyConfigurationFileLine(
        const wchar_t* const buf, const int length)
    {
      // Skip over all whitespace at the start of the input line.
      const wchar_t* realBuf = buf;
      int realLength = length;
      while (realLength != 0 && iswblank(realBuf[0]))
      {
        realLength -= 1;
        realBuf += 1;
      }

      // Sanity check: zero-length and all-whitespace lines can be safely ignored.
      // Also filter out comments this way.
      if (0 == realLength || L';' == realBuf[0] || L'#' == realBuf[0])
        return ELineClassification::Ignore;

      // Non-comments must, by definition, have at least three characters in them, excluding
      // all whitespace. For section headers, this must mean '[' + section name + ']'. For
      // values, this must mean name + '=' + value.
      if (realLength < 3) return ELineClassification::Error;

      if (L'[' == realBuf[0])
      {
        // The line cannot be a section header unless the second character is a valid
        // section name character.
        if (!IsAllowedSectionCharacter(realBuf[1])) return ELineClassification::Error;

        // Verify that the line is a valid section header by checking for valid section name
        // characters between two square brackets.
        int i = 2;
        for (; i < realLength && L']' != realBuf[i]; ++i)
        {
          if (!IsAllowedSectionCharacter(realBuf[i])) return ELineClassification::Error;
        }
        if (L']' != realBuf[i]) return ELineClassification::Error;

        // Verify that the remainder of the line is just whitespace.
        for (i += 1; i < realLength; ++i)
        {
          if (!iswblank(realBuf[i])) return ELineClassification::Error;
        }

        return ELineClassification::Section;
      }
      else if (IsAllowedNameCharacter(realBuf[0]))
      {
        // Search for whitespace or an equals sign, with all characters in between needing
        // to be allowed as value name characters.
        int i = 1;
        for (; i < realLength && L'=' != realBuf[i] && !iswblank(realBuf[i]); ++i)
        {
          if (!IsAllowedNameCharacter(realBuf[i])) return ELineClassification::Error;
        }

        // Skip over any whitespace present, then check for an equals sign.
        for (; i < realLength && iswblank(realBuf[i]); ++i)
          ;
        if (L'=' != realBuf[i]) return ELineClassification::Error;

        // Skip over any whitespace present, then verify the next character is allowed to
        // start a value setting.
        for (i += 1; i < realLength && iswblank(realBuf[i]); ++i)
          ;
        if (!IsAllowedValueCharacter(realBuf[i])) return ELineClassification::Error;

        // Skip over the value setting characters that follow.
        for (i += 1; i < realLength && IsAllowedValueCharacter(realBuf[i]); ++i)
          ;

        // Verify that the remainder of the line is just whitespace.
        for (; i < realLength; ++i)
        {
          if (!iswblank(realBuf[i])) return ELineClassification::Error;
        }

        return ELineClassification::Value;
      }

      return ELineClassification::Error;
    }

    /// Parses a Boolean from the supplied input string.
    /// @param [in] source String from which to parse.
    /// @param [out] dest Filled with the result of the parse.
    /// @return `true` if the parse was successful and able to consume the whole string, `false`
    /// otherwise.
    static bool ParseBoolean(const std::wstring_view source, TBooleanValue& dest)
    {
      static constexpr std::wstring_view kTrueStrings[] = {
          L"t", L"true", L"on", L"y", L"yes", L"enabled", L"1"};
      static constexpr std::wstring_view kFalseStrings[] = {
          L"f", L"false", L"off", L"n", L"no", L"disabled", L"0"};

      // Check if the string represents a value of TRUE.
      for (auto& trueString : kTrueStrings)
      {
        if (true == Strings::EqualsCaseInsensitive(trueString, source))
        {
          dest = (TBooleanValue) true;
          return true;
        }
      }

      // Check if the string represents a value of FALSE.
      for (auto& falseString : kFalseStrings)
      {
        if (true == Strings::EqualsCaseInsensitive(falseString, source))
        {
          dest = (TBooleanValue) false;
          return true;
        }
      }

      return false;
    }

    /// Parses a signed integer value from the supplied input string.
    /// @param [in] source String from which to parse.
    /// @param [out] dest Filled with the result of the parse.
    /// @return `true` if the parse was successful and able to consume the whole string, `false`
    /// otherwise.
    static bool ParseInteger(std::wstring_view source, TIntegerValue& dest)
    {
      intmax_t value = 0;
      wchar_t* endptr = nullptr;

      // Parse out a number in any representable base.
      value = wcstoll(source.data(), &endptr, 0);

      // Verify that the number is not out of range.
      if (ERANGE == errno && (LLONG_MIN == value || LLONG_MAX == value)) return false;
      if ((value > std::numeric_limits<TIntegerValue>::max()) ||
          (value < std::numeric_limits<TIntegerValue>::min()))
        return false;

      // Verify that the whole string was consumed. This uses pointer arithmetic, and so the logic
      // holds regardless of the size of the individual characters.
      if (source.length() != (endptr - source.data())) return false;

      dest = (TIntegerValue)value;
      return true;
    }

    /// Parses a name and a value for the specified configuration file line, which must first
    /// have been classified as containing a value. On return, the configuration file line is
    /// modified to add null termination such that the views are guaranteed to be
    /// null-terminated.
    /// @param [in] configFileLine Buffer containing the configuration file line.
    /// @param [out] nameString Filled with the name of the configuration setting.
    /// @param [out] valueString Filled with the value specified for the configuration setting.
    static void ParseNameAndValue(
        wchar_t* configFileLine, std::wstring_view& nameString, std::wstring_view& valueString)
    {
      // Skip to the start of the name part of the line.
      wchar_t* name = configFileLine;
      while (iswblank(name[0]))
        name += 1;

      // Find the length of the configuration name.
      int nameLength = 1;
      while (IsAllowedNameCharacter(name[nameLength]))
        nameLength += 1;

      // Advance to the value portion of the string.
      wchar_t* value = &name[nameLength + 1];

      // Skip over whitespace and the '=' sign.
      while ((L'=' == value[0]) || (iswblank(value[0])))
        value += 1;

      // Find the length of the configuration value.
      int valueLength = 1;
      while (IsAllowedValueCharacter(value[valueLength]))
        valueLength += 1;

      // Null-terminate both name and value strings so the resulting views are also
      // null-terminated.
      name[nameLength] = L'\0';
      value[valueLength] = L'\0';

      nameString = std::wstring_view(name, nameLength);
      valueString = std::wstring_view(value, valueLength);
    }

    /// Parses a section name from the specified configuration file line, which must first have
    /// been classified as containing a section name. On return, the configuration file line is
    /// modified to add null termination such that the view is guaranteed to be null-terminated.
    /// @param [in] configFileLine Buffer containing the configuration file line.
    /// @param String containing the name of the configuration section.
    static std::wstring_view ParseSection(wchar_t* configFileLine)
    {
      // Skip to the '[' character and then advance once more to the section name itself.
      wchar_t* section = configFileLine;
      while (L'[' != section[0])
        section += 1;
      section += 1;

      // Find the length of the section name.
      int sectionLength = 1;
      while (L']' != section[sectionLength])
        sectionLength += 1;

      // Null-terminate the section name so the resulting view is also null-terminated.
      section[sectionLength] = L'\0';

      return std::wstring_view(section, sectionLength);
    }

    /// Reads a single line from the specified handle, verifies that it fits within the
    /// specified buffer, and removes trailing whitespace. Default implementation does nothing
    /// and always returns an error.
    /// @tparam ReadHandleType Handle that implements the functions required to read one line at
    /// a time from an input source.
    /// @param [in] readHandle Handle to the configuration file data from which to read.
    /// @param [out] lineBuffer Filled with text read from the specified file.
    /// @param [in] lineBufferCount Length, in character units, of the line buffer.
    /// @return Length of the string that was read, with -1 indicating an error condition.
    template <typename ReadHandleType> static int ReadAndTrimLine(
        ReadHandleType& readHandle, wchar_t* const lineBuffer, const int lineBufferCount)
    {
      return -1;
    }

    /// Reads a single line from the specified file handle, verifies that it fits within the
    /// specified buffer, and removes trailing whitespace. This is a template specialization for
    /// reading from files.
    /// @param [in] fileHandle File handle for the configuration file.
    /// @param [out] lineBuffer Filled with text read from the specified file.
    /// @param [in] lineBufferCount Length, in character units, of the line buffer.
    /// @return Length of the string that was read, with -1 indicating an error condition.
    template <> static int ReadAndTrimLine<FileHandle>(
        FileHandle& fileHandle, wchar_t* const lineBuffer, const int lineBufferCount)
    {
      // Results in a null-terminated string guaranteed, but might not be the whole line if
      // the buffer is too small.
      if (lineBuffer != fgetws(lineBuffer, lineBufferCount, fileHandle)) return -1;

      // If the line fits in the buffer, then either its detected length is small by
      // comparison to the buffer size or, if it perfectly fits in the buffer, then the last
      // character is a newline.
      int lineLength = static_cast<int>(wcsnlen(lineBuffer, lineBufferCount));
      if (((lineBufferCount - 1) == lineLength) && (L'\n' != lineBuffer[lineLength - 1])) return -1;

      // Trim off any whitespace on the end of the line.
      while (iswspace(lineBuffer[lineLength - 1]))
        lineLength -= 1;

      lineBuffer[lineLength] = L'\0';

      return lineLength;
    }

    /// Reads a single line from the specified in-memory configuration file data, verifies that
    /// it fits within the specified buffer, and removes trailing whitespace. This is a template
    /// specialization for reading from an in-memory buffer.
    /// @param [in] memoryBufferHandle Handle object for reading from the in-memory buffer.
    /// @param [out] lineBuffer Filled with text read from the specified file.
    /// @param [in] lineBufferCount Length, in character units, of the line buffer.
    /// @return Length of the string that was read, with -1 indicating an error condition.
    template <> static int ReadAndTrimLine<MemoryBufferHandle>(
        MemoryBufferHandle& memoryBufferHandle,
        wchar_t* const lineBuffer,
        const int lineBufferCount)
    {
      if (memoryBufferHandle.IsEndOfInput()) return -1;

      int nextLineLength =
          static_cast<int>(memoryBufferHandle.remainingBuffer.find_first_of(L'\n'));

      if (std::wstring_view::npos == nextLineLength)
        nextLineLength = static_cast<int>(memoryBufferHandle.remainingBuffer.length());
      else
        nextLineLength += 1;

      int numCharsWritten = 0;
      for (; numCharsWritten < std::min(nextLineLength, (lineBufferCount - 1)); ++numCharsWritten)
        lineBuffer[numCharsWritten] = memoryBufferHandle.remainingBuffer[numCharsWritten];

      memoryBufferHandle.remainingBuffer.remove_prefix(numCharsWritten);

      // Trim off any whitespace on the end of the line.
      while (iswspace(lineBuffer[numCharsWritten - 1]))
        numCharsWritten -= 1;

      lineBuffer[numCharsWritten] = L'\0';

      return numCharsWritten;
    }

    template <> bool Value::TypeIs<TBooleanValue>(void) const
    {
      switch (GetType())
      {
        case EValueType::Boolean:
        case EValueType::BooleanMultiValue:
          return true;

        default:
          return false;
      }
    }

    template <> bool Value::TypeIs<TIntegerValue>(void) const
    {
      switch (GetType())
      {
        case EValueType::Integer:
        case EValueType::IntegerMultiValue:
          return true;

        default:
          return false;
      }
    }

    template <> bool Value::TypeIs<TStringValue>(void) const
    {
      switch (GetType())
      {
        case EValueType::String:
        case EValueType::StringMultiValue:
          return true;

        default:
          return false;
      }
    }

    template <> TBooleanValue Value::ExtractValue(void)
    {
      return std::move(boolValue);
    }

    template <> TIntegerValue Value::ExtractValue(void)
    {
      return std::move(intValue);
    }

    template <> TStringValue Value::ExtractValue(void)
    {
      return std::move(stringValue);
    }

    template <typename ValueType> std::optional<ValueType> Name::ExtractFirstValue(void)
    {
      if (false == TypeIs<ValueType>()) return std::nullopt;

      return values.extract(values.begin()).value().ExtractValue<ValueType>();
    }

    template std::optional<TBooleanValue> Name::ExtractFirstValue(void);
    template std::optional<TIntegerValue> Name::ExtractFirstValue(void);
    template std::optional<TStringValue> Name::ExtractFirstValue(void);

    template <typename ValueType> std::optional<std::vector<ValueType>> Name::ExtractValues(void)
    {
      if (false == TypeIs<ValueType>()) return std::nullopt;

      std::vector<ValueType> extractedValues;
      extractedValues.reserve(values.size());

      while (false == values.empty())
      {
        Value extractedValue = std::move(values.extract(values.begin()).value());
        extractedValues.emplace_back(extractedValue.ExtractValue<ValueType>());
      }

      return std::move(extractedValues);
    }

    template std::optional<std::vector<TBooleanValue>> Name::ExtractValues<TBooleanValue>(void);
    template std::optional<std::vector<TIntegerValue>> Name::ExtractValues<TIntegerValue>(void);
    template std::optional<std::vector<TStringValue>> Name::ExtractValues<TStringValue>(void);

    std::optional<std::pair<std::wstring, Name>> Section::ExtractFirstName(void)
    {
      auto nameIterator = names.begin();
      if (names.end() == nameIterator) return std::nullopt;

      auto extractedName = names.extract(nameIterator);
      return std::make_pair(std::move(extractedName.key()), std::move(extractedName.mapped()));
    }

    template <typename ValueType> std::optional<ValueType> Section::ExtractFirstValue(
        std::wstring_view name)
    {
      auto nameIterator = names.find(name);
      if (names.end() == nameIterator) return std::nullopt;

      if (false == nameIterator->second.TypeIs<ValueType>()) return std::nullopt;

      if (1 == nameIterator->second.ValueCount())
      {
        auto extractedName = names.extract(nameIterator);
        return extractedName.mapped().ExtractFirstValue<ValueType>();
      }
      else
      {
        return nameIterator->second.ExtractFirstValue<ValueType>();
      }
    }

    template std::optional<TBooleanValue> Section::ExtractFirstValue(std::wstring_view);
    template std::optional<TIntegerValue> Section::ExtractFirstValue(std::wstring_view);
    template std::optional<TStringValue> Section::ExtractFirstValue(std::wstring_view);

    template <typename ValueType> std::optional<std::vector<ValueType>> Section::ExtractValues(
        std::wstring_view name)
    {
      auto nameIterator = names.find(name);
      if (names.end() == nameIterator) return std::nullopt;

      if (false == nameIterator->second.TypeIs<ValueType>()) return std::nullopt;

      auto extractedName = names.extract(nameIterator);
      return extractedName.mapped().ExtractValues<ValueType>().value();
    }

    template std::optional<std::vector<TBooleanValue>> Section::ExtractValues<TBooleanValue>(
        std::wstring_view name);
    template std::optional<std::vector<TIntegerValue>> Section::ExtractValues<TIntegerValue>(
        std::wstring_view name);
    template std::optional<std::vector<TStringValue>> Section::ExtractValues<TStringValue>(
        std::wstring_view name);

    std::optional<TBooleanView> Section::GetFirstBooleanValue(std::wstring_view name) const
    {
      const auto nameIter = names.find(name);
      if (names.cend() == nameIter) return std::nullopt;

      switch (nameIter->second.GetType())
      {
        case EValueType::Boolean:
        case EValueType::BooleanMultiValue:
          break;

        default:
          return std::nullopt;
      }

      return nameIter->second.GetFirstValue().GetBooleanValue();
    }

    std::optional<TIntegerView> Section::GetFirstIntegerValue(std::wstring_view name) const
    {
      const auto nameIter = names.find(name);
      if (names.cend() == nameIter) return std::nullopt;

      switch (nameIter->second.GetType())
      {
        case EValueType::Integer:
        case EValueType::IntegerMultiValue:
          break;

        default:
          return std::nullopt;
      }

      return nameIter->second.GetFirstValue().GetIntegerValue();
    }

    std::optional<TStringView> Section::GetFirstStringValue(std::wstring_view name) const
    {
      const auto nameIter = names.find(name);
      if (names.cend() == nameIter) return std::nullopt;

      switch (nameIter->second.GetType())
      {
        case EValueType::String:
        case EValueType::StringMultiValue:
          break;

        default:
          return std::nullopt;
      }

      return nameIter->second.GetFirstValue().GetStringValue();
    }

    std::pair<std::wstring, Section> ConfigurationData::ExtractSection(
        TSections::const_iterator position)
    {
      auto extractedSection = sections.extract(position);
      return std::make_pair(
          std::move(extractedSection.key()), std::move(extractedSection.mapped()));
    }

    std::optional<std::pair<std::wstring, Section>> ConfigurationData::ExtractSection(
        std::wstring_view section)
    {
      auto sectionIterator = sections.find(section);
      if (sections.end() == sectionIterator) return std::nullopt;

      return ExtractSection(sectionIterator);
    }

    ConfigurationData ConfigurationFileReader::ReadConfigurationFile(
        std::wstring_view configFileName, bool mustExist)
    {
      FileHandle configFileHandle(configFileName.data(), L"r");
      if (nullptr == configFileHandle)
      {
        ConfigurationData configDataFromNonExistentFile;

        if (true == mustExist)
          configDataFromNonExistentFile.InsertReadErrorMessage(
              Strings::FormatString(L"%s: Unable to open file.", configFileName.data()));

        return configDataFromNonExistentFile;
      }

      return ReadConfiguration(configFileHandle, configFileName);
    }

    ConfigurationData ConfigurationFileReader::ReadInMemoryConfigurationFile(
        std::wstring_view configBuffer)
    {
      MemoryBufferHandle configBufferHandle(configBuffer);
      return ReadConfiguration(
          configBufferHandle,
          Strings::FormatString(
              L"[0x%0.*zx]",
              static_cast<int>(2 * sizeof(size_t)),
              reinterpret_cast<size_t>(configBuffer.data())));
    }

    template <typename ReadHandleType> ConfigurationData ConfigurationFileReader::ReadConfiguration(
        ReadHandleType& readHandle, std::wstring_view configSourceName)
    {
      ConfigurationData configToFill;

      BeginRead();

      // Parse the configuration file, one line at a time.
      std::set<std::wstring, std::less<>> seenSections;
      std::wstring_view thisSection = kSectionNameGlobal;

      int configLineNumber = 1;
      Infra::TemporaryBuffer<wchar_t> configLineBuffer;
      int configLineLength =
          ReadAndTrimLine(readHandle, configLineBuffer.Data(), configLineBuffer.Capacity());
      bool skipValueLines = false;

      while (configLineLength >= 0)
      {
        switch (ClassifyConfigurationFileLine(configLineBuffer.Data(), configLineLength))
        {
          case ELineClassification::Error:
            configToFill.InsertReadErrorMessage(Strings::FormatString(
                L"%s(%d): Unable to parse line.", configSourceName.data(), configLineNumber));
            break;

          case ELineClassification::Ignore:
            break;

          case ELineClassification::Section:
          {
            std::wstring_view section = ParseSection(configLineBuffer.Data());

            if (0 != seenSections.count(section))
            {
              configToFill.InsertReadErrorMessage(Strings::FormatString(
                  L"%s(%d): %s: Duplicated section name.",
                  configSourceName.data(),
                  configLineNumber,
                  section.data()));
              skipValueLines = true;
              break;
            }

            const EAction sectionAction = ActionForSection(section);
            switch (sectionAction)
            {
              case EAction::Error:
                if (true == HasLastErrorMessage())
                  configToFill.InsertReadErrorMessage(Strings::FormatString(
                      L"%s(%d): %s",
                      configSourceName.data(),
                      configLineNumber,
                      GetLastErrorMessage().c_str()));
                else
                  configToFill.InsertReadErrorMessage(Strings::FormatString(
                      L"%s(%d): %s: Unrecognized section name.",
                      configSourceName.data(),
                      configLineNumber,
                      section.data()));
                skipValueLines = true;
                break;

              case EAction::Process:
                thisSection = *(seenSections.emplace(section).first);
                skipValueLines = false;
                break;

              case EAction::Skip:
                skipValueLines = true;
                break;

              default:
                configToFill.InsertReadErrorMessage(Strings::FormatString(
                    L"%s(%d): Internal error while processing section name.",
                    configSourceName.data(),
                    configLineNumber));
                skipValueLines = true;
                break;
            }
            break;
          }

          case ELineClassification::Value:
            if (false == skipValueLines)
            {
              std::wstring_view name;
              std::wstring_view value;
              ParseNameAndValue(configLineBuffer.Data(), name, value);

              const EValueType valueType = TypeForValue(thisSection, name);
              bool shouldParseValue = true;

              // If the value type does not identify it as multi-valued, make sure
              // this is the first time the setting is seen.
              switch (valueType)
              {
                case EValueType::Integer:
                case EValueType::Boolean:
                case EValueType::String:
                  if (configToFill.SectionNamePairExists(thisSection, name))
                  {
                    configToFill.InsertReadErrorMessage(Strings::FormatString(
                        L"%s(%d): %s: Only a single value is allowed for this setting.",
                        configSourceName.data(),
                        configLineNumber,
                        name.data()));
                    shouldParseValue = false;
                  }
                  break;

                default:
                  break;
              }

              if (false == shouldParseValue) break;

              // Attempt to parse the value.
              switch (valueType)
              {
                case EValueType::Error:
                  configToFill.InsertReadErrorMessage(Strings::FormatString(
                      L"%s(%d): %s: Unrecognized configuration setting.",
                      configSourceName.data(),
                      configLineNumber,
                      name.data()));
                  break;

                case EValueType::Integer:
                case EValueType::IntegerMultiValue:
                {
                  TIntegerValue intValue = (TIntegerValue)0;

                  if (false == ParseInteger(value, intValue))
                  {
                    configToFill.InsertReadErrorMessage(Strings::FormatString(
                        L"%s(%d): %s: Failed to parse integer value.",
                        configSourceName.data(),
                        configLineNumber,
                        value.data()));
                    break;
                  }

                  switch (ActionForValue(thisSection, name, intValue))
                  {
                    case EAction::Error:
                      if (true == HasLastErrorMessage())
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s",
                            configSourceName.data(),
                            configLineNumber,
                            GetLastErrorMessage().c_str()));
                      else
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Invalid value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;

                    case EAction::Process:
                      if (false ==
                          configToFill.InsertValue(
                              thisSection, name, Value(TIntegerValue(intValue), configLineNumber)))
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Duplicated value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;
                  }
                  break;
                }

                case EValueType::Boolean:
                case EValueType::BooleanMultiValue:
                {
                  TBooleanValue boolValue = (TBooleanValue) false;

                  if (false == ParseBoolean(value, boolValue))
                  {
                    configToFill.InsertReadErrorMessage(Strings::FormatString(
                        L"%s(%d): %s: Failed to parse Boolean value.",
                        configSourceName.data(),
                        configLineNumber,
                        value.data()));
                    break;
                  }

                  switch (ActionForValue(thisSection, name, boolValue))
                  {
                    case EAction::Error:
                      if (true == HasLastErrorMessage())
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s",
                            configSourceName.data(),
                            configLineNumber,
                            GetLastErrorMessage().c_str()));
                      else
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Invalid value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;

                    case EAction::Process:
                      if (false ==
                          configToFill.InsertValue(
                              thisSection, name, Value(TBooleanValue(boolValue), configLineNumber)))
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Duplicated value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;
                  }
                  break;
                }

                case EValueType::String:
                case EValueType::StringMultiValue:
                {
                  switch (ActionForValue(thisSection, name, value))
                  {
                    case EAction::Error:
                      if (true == HasLastErrorMessage())
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s",
                            configSourceName.data(),
                            configLineNumber,
                            GetLastErrorMessage().c_str()));
                      else
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Invalid value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;

                    case EAction::Process:
                      if (false ==
                          configToFill.InsertValue(
                              thisSection, name, Value(TStringValue(value), configLineNumber)))
                        configToFill.InsertReadErrorMessage(Strings::FormatString(
                            L"%s(%d): %s: Duplicated value for configuration setting %s.",
                            configSourceName.data(),
                            configLineNumber,
                            value.data(),
                            name.data()));
                      break;
                  }
                  break;
                }

                default:
                  configToFill.InsertReadErrorMessage(Strings::FormatString(
                      L"%s(%d): Internal error while processing configuration setting.",
                      configSourceName.data(),
                      configLineNumber));
                  break;
              }
            }
            break;

          default:
            configToFill.InsertReadErrorMessage(Strings::FormatString(
                L"%s(%d): Internal error while processing line.",
                configSourceName.data(),
                configLineNumber));
            break;
        }

        configLineLength =
            ReadAndTrimLine(readHandle, configLineBuffer.Data(), configLineBuffer.Capacity());
        configLineNumber += 1;
      }

      if (!readHandle.IsEndOfInput())
      {
        // Stopped reading the configuration file early due to some condition other than
        // end-of-file. This indicates an error.

        if (readHandle.IsError())
        {
          configToFill.InsertReadErrorMessage(Strings::FormatString(
              L"%s(%d): I/O error while reading.", configSourceName.data(), configLineNumber));
          return configToFill;
        }
        else if (configLineLength < 0)
        {
          configToFill.InsertReadErrorMessage(Strings::FormatString(
              L"%s(%d): Line is too long.", configSourceName.data(), configLineNumber));
          return configToFill;
        }
      }

      EndRead();

      return configToFill;
    }

    Infra::TemporaryString ConfigurationData::ToConfigurationFileString(void) const
    {
      Infra::TemporaryString configurationFileString;

      for (const auto& sectionRecord : sections)
      {
        std::wstring_view section = sectionRecord.first;

        if (false == section.empty())
          configurationFileString << L'[' << sectionRecord.first << L']' << L'\n';

        for (const auto& nameRecord : sectionRecord.second.Names())
        {
          std::wstring_view name = nameRecord.first;

          for (const auto& valueRecord : nameRecord.second.Values())
          {
            switch (valueRecord.GetType())
            {
              case EValueType::Integer:
              case EValueType::IntegerMultiValue:
                configurationFileString << name << L" = " << valueRecord.GetIntegerValue() << L'\n';
                break;

              case EValueType::Boolean:
              case EValueType::BooleanMultiValue:
                configurationFileString << name << L" = " << valueRecord.GetBooleanValue() << L'\n';
                break;

              case EValueType::String:
              case EValueType::StringMultiValue:
                configurationFileString << name << L" = " << valueRecord.GetStringValue() << L'\n';
                break;
            }
          }
        }
      }

      return configurationFileString;
    }

    void ConfigurationFileReader::BeginRead(void) {}

    void ConfigurationFileReader::EndRead(void) {}
  } // namespace Configuration
} // namespace Hookshot
