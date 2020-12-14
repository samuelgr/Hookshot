/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file LauncherMain.cpp
 *   Entry point for the convenience launcher executable.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Message.h"
#include "Strings.h"
#include "TemporaryBuffer.h"

#include <shlwapi.h>
#include <sstream>
#include <string>
#include <string_view>


using namespace Hookshot;


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ----------------------------------------- //

    /// Prefix for any of the special command-line arguments that may be passed to the Hookshot Launcher to request it complete one specific task and then exit.
    static constexpr std::wstring_view kLauncherTaskArgPrefix = L"__hookshot_launcher_task:";

    /// Launcher task argument for creating an authorization file.
    static constexpr std::wstring_view kLauncherTaskCreateAuthorizationFile = L"create_auth_file";


    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Runs a launcher task.
    /// Spawns a new instance of the Hookshot Launcher with a single command-line argument that identifies a launcher task to execute.
    /// @param [in] launcherTask Launcher task to run. Should be one of the predefined launcher task constants.
    /// @param [in] elevationRequired Specifies if the launcher task should be invoked with elevation.
    /// @return System error code representing the result of the operation.
    static DWORD RunLauncherTask(std::wstring_view launcherTask, bool elevationRequired)
    {
        // Generate the command-line argument string using the specificied task.
        std::wstring launcherArg;
        std::wstring_view pieces[] = {kLauncherTaskArgPrefix, launcherTask};

        size_t totalLength = 0;
        for (int i = 0; i < _countof(pieces); ++i)
            totalLength += pieces[i].length();

        launcherArg.reserve(1 + totalLength);

        for (int i = 0; i < _countof(pieces); ++i)
            launcherArg.append(pieces[i]);

        // Attempt to execute the requested task.
        SHELLEXECUTEINFO taskAttemptInfo;
        ZeroMemory(&taskAttemptInfo, sizeof(taskAttemptInfo));
        taskAttemptInfo.cbSize = sizeof(taskAttemptInfo);
        taskAttemptInfo.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
        taskAttemptInfo.lpVerb = ((true == elevationRequired) ? L"runas" : L"open");
        taskAttemptInfo.lpFile = Strings::kStrExecutableCompleteFilename.data();
        taskAttemptInfo.lpParameters = launcherArg.c_str();
        taskAttemptInfo.nShow = SW_SHOWDEFAULT;

        if ((TRUE == ShellExecuteEx(&taskAttemptInfo)) && (nullptr != taskAttemptInfo.hProcess) && (INVALID_HANDLE_VALUE != taskAttemptInfo.hProcess))
        {
            if ((WAIT_OBJECT_0 == WaitForSingleObject(taskAttemptInfo.hProcess, INFINITE)))
            {
                DWORD taskAttemptResult = 0;
                if (0 != GetExitCodeProcess(taskAttemptInfo.hProcess, &taskAttemptResult))
                {
                    CloseHandle(taskAttemptInfo.hProcess);
                    return taskAttemptResult;
                }
            }

            CloseHandle(taskAttemptInfo.hProcess);
        }

        return GetLastError();
    }

    /// Implementation of a launcher task.
    /// Creates a Hookshot authorization file for the specified executable.
    /// @param [in] executablePath Path of the executable to authorize.
    /// @return System error code representing the result of the operation.
    static DWORD LauncherTaskCreateAuthorizationFile(std::wstring_view executablePath)
    {
        const std::wstring kAuthorizationFileToCreate = Strings::AuthorizationFilenameApplicationSpecific(executablePath);
        const HANDLE authorizationFileHandle = CreateFile(kAuthorizationFileToCreate.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        const DWORD authorizationFileResult = GetLastError();

        if (INVALID_HANDLE_VALUE != authorizationFileHandle)
            CloseHandle(authorizationFileHandle);

        return authorizationFileResult;
    }
    
    /// Checks if a Hookshot authorization file exists for the specified executable and, if not, attempts to create an executable-specific authorization file.
    /// @param [in] executablePath Path of the executable to check.
    /// @return System error code representing the result of the operation.
    static DWORD EnsureHookshotIsAuthorized(std::wstring_view executablePath)
    {
        const std::wstring kAuthorizationFileApplicationSpecific = Strings::AuthorizationFilenameApplicationSpecific(executablePath);
        const std::wstring kAuthorizationFileDirectoryWide = Strings::AuthorizationFilenameDirectoryWide(executablePath);

        if ((TRUE == PathFileExists(kAuthorizationFileApplicationSpecific.c_str())) || (TRUE == PathFileExists(kAuthorizationFileDirectoryWide.c_str())))
            return ERROR_SUCCESS;

        const DWORD authorizationFileCreateResult = RunLauncherTask(kLauncherTaskCreateAuthorizationFile, false);
        switch (authorizationFileCreateResult)
        {
        case ERROR_ACCESS_DENIED:
            Message::OutputFormatted(Message::ESeverity::ForcedInteractiveInfo, L"%s\n\n%s temporarily needs administrator access to create the authorization file for this executable.", executablePath.data(), Strings::kStrProductName.data());
            return RunLauncherTask(kLauncherTaskCreateAuthorizationFile, true);

        default:
            return authorizationFileCreateResult;
        }
    }
    
    /// Composes the full path of the executable to be launched.
    /// This is a transformation on the name of the currently-running lauuncher executable.
    /// @return Path of the executable to be launched.
    static std::wstring GetLaunchExecutablePath(void)
    {
        static constexpr std::wstring_view kStrExecutableNamePrefix = L"_HookshotLauncher_";

        std::wstring launchExePath;
        std::wstring_view pieces[] = {Strings::kStrExecutableDirectoryName, kStrExecutableNamePrefix, Strings::kStrExecutableBaseName};

        size_t totalLength = 0;
        for (int i = 0; i < _countof(pieces); ++i)
            totalLength += pieces[i].length();

        launchExePath.reserve(1 + totalLength);

        for (int i = 0; i < _countof(pieces); ++i)
            launchExePath.append(pieces[i]);

        return launchExePath;
    }
}


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCmdShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    const std::wstring kExecutableToLaunch(GetLaunchExecutablePath());
    if (TRUE == PathFileExists(kExecutableToLaunch.c_str()))
    {
        // Target executable exists and is accessible.
        // This is the nominal case.

        if ((2 == __argc) && (0 == wcsncmp(__wargv[1], kLauncherTaskArgPrefix.data(), kLauncherTaskArgPrefix.length())))
        {
            // Executed with an argument requesting completion of one specific task.
            // Attempt to complete that task and exit, returning a Windows system error code.

            const std::wstring_view kLauncherRequestedTask(&__wargv[1][kLauncherTaskArgPrefix.length()]);

            if (kLauncherRequestedTask == kLauncherTaskCreateAuthorizationFile)
                return LauncherTaskCreateAuthorizationFile(kExecutableToLaunch);

            return ERROR_INVALID_FUNCTION;
        }
        else
        {
            // Executed normally.
            // Attempt to launch the target executable via HookshotExe.

            // Verify Hookshot is authorized to act on the target executable and, if not, attempt to create the required authorization file.
            const DWORD authorizeResult = EnsureHookshotIsAuthorized(kExecutableToLaunch.c_str());
            if (ERROR_SUCCESS != authorizeResult)
            {
                Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s\n\n%s failed to launch this executable.\n\nUnable to create the authorization file (%s).", kExecutableToLaunch.c_str(), Strings::kStrProductName.data(), Strings::SystemErrorCodeString(authorizeResult).c_str());
                return __LINE__;
            }

            // Compose a full command-line string that contains the name of the Hookshot executable, the name of the executable to launch, and then all command-line arguments.
            // Each element must be enclosed in quotes, and for command-line arguments they must have any contained quote characters escaped.
            std::wstringstream commandLineStream;
            commandLineStream << L'\"' << Strings::kStrHookshotExecutableFilename << L"\" \"" << kExecutableToLaunch << L'\"';

            for (size_t argIndex = 1; argIndex < (size_t)__argc; ++argIndex)
            {
                const wchar_t* const argString = __wargv[argIndex];
                const size_t argLen = wcslen(argString);

                commandLineStream << L" \"";

                for (size_t i = 0; i < argLen; ++i)
                {
                    if (L'\"' == argString[i])
                        commandLineStream << L'\\';

                    commandLineStream << argString[i];
                }

                commandLineStream << L'\"';
            }

            // Command-line string must be placed into a mutable buffer, per CreateProcessW documentation.
            TemporaryBuffer<wchar_t> commandLine;
            if (0 != wcscpy_s(commandLine, commandLine.Count(), commandLineStream.str().c_str()))
            {
                Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"Specified command line exceeds the limit of %d characters.", (int)commandLine.Count());
                return __LINE__;
            }

            STARTUPINFO startupInfo;
            PROCESS_INFORMATION processInfo;

            memset((void*)&startupInfo, 0, sizeof(startupInfo));
            memset((void*)&processInfo, 0, sizeof(processInfo));

            if (0 == CreateProcess(nullptr, commandLine, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
            {
                Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s\n\n%s failed to launch this executable.\n\nUnable to start %s (%s).", kExecutableToLaunch.c_str(), Strings::kStrProductName.data(), Strings::kStrProductName.data(), Strings::SystemErrorCodeString(GetLastError()).c_str());
                return __LINE__;
            }

            Message::OutputFormatted(Message::ESeverity::Info, L"Successfully used %s to launch %s.", Strings::kStrHookshotExecutableFilename.data(), kExecutableToLaunch.c_str());
            return 0;
        }
    }
    else
    {
        // Target executable is not accessible.
        // For now this is just an error case, but in the future additional interactive setup functionality could be implemented here.

        Message::OutputFormatted(Message::ESeverity::ForcedInteractiveError, L"%s\n\n%s cannot access this executable.\n\n%s.", kExecutableToLaunch.c_str(), Strings::kStrProductName.data(), Strings::SystemErrorCodeString(GetLastError()).c_str());
        return __LINE__;
    }
}
