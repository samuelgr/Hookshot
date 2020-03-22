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
#include "UnicodeTypes.h"

#include <cstdarg>
#include <cstddef>
#include <cstdio>


namespace Hookshot
{
     /// Enumerates all supported severity levels for messages.
     /// These are primarily used to assist with output formatting.
     enum class EMessageSeverity
     {
         // Forced output.
         MessageSeverityForcedError,                                ///< Error, always output interactively.
         MessageSeverityForcedWarning,                              ///< Warning, always output interactively.
         MessageSeverityForcedInfo,                                 ///< Informational, always output interactively.

         // Boundary value between forced and optional output.
         MessageSeverityForcedBoundaryValue,                        ///< Not used as a value, but separates forced output from optional output.

         // Optional output.
         MessageSeverityError,                                      ///< Error. Causes a change in behavior if encountered, possibly leading to application termination.
         MessageSeverityWarning,                                    ///< Warning. May cause a change in behavior but is not critical and will not terminate the application.
         MessageSeverityInfo,                                       ///< Informational. Useful status-related remarks for tracking application behavior.
         MessageSeverityDebug,                                      ///< Debug. Only output in debug mode, and then only written to an attached debugger.
     };

     /// Enumerates all supported modes of outputting messages.
     enum class EMessageOutputMode
     {
         // Non-interactive output modes
         MessageOutputModeDebugString,                              ///< Message is output using a debug string, which debuggers will display.
         MessageOutputLogFile,                                      ///< Message is output to a log file.

         // Boundary value between non-interactive and interactive modes.
         MessageOutputModeInteractiveBoundaryValue,                 ///< Not used as a value, but separates non-interactive output modes from interactive output modes.

         // Interactive output modes
         MessageOutputModeMessageBox,                               ///< Message is output using a message box.
     };
     
    /// Encapsulates all message-related functionality.
    /// All methods are class methods.
    class Message
    {
    private:
        // -------- CONSTANTS ---------------------------------------------- //

        /// Specifies the maximum severity that requires a non-interactive mode of output be used.
        /// Messages at this severity or lower will be skipped unless a non-intneractive output mode is being used.
        static constexpr EMessageSeverity kMaximumSeverityToRequireNonInteractiveOutput = EMessageSeverity::MessageSeverityDebug;


        // -------- CLASS VARIABLES ---------------------------------------- //

        /// Handle to the log file, if enabled.
        static FILE* logFileHandle;
        
        /// Specifies the minimum severity required to output a message.
        /// Messages below this severity (i.e. above the integer value that represents this severity) are ignored.
        /// Default values are different between debug and release configurations but in either case can be overridden at runtime.
        static EMessageSeverity minimumSeverityForOutput;


    public:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        Message(void) = delete;


        // -------- CLASS METHODS ------------------------------------------ //
        
        /// Attempts to create and enable the log file.
        /// Will generate an error message on failure.
        /// Once logging to a file is enabled, it cannot be disabled.
        static void CreateAndEnableLogFile(void);
        
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

        /// Sets the minimum message severity required for a message to be output.
        /// Ineffective if the input is invalid.
        /// @param [in] severity New minimum severity setting.
        static inline void SetMinimumSeverityForOutput(EMessageSeverity severity)
        {
            if (severity > EMessageSeverity::MessageSeverityForcedBoundaryValue)
                minimumSeverityForOutput = severity;
        }
        
        /// Determines if a message of the specified severity will be output.
        /// Compares the supplied severity level to the configured minimum severity level.
        /// @param [in] severity Severity to test.
        /// @return `true` if a message of the specified severity should be output, `false` otherwise.
        static bool WillOutputMessageOfSeverity(const EMessageSeverity severity);


    private:
        // -------- HELPERS ------------------------------------------------ //

        /// Checks if logging to a file is enabled.
        /// @return `true` if so, `false` if not.
        static inline bool IsLogFileEnabled(void)
        {
            return (NULL != logFileHandle);
        }
        
        /// Checks if the specified output mode is interactive or non-interactive.
        /// @return `true` if the mode is interactive, `false` otherwise.
        static inline bool IsOutputModeInteractive(const EMessageOutputMode outputMode)
        {
            return (outputMode > EMessageOutputMode::MessageOutputModeInteractiveBoundaryValue);
        }

        /// Checks if the specified severity is forced (i.e. one of the elements that will always cause a message to be emitted).
        /// @return `true` if the severity is forced, `false` otherwise.
        static inline bool IsSeverityForced(const EMessageSeverity severity)
        {
            return (severity < EMessageSeverity::MessageSeverityForcedBoundaryValue);
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

        /// Outputs the specified message to the log file.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void OutputInternalUsingLogFile(const EMessageSeverity severity, LPCTSTR message);

        /// Outputs the specified message using a graphical message box.
        /// Requires both a severity and a message string.
        /// @param [in] severity Severity of the message.
        /// @param [in] message Message text.
        static void OutputInternalUsingMessageBox(const EMessageSeverity severity, LPCTSTR message);

        /// Determines the appropriate mode of output based on the current configuration and message severity.
        /// @param [in] severity Severity of the message for which an output mode is being chosen.
        /// @return Output mode to use.
        static EMessageOutputMode SelectOutputMode(const EMessageSeverity severity);
    };
}
