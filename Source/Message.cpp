/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Message.cpp
 *   Message output implementation.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <cstdarg>
#include <cstdio>
#include <psapi.h>
#include <shlobj.h>
#include <sstream>
#include <string>


namespace Hookshot
{
    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Selects a character to place in stamps for messages output non-interactively.
    /// The decision is based on the severity of the message.
    /// @param [in] severity Message severity.
    /// @return Character to use in the stamp.
    static wchar_t StampCharacterForSeverity(const EMessageSeverity severity)
    {
        switch (severity)
        {
        case EMessageSeverity::MessageSeverityForcedInteractiveError:
        case EMessageSeverity::MessageSeverityError:
            return L'E';

        case EMessageSeverity::MessageSeverityForcedInteractiveWarning:
        case EMessageSeverity::MessageSeverityWarning:
            return L'W';

        case EMessageSeverity::MessageSeverityForcedInteractiveInfo:
        case EMessageSeverity::MessageSeverityInfo:
            return L'I';

        case EMessageSeverity::MessageSeverityDebug:
            return L'D';

        default:
            return L'?';
        }
    }


    // -------- CLASS VARIABLES -------------------------------------------- //
    // See "Message.h" for documentation.

    FILE* Message::logFileHandle = NULL;

#if HOOKSHOT_DEBUG
    EMessageSeverity Message::minimumSeverityForOutput = EMessageSeverity::MessageSeverityDebug;
#else
    EMessageSeverity Message::minimumSeverityForOutput = EMessageSeverity::MessageSeverityError;
#endif


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Message.h" for documentation.

