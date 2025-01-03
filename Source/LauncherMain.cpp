/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file LauncherMain.cpp
 *   Entry point for the convenience launcher executable.
 **************************************************************************************************/

#include <sstream>
#include <string>
#include <string_view>

#include <Infra/Core/Message.h>
#include <Infra/Core/ProcessInfo.h>
#include <Infra/Core/Strings.h>
#include <Infra/Core/TemporaryBuffer.h>

#include "ApiWindows.h"
#include "Globals.h"
#include "RemoteProcessInjector.h"
#include "Strings.h"

namespace Hookshot
{
  /// Prefix for any of the special command-line arguments that may be passed to the Hookshot
  /// Launcher to request it complete one specific task and then exit.
  static constexpr std::wstring_view kLauncherTaskArgPrefix = L"__hookshot_launcher_task:";

  /// Launcher task argument for creating an authorization file.
  static constexpr std::wstring_view kLauncherTaskCreateAuthorizationFile = L"create_auth_file";

  /// Displays a graphical dialog box informing the user that elevation is temporarily required so
  /// that the Hookshot Launcher can create the required authorization file.
  /// @param [in] executablePath Path of the executable to authorize.
  /// @param [in] authorizationFile Path of the authorization file about which to prompt the user.
  /// @return `true` if the user approved the request, `false` otherwise.
  static bool GetUserPermissionForAuthorizationElevation(
      std::wstring_view executablePath, std::wstring_view authorizationFile)
  {
    std::wstringstream contentStream;
    contentStream
        << Infra::ProcessInfo::GetProductName()
        << L" temporarily needs administrator permission so it can create an authorization file for the executable it is attempting to launch.\n\nThis is a one-time operation unless the file is deleted.";
    const std::wstring contentString = contentStream.str();

    std::wstringstream expandedInformationStream;
    expandedInformationStream << L"\nExecutable to launch:\n"
                              << executablePath.data() << L"\n\nAuthorization file to be created:\n"
                              << authorizationFile << L"\n";
    const std::wstring expandedInformationString = expandedInformationStream.str();

    std::wstringstream footerStream;
    footerStream << L"An authorization file contains no data. Its existence tells "
                 << Infra::ProcessInfo::GetProductName()
                 << L" it has your permission to inject a particular executable.";
    const std::wstring footerString = footerStream.str();

    std::wstringstream buttonTextOkStream;
    buttonTextOkStream << L"Proceed\nCreate the authorization file and launch the executable with "
                       << Infra::ProcessInfo::GetProductName() << L".";
    const std::wstring buttonTextOk = buttonTextOkStream.str();

    std::wstringstream buttonTextCancelStream;
    buttonTextCancelStream << L"Cancel\nExit without creating the authorization file.";
    const std::wstring buttonTextCancel = buttonTextCancelStream.str();

    const TASKDIALOG_BUTTON kUserDialogCustomButtons[] = {
        {.nButtonID = IDOK, .pszButtonText = buttonTextOk.c_str()},
        {.nButtonID = IDCANCEL, .pszButtonText = buttonTextCancel.c_str()}};

    const TASKDIALOGCONFIG userDialogConfig = {
        .cbSize = sizeof(TASKDIALOGCONFIG),
        .dwFlags = TDF_USE_COMMAND_LINKS | TDF_SIZE_TO_CONTENT,
        .pszWindowTitle = Infra::ProcessInfo::GetProductName().data(),
        .pszContent = contentString.c_str(),
        .cButtons = _countof(kUserDialogCustomButtons),
        .pButtons = kUserDialogCustomButtons,
        .nDefaultButton = kUserDialogCustomButtons[0].nButtonID,
        .pszExpandedInformation = expandedInformationString.c_str(),
        .pszExpandedControlText = L"Fewer details",
        .pszCollapsedControlText = L"More details",
        .pszFooterIcon = TD_INFORMATION_ICON,
        .pszFooter = footerString.c_str(),
        .pfCallback =
            [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) -> HRESULT
        {
          switch (msg)
          {
            case TDN_CREATED:
              SetForegroundWindow(hwnd);
              SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDOK, 1);
              break;

            case TDN_HYPERLINK_CLICKED:
              ShellExecute(nullptr, L"open", (LPCWSTR)lParam, nullptr, nullptr, SW_SHOWNORMAL);
              break;

            default:
              break;
          }

          return S_OK;
        }};

