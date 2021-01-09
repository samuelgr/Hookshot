/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2021
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
#include <mutex>
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

        /// File extension for all files whose presence would be checked to determine if Hookshot is authorized to act on a process.
        static constexpr std::wstring_view kStrAuthorizationFileExtension = L".hookshot";


        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Generates the value for kStrProductName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetProductName(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                const wchar_t* stringStart = nullptr;
                int stringLength = LoadString(Globals::GetInstanceHandle(), IDS_HOOKSHOT_PRODUCT_NAME, (wchar_t*)&stringStart, 0);

                while ((stringLength > 0) && (L'\0' == stringStart[stringLength - 1]))
                    stringLength -= 1;

                if (stringLength > 0)
                    initString.assign(stringStart, &stringStart[stringLength]);
            });

            return initString;
        }

        /// Generates the value for kStrExecutableCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetExecutableCompleteFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                TemporaryBuffer<wchar_t> buf;
                GetModuleFileName(nullptr, buf, (DWORD)buf.Count());

                initString.assign(buf);
            });

            return initString;
        }

        /// Generates the value for kStrExecutableBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetExecutableBaseName(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view executableBaseName = GetExecutableCompleteFilename();

                const size_t lastBackslashPos = executableBaseName.find_last_of(L"\\");
                if (std::wstring_view::npos != lastBackslashPos)
                    executableBaseName.remove_prefix(1 + lastBackslashPos);

                initString.assign(executableBaseName);
            });

            return initString;
        }

        /// Generates the value for kStrExecutableDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetExecutableDirectoryName(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view executableDirectoryName = GetExecutableCompleteFilename();

                const size_t lastBackslashPos = executableDirectoryName.find_last_of(L"\\");
                if (std::wstring_view::npos != lastBackslashPos)
                {
                    executableDirectoryName.remove_suffix(executableDirectoryName.length() - lastBackslashPos - 1);
                    initString.assign(executableDirectoryName);
                }
            });

            return initString;
        }

        /// Generates the value for kStrHookshotCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotCompleteFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                TemporaryBuffer<wchar_t> buf;
                GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

                initString.assign(buf);
            });

            return initString;
        }

        /// Generates the value for kStrHookshotBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotBaseName(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view hookshotBaseName = GetHookshotCompleteFilename();

                const size_t lastBackslashPos = hookshotBaseName.find_last_of(L"\\");
                if (std::wstring_view::npos != lastBackslashPos)
                    hookshotBaseName.remove_prefix(1 + lastBackslashPos);

                initString.assign(hookshotBaseName);
            });

            return initString;
        }

        /// Generates the value for kStrHookshotDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotDirectoryName(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view hookshotDirectoryName = GetHookshotCompleteFilename();

                const size_t lastBackslashPos = hookshotDirectoryName.find_last_of(L"\\");
                if (std::wstring_view::npos != lastBackslashPos)
                {
                    hookshotDirectoryName.remove_suffix(hookshotDirectoryName.length() - lastBackslashPos - 1);
                    initString.assign(hookshotDirectoryName);
                }
            });

            return initString;
        }

        /// Generates the value for kStrHookshotConfigurationFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotConfigurationFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view pieces[] = {GetExecutableDirectoryName(), GetProductName(), kStrHookshotConfigurationFileExtension};

                size_t totalLength = 0;
                for (int i = 0; i < _countof(pieces); ++i)
                    totalLength += pieces[i].length();

                initString.reserve(1 + totalLength);

                for (int i = 0; i < _countof(pieces); ++i)
                    initString.append(pieces[i]);
            });

            return initString;
        }

        /// Generates the value for kStrHookshotLogFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotLogFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstringstream logFilename;

                PWSTR knownFolderPath;
                const HRESULT result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &knownFolderPath);

                if (S_OK == result)
                {
                    logFilename << knownFolderPath << L'\\';
                    CoTaskMemFree(knownFolderPath);
                }

                logFilename << GetProductName() << L'_' << GetExecutableBaseName() << L'_' << Globals::GetCurrentProcessId() << kStrHookshotLogFileExtension;

                initString.assign(logFilename.str());
            });

            return initString;
        }

        /// Generates the value for kStrHookshotDynamicLinkLibraryFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotDynamicLinkLibraryFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view pieces[] = {GetHookshotDirectoryName(), GetProductName(), kStrHookshotDynamicLinkLibraryExtension};

                size_t totalLength = 0;
                for (int i = 0; i < _countof(pieces); ++i)
                    totalLength += pieces[i].length();

                initString.reserve(1 + totalLength);

                for (int i = 0; i < _countof(pieces); ++i)
                    initString.append(pieces[i]);
            });

            return initString;
        }

        /// Generates the value for kStrHookshotExecutableFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotExecutableFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view pieces[] = {GetHookshotDirectoryName(), GetProductName(), kStrHookshotExecutableExtension};

                size_t totalLength = 0;
                for (int i = 0; i < _countof(pieces); ++i)
                    totalLength += pieces[i].length();

                initString.reserve(1 + totalLength);

                for (int i = 0; i < _countof(pieces); ++i)
                    initString.append(pieces[i]);
            });

            return initString;
        }

        /// Generates the value for kStrHookshotExecutableOtherArchitectureFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static const std::wstring& GetHookshotExecutableOtherArchitectureFilename(void)
        {
            static std::wstring initString;
            static std::once_flag initFlag;

            std::call_once(initFlag, []() -> void {
                std::wstring_view pieces[] = {GetHookshotDirectoryName(), GetProductName(), kStrHookshotExecutableOtherArchitectureExtension};

                size_t totalLength = 0;
                for (int i = 0; i < _countof(pieces); ++i)
                    totalLength += pieces[i].length();

                initString.reserve(1 + totalLength);

                for (int i = 0; i < _countof(pieces); ++i)
                    initString.append(pieces[i]);
            });

            return initString;
        }


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // See "Strings.h" for documentation.

        extern const std::wstring_view kStrProductName(GetProductName());
        extern const std::wstring_view kStrExecutableCompleteFilename(GetExecutableCompleteFilename());
        extern const std::wstring_view kStrExecutableBaseName(GetExecutableBaseName());
        extern const std::wstring_view kStrExecutableDirectoryName(GetExecutableDirectoryName());
        extern const std::wstring_view kStrHookshotCompleteFilename(GetHookshotCompleteFilename());
        extern const std::wstring_view kStrHookshotBaseName(GetHookshotBaseName());
        extern const std::wstring_view kStrHookshotDirectoryName(GetHookshotDirectoryName());
        extern const std::wstring_view kStrHookshotConfigurationFilename(GetHookshotConfigurationFilename());
        extern const std::wstring_view kStrHookshotLogFilename(GetHookshotLogFilename());
        extern const std::wstring_view kStrHookshotDynamicLinkLibraryFilename(GetHookshotDynamicLinkLibraryFilename());
        extern const std::wstring_view kStrHookshotExecutableFilename(GetHookshotExecutableFilename());
        extern const std::wstring_view kStrHookshotExecutableOtherArchitectureFilename(GetHookshotExecutableOtherArchitectureFilename());


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

            const size_t lastBackslashPos = executablePath.find_last_of(L"\\");
            if (std::wstring_view::npos == lastBackslashPos)
            {
                authorizationFilename.reserve(1 + kStrAuthorizationFileExtension.length());
                authorizationFilename.append(kStrAuthorizationFileExtension);
            }
            else
            {
                executablePath.remove_suffix(executablePath.length() - lastBackslashPos - 1);

                authorizationFilename.reserve(1 + executablePath.length() + kStrAuthorizationFileExtension.length());
                authorizationFilename.append(executablePath);
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