    void Message::CreateAndEnableLogFile(void)
    {
        if (false == IsLogFileEnabled())
        {
            // Open the log file.
            if (0 != _wfopen_s(&logFileHandle, Strings::kStrHookshotLogFilename.data(), L"w"))
            {
                logFileHandle = NULL;
                OutputFormatted(EMessageSeverity::MessageSeverityError, L"%s - Unable to create log file.", Strings::kStrHookshotLogFilename.data());
                return;
            }

            // Output the log file header.
            static constexpr wchar_t kLogHeaderSeparator[] = L"---------------------------------------------";

            TemporaryBuffer<wchar_t> hookshotProductName;
            if (0 == LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, hookshotProductName, hookshotProductName.Count()))
                hookshotProductName[0] = L'\0';

            fwprintf_s(logFileHandle, L"%s\n", kLogHeaderSeparator);
            fwprintf_s(logFileHandle, L"%s Log\n", (wchar_t*)hookshotProductName);
            fwprintf_s(logFileHandle, L"%s\n", kLogHeaderSeparator);
            fwprintf_s(logFileHandle, L"Method:    %s\n", Globals::GetHookshotLoadMethodString().data());
            fwprintf_s(logFileHandle, L"Program:   %s\n", Strings::kStrExecutableCompleteFilename.data());
            fwprintf_s(logFileHandle, L"PID:       %d\n", GetProcessId(GetCurrentProcess()));
            fwprintf_s(logFileHandle, L"%s\n", kLogHeaderSeparator);
        }
    }

    // --------

    void Message::Output(const EMessageSeverity severity, const wchar_t* message)
    {
        if (false == WillOutputMessageOfSeverity(severity))
            return;

        OutputInternal(severity, message);
    }

    // ---------

    void Message::OutputFormatted(const EMessageSeverity severity, const wchar_t* format, ...)
    {
        if (false == WillOutputMessageOfSeverity(severity))
            return;

        va_list args;
        va_start(args, format);

        OutputFormattedInternal(severity, format, args);

        va_end(args);
    }

    // --------

    bool Message::WillOutputMessageOfSeverity(const EMessageSeverity severity)
    {
        // Messages of all severities are output unconditionally if a debugger is present.
        // #SelectOutputModes takes care of selecting the appropriate modes, given the message severity.
        if (IsDebuggerPresent())
            return true;
        
        if ((severity < EMessageSeverity::MessageSeverityForcedInteractiveBoundaryValue) || (severity <= minimumSeverityForOutput))
        {
            // Counter-intuitive: severity *values* increase as severity *levels* decrease.
            // This is checking if the actual severity *level* is above (*value* is below) the highest severity *level* that requires output be non-interactive.
            // If so, then there is no requirement that output be non-interactive.
            if (severity < kMaximumSeverityToRequireNonInteractiveOutput)
                return true;

            // Check all the selected output modes.
            // If any are interactive, then this message is skipped over.
            EMessageOutputMode outputModes[EMessageOutputMode::MessageOutputModeUpperBoundValue];
            const int numOutputModes = SelectOutputModes(severity, outputModes);

            for (int i = 0; i < numOutputModes; ++i)
            {
                if (true == IsOutputModeInteractive(outputModes[i]))
                    return false;
            }

            return true;
        }
        else
            return false;
    }


    // -------- HELPERS ---------------------------------------------------- //
    // See "Message.h" for documentation.

    void Message::OutputFormattedInternal(const EMessageSeverity severity, const wchar_t* format, va_list args)
    {
        TemporaryBuffer<wchar_t> messageBuf;

        vswprintf_s(messageBuf, messageBuf.Count(), format, args);
        OutputInternal(severity, messageBuf);
    }

    // --------

    void Message::OutputInternal(const EMessageSeverity severity, const wchar_t* message)
    {
        EMessageOutputMode outputModes[EMessageOutputMode::MessageOutputModeUpperBoundValue];
        const int numOutputModes = SelectOutputModes(severity, outputModes);
        
        for (int i = 0; i < numOutputModes; ++i)
        {
            switch (outputModes[i])
            {
            case EMessageOutputMode::MessageOutputModeDebugString:
                OutputInternalUsingDebugString(severity, message);
                break;

            case EMessageOutputMode::MessageOutputLogFile:
                OutputInternalUsingLogFile(severity, message);
                break;

            case EMessageOutputMode::MessageOutputModeMessageBox:
                OutputInternalUsingMessageBox(severity, message);
                break;

            default:
                break;
            }
        }
    }

    // --------

    void Message::OutputInternalUsingDebugString(const EMessageSeverity severity, const wchar_t* message)
    {
        std::wstringstream outputString;

        // First compose the output string stamp.
        // Desired format is "(base name of this form of Hookshot): [(severity)]"
        outputString << Strings::kStrHookshotBaseName << L": [" << StampCharacterForSeverity(severity) << L"] ";

        // Append the message itself.
        outputString << message << L"\n";

        // Output to the debugger.
        OutputDebugString(outputString.str().c_str());
    }

    // --------

    void Message::OutputInternalUsingLogFile(const EMessageSeverity severity, const wchar_t* message)
    {
        std::wstringstream outputString;
        
        // First compose the output string stamp.
        // Desired format is "[(current date) (current time)] [(severity)]"
        outputString << L'[';

        TemporaryBuffer<wchar_t> bufferTimestamp;

        if (0 != GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, NULL, L"MM'/'dd'/'yyyy", bufferTimestamp, bufferTimestamp.Count(), NULL))
            outputString << (wchar_t*)bufferTimestamp;
        else
            outputString << L"(date not available)";

        if (0 != GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, NULL, L"HH':'mm':'ss", bufferTimestamp, bufferTimestamp.Count()))
            outputString << L' ' << (wchar_t*)bufferTimestamp;
        else
            outputString << L" (time not available)";

        // Finish up the stamp and append the message itself.
        outputString << L"] [" << StampCharacterForSeverity(severity) << "] " << message << L'\n';

        // Write to the log file.
        fputws(outputString.str().c_str(), logFileHandle);
    }

    // --------
    
    void Message::OutputInternalUsingMessageBox(const EMessageSeverity severity, const wchar_t* message)
    {
        TemporaryBuffer<wchar_t> productNameBuf;
        if (0 == LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, (wchar_t*)productNameBuf, productNameBuf.Count()))
            productNameBuf[0] = L'\0';

        UINT messageBoxType = 0;

        switch (severity)
        {
        case EMessageSeverity::MessageSeverityForcedInteractiveError:
        case EMessageSeverity::MessageSeverityError:
            messageBoxType = MB_ICONERROR;
            break;

        case EMessageSeverity::MessageSeverityForcedInteractiveWarning:
        case EMessageSeverity::MessageSeverityWarning:
            messageBoxType = MB_ICONWARNING;
            break;

        case EMessageSeverity::MessageSeverityForcedInteractiveInfo:
        case EMessageSeverity::MessageSeverityInfo:
            messageBoxType = MB_ICONINFORMATION;
            break;

        default:
            messageBoxType = MB_OK;
            break;
        }

        MessageBox(NULL, message, productNameBuf, messageBoxType);
    }

    // --------

    int Message::SelectOutputModes(const EMessageSeverity severity, EMessageOutputMode* selectedOutputModes)
    {
        int numOutputModes = 0;
        
        if (IsSeverityForcedInteractive(severity))
        {
            // If the severity level is forced interactive, then the only output mode is interactive.
            // Therefore, an interactive message box is the only output mode used.

            selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeMessageBox;
        }
        else if (IsDebuggerPresent())
        {
            // If a debugger is present, #WillOutputMessageOfSeverity will always return `true`.
            // The goal is tn ensure that debug strings are sent for all messages irrespective of severity (assuming they are not forced interactive).
            // For other configured output modes, it is necessary to filter based on severity.

            selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeDebugString;

            if (severity <= minimumSeverityForOutput)
            {
                if (IsLogFileEnabled())
                    selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputLogFile;
            }
        }
        else
        {
            // Since a debugger is not present, #WillOutputMessageOfSeverity has already validated that the severity of the message justifies outputting it.
            // It is therefore sufficient just to pick appropriate output modes depending on message subsystem configuration.
            // Prefer a log file if enabled, otherwise display an interactive message box.

            if (IsLogFileEnabled())
                selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputLogFile;
            else
                selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeMessageBox;
        }
        
        return numOutputModes;
    }
}
