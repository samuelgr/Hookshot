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
#include "UnicodeTypes.h"

#include <cstddef>
#include <cstdlib>
#include <intrin.h>
#include <psapi.h>
#include <shlobj.h>


namespace Hookshot
{
    namespace Strings
    {
        // -------- INTERNAL CONSTANTS ------------------------------------- //
        
        /// File extension of the dynamic-link library form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr TStdStringView kStrHookshotDynamicLinkLibraryExtension = _T(".64.dll");
#else
        static constexpr TStdStringView kStrHookshotDynamicLinkLibraryExtension = _T(".32.dll");
#endif

        /// File extension of the executable form of Hookshot.
#ifdef HOOKSHOT64
        static constexpr TStdStringView kStrHookshotExecutableExtension = _T(".64.exe");
#else
        static constexpr TStdStringView kStrHookshotExecutableExtension = _T(".32.exe");
#endif

        /// File extension of the executable form of Hookshot but targeting the opposite processor architecture.
#ifdef HOOKSHOT64
        static constexpr TStdStringView kStrHookshotExecutableOtherArchitectureExtension = _T(".32.exe");
#else
        static constexpr TStdStringView kStrHookshotExecutableOtherArchitectureExtension = _T(".64.exe");
#endif

        /// File extension for a Hookshot configuration file.
        static constexpr TStdStringView kStrHookshotConfigurationFileExtension = _T(".ini");

        /// File extension for a Hookshot log file.
        static constexpr TStdStringView kStrHookshotLogFileExtension = _T(".log");

        /// File extension for all hook modules.
#ifdef HOOKSHOT64
        static constexpr TStdStringView kStrHookModuleExtension = _T(".HookModule.64.dll");
#else
        static constexpr TStdStringView kStrHookModuleExtension = _T(".HookModule.32.dll");
#endif


        // -------- INTERNAL FUNCTIONS ------------------------------------- //

        /// Generates the directory name of the current running form of Hookshot, including the trailing backslash if available.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "C:\Directory\Program\"
        static TStdString GetHookshotDirectory(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            TCHAR* const lastBackslash = _tcsrchr(buf, _T('\\'));
            if (NULL == lastBackslash)
                buf[0] = _T('\0');
            else
                lastBackslash[1] = _T('\0');

            return (TStdString(buf));
        }

        /// Generates the base name of the current running form of Hookshot, minus the extension.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "Hookshot"
        /// @return Base name.
        static TStdString GetHookshotBaseName(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            TCHAR* hookshotBaseName = _tcsrchr(buf, _T('\\'));
            if (NULL == hookshotBaseName)
                hookshotBaseName = buf;
            else
                hookshotBaseName += 1;
            
            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            TCHAR* const lastDot = _tcsrchr(hookshotBaseName, _T('.'));

            if (NULL == lastDot)
                return (TStdString(hookshotBaseName));

            *lastDot = _T('\0');

            TCHAR* const secondLastDot = _tcsrchr(hookshotBaseName, _T('.'));

            if (NULL == secondLastDot)
                return (TStdString(hookshotBaseName));

            *secondLastDot = _T('\0');

            return (TStdString(hookshotBaseName));
        }
        
        /// Generates the fully-qualified path of the current running form of Hookshot, minus the extension.
        /// This is useful for determining the correct path of the next file or module to load.
        /// For example: "C:\Directory\Program\Hookshot.32.dll" -> "C:\Directory\Program\Hookshot"
        /// @return Fully-qualified base path.
        static TStdString GetHookshotBasePath(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(Globals::GetInstanceHandle(), buf, (DWORD)buf.Count());

            // Hookshot module filenames are expected to end with a double-extension, the first specifying the platform and the second the actual file type.
            // Therefore, look for the last two dot characters and truncate them.
            TCHAR* const lastDot = _tcsrchr(buf, _T('.'));

            if (NULL == lastDot)
                return (TStdString(buf));

            *lastDot = _T('\0');

            TCHAR* const secondLastDot = _tcsrchr(buf, _T('.'));

            if (NULL == secondLastDot)
                return (TStdString(buf));

            *secondLastDot = _T('\0');

            return (TStdString(buf));
        }
        
        /// Generates the value for #kStrExecutableBaseName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static TStdString GetExecutableBaseName(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            TCHAR* executableBaseName = _tcsrchr(buf, _T('\\'));
            if (NULL == executableBaseName)
                executableBaseName = buf;
            else
                executableBaseName += 1;

            return (TStdString(executableBaseName));
        }

        /// Generates the value for #kStrExecutableDirectoryName; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static TStdString GetExecutableDirectoryName(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            TCHAR* const lastBackslash = _tcsrchr(buf, _T('\\'));
            if (NULL == lastBackslash)
                buf[0] = _T('\0');
            else
                lastBackslash[1] = _T('\0');

