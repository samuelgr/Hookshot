/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Message.h
 *   Message output interface declaration.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"

#include <cstdarg>
#include <cstddef>


 namespace Hookshot
{
     /// Enumerates all supported severity levels for messages.
     /// These are primarily used to assist with output formatting.
     enum EMessageSeverity
     {
         MessageSeverityError = 0,                                   ///< Error. Causes a change in behavior if encountered, possibly leading to application termination.
         MessageSeverityWarning = 1,                                 ///< Warning. May cause a change in behavior but is not critical and will not terminate the application.
         MessageSeverityInfo = 2,                                    ///< Informational. Useful status-related remarks for tracking application behavior.
     };
     
    /// Encapsulates all message-related functionality.
    /// All methods are class methods.
    class Message
    {
    private:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Buffer size, in characters, for the temporary buffer to hold strings read from a resource identifier and formatted.
        /// When writing log messages using a resource identifier (rather than a raw string), a temporary buffer is created to hold the loaded resource string.
        /// Similarly, when formatting messages, a temporary buffer is created to hold the output formatted string.
        static const size_t kMessageBufferSize = 2048;


        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Specifies the minimum severity required to output a message.
        /// Messages below this severity are ignored.
        static const EMessageSeverity kMinimumSeverityForOutput;

        
    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Message(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //
        
        /// Outputs the specified message.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void Output(const EMessageSeverity severity, LPCTSTR message);
        
        /// Formats and outputs the specified message.
        /// Requires a severity, a message string with standard format specifiers, and values to be formatted.
        /// @param [in] severity Severity of the message.
        /// @param [in] format Message string, possibly with format specifiers.
        static void OutputFormatted(const EMessageSeverity severity, LPCTSTR format, ...);

        /// Outputs the specified message.
        /// Requires both a severity and a resource identifier, which identifies the string resource that contains the message to be output.
        /// @param [in] severity Severity of the message.
        /// @param [in] resourceIdentifier String resource identifier from which the message text should be loaded.
        static void OutputFromResource(const EMessageSeverity severity, const unsigned int resourceIdentifier);
        
        /// Formats and outputs the specified message.
        /// Requires a severity, a resource identifier that corresponds to the format string, and values to be formatted.
        /// @param [in] severity Severity of the message.
        /// @param [in] resourceIdentifier String resource identifier from which the message string, possibly with format specifiers, should be loaded.
        static void OutputFormattedFromResource(const EMessageSeverity severity, const unsigned int resourceIdentifier, ...);


    private:
        // -------- HELPERS ------------------------------------------------ //

        /// Formats and outputs some text of the given severity.
        /// @param [in] severity Severity of the message.
        /// @param [in] format Message string, possibly with format specifiers.
        /// @param [in] args Variable-length list of arguments to be used for any format specifiers in the message string.
        static void OutputFormattedInternal(const EMessageSeverity severity, LPCTSTR format, va_list args);

        /// Outputs the specified message.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void OutputInternal(const EMessageSeverity severity, LPCTSTR message);
        
        /// Outputs the specified message using a debug string.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void OutputInternalUsingDebugString(const EMessageSeverity severity, LPCTSTR message);

        /// Outputs the specified message using a graphical message box.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void OutputInternalUsingMessageBox(const EMessageSeverity severity, LPCTSTR message);

        /// Determines if a message of the specified severity should be output.
        /// Compares the supplied severity level to the configured minimum severity level.
        /// @param [in] severity Severity to test.
        /// @return `true` if a message of the specified severity should be output, `false` otherwise.
        static bool ShouldOutputMessageOfSeverity(const EMessageSeverity severity);
    };
}
