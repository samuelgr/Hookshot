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
#include "DependencyProtect.h"
#include "Globals.h"
#include "TemporaryBuffer.h"

#include <cstdlib>
#include <intrin.h>
#include <psapi.h>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <string_view>


// -------- MACROS --------------------------------------------------------- //

/// Defines and initializes a string, given a string name and an initializer.
/// Used to create an internal string object and export it as a string_view object, all in one step.
#define DEFINE_STRING(name, initializer) \
    static const std::wstring name##Impl(initializer); \
    extern const std::wstring_view name(name##Impl);


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

        /// File extension for all files whose presence would be checked to determine if Hookshot is authorized to act on a process.
        static constexpr std::wstring_view kStrAuthorizationFileExtension = L".hookshot";


        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Generates the value for kStrProductName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetProductName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, (wchar_t*)buf, buf.Count());

            return (std::wstring(buf));
        }

        /// Generates the base name of the current running form of Hookshot, minus the extension.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "Hookshot"
        /// @return Base name without extension.
        static std::wstring GetHookshotBaseNameWithoutExtension(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* hookshotBaseName = wcsrchr(buf, L'\\');
            if (nullptr == hookshotBaseName)
                hookshotBaseName = buf;
            else
                hookshotBaseName += 1;

            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            wchar_t* const lastDot = wcsrchr(hookshotBaseName, L'.');

            if (nullptr == lastDot)
                return (std::wstring(hookshotBaseName));

            *lastDot = L'\0';

            wchar_t* const secondLastDot = wcsrchr(hookshotBaseName, L'.');

            if (nullptr == secondLastDot)
                return (std::wstring(hookshotBaseName));

            *secondLastDot = L'\0';

            return (std::wstring(hookshotBaseName));
        }

        /// Generates the fully-qualified path of the current running form of Hookshot, minus the extension.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "C:\Directory\Program\Hookshot"
        /// @return Fully-qualified base path.
        static std::wstring GetHookshotCompleteFilenameWithoutExtension(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            wchar_t* const lastDot = wcsrchr(buf, L'.');

            if (nullptr == lastDot)
                return (std::wstring(buf));

            *lastDot = L'\0';

            wchar_t* const secondLastDot = wcsrchr(buf, L'.');

            if (nullptr == secondLastDot)
                return (std::wstring(buf));

            *secondLastDot = L'\0';

            return (std::wstring(buf));
        }

        /// Generates the value for kStrExecutableBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableBaseName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(nullptr, buf, (DWORD)buf.Count());

            wchar_t* executableBaseName = wcsrchr(buf, L'\\');
            if (nullptr == executableBaseName)
                executableBaseName = buf;
            else
                executableBaseName += 1;

            return (std::wstring(executableBaseName));
        }

        /// Generates the value for kStrExecutableDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableDirectoryName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(nullptr, buf, (DWORD)buf.Count());

            wchar_t* const lastBackslash = wcsrchr(buf, L'\\');
            if (nullptr == lastBackslash)
                buf[0] = L'\0';
            else
                lastBackslash[1] = L'\0';

            return (std::wstring(buf));
        }

        /// Generates the value for kStrExecutableCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetExecutableCompleteFilename(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(nullptr, buf, (DWORD)buf.Count());

            return (std::wstring(buf));
        }

        /// Generates the value for kStrHookshotBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotBaseName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* hookshotBaseName = wcsrchr(buf, L'\\');
            if (nullptr == hookshotBaseName)
                hookshotBaseName = buf;
            else
                hookshotBaseName += 1;

            return (std::wstring(hookshotBaseName));
        }

        /// Generates the value for kStrHookshotDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotDirectoryName(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            wchar_t* const lastBackslash = wcsrchr(buf, L'\\');
            if (nullptr == lastBackslash)
                buf[0] = L'\0';
            else
                lastBackslash[1] = L'\0';

            return (std::wstring(buf));
        }

        /// Generates the value for kStrHookshotCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotCompleteFilename(void)
        {
            TemporaryBuffer<wchar_t> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            return (std::wstring(buf));
        }

        /// Generates the value for kStrHookshotConfigurationFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotConfigurationFilename(void)
        {
            return GetExecutableDirectoryName() + GetHookshotBaseNameWithoutExtension() + kStrHookshotConfigurationFileExtension.data();
        }

        /// Generates the value for kStrHookshotLogFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static std::wstring GetHookshotLogFilename(void)
        {
            std::wstringstream logFilename;

            PWSTR knownFolderPath;
            const HRESULT result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &knownFolderPath);

            if (S_OK == result)
            {
                logFilename << knownFolderPath << L'\\';
                CoTaskMemFree(knownFolderPath);
            }

            logFilename << GetHookshotBaseNameWithoutExtension() << L'_' << GetExecutableBaseName() << L'_' << Globals::GetCurrentProcessId() << kStrHookshotLogFileExtension;

            return logFilename.str();
        }

        /// Generates the value for kStrHookshotDynamicLinkLibraryFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotDynamicLinkLibraryFilename(void)
        {
#ifdef HOOKSHOT_LAUNCHER
            return GetExecutableDirectoryName() + GetProductName() + kStrHookshotDynamicLinkLibraryExtension.data();
#else
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotDynamicLinkLibraryExtension.data();
#endif
        }

        /// Generates the value for kStrHookshotExecutableFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotExecutableFilename(void)
        {
#ifdef HOOKSHOT_LAUNCHER
            return GetExecutableDirectoryName() + GetProductName() + kStrHookshotExecutableExtension.data();
#else
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotExecutableExtension.data();
#endif
        }

        /// Generates the value for kStrHookshotExecutableOtherArchitectureFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static std::wstring GetHookshotExecutableOtherArchitectureFilename(void)
        {
#ifdef HOOKSHOT_LAUNCHER
            return GetExecutableDirectoryName() + GetProductName() + kStrHookshotExecutableOtherArchitectureExtension.data();
#else
            return GetHookshotCompleteFilenameWithoutExtension() + kStrHookshotExecutableOtherArchitectureExtension.data();
#endif
        }


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // See "Strings.h" for documentation.

        DEFINE_STRING(kStrProductName, GetProductName());
        DEFINE_STRING(kStrExecutableBaseName, GetExecutableBaseName());
        DEFINE_STRING(kStrExecutableDirectoryName, GetExecutableDirectoryName());
        DEFINE_STRING(kStrExecutableCompleteFilename, GetExecutableCompleteFilename());
        DEFINE_STRING(kStrHookshotBaseName, GetHookshotBaseName());
        DEFINE_STRING(kStrHookshotDirectoryName, GetHookshotDirectoryName());
        DEFINE_STRING(kStrHookshotCompleteFilename, GetHookshotCompleteFilename());
        DEFINE_STRING(kStrHookshotConfigurationFilename, GetHookshotConfigurationFilename());
        DEFINE_STRING(kStrHookshotLogFilename, GetHookshotLogFilename());
        DEFINE_STRING(kStrHookshotDynamicLinkLibraryFilename, GetHookshotDynamicLinkLibraryFilename());
        DEFINE_STRING(kStrHookshotExecutableFilename, GetHookshotExecutableFilename());
        DEFINE_STRING(kStrHookshotExecutableOtherArchitectureFilename, GetHookshotExecutableOtherArchitectureFilename());


        // -------- FUNCTIONS ---------------------------------------------- //
        // See "Strings.h" for documentation.

        std::wstring AuthorizationFilenameApplicationSpecific(std::wstring_view executablePath)
        {
            std::wstring authorizationFilename;

            authorizationFilename.reserve(1 + executablePath.length() + kStrAuthorizationFileExtension.length());
            authorizationFilename.append(executablePath);
            authorizationFilename.append(kStrAuthorizationFileExtension);

            return authorizationFilename;
        }

        // --------

        std::wstring AuthorizationFilenameDirectoryWide(std::wstring_view executablePath)
        {
            std::wstring authorizationFilename;

            const wchar_t* const lastBackslash = wcsrchr(executablePath.data(), L'\\');
            if (nullptr == lastBackslash)
            {
                authorizationFilename.reserve(1 + kStrAuthorizationFileExtension.length());
                authorizationFilename.append(kStrAuthorizationFileExtension);
            }
            else
            {
                authorizationFilename.reserve(1 + executablePath.length() + kStrAuthorizationFileExtension.length());
                authorizationFilename.append(&executablePath[0], &lastBackslash[1]);
                authorizationFilename.append(kStrAuthorizationFileExtension);
            }
            
            return authorizationFilename;
        }

        // --------

        std::wstring HookModuleFilename(std::wstring_view moduleName)
        {
            std::wstring hookModuleFilename;

            hookModuleFilename.reserve(1 + kStrExecutableDirectoryName.length() + moduleName.length() + kStrHookModuleExtension.length());
            hookModuleFilename.append(kStrExecutableDirectoryName);
            hookModuleFilename.append(moduleName);
            hookModuleFilename.append(kStrHookModuleExtension);

            return hookModuleFilename;
        }

        // --------

        std::wstring SystemErrorCodeString(const unsigned long systemErrorCode)
        {
            TemporaryBuffer<wchar_t> systemErrorString;
            DWORD systemErrorLength = Protected::Windows_FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, systemErrorCode, 0, systemErrorString, systemErrorString.Count(), nullptr);

            if (0 == systemErrorLength)
            {
                swprintf_s(systemErrorString, systemErrorString.Count(), L"System error %u", (unsigned int)systemErrorCode);
            }
            else
            {
                for (; systemErrorLength > 0; --systemErrorLength)
                {
                    if ((L'\0' != systemErrorString[systemErrorLength]) && (L'.' != systemErrorString[systemErrorLength]) && !iswspace(systemErrorString[systemErrorLength]))
                        break;

                    systemErrorString[systemErrorLength] = L'\0';
                }
            }

            return std::wstring(systemErrorString);
        }
    }
}
