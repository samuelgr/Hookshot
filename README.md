<h1 id="hookshot">Introduction</h1>

Hookshot is a library that exposes a simple API to facilitate the interception of arbitrary binary functions in 32-bit (x86) and 64-bit (x64) Windows applications, a practice more commonly known as *hooking function calls*.  The Hookshot API allows a developer to divert execution from any function (the *original function*) in the target application to a different function (the *hook function*) of the developer's choice.  Hookshot can thus be used to monitor the function calls made by the application, to modify the behavior of the functions called by the application (in essence modifying the behavior of the application itself), or to do both.  The specific technique Hookshot uses to implement function call hooking allow it to hook functions located anywhere: they can be defined within the application itself or imported into the application via an external API, including the Windows API.

Hookshot acts only at execution time, modifying the target application's binary code after it is already loaded into memory and without making changes to any executable or library files.  At a high level, when a request is made to hook a function via the Hookshot API, Hookshot does two things:
- Edits the contents of memory such that all future invocations of the original function are redirected to the hook function.  The locations edited include both memory allocated by Hookshot for its own use and memory that holds the original function's binary code.
- Generates, stores, and exposes via a different Hookshot API an *original functionality pointer*.  When this pointer is invoked as a function, the behavior that results is as if the original function were invoked directly and without being hooked.  What this means is that the operation of hooking a function does not cause a loss of access to that function's original functionality.

In addition to its primary use of hooking function calls, Hookshot can also be used to cause a process to load a library of the user's choice, even if the process would not normally load said library.  This practice is more commonly known as *DLL injection*.

Finally, it should be noted that **Hookshot cannot operate in any way whatsoever on existing, already-running processes.  This is by design.**  Rather, Hookshot can only hook function calls and inject DLLs into processes that Hookshot itself spawns under the control and direction of the end user.


<h2 id="hookshot-features">Key Features</h2>

- Hooks function calls without losing access to original functionality.  Given an original function and a hook function, Hookshot can redirect all future invocations of the original function to the hook function.  Hookshot maintains and makes available a pointer that can be invoked as a function to access the un-hooked behavior of the original function.

- Injects DLLs.


<h2 id="hookshot-audience">Target Audience</h2>

Hookshot targets both developers and end users.
- Developers use the Hookshot API to build modules that modify the behavior of applications.  Each such module might be built to target one specific application or it may be reusable across multiple applications.
- End users obtain these modules and configure Hookshot such that it loads them into an application of their choosing.  They may also obtain other DLLs they wish to inject and configure Hookshot accordingly.


<h2 id="hookshot-components">Components</h2>

Hookshot itself consists of the following parts.
- An executable, *HookshotExe*, which spawns a new process and forces the new process to load (i.e. injects) HookshotDll.  The file name of the executable to use for the new process and any command line arguments with which to supply it are all specified as command line arguments to HookshotExe.
- A library, *HookshotDll*, which implements the Hookshot API.  HookshotDll contains all of the logic required to hook functions and load DLLs.  Upon injection into a process by HookshotExe, HookshotDll reads a *configuration file* and uses it to determine which additional binaries to load.

Deverlopers who wish to use Hookshot do so by creating *hook modules*.  A hook module is a DLL built to use the Hookshot API and comply with certain interface requirements.  Hook modules would typically request that Hookshot hook specific functions and supply the hook functions to be used.  HookshotDll uses the contents of a configuration file to determine which hook modules it should load.

End users who wish to use one or more hook modules to modify an application do so by placing the hook modules into the same directory as the application's executable along with a Hookshot configuration file that lists the hook modules to be loaded.  They would then run HookshotExe and specify the desired application's command line as command line arguments to HookshotExe.  For repeatedly running the application with HookshotExe, this process can be simplified by using Windows Explorer to create a shortcut.

**TODO: EXAMPLE**


<h1 id="navigation">Navigation</h1>

The remainder of this document is organized as follows.

