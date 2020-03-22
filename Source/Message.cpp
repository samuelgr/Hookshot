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
#include "UnicodeTypes.h"

#include <cstdarg>
#include <cstdio>
#include <psapi.h>
#include <shlobj.h>
#include <string>


namespace Hookshot
{
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
            if (0 != _tfopen_s(&logFileHandle, Strings::kStrHookshotLogFilename.data(), _T("w")))
            {
                logFileHandle = NULL;
                OutputFormatted(EMessageSeverity::MessageSeverityError, _T("%s - Unable to create log file."), Strings::kStrHookshotLogFilename.data());
                return;
            }

            // Log file header part 1: Hookshot product name.
            TemporaryBuffer<TCHAR> hookshotProductName;
            if (0 != LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, hookshotProductName, hookshotProductName.Count()))
            {
                _fputts(hookshotProductName, logFileHandle);
                _fputts(_T("\n"), logFileHandle);
            }

            // Log file header part 2: Executable file name.
            _fputts(Strings::kStrExecutableCompleteFilename.data(), logFileHandle);
            _fputts(_T("\n"), logFileHandle);

            // Log file header part 3: Separator
            _fputts(_T("-------------------------\n"), logFileHandle);
        }
    }

    // --------

    void Message::Output(const EMessageSeverity severity, LPCTSTR message)
    {
        if (false == WillOutputMessageOfSeverity(severity))
            return;

        OutputInternal(severity, message);
    }

    // ---------

    void Message::OutputFormatted(const EMessageSeverity severity, LPCTSTR format, ...)
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
        if ((severity < EMessageSeverity::MessageSeverityForcedBoundaryValue) || (severity <= minimumSeverityForOutput))
        {
            if ((severity >= kMaximumSeverityToRequireNonInteractiveOutput) && (true == IsOutputModeInteractive(SelectOutputMode(severity))))
                return false;
            else
                return true;
        }
        else
            return false;
    }


    // -------- HELPERS ---------------------------------------------------- //
    // See "Message.h" for documentation.

    void Message::OutputFormattedInternal(const EMessageSeverity severity, LPCTSTR format, va_list args)
    {
        TemporaryBuffer<TCHAR> messageBuf;

        _vstprintf_s(messageBuf, messageBuf.Count(), format, args);
        OutputInternal(severity, messageBuf);
    }

    // --------

    void Message::OutputInternal(const EMessageSeverity severity, LPCTSTR message)
    {
        switch (SelectOutputMode(severity))
        {
        case EMessageOutputMode::MessageOutputModeDebugString:
            OutputInternalUsingDebugString(severity, message);
            break;

        case EMessageOutputMode::MessageOutputModeMessageBox:
            OutputInternalUsingMessageBox(severity, message);
            break;

        default:
            break;
        }
    }

    // --------

    void Message::OutputInternalUsingDebugString(const EMessageSeverity severity, LPCTSTR message)
    {
        TemporaryBuffer<TCHAR> moduleBaseNameBuf;
        GetModuleBaseName(GetCurrentProcess(), Globals::GetInstanceHandle(), moduleBaseNameBuf, moduleBaseNameBuf.Count());

        TCHAR messageStampSeverity;
        switch (severity)
        {
        case EMessageSeverity::MessageSeverityForcedError:
        case EMessageSeverity::MessageSeverityError:
            messageStampSeverity = _T('E');
            break;

        case EMessageSeverity::MessageSeverityForcedWarning:
        case EMessageSeverity::MessageSeverityWarning:
            messageStampSeverity = _T('W');
            break;

        case EMessageSeverity::MessageSeverityForcedInfo:
        case EMessageSeverity::MessageSeverityInfo:
            messageStampSeverity = _T('I');
            break;

        case EMessageSeverity::MessageSeverityDebug:
            messageStampSeverity = _T('D');
            break;

        default:
            messageStampSeverity = _T('?');
            break;
        }

        TemporaryBuffer<TCHAR> messageStampBuf;
        _stprintf_s(messageStampBuf, messageStampBuf.Count(), _T("%s: [%c] "), (TCHAR*)moduleBaseNameBuf, messageStampSeverity);

        OutputDebugString(messageStampBuf);
        OutputDebugString(message);
        OutputDebugString(_T("\n"));
    }

    // --------

    void Message::OutputInternalUsingLogFile(const EMessageSeverity severity, LPCTSTR message)
    {
        // TODO: fill this in.
    }

    // --------
    
    void Message::OutputInternalUsingMessageBox(const EMessageSeverity severity, LPCTSTR message)
    {
        TemporaryBuffer<TCHAR> productNameBuf;
        if (0 == LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, (TCHAR*)productNameBuf, productNameBuf.Count()))
            productNameBuf[0] = _T('\0');

        UINT messageBoxType = 0;

        switch (severity)
        {
        case EMessageSeverity::MessageSeverityForcedError:
        case EMessageSeverity::MessageSeverityError:
            messageBoxType = MB_ICONERROR;
            break;

        case EMessageSeverity::MessageSeverityForcedWarning:
        case EMessageSeverity::MessageSeverityWarning:
            messageBoxType = MB_ICONWARNING;
            break;

        case EMessageSeverity::MessageSeverityForcedInfo:
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

    EMessageOutputMode Message::SelectOutputMode(const EMessageSeverity severity)
    {
        if (IsSeverityForced(severity))
            return EMessageOutputMode::MessageOutputModeMessageBox;
        else if (IsDebuggerPresent())
            return EMessageOutputMode::MessageOutputModeDebugString;
        else if (IsLogFileEnabled())
            return EMessageOutputMode::MessageOutputLogFile;
        else
            return EMessageOutputMode::MessageOutputModeMessageBox;
    }
}
