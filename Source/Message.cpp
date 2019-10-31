/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Message.cpp
 *   Message output implementation.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Message.h"
#include "TemporaryBuffers.h"

#include <cstdarg>
#include <cstdio>
#include <psapi.h>
#include <shlobj.h>
#include <string>


namespace Hookshot
{
    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Message.h" for documentation.

    void Message::Output(const EMessageSeverity severity, LPCTSTR message)
    {
        if (false == ShouldOutputMessageOfSeverity(severity))
            return;

        OutputInternal(severity, message);
    }

    // ---------

    void Message::OutputFormatted(const EMessageSeverity severity, LPCTSTR format, ...)
    {
        if (false == ShouldOutputMessageOfSeverity(severity))
            return;

        va_list args;
        va_start(args, format);

        OutputFormattedInternal(severity, format, args);

        va_end(args);
    }

    // ---------

    void Message::OutputFromResource(const EMessageSeverity severity, const unsigned int resourceIdentifier)
    {
        if (false == ShouldOutputMessageOfSeverity(severity))
            return;

        TemporaryBuffer<TCHAR> resourceStringBuf;

        if (0 != LoadString(Globals::GetInstanceHandle(), resourceIdentifier, resourceStringBuf, resourceStringBuf.Count()))
            OutputInternal(severity, resourceStringBuf);
    }

    // ---------

    void Message::OutputFormattedFromResource(const EMessageSeverity severity, const unsigned int resourceIdentifier, ...)
    {
        if (false == ShouldOutputMessageOfSeverity(severity))
            return;

        TemporaryBuffer<TCHAR> resourceStringBuf;

        if (0 != LoadString(Globals::GetInstanceHandle(), resourceIdentifier, resourceStringBuf, resourceStringBuf.Count()))
        {
            va_list args;
            va_start(args, resourceIdentifier);

            OutputFormattedInternal(severity, resourceStringBuf, args);

            va_end(args);
        }
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
        if (IsDebuggerPresent())
        {
            OutputInternalUsingDebugString(severity, message);
        }
        else
        {
            OutputInternalUsingMessageBox(severity, message);
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
        case EMessageSeverity::MessageSeverityError:
            messageStampSeverity = _T('E');
            break;

        case EMessageSeverity::MessageSeverityWarning:
            messageStampSeverity = _T('W');
            break;

        case EMessageSeverity::MessageSeverityInfo:
            messageStampSeverity = _T('I');
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

    void Message::OutputInternalUsingMessageBox(const EMessageSeverity severity, LPCTSTR message)
    {
        TemporaryBuffer<TCHAR> productNameBuf;
        if (0 == LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, (TCHAR*)productNameBuf, productNameBuf.Count()))
            productNameBuf[0] = _T('\0');

        UINT messageBoxType = 0;

        switch (severity)
        {
        case EMessageSeverity::MessageSeverityError:
            messageBoxType = MB_ICONERROR;
            break;

        case EMessageSeverity::MessageSeverityWarning:
            messageBoxType = MB_ICONWARNING;
            break;

        case EMessageSeverity::MessageSeverityInfo:
            messageBoxType = MB_ICONINFORMATION;
            break;

        default:
            messageBoxType = MB_ICONQUESTION;
            break;
        }

        MessageBox(NULL, message, productNameBuf, messageBoxType);
    }

    // --------

    bool Message::ShouldOutputMessageOfSeverity(const EMessageSeverity severity)
    {
        return (severity <= kMinimumSeverityForOutput);
    }
}
