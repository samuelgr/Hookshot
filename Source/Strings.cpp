/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Strings.cpp
 *   Implementation of functions for manipulating Hookshot-specific strings.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstdlib>
#include <intrin.h>
#include <psapi.h>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <string_view>


namespace Hookshot
{
    namespace Strings
    {
        // -------- INTERNAL CONSTANTS ------------------------------------- //
        
        /// File extension of the dynamic-link library form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr std::wstring_view kStrHookshotDynamicLinkLibraryExtension = L".64.dll";
#else
        static constexpr std::wstring_view kStrHookshotDynamicLinkLibraryExtension = L".32.dll";
#endif

        /// File extension of the executable form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr std::wstring_view kStrHookshotExecutableExtension = L".64.exe";
#else
        static constexpr std::wstring_view kStrHookshotExecutableExtension = L".32.exe";
#endif

        /// File extension of the executable form of Hookshot but targeting the opposite processor architecture.
#ifdef HOOKSHOT64
        static constexpr std::wstring_view kStrHookshotExecutableOtherArchitectureExtension = L".32.exe";
#else
        static constexpr std::wstring_view kStrHookshotExecutableOtherArchitectureExtension = L".64.exe";
#endif

        /// File extension for a Hookshot configuration file.
        static constexpr std::wstring_view kStrHookshotConfigurationFileExtension = L".ini";

        /// File extension for a Hookshot log file.
        static constexpr std::wstring_view kStrHookshotLogFileExtension = L".log";

        /// File extension for all hook modules.
#ifdef HOOKSHOT64
        static constexpr std::wstring_view kStrHookModuleExtension = L".HookModule.64.dll";
#else
        static constexpr std::wstring_view kStrHookModuleExtension = L".HookModule.32.dll";
#endif


        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Generates the base name of the current running form of Hookshot, minus the extension.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "Hookshot"
        /// @return Base name without extension.
        static std::wstring GetHookshotBaseNameWithoutExtension(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* hookshotBaseName = wcsrchr(buf, L'\\');
            if (NULL == hookshotBaseName)
                hookshotBaseName = buf;
            else
                hookshotBaseName += 1;
            
            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            wchar_t* const lastDot = wcsrchr(hookshotBaseName, L'.');

            if (NULL == lastDot)
                return (std::wstring(hookshotBaseName));

            *lastDot = L'\0';

            wchar_t* const secondLastDot = wcsrchr(hookshotBaseName, L'.');

            if (NULL == secondLastDot)
                return (std::wstring(hookshotBaseName));

            *secondLastDot = L'\0';

            return (std::wstring(hookshotBaseName));
        }
        
        /// Generates the fully-qualified path of the current running form of Hookshot, minus the extension.
        /// This is useful for determining the correct path of the next file or module to load.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "C:\Directory\Program\Hookshot"
        /// @return Fully-qualified base path.
        static std::wstring GetHookshotCompleteFilenameWithoutExtension(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            wchar_t* const lastDot = wcsrchr(buf, L'.');

            if (NULL == lastDot)
                return (std::wstring(buf));

            *lastDot = L'\0';

            wchar_t* const secondLastDot = wcsrchr(buf, L'.');

            if (NULL == secondLastDot)
                return (std::wstring(buf));

            *secondLastDot = L'\0';

            return (std::wstring(buf));
        }
        
        /// Generates the value for #kStrExecutableBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableBaseName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            wchar_t* executableBaseName = wcsrchr(buf, L'\\');
            if (NULL == executableBaseName)
                executableBaseName = buf;
            else
                executableBaseName += 1;

            return (std::wstring(executableBaseName));
        }

        /// Generates the value for #kStrExecutableDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableDirectoryName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            wchar_t* const lastBackslash = wcsrchr(buf, L'\\');
            if (NULL == lastBackslash)
                buf[0] = L'\0';
            else
                lastBackslash[1] = L'\0';

