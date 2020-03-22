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
#include <string>
#include <sstream>


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
            if (0 != _wfopen_s(&logFileHandle, Strings::kStrHookshotLogFilename.data(), L"w"))
            {
                logFileHandle = NULL;
                OutputFormatted(EMessageSeverity::MessageSeverityError, L"%s - Unable to create log file.", Strings::kStrHookshotLogFilename.data());
                return;
            }

            // Log file header part 1: Hookshot product name.
            TemporaryBuffer<wchar_t> hookshotProductName;
            if (0 != LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, hookshotProductName, hookshotProductName.Count()))
            {
                fputws(hookshotProductName, logFileHandle);
                fputws(L" Log\n", logFileHandle);
            }

            // Log file header part 2: Executable file name.
            fputws(Strings::kStrExecutableCompleteFilename.data(), logFileHandle);
            fputws(L"\n", logFileHandle);

            // Log file header part 3: Separator
            fputws(L"-------------------------\n", logFileHandle);
        }
    }

    // --------

    void Message::Output(const EMessageSeverity severity, LPCWSTR message)
    {
        if (false == WillOutputMessageOfSeverity(severity))
            return;

        OutputInternal(severity, message);
    }

    // ---------

    void Message::OutputFormatted(const EMessageSeverity severity, LPCWSTR format, ...)
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

    void Message::OutputFormattedInternal(const EMessageSeverity severity, LPCWSTR format, va_list args)
    {
        TemporaryBuffer<wchar_t> messageBuf;

        vswprintf_s(messageBuf, messageBuf.Count(), format, args);
        OutputInternal(severity, messageBuf);
    }

    // --------

    void Message::OutputInternal(const EMessageSeverity severity, LPCWSTR message)
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

    void Message::OutputInternalUsingDebugString(const EMessageSeverity severity, LPCWSTR message)
    {
        TemporaryBuffer<wchar_t> moduleBaseNameBuf;
        GetModuleBaseName(GetCurrentProcess(), Globals::GetInstanceHandle(), moduleBaseNameBuf, moduleBaseNameBuf.Count());

        const wchar_t messageStampSeverity = StampCharacterForSeverity(severity);

        TemporaryBuffer<wchar_t> messageStampBuf;
        swprintf_s(messageStampBuf, messageStampBuf.Count(), L"%s: [%c] ", (wchar_t*)moduleBaseNameBuf, messageStampSeverity);

        OutputDebugString(messageStampBuf);
        OutputDebugString(message);
        OutputDebugString(L"\n");
    }

    // --------

    void Message::OutputInternalUsingLogFile(const EMessageSeverity severity, LPCWSTR message)
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
    
    void Message::OutputInternalUsingMessageBox(const EMessageSeverity severity, LPCWSTR message)
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
            selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeMessageBox;
        }
        else if (IsDebuggerPresent())
        {
            selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeDebugString;

            if (IsLogFileEnabled())
                selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputLogFile;
        }
        else
        {
            if (IsLogFileEnabled())
                selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputLogFile;
            else
                selectedOutputModes[numOutputModes++] = EMessageOutputMode::MessageOutputModeMessageBox;
        }
        
        return numOutputModes;
    }
}
