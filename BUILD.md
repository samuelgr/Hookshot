# Building Hookshot{#top}

This document describes how to build Hookshot from its source code.


## Navigation{#navigation}

This document is organized as follows.

- [Prerequisites](#prereq)
- [Steps](#steps)

Other available documents are listed in the [top-level document](README.md).


## Prerequisites{#prereq}

In order to build Hookshot, the following software must be installed on the system.

- Windows 10
   - Hookshot is built to target Windows 10 and does not support older versions of Windows.

- Visual Studio 2017
   - Hookshot's build system uses Visual Studio, and Hookshot releases were originally built using Visual Studio 2017 Community Edition.
   - Later versions of Visual Studio might also work, but this has not been tested.

- [Python for Windows](https://www.python.org/downloads/windows/) version 3.0 or newer
   - One of Hookshot's dependencies uses a build system implemented in Python.
   - When installing Python for Windows, ensure Python is added to the PATH environment variable.  This is an option offered by the installer.


## Steps{#steps}

Once the prerequisites have been met, Hookshot can be built using the following steps.

1. Build Hookshot's dependencies using `ThirdParty\ThirdParty.sln`.
    - `ThirdParty.sln` and the associated Visual C++ projects act as a simple way of building all the third party dependencies from within Visual Studio.
    - It is recommended that bulk building be used to build multiple configurations at the same time: at least Release Win32/x64, and optionally Debug Win32/x64.
    - Hookshot depends on two third-party libraries:
      - [XED](https://github.com/intelxed/xed) from Intel, for understanding and manipulating x86 instructions.  Used in HookshotDll.
      - [cpu_features](https://github.com/google/cpu_features) from Google, for checking which features the system's CPU supports.  Used in HookshotTest.

1. Build Hookshot itself using `Hookshot.sln`.
    - It is recommended that bulk building be used to build multiple configurations at the same time: at least Release Win32/x64, and optionally Debug Win32/x64.
    - After the builds complete, optionally run HookshotTest and verify that all the tests pass.
      - It is acceptable for some tests to be skipped.
      - Certain tests are only valid in 64-bit mode, and others depend on features present only on very modern CPUs.

1. Optionally build and run the hook module examples using `Examples\HookModuleExamples.sln`.
   - These examples are demonstrations of the various ways that hook modules can be built to interact with the Hookshot API.
   - The test program can be run on its own first so that its unmodified behavior can be observed prior to using the example hook modules.
   - To run an example, right-click its project in Visual Studio, click "Set as StartUp Project," then press F5 or click "Local Windows Debugger" in the toolbar.
   - Each example hook module is set up so that, when run this way, Visual Studio invokes the `RunExample.bat` script.  This script creates a configuration file and then uses HookshotExe to run the test program.
