/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
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
         MessageSeverityError,                                      ///< Error. Causes a change in behavior if encountered, possibly leading to application termination.
         MessageSeverityWarning,                                    ///< Warning. May cause a change in behavior but is not critical and will not terminate the application.
         MessageSeverityInfo,                                       ///< Informational. Useful status-related remarks for tracking application behavior.
         MessageSeverityDebug,                                      ///< Debug. Only output in debug mode, and then only written to an attached debugger.
     };

     /// Enumerates all supported modes of outputting messages.
     enum EMessageOutputMode
     {
         // Non-interactive output modes
         MessageOutputModeDebugString,                              ///< Message is output using a debug string, which debuggers will display.

         // Boundary value between non-interactive and interactive modes.
         MessageOutputModeInteractiveBoundaryValue,                 ///< Not used as a value, but separates non-interactive output modes from interactive output modes.

         // Interactive output modes
         MessageOutputModeMessageBox,                               ///< Message is output using a message box.
     };
     
    /// Encapsulates all message-related functionality.
    /// All methods are class methods.
    class Message
    {
    public:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Specifies the minimum severity required to output a message.
        /// Messages below this severity (i.e. above the integer value that represents this severity) are ignored.
#if HOOKSHOT_DEBUG
        static constexpr EMessageSeverity kMinimumSeverityForOutput = EMessageSeverity::MessageSeverityDebug;
#else
        static constexpr EMessageSeverity kMinimumSeverityForOutput = EMessageSeverity::MessageSeverityError;
#endif

        /// Specifies the maximum severity that requires a non-interactive mode of output be used.
        /// Messages at this severity or lower will be skipped unless a non-intneractive output mode is being used.
        static constexpr EMessageSeverity kMaximumSeverityToRequireNonInteractiveOutput = EMessageSeverity::MessageSeverityDebug;


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

        /// Determines if a message of the specified severity will be output.
        /// Compares the supplied severity level to the configured minimum severity level.
        /// @param [in] severity Severity to test.
        /// @return `true` if a message of the specified severity should be output, `false` otherwise.
        static bool WillOutputMessageOfSeverity(const EMessageSeverity severity);


    private:
        // -------- HELPERS ------------------------------------------------ //

        static inline bool IsOutputModeInteractive(const EMessageOutputMode outputMode)
        {
            return (outputMode > MessageOutputModeInteractiveBoundaryValue);
        }
        
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

        /// Determines the appropriate mode of output based on the current configuration.
        /// @return Output mode to use.
        static EMessageOutputMode SelectOutputMode(void);
    };
}