    int dialogResponse = 0;
    TaskDialogIndirect(&userDialogConfig, &dialogResponse, nullptr, nullptr);
    return (kUserDialogCustomButtons[0].nButtonID == dialogResponse);
  }

  /// Runs a launcher task. Spawns a new instance of the Hookshot Launcher with a single
  /// command-line argument that identifies a launcher task to execute.
  /// @param [in] launcherTask Launcher task to run. Should be one of the predefined launcher task
  /// constants.
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
    taskAttemptInfo.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC |
        SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
    taskAttemptInfo.lpVerb = ((true == elevationRequired) ? L"runas" : L"open");
    taskAttemptInfo.lpFile = Infra::ProcessInfo::GetExecutableCompleteFilename().data();
    taskAttemptInfo.lpParameters = launcherArg.c_str();
    taskAttemptInfo.nShow = SW_SHOWDEFAULT;

    if ((TRUE == ShellExecuteEx(&taskAttemptInfo)) && (nullptr != taskAttemptInfo.hProcess) &&
        (INVALID_HANDLE_VALUE != taskAttemptInfo.hProcess))
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

  /// Implementation of a launcher task. Creates a Hookshot authorization file for the specified
  /// executable.
  /// @param [in] executablePath Path of the executable to authorize.
  /// @return System error code representing the result of the operation.
  static DWORD LauncherTaskCreateAuthorizationFile(std::wstring_view executablePath)
  {
    const Infra::TemporaryString authorizationFileToCreate =
        Strings::AuthorizationFilenameApplicationSpecific(executablePath);
    const HANDLE authorizationFileHandle = CreateFile(
        authorizationFileToCreate.AsCString(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    const DWORD authorizationFileResult = GetLastError();

    if (INVALID_HANDLE_VALUE != authorizationFileHandle) CloseHandle(authorizationFileHandle);

    return authorizationFileResult;
  }

  /// Checks if a Hookshot authorization file exists for the specified executable and, if not,
  /// attempts to create an executable-specific authorization file.
  /// @param [in] executablePath Path of the executable to check.
  /// @return System error code representing the result of the operation.
  static DWORD EnsureHookshotIsAuthorized(std::wstring_view executablePath)
  {
    const Infra::TemporaryString authorizationFileApplicationSpecific =
        Strings::AuthorizationFilenameApplicationSpecific(executablePath);
    const Infra::TemporaryString authorizationFileDirectoryWide =
        Strings::AuthorizationFilenameDirectoryWide(executablePath);

    if ((TRUE == PathFileExists(authorizationFileApplicationSpecific.AsCString())) ||
        (TRUE == PathFileExists(authorizationFileDirectoryWide.AsCString())))
      return ERROR_SUCCESS;

    const DWORD authorizationFileCreateResult =
        RunLauncherTask(kLauncherTaskCreateAuthorizationFile, false);
    switch (authorizationFileCreateResult)
    {
      case ERROR_ACCESS_DENIED:
        return (
            (true ==
             GetUserPermissionForAuthorizationElevation(
                 executablePath, authorizationFileApplicationSpecific))
                ? RunLauncherTask(kLauncherTaskCreateAuthorizationFile, true)
                : ERROR_ACCESS_DENIED);

      default:
        return authorizationFileCreateResult;
    }
  }

  /// Composes the full path of the executable to be launched. This is a transformation on the name
  /// of the currently-running lauuncher executable.
  /// @return Path of the executable to be launched.
  static std::wstring GetLaunchExecutablePath(void)
  {
    static constexpr std::wstring_view kStrExecutableNamePrefix = L"_HookshotLauncher_";

    std::wstring launchExePath;
    std::wstring_view pieces[] = {
        Infra::ProcessInfo::GetExecutableDirectoryName(),
        L"\\",
        kStrExecutableNamePrefix,
        Infra::ProcessInfo::GetExecutableBaseName()};

    size_t totalLength = 0;
    for (int i = 0; i < _countof(pieces); ++i)
      totalLength += pieces[i].length();

    launchExePath.reserve(1 + totalLength);

    for (int i = 0; i < _countof(pieces); ++i)
      launchExePath.append(pieces[i]);

    return launchExePath;
  }
} // namespace Hookshot

/// Program entry point.
/// @param [in] hInstance Instance handle for this executable.
/// @param [in] hPrevInstance Unused, always `nullptr`.
/// @param [in] lpCmdLine Command-line arguments specified after the executable name.
/// @param [in] nCmdShow Flag that specifies how the main application window should be shown. Not
/// applicable to this executable.
/// @return Exit code from this program.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
  using namespace Hookshot;

  Hookshot::Globals::Initialize(Hookshot::Globals::ELoadMethod::Executed);

  const std::wstring kExecutableToLaunch(GetLaunchExecutablePath());
  if (TRUE == PathFileExists(kExecutableToLaunch.c_str()))
  {
    // Target executable exists and is accessible.
    // This is the nominal case.

    if ((2 == __argc) &&
        (0 == wcsncmp(__wargv[1], kLauncherTaskArgPrefix.data(), kLauncherTaskArgPrefix.length())))
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
      // Attempt to launch the target executable and then inject it with HookshotExe.

      // Verify Hookshot is authorized to act on the target executable and, if not, attempt to
      // create the required authorization file.
      const DWORD authorizeResult = EnsureHookshotIsAuthorized(kExecutableToLaunch.c_str());
      switch (authorizeResult)
      {
        case ERROR_SUCCESS:
          break;

        case ERROR_ACCESS_DENIED:
        case ERROR_CANCELLED:
          // Elevation is required but the user either declined or cancelled the operation.
          // In both cases the user clicked a button to terminate the operation.
          return __LINE__;

        default:
          Infra::Message::OutputFormatted(
              Infra::Message::ESeverity::ForcedInteractiveError,
              L"%s\n\n%.*s failed to launch this executable.\n\nUnable to create the authorization file (%s).",
              kExecutableToLaunch.c_str(),
              static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
              Infra::ProcessInfo::GetProductName().data(),
              Infra::Strings::FromSystemErrorCode(authorizeResult).AsCString());
          return __LINE__;
      }

      // Compose a full command-line string that contains the name of the executable to launch then
      // all command-line arguments, which were all originally supplied to this executable. Each
      // element must be enclosed in quotes, and for command-line arguments they must have any
      // contained quote characters escaped.
      Infra::TemporaryString commandLine;
      commandLine << L'\"' << kExecutableToLaunch << L'\"';

      for (size_t argIndex = 1; argIndex < (size_t)__argc; ++argIndex)
      {
        const wchar_t* const argString = __wargv[argIndex];
        const size_t argLen = wcslen(argString);

        commandLine << L" \"";

        for (size_t i = 0; i < argLen; ++i)
        {
          if (L'\"' == argString[i]) commandLine << L'\\';

          commandLine << argString[i];
        }

        commandLine << L'\"';
      }

      STARTUPINFO startupInfo;
      PROCESS_INFORMATION processInfo;

      memset(reinterpret_cast<void*>(&startupInfo), 0, sizeof(startupInfo));
      memset(reinterpret_cast<void*>(&processInfo), 0, sizeof(processInfo));

      HANDLE launchedProcess = NULL;

      if (0 ==
          CreateProcess(
              nullptr,
              commandLine.Data(),
              nullptr,
              nullptr,
              FALSE,
              CREATE_SUSPENDED,
              nullptr,
              nullptr,
              &startupInfo,
              &processInfo))
      {
        // Execution of the requested executable failed.
        // Either re-attempt by re-launching this launcher with elevation or fail completely with an
        // error.

        if (ERROR_ELEVATION_REQUIRED != GetLastError())
        {
          Infra::Message::OutputFormatted(
              Infra::Message::ESeverity::ForcedInteractiveError,
              L"%s\n\n%.*s failed to launch this executable (%s).",
              kExecutableToLaunch.c_str(),
              static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
              Infra::ProcessInfo::GetProductName().data(),
              Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());
          return __LINE__;
        }

        // Format of the command-line string is quote (1 character), executable name (N characters),
        // end-quote (1 character), space (1 character), command-line arguments. Getting to the
        // command-line arguments means skipping the length of the executable plus three more
        // characters.
        const wchar_t* commandLineArgs =
            ((__argc > 1)
                 ? &commandLine[static_cast<unsigned int>(kExecutableToLaunch.length() + 3)]
                 : L"");

        SHELLEXECUTEINFO elevationAttemptInfo = {
            .cbSize = sizeof(SHELLEXECUTEINFO),
            .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC,
            .lpVerb = L"runas",
            .lpFile = Infra::ProcessInfo::GetExecutableCompleteFilename().data(),
            .lpParameters = commandLineArgs,
            .nShow = SW_SHOWDEFAULT};

        const BOOL executeElevatedResult = ShellExecuteEx(&elevationAttemptInfo);
        if ((TRUE != executeElevatedResult) || (NULL == elevationAttemptInfo.hProcess))
        {
          Infra::Message::OutputFormatted(
              Infra::Message::ESeverity::ForcedInteractiveError,
              L"%s\n\n%.*s failed to launch this executable because it requires elevation (%s).",
              kExecutableToLaunch.c_str(),
              static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
              Infra::ProcessInfo::GetProductName().data(),
              Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());
          return __LINE__;
        }

        launchedProcess = elevationAttemptInfo.hProcess;
      }
      else
      {
        // Execution of the requested executable succeeded.
        // It is currently in a suspended state, ready for Hookshot's executable form to inject it.

        const EInjectResult injectResult = RemoteProcessInjector::InjectProcess(
            processInfo.hProcess, processInfo.hThread, false, false);
        if (EInjectResult::Success != injectResult)
        {
          Infra::Message::OutputFormatted(
              Infra::Message::ESeverity::ForcedInteractiveError,
              L"%s\n\n%.*s failed to inject this executable.\n\n%s (%s).",
              kExecutableToLaunch.c_str(),
              static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
              Infra::ProcessInfo::GetProductName().data(),
              InjectResultString(injectResult).data(),
              Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());
          TerminateProcess(processInfo.hProcess, (UINT)-1);
          return __LINE__;
        }

        ResumeThread(processInfo.hThread);
        CloseHandle(processInfo.hThread);

        launchedProcess = processInfo.hProcess;
      }

      Infra::Message::OutputFormatted(
          Infra::Message::ESeverity::Info,
          L"Successfully used %.*s to inject %s.",
          static_cast<int>(Strings::GetHookshotExecutableFilename().length()),
          Strings::GetHookshotExecutableFilename().data(),
          kExecutableToLaunch.c_str());

      if (WAIT_FAILED == WaitForSingleObject(launchedProcess, INFINITE))
        Infra::Message::OutputFormatted(
            Infra::Message::ESeverity::Error,
            L"Failed to wait for %s to terminate (%s).",
            kExecutableToLaunch.c_str(),
            Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());

      CloseHandle(launchedProcess);
      return 0;
    }
  }
  else
  {
    // Target executable is not accessible. For now this is just an error case, but in the future
    // additional interactive setup functionality could be implemented here.
    Infra::Message::OutputFormatted(
        Infra::Message::ESeverity::ForcedInteractiveError,
        L"%s\n\n%.*s cannot access this executable.\n\n%s",
        kExecutableToLaunch.c_str(),
        static_cast<int>(Infra::ProcessInfo::GetProductName().length()),
        Infra::ProcessInfo::GetProductName().data(),
        Infra::Strings::FromSystemErrorCode(GetLastError()).AsCString());
    return __LINE__;
  }
}