- [For Users](#users)
   - [Getting Started](#users-gettingstarted)
- [For Developers](#developers)
   - [Getting Started](#developers-gettingstarted)
   

<h1 id="users">For Users</h1>

**TODO: Text.**


<h2 id="users-gettingstarted">Getting Started</h2>

1. Ensure the [Visual C++ 2015, 2017, and 2019 Runtime](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) is installed.  Hookshot is linked against this runtime and will not work without it.  If running a 64-bit operating system, install both the x86 and the x64 versions of this runtime, otherwise install just the x86 version.

1. Download the latest release of Hookshot, and place all of the Hookshot executables and DLLs into any directory.  Unless running a 32-bit operating system, it is recommended that both 32-bit and 64-bit versions of HookshotExe and HookshotDll be placed into this directory.
   - HookshotExe expects to find HookshotDll in the same directory as itself.
   - If the intention is to use Hookshot to target one particular application, it is perfectly fine to place these files into the same directory as the application.  Otherwise, it may be desirable to place them into a shared location.

1. Place any desired hook modules into the same directory as the application's executable file, or into a subdirectory.  DLLs to be injected, which are not hook modules, can be located anywhere.
   - HookshotDll looks for hook modules using paths relative to the directory of the application's executable file.
   - If both 32-bit and 64-bit versions of a hook module is available, Hookshot will automatically load the correct version to match the target application.  On the contrary, if the correct version is not available, Hookshot will be unable to load the hook module.

1. Place a [configuration file](#users-configuration), `Hookshot.ini`, into the same directory as the application's executable file.
   - HookshotDll expects a file with this specific name to exist in the same directory as the application's executable file.
   - If not found, HookshotDll will display an error message showing the full path of the configuration file it expected to find.

1. Use HookshotExe to run the application.
   - If both the 32-bit and 64-bit versions of HookshotExe and HookshotDll are present, then it does not matter which version is used to launch the application.  Hookshot will automatically switch to the correct version.
   - Example: to run the application `C:\Directory\Application.exe`, execute or create a shortcut that runs `Hookshot.32.exe C:\Directory\Application.exe` or `Hookshot.64.exe C:\Directory\Application.exe`.


<h2 id="users-configuration">Configuring Hookshot</h2>

Hookshot requires a configuration file be present to list the hook modules and DLLs it should load.  The configuration file, `Hookshot.ini`, follows standard INI format: name-value pairs scoped into different sections.

At *global scope* (i.e. at the top of the file, before any sections are listed), the following settings are supported.  These are applied to all executables in the same directory as the configuration file.
- **LogLevel**, an integer value used to configure logging.  This is generally only useful when troubleshooting.  Messages produced by Hookshot are written to a file on the current user's desktop that is named to identify Hookshot, the executable name, and the process ID number.  A value for this setting should only be specified once.
   - 0 means logging is disabled.  This is the default.
   - 1 means to include error messages only.
   - 2 means to include errors and warnings.
   - 3 means to include errors, warnings, and informational messages.
   - 4 means to include errors, warnings, informational messages, and internal debugging messages.
- **HookModule**, which names a hook module to load.  Multiple hook modules may be specified.
   - Hook modules have file names of the form `Name.HookModule.(32|64).dll`.  Only the `Name` part should be specified in the configuration file.
   - Example: a hook module distributed as `SuperFancy.HookModule.32.dll` and `SuperFancy.HookModule.64.dll` would be identified as `SuperFancy` in the configuration file.
- **Inject**, which identifies the file name a DLL to inject.  Either a full absolute path can be specified or just the name of the DLL.
   - Hookshot passes this path directly to the system as is.
   - The system's search order for locating DLLs determines how the path is interpreted.

Hookshot additionally allows per-executable scopes to be created and applies the settings contained within each to the executable whose name is also the section name.  Each per-executable scope can contain values for **HookModule** and **Inject**.

To better illustrate the composition of a Hookshot configuration file, consider an example in which three applications, `App1.exe`, `App2.exe`, and `App3.exe`, all located in `C:\Directory\`, are to be configured for use with Hookshot.  The contents of `C:\Directory\Hookshot.ini` is shown below.

```
LogLevel = 0
HookModule = SomeHookModule
Inject = C:\AnyLibrary.dll

[App1.exe]
HookModule = HooksA

[App2.exe]
HookModule = HooksB
Inject = C:\LibB.dll
```

This configuration file would result in the following behavior.
- Hook module `SomeHookModule` would be loaded for all of `App1.exe`, `App2.exe`, and `App3.exe`, and likewise the DLL file `C:\AnyLibrary.dll` would be injected into all three.
- Hook module `HooksA` would additionally be loaded for `App1.exe` only.
- Hook module `HooksB` would additionally be loaded for `App2.exe` only, and likewise the DLL file `C:\LibB.dll` would only be injected into `App2.exe`.

The behavior described above would be the same if the following equivalent configuration file were supplied instead.  Other than the fact that the global scope must be at the top of the file, the ordering of sections with respect to one another does not matter.  Furthermore, the ordering of `HookModule` and `Inject` settings within a section also generally does not matter.  Hookshot first loads hook modules in the order they appear in the configuration file and second injects DLLs in the order they appear in the configuration file.

```
[App2.exe]
HookModule = SomeHookModule
HookModule = HooksB
Inject = C:\LibB.dll
Inject = C:\AnyLibrary.dll

[App3.exe]
HookModule = SomeHookModule
Inject = C:\AnyLibrary.dll

[App1.exe]
HookModule = SomeHookModule
Inject = C:\AnyLibrary.dll
HookModule = HooksA
```


<h1 id="developers">For Developers</h1>

Text.


<h2 id="developers-gettingstarted">Getting Started</h2>

Text.
