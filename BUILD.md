# Building Hookshot

This document describes how to build Hookshot from its source code.


## Navigation

This document is organized as follows.

- [Prerequisites](#prerequisites)
- [Steps](#steps)

Other available documents are listed in the [top-level document](README.md).


## Prerequisites

In order to build Hookshot, the following software must be installed on the system.

- Windows 10 or 11
   - Hookshot is built to target Windows 10 or 11 and does not support older versions of Windows.

- Visual Studio 2022
   - Later versions of Visual Studio might also work, but this has not been tested.

- [Python for Windows](https://docs.python.org/3/using/windows.html) version 3.0 or newer
   - One of Hookshot's dependencies uses a build system implemented in Python.
   - Either the [full installer](https://www.python.org/downloads/windows) or the Microsoft Store package versions will work.
   - When installing Python for Windows, ensure Python is added to the PATH environment variable.
      - The full installer offers this option at install-time.
      - The Microsoft Store package does this automatically.


## Steps

Once the prerequisites have been met, Hookshot can be built using the following steps.

1. Build Hookshot's external dependencies using `ThirdParty\ThirdParty.sln`.
    - `ThirdParty.sln` and the associated Visual C++ projects act as a simple way of building all the third party dependencies from within Visual Studio.
    - It is recommended that batch building be used to build multiple configurations at the same time: at least Release Win32/x64, and optionally Debug Win32/x64.
    - There is no need to move or copy any of the build output files. Hookshot's build system will automatically locate them in place.
    - Hookshot depends on two third-party libraries:
      - [XED](https://github.com/intelxed/xed) from Intel, for understanding and manipulating x86 instructions. Used in HookshotDll.
      - [cpu_features](https://github.com/google/cpu_features) from Google, for checking which features the system's CPU supports. Used in HookshotTest.

1. Build Hookshot itself using `Hookshot.sln`.
    - It is recommended that batch building be used to build multiple configurations at the same time: at least Release Win32/x64, and optionally Debug Win32/x64.
    - Output files are placed into the `Output\` subdirectory.
    - After the builds complete, optionally run HookshotTest and verify that all the tests pass.
       - It is acceptable for some tests to be skipped.
       - Certain tests are only valid in 64-bit mode, and others depend on features present only on very modern CPUs.

1. Optionally build and run the hook module examples using `Examples\HookModuleExamples.sln`.
   - These examples are demonstrations of the various ways that hook modules can be built to interact with the Hookshot API.
   - The test program can be run on its own first so that its unmodified behavior can be observed prior to using the example hook modules.
   - To run an example, right-click its project in Visual Studio, click "Set as StartUp Project," then press F5 or click "Local Windows Debugger" in the toolbar.
   - Each example hook module is set up so that, when run this way, Visual Studio invokes the `RunExample.bat` script. This script creates a configuration file and then uses HookshotExe to run the test program.
