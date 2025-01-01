Hookshot enables compiled binary 32-bit (x86) and 64-bit (x64) applications to be modified by hooking function calls and injecting DLLs. For developers, Hookshot offers an extremely simple API for hooking function calls. These function call hooks can be encapsulated and distributed in the form of *hook modules* or they can included in a larger application that simply links against Hookshot. For end users, Hookshot makes it easy to use hook modules, while at the same time offering a convenient way to inject DLLs should the need arise. Because Hookshot acts in memory and at execution time, Hookshot does not make any changes to a target application's binary files on disk.


## Key Features

Hookshot supports both 32-bit (x86) and 64-bit (x64) Windows applications and offers the ability to:

- Hook function calls.

- Inject DLLs.

For developers, Hookshot offers a simple API for hooking functions, and for end users, Hookshot is easy to configure and use.


## Limitations

**Hookshot cannot operate in any way whatsoever on existing, already-running processes. This is by design.**  Hookshot can only hook function calls and inject DLLs into processes that Hookshot itself spawns under the control and direction of the end user.


## Further Reading

See the [Wiki](https://github.com/samuelgr/Hookshot/wiki) for complete documentation.
