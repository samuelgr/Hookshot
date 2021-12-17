# Using Hookshot

This document describes how to set up, configure, and run Hookshot. It assumes the user is already in possession of any hook modules and DLLs and have one or more target applications in mind. Its target audience is end users.


## Navigation

This document is organized as follows.

- [Getting Started](#getting-started)
- [File and Directories](#files-and-directories)
- [Authorizing Hookshot](#authorizing-hookshot)
- [Configuring Hookshot](#configuring-hookshot)
  - [Examples of Configuration Files](#examples-of-configuration-files)
  - [Available Scopes and Settings](#available-scopes-and-settings)

Other available documents are listed in the [top-level document](README.md).


## Getting Started

1. Ensure the system is running Windows 10 or 11. Hookshot is built to target Windows 10 or 11 and does not support older versions of Windows.

1. Ensure the [Visual C++ Runtime for Visual Studio 2022](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) is installed. Hookshot is linked against this runtime and will not work without it. If running a 64-bit operating system, install both the x86 and the x64 versions of this runtime, otherwise install just the x86 version.

1. Download the latest release of Hookshot and place all of the Hookshot executables and DLLs into any directory. Unless running a 32-bit operating system, it is recommended that both 32-bit and 64-bit versions of HookshotExe and HookshotDll be placed into this directory.
   - In general it is a good idea to place HookshotExe and HookshotDll into the same directory as the desired target applications. This is actually required if using the launcher.

1. Place any desired hook modules into the same directory as the application's executable file. If there are any DLLs to be injected, which are not themselves hook modules, they can be located anywhere.
   - If both 32-bit and 64-bit versions of a hook module is available, Hookshot will automatically load the correct version to match the target application.
   - If the correct version to match the target application is not available, Hookshot will be unable to load the hook module.

1. Optionally create or place a [configuration file](#configuring-hookshot) into the same directory as the application's executable file.
   - If Hookshot's default behavior is acceptable, then a configuration file is not necessary. Hookshot's default behavior is to load all hook modules it finds in the same directory as the application's executable file.

1. Run the application with Hookshot. There are two ways to do this. The more convenient option is to use the launcher, but it is also possible to use HookshotExe directly.
   - If the target application requires administrator permission, HookshotExe will request administrator permission before running it.


### Option 1: Using HookshotLauncher

1. Place HookshotLauncher into the same directory as the application's executable file.
   - HookshotLauncher requires that HookshotExe and HookshotDll also be located in this directory. This is not necessary when using HookshotExe directly.

1. Rename the application's executable file by adding the text `_HookshotLauncher_` to the beginning.
   - For example, `Application.exe` would be renamed `_HookshotLauncher_Application.exe`.

1. Rename HookshotLauncher's executable file so that it exactly matches the original name of the application's executable file.
   - Continuing with the preceding example, `HookshotLauncher.32.exe` would be renamed `Application.exe`.

1. Run HookshotLauncher in the same way as the original application would normally be run.
   - Continuing with the preceding example, launch `Application.exe` however it is normally launched: by Start Menu shortcut, double-click in File Explorer, and so on.
   - HookshotLauncher will automatically create an [application-specific authorization file](#authorizing-hookshot) for the executable it is launching. If creating this file requires administrator permission, HookshotLauncher will display a prompt for approval before requesting it.


### Option 2: Using HookshotExe

1. [Authorize](#authorizing-hookshot) Hookshot to act on the target application.

1. Use HookshotExe to run the application.
   - If both the 32-bit and 64-bit versions of HookshotExe and HookshotDll are present, then it does not matter which version is used to launch the application. Hookshot will automatically switch to the correct version.
   - For example, to run the application `C:\Directory\Application.exe`, do one of these two things:
       - In File Explorer, drag and drop `C:\Directory\Application.exe` onto `Hookshot.32.exe` or `Hookshot.64.exe`.
       - Execute or create a shortcut that runs `Hookshot.32.exe C:\Directory\Application.exe` or `Hookshot.64.exe C:\Directory\Application.exe`.
   - If the application's path contains spaces, enclose it in quotes. Example: `Hookshot.64.exe "C:\Directory with Spaces\Application with Spaces.exe"`
   - Command line arguments can be passed to the application. Example: `Hookshot.32.exe C:\Directory\Application.exe --arg=val1 "--arg2=value with spaces"`


## Files and Directories

HookshotExe and HookshotDll exist in both 32-bit (`Hookshot.32.exe` and `Hookshot.32.dll`) and 64-bit (`Hookshot.64.exe` and `Hookshot.64.dll`) forms. These files can be placed into any directory as long as they remain together in the same directory. This is because HookshotExe looks for HookshotDll in the same directory as itself, and when injecting itself into child processes spawned by a target process, HookshotDll likewise looks for HookshotExe in the same directory as itself.

HookshotLauncher is similarly available in both 32-bit (`HookshotLauncher.32.exe`) and 64-bit (`HookshotLauncher.64.exe`) forms. It exists purely as a convenience to users: the only things it does is automates some of the bootstrapping steps and runs HookshotExe with some predetermined arguments. In other words, it is fancy wrapper around HookshotExe. HookshotLauncher requires that it be located in the same directory as both the target executable and HookshotExe. The 32-bit version of HookshotLauncher looks for the 32-bit version of HookshotExe, and similarly the 64-bit version of HookshotLauncher looks for the 64-bit version of HookshotExe.

Configuration files, if supplied, must be named `Hookshot.ini` and placed into the same directory as the target application's executable file. If HookshotDll is unable to locate a configuration file it will silently apply default behaviors and settings.

Hook modules are generally expected to exist in both 32-bit and 64-bit form, which respectively have file names that follow the convention `[Name].HookModule.32.dll` and `[Name].HookModule.64.dll`, where `[Name]` is the name of the hook module. Hook modules should be placed in the same directory as the target application's executable. This is because hook modules are identified to HookshotDll by name (i.e. using only the `[Name]` part of the filename), and HookshotDll looks for hook modules using paths relative to the directory containing the target application's executable.

Injected DLLs can be located anywhere. HookshotDll relies on the system's built in mechanism for locating DLLs, and full absolute path names can be specified.

To see how this all fits together, consider an example:
- `C:\Hookshot` contains HookshotExe and HookshotDll.
- `C:\Directory\Application.exe` is the target application.
- `SomeRandomHooks.HookModule.32.dll` and `SomeRandomHooks.HookModule.64.dll` are the hook module files.
- `LibraryToInject.dll` is a DLL to be injected.

In this scenario, `Hookshot.ini` would be located in `C:\Directory`, and both `SomeRandomHooks.HookModule.32.dll` and `SomeRandomHooks.HookModule.64.dll` should be placed into `C:\Directory`. `LibraryToInject.dll` can be located anywhere.

Following the same scenario, if `C:\Directory\Application.exe` spawns a child process `C:\SomeOtherDirectory\SomeOtherApplication.exe`, then HookshotDll will look for another configuration file in `C:\SomeOtherDirectory` and will expect to find hook modules to be loaded into the child process in `C:\SomeOtherDirectory`. The key takeaway is that, from a configuration standpoint, HookshotDll treats being injected into a target process the same way irrespective of how that target process was spawned.


## Authorizing Hookshot

Because the modifications that Hookshot performs on a target application can be invasive, Hookshot requires explicit permission from the end user before it will act on any application.

There are two ways to give Hookshot the permission it needs, both illustrated by way of example:
* **Application-Specific Authorizaztion:** To authorize Hookshot to act on `C:\Directory\Application.exe`, create the file `Application.exe.hookshot` and place it in `C:\Directory`. The contents of the file do not matter. Hookshot only checks that the file exists.
* **Directory-Wide Authorization:** To authorize Hookshot to act on all executables located in `C:\Directory`, create the file `.hookshot` and place it in `C:\Directory`. The contents of the file do not matter. Hookshot only checks that the file exists.

Hookshot can additionally act on any programs spawned as children of the target application. It will do so automatically, but only if authorized. For example, while `C:\Directory\Application.exe` is running it might launch another program, `C:\Dir2\App2.exe`. If Hookshot is already acting on `C:\Directory\Application.exe`, it will automatically also act on `C:\Dir2\App2.exe`, but only if either `C:\Dir2\App2.exe.hookshot` or `C:\Dir2\.hookshot` exists.


## Configuring Hookshot

HookshotDll can read from an optional configuration file that lists the hook modules and injected DLLs it should load. The configuration file, `Hookshot.ini`, follows standard INI format: name-value pairs scoped into different sections.

If HookshotDll cannot find a configuration file, it applies the following default settings and behaviors.
- Logging is disabled.
- All hook modules located in the same directory of the target application are loaded.
- No injected DLLs are loaded.

If HookshotDll finds a malformed configuration file, it will output an error message indicating the problem, load no hook modules, load no injected DLLs, and disable logging.


### Examples of Configuration Files

Suppose three applications, `App1.exe`, `App2.exe`, and `App3.exe`, all located in `C:\Directory\`, are to be configured for use with Hookshot. The contents of `C:\Directory\Hookshot.ini` are shown below.

```ini
LogLevel = 0
HookModule = SomeRandomHooks
Inject = C:\AnyLibrary.dll

; Comments are also supported,
; as long as they are on lines of their own.

[App1.exe]
HookModule = HooksA
HookModule = HooksA2

; Hook module names can contain spaces.
HookModule = Hooks A 3

[App2.exe]
HookModule = HooksB
Inject = C:\LibB.dll
Inject = C:\LibB2.dll
```

This configuration file would result in the following behavior.
- Hook module `SomeRandomHooks` would be loaded for all of `App1.exe`, `App2.exe`, and `App3.exe`, and likewise the DLL file `C:\AnyLibrary.dll` would be injected into all three.
- Hook modules `HooksA`, `HooksA2`, and `Hooks A 3` would additionally be loaded for `App1.exe` only.
- Hook module `HooksB` would additionally be loaded for `App2.exe` only, and likewise the DLL files `C:\LibB.dll` and `C:\LibB2.dll` would only be injected into `App2.exe`.

A second example is shown below, demonstrating how to supply a configuration file but still preserve Hookshot's default behavior of loading all the hook modules it finds in the same directory as the target application's executable file. This is what Hookshot does in the absence of a configuration file, but Hookshot still offers that behavior if it is specifically requested in a configuration file.

```ini
LogLevel = 2
UseConfiguredHookModules = no
Inject = C:\AnyLibrary.dll

; Because UseConfiguredHookModules is set to "no" Hookshot ignores all HookModule lines.
HookModule = SomeRandomHooks

[App2.exe]
Inject = C:\LibB.dll
Inject = C:\LibB2.dll

; This is also ignored because of the value of UseConfiguredHookModules.
HookModule = HooksB
```

In the example above, logging is enabled at [verbosity level](#available-scopes-and-settings) 2, and Hookshot will load all hook modules it finds, irrespective of the application's executable file name. Injected DLLs can still be specified as in the previous example and are still effective the same way.


### Available Scopes and Settings

There are no settings that are required to be specified in a configuration file for said configuration file to be valid. All settings are optional. A completely empty configuration file is perfectly valid but would result in logging being disabled, no hook modules being loaded, and no DLLs being injected into the target application.

At global scope (i.e. at the top of the file, before any sections are listed), the following settings are supported. Settings in this scope are applied to all executables in the same directory as the configuration file.

- **LogLevel**, an integer value used to configure logging. This is generally only useful when troubleshooting. When logging is enabled, messages produced by Hookshot are written to a file on the current user's desktop that is named to identify Hookshot, the executable name, and the process ID number. A value for this setting should only be specified once.
   - 0 means logging is disabled. This is the default.
   - 1 means to include error messages only.
   - 2 means to include errors and warnings.
   - 3 means to include errors, warnings, and informational messages.
   - 4 means to include errors, warnings, informational messages, and internal debugging messages.
- **HookModule**, which names a hook module to load. Multiple hook modules may be specified.
   - Hook modules have file names of the form `[Name].HookModule.(32|64).dll`. Only the `[Name]` part should be specified in the configuration file.
   - For example, a hook module distributed as `SomeRandomHooks.HookModule.32.dll` and `SomeRandomHooks.HookModule.64.dll` would be identified as `SomeRandomHooks` in the configuration file.
- **Inject**, which identifies the file name of an injected DLL.
   - Hookshot passes the value of this setting directly to the system without modification.
   - The system's directory search order for locating DLLs determines how the path is interpreted.
- **UseConfiguredHookModules**, a Boolean value that is used to tell Hookshot if it should load the hook modules specified in the configuration file or use its default behavior of loading all hook modules in the same directory as the target application's executable file.
   - Default value is `yes` if a configuration file is present and `no` if a configuration file is not present.
   - This is useful for turning on logging or specifying injected DLLs without affecting how Hookshot looks for hook modules.

Hookshot additionally supports per-executable scopes in the configuration file. The settings contained within each are applied only to the executable whose file name matches the configuration file section name. Each per-executable scope can contain values for **HookModule** and **Inject**.

Other than the fact that the global scope must be at the top of the file, the ordering of sections with respect to one another does not matter. Furthermore, the ordering of `HookModule` and `Inject` settings within a section also generally does not matter. Hookshot first loads all applicable hook modules in the order they appear in the configuration file and, once this is complete, loads applicable injected DLLs in the order they appear in the configuration file.
