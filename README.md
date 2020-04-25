# Hookshot

Hookshot enables compiled binary 32-bit (x86) and 64-bit (x64) applications to be modified by hooking function calls and injecting DLLs. For developers, Hookshot offers an extremely simple API for hooking function calls. These function call hooks can be encapsulated and distributed in the form of *hook modules* or they can included in a larger application that simply links against Hookshot. For end users, Hookshot makes it easy to use hook modules, while at the same time offering a convenient way to inject DLLs should the need arise. Because Hookshot acts in memory and at execution time, Hookshot does not make any changes to a target application's binary files on disk.


## Navigation

This document is intended to provide an introduction to Hookshot at a very high level. It is organized as follows.

- [Key Features](#key-features)
- [Limitations](#limitations)
- [Concepts and Terminology](#concepts-and-terminology)
  - [Roles of Developers and End Users](#roles-of-developers-and-end-users)
  - [Parts of Hookshot](#parts-of-hookshot)
- [Next Steps](#next-steps)

Hookshot's documentation additionally includes the documents listed below.

- [Using Hookshot](USERS.md)
  - Topics include how to set up and configure Hookshot.
  - Target audience is end users.
- [Developing with Hookshot](DEVELOPERS.md)
  - Topics include how to write a hook module, how to debug a hook module, and how link with Hookshot without creating a hook module.
  - Target audience is developers who wish to use the Hookshot API.
- [Building Hookshot](BUILD.md)
  - Topics include how to build Hookshot from source.
- [Hookshot Internals](INTERNALS.md)
  - Topics include how Hookshot is designed, why Hookshot is designed the way it is, and what each of Hookshot's source code modules (i.e. combination of header file and source file) does.
  - Target audience is anyone who is interested in how library injection and function call hooks work and anyone whose goal is to make modifications to Hookshot's code.
  - This is a very low-level and technical document. It is assumed that the reader has a basic understanding of virtual memory and assembly code on the x86 architecture.


## Key Features

Hookshot supports both 32-bit (x86) and 64-bit (x64) Windows applications and offers the ability to:

- Hook function calls.

- Inject DLLs.

For developers, Hookshot offers a simple API for hooking functions, and for end users, Hookshot is easy to configure and use.


## Limitations

**Hookshot cannot operate in any way whatsoever on existing, already-running processes. This is by design.**  Hookshot can only hook function calls and inject DLLs into processes that Hookshot itself spawns under the control and direction of the end user.


## Concepts and Terminology

*Hooking a function call* means redirecting execution from any function (the *original function*) to a different function (the *hook function*). Whenever a call is made to the original function, that call is transparently diverted to the hook function instead. Developers retain the ability to invoke the original function even after it is hooked.

A hook function can be coded to do anything of the developer's choosing. For example, it could monitor the target application's function calls. This is achieved by making a note that the function call was made and then forwarding the call to the original function. Alternatively, it could modify the behavior of the target application in one of two ways. First, it could change the parameters before forwarding them to the original function. Second, it could do something else entirely and never invoke the original function at all.

*Injecting a DLL* means forcing an application to load a DLL it would not ordinarily load. End users who are in possession of self-contained DLLs that they wish to cause an application to load will find that Hookshot offers a convenient mechanism for performing the injection.

*Target application* refers to the program that is launched and modified by Hookshot. It is identified to Hookshot using the full path of its executable file. Hookshot spawns a new process using the command line with which it was provided, which can include command line arguments to pass to the target application. After the new process is spawned, but before it starts running, Hookshot causes it to load hook modules and injected DLLs.

*Hook modules* can be viewed conceptually as *dynamic patches*: *dynamic* because  that are applied by Hookshot in memory and at execution time, and *patches* because they are used to change an application's behavior by modifying its code. In practice, a hook module is a DLL that has access to the Hookshot API by virtue of having been built to comply with certain Hookshot-imposed requirements.

*Injected DLLs* are arbitrary DLLs that the user asks Hookshot to load into the target application. The difference between a hook module and an injected DLL is that Hookshot does not interact with injected DLLs in any way after loading them. Hook modules have access to the Hookshot API, whereas injected DLLs do not. Hookshot offers the ability to inject arbitrary DLLs as a convenience to users who are already in possession of self-contained DLLs and simply need an injection mechanism.


### Roles of Developers and End Users

Developers who wish to use Hookshot do so in one of two ways. First, developers can create hook modules, which can be distributed to end users. A hook module can be built with the intention of targeting a single specific application or it can be built to work with multiple target applications. Second, developers can use the Hookshot API in their own code by linking it with Hookshot.

End users can use Hookshot to achieve one or both of the following goals. First, end users can modify the behavior of applications by obtaining and using hook modules. Second, end users can inject arbitrary DLLs into applications. These should be performed only under the direction of the respective developers of the hook modules and DLLs. It may also be the case that the end user and the developer are one and the same.


### Parts of Hookshot

Hookshot itself consists of the following parts.
- An executable, *HookshotExe*, which is used to bootstrap the process of hooking functions and injecting DLLs. HookshotExe spawns a new process using the target application's executable file and injects HookshotDll into it.
   - Command line arguments must be supplied to HookshotExe to specify the command line to use when launching the target application.
- A library, *HookshotDll*, which implements the Hookshot API. HookshotDll contains all of the logic required to hook functions and load DLLs.
   - Upon injection into a process by HookshotExe, HookshotDll reads a configuration file and uses it to determine which hook modules and DLL files to load.
   - Once HookshotDll is injected into a target process, and with the end user's permission, it will automatically attempt to cause itself to be injected into child processes spawned by the target process.


## Next Steps

Review some of the [other documents](#navigation) that make up Hookshot's documentation. They are divided by target audience and intended to be reviewed in order, based on the reader's level of interest.