            return (TStdString(buf));
        }

        /// Generates the value for #kStrExecutableCompleteFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static TStdString GetExecutableCompleteFilename(void)
        {
            TemporaryBuffer<TCHAR> buf;
            GetModuleFileName(NULL, buf, (DWORD)buf.Count());

            return (TStdString(buf));
        }

        /// Generates the value for #kStrHookshotConfigurationFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static TStdString GetHookshotConfigurationFilename(void)
        {
            return GetExecutableDirectoryName() + GetHookshotBaseName() + kStrHookshotConfigurationFileExtension.data();
        }

        /// Generates the value for #kStrHookshotLogFilename; see documentation of this run-time constant for more information.
        /// @return Corresponding run-time constant value.
        static TStdString GetHookshotLogFilename(void)
        {
            TStdStringStream logFilename;
            
            PWSTR knownFolderPath;
            const HRESULT result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &knownFolderPath);

            if (S_OK == result)
            {
#ifdef UNICODE
                logFilename << knownFolderPath;
#else
                TemporaryBuffer<char> knownFolderPathConverted;
                if (0 == wcstombs_s(NULL, knownFolderPathConverted, knownFolderPathConverted.Size(), knownFolderPath, knownFolderPathConverted.Size()))
                    logFilename << knownFolderPathConverted;
#endif
                logFilename << _T('\\');
                CoTaskMemFree(knownFolderPath);
            }

            logFilename << GetHookshotBaseName() << _T('_') << GetExecutableBaseName() << _T('_') << GetProcessId(GetCurrentProcess()) << kStrHookshotLogFileExtension;

            return logFilename.str();
        }

        /// Generates the value for #kStrHookshotDynamicLinkLibraryFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static TStdString GetHookshotDynamicLinkLibraryFilename(void)
        {
            return GetHookshotBasePath() + kStrHookshotDynamicLinkLibraryExtension.data();
        }

        /// Generates the value for #kStrHookshotExecutableFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static TStdString GetHookshotExecutableFilename(void)
        {
            return GetHookshotBasePath() + kStrHookshotExecutableExtension.data();
        }

        /// Generates the value for #kStrHookshotExecutableOtherArchitectureFilename; see documentation of this run-time constant for more information.
        /// @return coorresponding run-time constant value.
        static TStdString GetHookshotExecutableOtherArchitectureFilename(void)
        {
            return GetHookshotBasePath() + kStrHookshotExecutableOtherArchitectureExtension.data();
        }


        // -------- INTERNAL CONSTANTS ------------------------------------- //
        // Used to implement run-time constants; see "Strings.h" for documentation.

        static const TStdString kStrExecutableBaseNameImpl(GetExecutableBaseName());

        static const TStdString kStrExecutableDirectoryNameImpl(GetExecutableDirectoryName());

        static const TStdString kStrExecutableCompleteFilenameImpl(GetExecutableCompleteFilename());

        static const TStdString kStrHookshotConfigurationFilenameImpl(GetHookshotConfigurationFilename());

        static const TStdString kStrHookshotLogFilenameImpl(GetHookshotLogFilename());

        static const TStdString kStrHookshotDynamicLinkLibraryFilenameImpl(GetHookshotDynamicLinkLibraryFilename());

        static const TStdString kStrHookshotExecutableFilenameImpl(GetHookshotExecutableFilename());

        static const TStdString kStrHookshotExecutableOtherArchitectureFilenameImpl(GetHookshotExecutableOtherArchitectureFilename());


        // -------- RUN-TIME CONSTANTS ------------------------------------- //
        // See "Strings.h" for documentation.

        extern const TStdStringView kStrExecutableBaseName(kStrExecutableBaseNameImpl);

        extern const TStdStringView kStrExecutableDirectoryName(kStrExecutableDirectoryNameImpl);

        extern const TStdStringView kStrExecutableCompleteFilename(kStrExecutableCompleteFilenameImpl);

        extern const TStdStringView kStrHookshotConfigurationFilename(kStrHookshotConfigurationFilenameImpl);

        extern const TStdStringView kStrHookshotLogFilename(kStrHookshotLogFilenameImpl);
        
        extern const TStdStringView kStrHookshotDynamicLinkLibraryFilename(kStrHookshotDynamicLinkLibraryFilenameImpl);

        extern const TStdStringView kStrHookshotExecutableFilename(kStrHookshotExecutableFilenameImpl);

        extern const TStdStringView kStrHookshotExecutableOtherArchitectureFilename(kStrHookshotExecutableOtherArchitectureFilenameImpl);


        
        // -------- FUNCTIONS ---------------------------------------------- //
        // See "Strings.h" for documentation.
        
        TStdString MakeHookModuleFilename(TStdStringView moduleName)
        {
            return kStrExecutableDirectoryNameImpl + moduleName.data() + kStrHookModuleExtension.data();
        }
    }
}
