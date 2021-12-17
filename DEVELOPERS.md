# Developing with Hookshot

This document describes how to write code that has access to, and uses, the Hookshot API. Its target audience is developers who wish to use Hookshot in their projects.


## Navigation

This document is organized as follows.

- [Getting Started](#getting-started)
- [Hookshot API](#hookshot-api)
  - [Low-Level API](#low-level-api)
    - [CreateHook](#createhook)
    - [DisableHookFunction](#disablehookfunction)
    - [GetOriginalFunction](#getoriginalfunction)
    - [ReplaceHookFunction](#replacehookfunction)
  - [Higher-Level Wrappers](#higher-level-wrappers)
    - [StaticHook](#statichook)
    - [DynamicHook](#dynamichook)
- [Writing a Hook Module](#writing-a-hook-module)  
  - [Hook Module Interface](#hook-module-interface)
  - [Debugging a Hook Module](#debugging-a-hook-module)
  - [Examples of Hook Modules](#examples-of-hook-modules)
- [Loading HookshotDll Without Hook Modules](#loading-hookshotdll-without-hook-modules)

Other available documents are listed in the [top-level document](README.md).


## Getting Started

1. If developing using tools other than Visual Studio 2022, ensure the [Visual C++ Runtime for Visual Studio 2022](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) is installed. Hookshot is linked against this runtime and will not run without it. If running a 64-bit operating system, install both the x86 and the x64 versions of this runtime, otherwise install just the x86 version.

1. Download the latest release of Hookshot and place all of the Hookshot executables and DLLs into any directory. Unless running a 32-bit operating system, it is recommended that both 32-bit and 64-bit versions of HookshotExe and HookshotDll be placed into this directory.

1. Place the Hookshot header files into a directory accessible by whatever build system is being used to build hook modules. Configure the hook module project's include paths accordingly. It is not necessary to link against HookshotDll.

1. Either write, build, and debug a hook module or link against Hookshot in existing code.


## Hookshot API

Function call hooks can be set and manipulated in one of two ways. First, Hookshot makes available its raw low-level API, which is just a set of interface methods that can be invoked. Second, Hookshot offers higher-level wrappers around its low-level API, which are intended to make the process of setting hooks more convenient and type-safe. Wherever possible, it is recommended that the latter be used, although the former remains available.


### Low-Level API

All Hookshot API functions are exposed as methods of an interface class. The interface class declaration is shown below and can be accessed by including the `Hookshot.h` header file. All methods are completely concurrency-safe. `EResult` is an enumeration of possible result codes. Refer to `HookshotTypes.h` to see the enumerators and their meanings.


```cpp
namespace Hookshot {
    class IHookshot {
    public:
        virtual EResult CreateHook(void* originalFunc, const void* hookFunc) = 0;
        virtual EResult DisableHookFunction(const void* originalOrHookFunc) = 0;
        virtual const void* GetOriginalFunction(const void* originalOrHookFunc) = 0;
        virtual EResult ReplaceHookFunction(const void* originalOrHookFunc, const void* newHookFunc) = 0;
    };
}
```


#### CreateHook

Causes Hookshot to attempt to hook the specified function. Requires that an original function and a hook function be specified.

##### Parameters

###### `originalFunc`

Address of the original function.

###### `hookFunc`

Address of the hook function.

##### Return Value

Indicator of the result of the operation. Refer to `HookshotTypes.h` to see possible enumerators and their meanings.

##### Notes

If this method succeeds, then 5 bytes are overwritten at the address specified by the `originalFunc` parameter. If this method fails, then the memory at the address specified by `originalFunc` is not modified.

This method will fail and indicate an invalid argument error if:
- `originalFunc` is `nullptr`.
- `hookFunc` is `nullptr`.
- `originalFunc` is already part of another function call hook.
- `hookFunc` is already part of another function call hook.
- `hookFunc` specifies an address within the part of `originalFunc` that will be overwritten.
- `originalFunc` specifies an address within HookshotDll.

Other than failures due to invalid arguments, a failure during `CreateHook` is not possible to correct at runtime. Such a failure typically represents a situation in which Hookshot is unable to create a particular hook for a very low-level technical reason. Determining the specific reason for the failure, beyond just the enumerator value, is possible by enabling logging or by running Hookshot through a debugger. Overcoming the reason for the failure may not be possible and in any case very likely requires modifications to Hookshot itself.

Hookshot does not check that the prototypes (return value type, parameter types, calling convention, etc.) of the original function and the hook function match. It is up to the developer to make sure they match exactly. Failure to do so can lead to the function call hook not working correctly even if `CreateHook` succeeds, in some situations leading to an application crash.

Hookshot does not offer the ability to remove a hook after it has been created. However, a hook can be disabled using the [`DisableHookFunction`](#disablehookfunction) method and then re-enabled using the [`ReplaceHookFunction`](#replacehookfunction) method.


#### DisableHookFunction

Disables the hook function associated with the specified hook. This has the effect of disabling the function call hook entirely, so that all future calls to the original function act as though there is no function call hook at all.

##### Parameters

###### `originalOrHookFunc`

Address of either the original function or the hook function.

##### Return Value

Indicator of the result of the operation. Refer to `HookshotTypes.h` to see possible enumerators and their meanings.

##### Notes

This method will fail if `originalOrHookFunc` does not correspond to an existing function call hook.

If this method succeeds, then Hookshot no longer associates the hook function with this function call hook. The hook function is free to be used with another function call hook. Additionally, the hook function's address can no longer be used as the value of `originalOrHookFunc` parameters.

To re-enable a function call hook after it has been disabled, invoke [`ReplaceHookFunction`](#replacehookfunction) and specify the hook function address as the `newHookFunc` parameter.


#### GetOriginalFunction

Retrieves and returns the address of a function that, when called, has the effect of invoking the original function as if it were not hooked. This is useful for accessing the original behavior of the function being hooked.

##### Parameters

###### `originalOrHookFunc`

Address of either the original function or the hook function.

##### Return Value

If this method succeeds, the return value is a function pointer that can be called to have the effect of invoking the original function.

If this method fails, the return value is `nullptr`.

##### Notes

This method will fail if `originalOrHookFunc` does not correspond to an existing function call hook.

Hookshot does not ensure that the prototype (return value type, parameter types, calling convention, etc.) at the time of invocation matches that of the original function. It is up to the developer to make sure they match exactly, such as by saving the returned pointer into a typed function pointer. Failure to do so can lead to unpredictable results, including an application crash in some situations.


#### ReplaceHookFunction

Modifies an existing function call hook by replacing its hook function.

##### Parameters

###### `originalOrHookFunc`

Address of either the original function or the hook function. These were respectively supplied to Hookshot as `originalFunc` and `hookFunc` during the initial `CreateHook` call.

###### `newHookFunc`

Address of the new hook function.

##### Return Value

Indicator of the result of the operation. Refer to `HookshotTypes.h` to see possible enumerators and their meanings.

##### Notes

This method will fail if `originalOrHookFunc` does not correspond to an existing function call hook, if `newHookFunc` is `nullptr`, or if `newHookFunc` is already part of another function call hook.

Hookshot does not ensure that the prototype (return value type, parameter types, calling convention, etc.) at the time of invocation matches that of the new hook function. It is up to the developer to make sure they match exactly. Failure to do so can lead to unpredictable results, including an application crash in some situations.

If this method succeeds, then the old hook function is no longer associated with this function call hook. Future calls to methods with `originalOrHookFunc` parameters will accept either the original function address or the address supplied as `newHookFunc`.


### Higher-Level Wrappers

Despite its flexibility, one of the key limitations of the low-level Hookshot API is that it does not offer any type-safety. In many situations it is up to the developer to ensure that all references to a function associated with a hook exactly match the original function's return type, parameter types, and calling convention. Hookshot offers higher-level wrappers that trade off some flexibility in exchange for convenience and automatic compiler-enforced type safety. For example, the higher-level wrappers do not support arbitrary replacement of hook functions, even though this is possible using the low level API method `ReplaceHookFunction`. They do, however, offer the ability to disable and re-enable the function call hook with ease.

Hookshot offers two API wrappers, *StaticHook* and *DynamicHook*. They both provide equivalent functionality but differ in two ways: how the return type, parameter types, and calling convention of the original function is specified, and when the address of the original function is known. StaticHook requires that the original function be declared in prototype form somewhere (likely a header file) and infers all type information from that prototype. It also requires that the address of the original function be available at compile-time or at link-time. DynamicHook, on the other hand, accepts the original function's address at runtime. It can extract type information from a function prototype, but it also supports more manual ways of constructing the function type. In both cases, the function type (including calling convention) is enforced when implementing the hook function and invoking the original function.

Both API wrappers are implemented as class templates whose template parameters are intended to form a unique way of identifying the original function. The only public-facing members of these classes are methods declared `static`. This is required so that each instantiation of a wrapper generates a new set of free functions, one set per function call hook. Linker errors result from attempting to create multiple hooks for the same function or forgetting to implement the hook function.


#### StaticHook

StaticHook is accessed via a macro declared in the `StaticHook.h` header file:

```cpp
#define HOOKSHOT_STATIC_HOOK(func)
```

The macro parameter is simply the name of the function to hook, whose prototype must be available somewhere, likely in a header file. For example, to create a StaticHook for the Windows API function `MessageBoxW`:

```cpp
#include <Windows.h>
HOOKSHOT_STATIC_HOOK(MessageBoxW);
```

This macro has the same semantics as a type definition, so it is safe to place in a header file. Continuing with the `MessageBoxW` example, the macro above essentially does the following.

```cpp
using StaticHook_MessageBoxW = ::Hookshot::StaticHook<"MessageBoxW", (void*)(&MessageBoxW), decltype(MessageBoxW)>
```

In other words, each use of the `HOOKSHOT_STATIC_HOOK` macro defines a new type alias for a template that Hookshot supplies. The alias is placed into the surrounding namespace. It uses the name, address, and entire prototype of the original function (including calling convention) to embed information into the type that is created. One limitation of this approach is that the scoping operator `::` cannot be part of the macro parameter. For example, `HOOKSHOT_STATIC_HOOK(my_namespace::Function);` would not work. A workaround is to place the `HOOKSHOT_STATIC_MACRO` invocation into the `my_namespace` namespace or use a `using` alias for `my_namespace::Function`.

The StaticHook template defines and makes available various convenient methods. These methods are shown below. Note that the template definition has been abbreviated to make the code snippet more clear. Calling convention, though not shown, is also part of the template specialization.

```cpp
namespace Hookshot {
    template <typename ReturnType, typename... ArgumentTypes> class StaticHook<...> {
    public:
        static ReturnType Hook(ArgumentTypes...);
        static ReturnType Original(ArgumentTypes...);
        static EResult SetHook(IHookshot* const hookshot);
        static EResult DisableHook(IHookshot* const hookshot);
        static EResult EnableHook(IHookshot* const hookshot);
    };
}
```

`Hook` is the hook function. This method must be implemented by the developer. Continuing with the `MessageBoxW` example from above, the implementation would look like the following.

```cpp
int StaticHook_MessageBoxW::Hook(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    // Code goes here.
}
```

The function body should be placed in a source file that has access to the `HOOKSHOT_STATIC_HOOK` type definition. Multiple implementations will result in a linker error. Function signature is taken from Microsoft's documentation on the `MessageBoxW` function. Note that it is not necessary to specify the calling convention here. The compiler will use the `MessageBoxW` StaticHook template specialization and automatically determine the correct convention for this function. Furthermore, the `Hook` method is typed, so an incorrect return value or parameter type will result in a compiler error.

`Original` invokes the function pointer returned from Hookshot via [`GetOriginalFunction`](#getoriginalfunction). In other words, this is how the original function is made available to the developer. It can be invoked both within and outside of the hook function. The method declaration is typed, meaning that the return value, number of parameters, and types of parameters are all enforced by the compiler.

`SetHook` must be called at runtime once the `IHookshot` interface pointer is available. Internally it invokes the [`CreateHook`](#createhook) method. Failing to call `SetHook` means the hook will not be functional.

`DisableHook` and `EnableHook`, as their names suggest, respectively disable and re-enable the function call hook by invoking [`DisableHookFunction`](#disablehookfunction) and [`ReplaceHookFunction`](#replacehookfunction) with appropriate parameters.


#### DynamicHook

DynamicHook is accessed via macros declared in the `DynamicHook.h` header file. Only one of these macros is needed per DynamicHook; they differ based on how the function type is constructed.

```cpp
// Option 1: Accepts a function prototype, just like StaticHook.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION(func)

// Option 2: Accepts a typed function pointer.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_POINTER(name, ptr)

// Option 3: Accepts a manually-constructed function type.
#define HOOKSHOT_DYNAMIC_HOOK_FROM_TYPESPEC(name, typespec)
```

For example, using the `MessageBoxW` Windows API function, we can specify an appropriate DynamicHook using any of these three options:

```cpp
// Option 1
HOOKSHOT_DYNAMIC_HOOK_FROM_FUNCTION(MessageBoxW);

// Option 2
static int(__stdcall * const functionPointerOfTypeMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT) = nullptr;
HOOKSHOT_DYNAMIC_HOOK_FROM_POINTER(MessageBoxW, functionPointerOfTypeMessageBoxW);

// Option 3
HOOKSHOT_DYNAMIC_HOOK_FROM_TYPESPEC(MessageBoxW, int(__stdcall)(HWND, LPCWSTR, LPCWSTR, UINT))
```

As with StaticHook, using one of these macros creates a type alias:

```cpp
using DynamicHook_MessageBoxW = ...    // Type definition varies by macro.
```

Unlike StaticHook, the address of the original function is not part of the type definition. The name of the function, however, is part of the type definition and is used to disambiguate multiple DynamicHook definitions with the same function signature. Everything else about DynamicHook is the same as with StaticHook, except that the `SetHook` function requires that the address of the original function be specified. For the sake of completeness, the DynamicHook template definition and its methods are shown below. Note that the template definition has been abbreviated to make the code snippet more clear. Calling convention, though not shown, is also part of the template specialization.

```cpp
namespace Hookshot {
    template <typename ReturnType, typename... ArgumentTypes> class DynamicHook<...> {
    public:
        static ReturnType Hook(ArgumentTypes...);
        static ReturnType Original(ArgumentTypes...);
        static EResult SetHook(IHookshot* const hookshot, void* const originalFunc);
        static EResult DisableHook(IHookshot* const hookshot);
        static EResult EnableHook(IHookshot* const hookshot);
    };
}
```

`Original` and `Hook` return types and parameter types are enforced by the compiler. `Original` invokes the original function, and `Hook` is implemented in the same way. For `MessageBoxW`, irrespective of the macro used to create the DynamicHook, this looks like:

```cpp
int DynamicHook_MessageBoxW::Hook(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    // Code goes here.
}
```

Note that, in the case of macro options 2 and 3, the name supplied as the first macro parameter (shown as `MessageBoxW` in the example) can be anything. The name of the generated type would be modified accordingly. For example, if `MyNiceMessageBox` were supplied as the first macro parameter instead of `MessageBoxW`, then the resulting type would be `DynamicHook_MyNiceMessageBox`.


## Writing a Hook Module

A hook module is simply a DLL that complies with Hookshot-imposed interface requirements. Developers can create hook modules to encapsulate the modifications they make to applications, and these hook modules can subsequently be distributed. This section describes how hook modules are created and debugged. The last of its subsections outlines the various examples of hook modules that are included with Hookshot.


### Hook Module Interface

Hookshot requires that the hook module export an entry point function called `HookshotMain` using a specific calling convention and with a specific signature. `HookshotMain` is invoked by HookshotDll after the hook module has been loaded and after any call to [`DllMain`](https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain) has completed. The complete prototype for `HookshotMain` is located in `Hookshot.h`, and an empty example implementation is shown below.

```cpp
extern "C" __declspec(dllexport) void __fastcall HookshotMain(Hookshot::IHookshot* hookshot) {
    // This is the entry point function declared and exported by a hook module.
}
```

As this prototype contains quite a bit of boilerplate code, Hookshot makes a vailable a macro named `HOOKSHOT_HOOK_MODULE_ENTRY` that can optionally be used instead. Thus, the above example is identical to the below example.

```cpp
HOOKSHOT_HOOK_MODULE_ENTRY(hookshot) {
    // This is the entry point function declared and exported by a hook module.
    // Macro parameter specifies the name of the variable to use for the function parameter.
}
```

Both of these examples show the minimum required code to create a hook module. Simply placing one of these entry point definitions into a C++ source file that includes `Hookshot.h` and building it into a DLL that follows the hook module naming convention is sufficient to produce a valid hook module that Hookshot will load. Hook modules are named according to the templates `[Name].HookModule.32.dll` and `[Name].HookModule.64.dll`, depending on whether the DLL is compiled for use in 32-bit mode or 64-bit mode. In general, hook modules should be made available in both. Hook modules should not link against HookshotDll.


It is by calling `HookshotMain` that Hookshot provides the module with an instance of `IHookshot` that it can use to access the Hookshot API. The pointer supplied to the hook module is valid throughout the lifetime of the process, and the object to which the pointer refers remains under Hookshot's ownership at all times. Hook modules may cache and use the pointer as needed but must never attempt to deallocate it.

At the time that `HookshotMain` is invoked, the state of the process is as follows.
- All of the target application's dependent DLLs have also been loaded into memory, and their `DllMain` function calls have completed.
- The target application itself is loaded into memory, but it has not yet started running. Even the runtime library upon which it depends is uninitialized.
- No injected DLLs have been loaded.
- The hook module has completed its execution of `DllMain`.

There are no restrictions on what it is safe to do inside the `HookshotMain` function. Furthermore, because the target application has not yet started running, `HookshotMain` provides the safest opportunity to use the Hookshot API to create function call hooks.

It is strongly recommended that all hooks be created during `HookshotMain` and not thereafter. This is because, once the target application starts running, it is not possible to guarantee that no threads are executing the part of the original function being overwritten by Hookshot during a call to `CreateHook`.


### Debugging a Hook Module

Hookshot is designed with debuggability in mind. To debug a hook module using a debugger:

1. Set up Hookshot to load the hook module to be debugged by following the [instructions on how to use Hookshot](USERS.md). This means creating an appropriate directory structure, authorizing Hookshot to act on the target application, optionally providing a configuration file, and generating the correct command line for running HookshotExe.

1. Start HookshotExe through a debugger using the generated command line. HookshotExe spawns a new process and exits, so the initial debugging session will be very short.
   - HookshotExe specifically checks if it is being debugged. If so, when it spawns the new process and injects HookshotDll, it informs HookshotDll of this fact.
   - In this situation, HookshotDll pauses execution before loading any hook modules or injected DLLs and displays a message box. This message box shows the target application's executable file name and process ID.

1. When prompted by HookshotDll, attach the debugger to the indicated process. Set break points as desired in the hook module, then dismiss the message box.
   - These break points will appear unbound at the time they are set because the hook module has not yet loaded. However, once Hookshot loads the hook module, they should bind and become effective.
   - Break points can be set anywhere that contains executable code in the hook module. This includes within `HookshotMain` and within hook functions.
   - Break points should not be set close to the beginning of an original function because the technique Hookshot uses to implement function call hooks is not compatible with the technique debuggers typically use to implement break points.

The preceding steps have been verified with the debugger included with Visual Studio 2022. Other debugging tools might need some adjustments to make this process work.

When running through a debugger, Hookshot outputs all internal messages, irrespective of severity, as debug strings that show up in the debugger's output window. If logging is enabled while a debugger is attached, messages up to the configured severity will be written to the log file as usual.


### Examples of Hook Modules

Included with Hookshot are some complete code and build system examples of simple hook modules. They all demonstrate how to hook the `MessageBoxW` Windows API function to modify graphical message boxes in various ways. A test program is also supplied, which just invokes `MessageBoxW` and then exits. To run the examples from Visual Studio, follow the [instructions on how to build Hookshot from source](BUILD.md). Otherwise they can be built and subsequently run manually by following the [instructions on how to use Hookshot](USERS.md).

All examples are contained within the `Examples\HookModuleExamples.sln` Visual Studio solution file, and each is encapsulated within its own Visual C++ project file. The examples are as follows.

- `TestProgram` is the test program that calls `MessageBoxW`. This program is the intended target application for the example hook modules.
- `Empty` is a skeleton project, showing the minimum amount of code needed to produce a hook module. The build system is set up to follow the correct DLL naming conventions. This hook module does not create any hooks, so running it does not result in any modifications to the test program's displayed message box.
- `HookshotFunctions` shows how to use the [low level API functions](#low-level-api) to hook `MessageBoxW`.
- `StaticHook` shows how to use the [StaticHook higher-level wrapper](#statichook) to hook `MessageBoxW`.
- `DynamicHook` shows how to use the [DynamicHook higher-level wrapper](#dynamichook) to hook `MessageBoxW`.


## Loading HookshotDll Without Hook Modules

While creating a hook module is the primary way of accessing the Hookshot API, it is also possible to use Hookshot without creating a hook module. HookshotTest, the Visual C++ project that contains Hookshot's unit tests, is an example of a project that loads HookshotDll by linking with it directly rather than by offering it a hook module to load. Loading HookshotDll as a library is useful when a larger application or library already exists that does not need Hookshot to inject HookshotDll into a new process but rather needs Hookshot for its implementation of function call hooks.

Loading HookshotDll can be done either by linking with the HookshotDll import library or by loading HookshotDll dynamically at run time. In both cases, the initialization function `HookshotLibraryInitialize` must be invoked to initialize HookshotDll and gain access to the Hookshot API. The prototype for `HookshotLibraryInitialize` is available via the `Hookshot.h` header file and is shown below. On success, the return value is the `IHookshot` interface pointer, and on failure, the return value is `nullptr`. Calling this function multiple times will result in `nullptr` for all calls after the first. Once `HookshotLibraryInitialize` completes successfully, the Hookshot API is available to the caller.

```cpp
extern "C" Hookshot::IHookshot* __fastcall HookshotLibraryInitialize(void);
```

If linking with the HookshotDll import library, define the preprocessor symbol `HOOKSHOT_LINK_WITH_LIBRARY`. This way, the initialization function is made available as a function imported from HookshotDll. It can then be called directly wherever appropriate in the project's code.

If loading HookshotDll dynamically at runtime, do not define the preprocessor symbol `HOOKSHOT_LINK_WITH_LIBRARY`. Instead, make use of the `Hookshot::TLibraryInitializeProc` type definition to create a function pointer. Save to that function pointer the result of a call to `GetProcAddress` using the string constant `Hookshot::kLibraryInitializeProcName` as the procedure name parameter.

When HookshotDll is loaded as a library, it does not attempt to load any hook modules or injected DLLs, and it does not attempt to inject itself into child processes. HookshotDll still looks for and attempts to read a configuration file, but the only setting it applies from the configuraition file is `LogLevel`.