            return (std::wstring(buf));
        }

        /// Generates the value for #kStrExecutableCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableCompleteFilename(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            return (std::wstring(buf));
        }

        /// Generates the value for #kStrHookshotBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotBaseName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* hookshotBaseName = wcsrchr(buf, L'\\');
            if (NULL == hookshotBaseName)
                hookshotBaseName = buf;
            else
                hookshotBaseName += 1;

            return (std::wstring(hookshotBaseName));
        }
        
        /// Generates the value for #kStrHookshotDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotDirectoryName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* const lastBackslash = wcsrchr(buf, L'\\');
            if (NULL == lastBackslash)
                buf[0] = L'\0';
            else
                lastBackslash[1] = L'\0';

            return (std::wstring(buf));
        }

        /// Generates the value for #kStrHookshotCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotCompleteFilename(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            return (std::wstring(buf));
        }
        
        /// Generates the value for #kStrHookshotConfigurationFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotConfigurationFilename(void)
        {
            return GetExecutableDirectoryName() + GetHookshotBaseNameWithoutExtension() + kStrHookshotConfigurationFileExtension.data();
        }

        /// Generates the value for #kStrHookshotLogFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotLogFilename(void)
        {
            std::wstringstream logFilename;
            
            PWSTR knownFolderPath;
            const HRESULT result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &knownFolderPath);

            if (S_OK == result)
            {
                logFilename << knownFolderPath << L'\\';
                CoTaskMemFree(knownFolderPath);
            }

            logFilename << GetHookshotBaseNameWithoutExtension() << L'_' << GetExecutableBaseName() << L'_' << GetProcessId(GetCurrentProcess()) << kStrHookshotLogFileExtension;

            return logFilename.str();
        }

        /// Generates the value for #kStrHookshotDynamicLinkLibraryFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotDynamicLinkLibraryFilename(void)
        {
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotDynamicLinkLibraryExtension.data();
        }

        /// Generates the value for #kStrHookshotExecutableFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotExecutableFilename(void)
        {
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotExecutableExtension.data();
        }

        /// Generates the value for #kStrHookshotExecutableOtherArchitectureFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotExecutableOtherArchitectureFilename(void)
        {
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotExecutableOtherArchitectureExtension.data();
        }


        // -------- INTERNAL CONSTANTS ------------------------------------- //
        // Used to implement run-time constants; see "Strings.h" for documentation.

        static const std::wstring kStrExecutableBaseNameImpl(GetExecutableBaseName());

        static const std::wstring kStrExecutableDirectoryNameImpl(GetExecutableDirectoryName());

        static const std::wstring kStrExecutableCompleteFilenameImpl(GetExecutableCompleteFilename());

        static const std::wstring kStrHookshotBaseNameImpl(GetHookshotBaseName());

        static const std::wstring kStrHookshotDirectoryNameImpl(GetHookshotDirectoryName());

        static const std::wstring kStrHookshotCompleteFilenameImpl(GetHookshotCompleteFilename());

        static const std::wstring kStrHookshotConfigurationFilenameImpl(GetHookshotConfigurationFilename());

        static const std::wstring kStrHookshotLogFilenameImpl(GetHookshotLogFilename());

        static const std::wstring kStrHookshotDynamicLinkLibraryFilenameImpl(GetHookshotDynamicLinkLibraryFilename());

        static const std::wstring kStrHookshotExecutableFilenameImpl(GetHookshotExecutableFilename());

        static const std::wstring kStrHookshotExecutableOtherArchitectureFilenameImpl(GetHookshotExecutableOtherArchitectureFilename());


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // See "Strings.h" for documentation.

        extern const std::wstring_view kStrExecutableBaseName(kStrExecutableBaseNameImpl);

        extern const std::wstring_view kStrExecutableDirectoryName(kStrExecutableDirectoryNameImpl);

        extern const std::wstring_view kStrExecutableCompleteFilename(kStrExecutableCompleteFilenameImpl);

        extern const std::wstring_view kStrHookshotBaseName(kStrHookshotBaseNameImpl);

        extern const std::wstring_view kStrHookshotDirectoryName(kStrHookshotDirectoryNameImpl);

        extern const std::wstring_view kStrHookshotCompleteFilename(kStrHookshotCompleteFilenameImpl);

        extern const std::wstring_view kStrHookshotConfigurationFilename(kStrHookshotConfigurationFilenameImpl);

        extern const std::wstring_view kStrHookshotLogFilename(kStrHookshotLogFilenameImpl);
        
        extern const std::wstring_view kStrHookshotDynamicLinkLibraryFilename(kStrHookshotDynamicLinkLibraryFilenameImpl);

        extern const std::wstring_view kStrHookshotExecutableFilename(kStrHookshotExecutableFilenameImpl);

        extern const std::wstring_view kStrHookshotExecutableOtherArchitectureFilename(kStrHookshotExecutableOtherArchitectureFilenameImpl);


        
        // -------- FUNCTIONS ---------------------------------------------- //
        // See "Strings.h" for documentation.
        
        std::wstring MakeHookModuleFilename(std::wstring_view moduleName)
        {
            return kStrExecutableDirectoryNameImpl + moduleName.data() + kStrHookModuleExtension.data();
        }
    }
}
