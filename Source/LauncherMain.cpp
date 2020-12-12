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


using namespace Hookshot;


// -------- ENTRY POINT ---------------------------------------------------- //

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCmdShow Flag that specifies how the main application window should be shown. Not applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    const std::wstring kExecutableNamePrefix = L"_HookshotLauncher_";
    const std::wstring kExecutableToLaunch = std::wstring(Strings::kStrExecutableDirectoryName) + kExecutableNamePrefix + std::wstring(Strings::kStrExecutableBaseName);

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
