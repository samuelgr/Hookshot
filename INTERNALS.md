# Hookshot Internals{#top}

This documentation covers the internals of Hookshot.  It is intended for those who wish to modify Hookshot itself or just learn about how Hookshot is designed and implemented.

Function call hooking and code injection are very intricate and technical operations, and accordingly discussions about how Hookshot works tend to be very low-level.  This document assumes the reader has a basic understanding of virtual memory and assembly code on the x86 architecture.


## Navigation{#navigation}

This document is organized as follows.

- [Technical Design](#design)
   - [Library Injection](#design-inject)
   - [Function Call Hooking](#design-hook)
- [Hookshot Implementation](#implementation)
   - [Source Code Organization](#implementation-sln)
   - [Key Operations](#implementation-ops)

Other available documents are listed in the [top-level document](README.md).


## Technical Design{#design}

This section discusses the principles techniques Hookshot uses to support its feature set without specific reference to its source code.  Hookshot's most fundamental operations really boil down to achieving two key goals.  First, Hookshot must get its own code running inside the target process' address space.  Second, Hookshot's own code, which is now running in the target process, must be capable of hooking function calls.

Achieving the first key goal is challenging because the target application is generally not programmed to load Hookshot code.  This is why HookshotExe exists: its sole purpose is to force the target application to load HookshotDll.  Once HookshotDll is running inside the target application's process, the job of HookshotExe is completed.  HookshotDll achieves the second key goal by implementing Hookshot's feature set.

The remainder of this section is split into two subsections, each covering one of the key goals in more detail.  The first, *Library Injection*, focuses on the specific mechanism by which HookshotExe injects HookshotDll into the target application's process.  The second, *Function Call Hooking*, explores how HookshotDll hooks a function call.  In both cases, various alternative designs are described, and the trade-offs between different possible designs are evaluated.


### Library Injection{#design-inject}

In selecting a mechanism by which HookshotExe forces the target process to load HookshotDll, we must be cognizant of the intended purpose of HookshotDll: to hook function calls.  While in theory hooking a function call is nothing more than editing the contents of memory in a specific way, in practice we must remember that we are editing regions of memory generally expected by the target application to be read-only.  This gives rise to concurrency control challenges.

It is not safe to edit a process' executable code while that process is running.  This is because any of the threads of the target process could be attempting to execute the area of memory that HookshotDll would be overwriting.  Even atomically updating memory is insufficient.  To see why, consider the 32-bit assembly code below.

```asm
    push eax
    push ebx
    push ecx
    push edx
```

Each shown instruction is one byte, for a total of four bytes.  Suppose Hookshot replaces this entire sequence with a single four-byte instruction.  Even if the update happens atomically, a thread that just finished `push ebx` and is about to start `push ecx` would potentially read an invalid instruction, leading to either a completely different operation taking place or, more likely, a program crash.

Concurrency control mechanisms cannot be used to solve this problem because the target application is not expecting its code to change at arbitrary times and therefore is not designed to take a lock before executing this sequence of instructions.  If concurrency control were introduced as a way of overcoming this issue, this necessarily would require changing the code of the target application, which itself gives rise to the same problem but in a different memory location.

There are two possible ways of dealing with this problem:
- Suspend all threads in the process, except the thread making the update.
   - Iterate over the contexts of all threads and ensure that none of them contain instruction pointers directed at any instruction in the sequence.
      - For threads that do, either manually undo the instruction (difficult) or let it run a bit longer until it clears the affected area (much easier, but not guaranteed to work if the thread is running in a tight loop).
   - The problem with this approach is that, on Windows, getting a list of threads in a process involves [taking a snapshot of all the threads in the system](https://docs.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot).
      - If the target process continues to create new threads after the snapshot is already taken, then some threads will be missed.
      - Taking a new snapshot does not guarantee a solution either, because new threads can still be created before all have been suspended.
- Hook all functions before any threads exist that could possibly cause this type of interference.
   - This can be achieved by ensuring all function call hooks are set before the target process starts running.

With safety in mind, a key design principle of HookshotExe is therefore that it injects HookshotDll into the target process before the target process begins running.  In [*Extending Applications using an Advanced Approach to DLL Injection and API Hooking*](http://lkm.fri.uni-lj.si/zoranb/research/berdajs-bosnic%20SPE%202011.pdf), Berdajs and Bosnić identify another benefit to this design decision: it becomes possible to hook function calls made during initialization.  These could be missed by function call hooks that are set after the process has been running even for a short time.

"Before the target process starts running" is a fairly vague definition because there are multiple points that HookshotDll could be injected that would satisfy this requirement:
- Before the Windows loader runs in the target process.
   - The target process is completely uninitialized.
   - None of the DLLs on which it depends have been loaded.
   - Operating system data structures are mostly not filled in.
   - HookshotDll would have very little functionality available to it due to the uninitialized state of the process.
   - Loading a DLL at this stage would require that HookshotExe implement its own loader because, by definition, the Windows loader has not yet been started.
- While the Windows loader is running in the target process.
   - DLLs, including HookshotDll, are being loaded, and their `DllMain` entry points are being executed by the Windows loader.
   - HookshotDll would need to do all its work in `DllMain`, which comes with [very severe functional limitations](https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).
      - One such limitation is that loading more DLLs is forbidden, so neither hook modules nor injected DLLs could be loaded.
- After the Windows loader runs in the target process, but before control is transferred to the target process' entry point.
   - All DLLs have been loaded and operating system data structures filled in.
   - Not a single instruction in the target application's executable has been executed yet, not even to initialize its runtime environment.
   - If control were transferred to HookshotDll at this time, there would be no restrictions on what it is allowed to do.

The only of these options that allows HookshotDll unrestricted functionality while still ensuring no interference from threads in the target process is the last one.  Therefore, HookshotExe will need to inject HookshotDll and transfer control to it after the Windows loader has completed running but before control transfers to the target process' entry point.

Several known techniques exist for [injecting DLLs on Windows](https://en.wikipedia.org/wiki/DLL_injection).  The registry-based techniques are unsuitable for Hookshot because they apply globally to all programs.  Furthermore, on Windows 10 these avenues for loading DLLs are either disabled or require libraries to be digitally signed.  Application-specific techniques are also unsuitable because by their nature they cannot be applied uniformly to any arbitrary target application.  The only potential techniques listed are those that involve process manipulation: `CreateRemoteThread` and `SetWindowsHookEx`, for example.  However, all such listed techniques require that the target process already be running, thus making them incompatible with the key design principle identified previously.

HookshotExe implements a custom technique for injecting HookshotDll into a target process.  Because it is required that the injection take place before the target process starts running, the only safe option is for HookshotExe to spawn a new process and inject that process.  Spawning a new process also allows HookshotExe to guarantee that it has full access to the [new process](https://docs.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights) and its [main thread](https://docs.microsoft.com/en-us/windows/win32/procthread/thread-security-and-access-rights) and is therefore able to, among other things manipulate the contents of the new process' memory.

To inject a process with HookshotDll, HookshotExe performs the following operations.
1. Creates the new process using `CreateProcess`.  The process is created using the `CREATE_SUSPENDED` [process creation flag](https://docs.microsoft.com/en-us/windows/win32/procthread/process-creation-flags), so upon creation it does not run.

1. Allocates some memory in the new process' address space.

1. Copies over an injection payload into the newly-allocated buffer.  The injection payload consists of code and data.

1. Takes a backup of the code at the target process' entry point and then overwrites it with an unconditional `jmp` instruction that transfers control to the injection payload code.

1. Lets the target process run.  This causes it to jump immediately to the injection payload code which, among other things, does the following.
   - Fixes up the stack so that the return address is the target process' entry point address.
   - Calls `LoadLibrary` to load HookshotDll.  At this point HookshotDll's `DllMain` entry point is invoked by the Windows loader.
   - Calls `GetProcAddress` to locate and invoke HookshotDll's initialization function.  The initialization function runs and returns the address of a control transfer landing point inside HookshotDll.
   - Hits a synchronization barrier, which it cannot pass at this point.

1. Suspends the target process while it is waiting at the synchronization barrier.

1. Sets the flag in the target process' address space that will allow it to advance past the synchronization barrier once it is resumed.

1. Restores the original contents of memory at the target process' entry point using the backup that was previously taken.  This way, when the target process executes a `ret` instruction from its current stack frame it will return control to the intended entry point of the target process, and the target process will run normally.

1. Resumes execution of the target process.  The target process advances past the synchronization barrier and executes a `jmp` instruction targetting the control transfer landing point received from HookshotDll.

At this point, control has successfully transferred to HookshotDll after the Windows loader is finished but before the target application has started running.  Once HookshotDll has finished loading hook modules and injected DLLs, it executes a final `ret` instruction to allow the target process to run as intended.


### Function Call Hooking{#design-hook}

In §5 of [*A Survey on Function and System Call Hooking Approaches*](https://www.researchgate.net/publication/319970632_A_Survey_on_Function_and_System_Call_Hooking_Approaches), Lopez, Babun, Aksu, and Uluagac describe various known techniques for hooking function calls.  Static hooks (§5.2) generally require changes to the operating system or runtime environment.  [Xidi](https://www.github.com/samuelgr/Xidi), for example, uses the "Injecting Proxy Libraries" technique (§5.2.1): it exploits the behavior of the dynamic library loader to cause it to load its own alternate versions of DLLs and, in doing so, intercepts all the calls made to the functions exported by those DLLs.  For this to work, however, Xidi must provide an implementation of every single function exported by those DLLs that are imported by the target application, and even when this does work Xidi's scope is limited to just the functions exported by the DLLs it proxies.

Given the limitations of static hooks, Hookshot favors dynamic hooks (§5.1).  The two key techniques for dynamic hooking are "Inline Hooking" (§5.1.1) and "Import Address Table Modification" (§5.1.2).  Whereas the former can be used to hook any function, the latter is limited to hooking functions exported by DLLs that are part of the import table.  In other words, the only functions that can be hooked through IAT modification are those that are linked to the target application and its dependent DLLs.  These are also the functions that show up as being imported in an import table viewer like [Dependency Walker](https://www.dependencywalker.com).  Any functions exported by DLLs loaded dynamically at runtime, such as by using `LoadLibrary`, are excluded.  Because inline hooking modifies code directly, rather than updating pointers in the import address table, it can be applied to any arbitrary function, even functions internal to the target application itself.

Due to its flexibility, Hookshot uses inline hooking to implement its function call hooks: it overwrites the first 5 bytes of the functions being hooked with a `jmp` instruction whose operand is a 32-bit relative branch displacement.  Hookshot uses the term *hook function* to refer to what might otherwise be known as the *detour function*, the latter being a term from the [Microsoft Research Detours](https://www.microsoft.com/en-us/research/project/detours) project (specifically, the [wiki](https://github.com/microsoft/Detours/wiki)).

Unlike the single-trampoline-function implementation in Detours and described by Lopez, Babun, Aksu, and Uluagac, Hookshot uses two trampoline functions: a "hook" trampoline to transfer control to the hook function and an "original" trampoline whose behavior matches the cited literature's definition of *trampoline function*.  The "hook" trampoline contains an unconditional `jmp` instruction that targets the hook function using its full (32-bit or 64-bit) absolute address.  In 32-bit mode this is unnecessary because a 32-bit relative branch displacement can reach the entire address space.  In 64-bit mode, however, having the ability to supply a full 64-bit hook function address eliminates any constraints on the distance in memory between hook function and original function.  Without this feature, it is possible that hooking a function could fail due to the DLL containing the hook function being loaded in memory too far away from the original function.

Each function call hook is associated with a single 64-byte trampoline buffer, which is divided into a 48-byte "original" section and a 16-byte "hook" section.  These sizes, while somewhat erring towards the generous side, were selected to ensure individual trampolines are cache-line aligned and arrays of trampolines can be page-aligned.  While the code that is placed into the "original" section varies based on the original function, the code that is placed into the "hook" section is almost entirely static.  The "hook" section code is shown below, first the 32-bit version and then the 64-bit version.  Quantities shown in angle brackets are placeholders.

```asm
    ; 32-bit
    nop                             ; 9-byte form
    nop                             ; 2-byte form
    jmp near ptr <hook_func_rel32>  ; 1-byte opcode + 4 bytes for the rel32 branch displacement

    ; 64-bit
    nop                             ; 2-byte form
    jmp QWORD PTR [rip]             ; 6 bytes
    QWORD <hook_func_address>       ; 8 bytes, overwritten with the 64-bit address of the hook function
```

Much more challenging is filling the "original" section of the trampoline because this requires moving instructions from one location in memory to another.  While many instructions perform the same operation and access the same data irrespective of its position in memory, there are several instructions whose position matters.  We will call the former *position-independent instructions* and the latter *position-dependent instructions*.

Examples of position-independent instructions:
- `push eax`
- `xor QWORD PTR [rcx+128], rax`
- `rdrand`

Examples of position-dependent instructions:
- Unconditional `jmp` and conditional `jcc`-type instructions whose operand is a relative branch displacement
- `loop`, whose operand is an 8-bit relative branch displacement
- `xbegin`, whose operand is a 16-bit or 32-bit relative branch displacement
- `add rax, QWORD PTR [rip+64]` and any other instruction that uses RIP-relative addressing in 64-bit mode

Position-independent instructions can simply be copied from the original function to the "original" trampoline.  This change in location does not impact their ability to perform their intended operations.  Position-dependent instructions, on the other hand, require more work.  Specifically, they must be patched so that the memory operand still refers to the same address even after being moved to a new location.  Hookshot does this by replacing the value of the displacement in the memory operand itself.  To see how this works in principle, observe that the `add` instruction in all three of the 64-bit instruction sequences below refer to the same value 1 in memory.  The only difference between them is the displacement (i.e. the number that is added to `rip` to obtain the effective address of the operand), which varies based on the location of the `add` instruction relative to the value it references.  All `add` and `jmp` instructions are respectively encoded as 7 and 5 bytes in length.

```asm
    jmp label1

label1:
    add rax, QWORD PTR [rip+13]
    jmp label2

    QWORD 0
    QWORD 1
    QWORD 2
    QWORD 3

label2:
```

```asm
    jmp label1

    QWORD 0
    QWORD 1

label1:
    add rax, QWORD PTR [rip-15]
    jmp label2

    QWORD 2
    QWORD 3

label2:
```

```asm
    jmp label1
    
    QWORD 0
    QWORD 1
    QWORD 2
    QWORD 3

label1:
    add rax, QWORD PTR [rip-31]
    jmp label2

label2:
```

Simply replacing the displacement value itself is not always sufficient.  There are two additional complexities that must be considered: if the absolute value of the displacement is too small and if it is too large.  It may seem counterintuitive to worry about what happens if the absolute value of the displacement is too small.  However, it is possible that the displacement is small enough that the effective address also falls within the memory range being moved to the trampoline.  Hookshot detects this scenario and handles it by moving the position-dependnent instruction without patching its displacement.  This behavior is reflected in the example below.  Labels refer to the location of the code, be it in the original function or in the "original" part of the trampoline.  Recall that 5 bytes of original function are overwritten, so at least 5 bytes (in this case, exactly 5 bytes) need to be moved.

```asm
original_function:
    xor eax, eax                        ; 2 bytes
    je short label_original             ; 2 bytes, displacement value is 0
label_original:
    nop                                 ; 1-byte form
original_function_continued:
    ...                                 ; the rest of the original function

trampoline:
   xor eax, eax
   je short label_trampoline            ; no change to displacement value because it is small enough
label_trampoline:
   nop
   jmp original_function_continued
```

The second complexity is when the absolute value of the displacement is too large.  In 32-bit mode, a position-dependent instruction whose displacement is 32 bits wide can always be moved because a 32-bit displacement is wide enough to refer to any address in the entire address space.  In 64-bit mode, the new displacement value might be out of the range of representable 32-bit values, in which case Hookshot will not be able to move the instruction and will accordingly fail to complete the function call hook.  In both 32-bit and 64-bit modes, there are some instructions that use relative branch displacements encoded with less than 32 bits.  Short `jmp` and `jcc`-type instructions, for example, support 8-bit displacements, and `loop` can only be encoded to use an 8-bit displacement.  Most likely the new displacement value will be out of range for an 8-bit or a 16-bit displacement encoding.

Hookshot uses a technique called *jump assist* to solve this problem.  The jump assist technique works by inserting into the trampoline an extra unconditional `jmp` instruction with a full 32-bit relative branch displacement operand.  The problematic instruction with an 8-bit or 16-bit displacement targets the extra `jmp` instruction using its original encoded displacement width, and the `jmp` instruction targets the original effective address of the problematic instruction.  These extra `jmp` instructions are allocated starting from the end of the trampoline's 48-byte buffer and working backwards.  To see how this works, consider the code example below and recall that by the time this is executing the original function address is overwritten with the 5-byte instruction `jmp trampoline`.

```asm
original_function:
    loop some_label                     ; 2 bytes, 8-bit encoding for the relative branch displacement
    nop                                 ; 3-byte form
original_function_continued:
    ...                                 ; the rest of the original function
some_label:
    ...                                 ; some other code in the loop
    jmp original_function

trampoline:
    loop jump_assist_label              ; 2 bytes, 8-bit encoding for the relative branch displacement
    nop                                 ; 3-byte form
    jmp original_function_continued
    ...                                 ; the rest of the trampoline buffer space
jump_assist_label:
    jmp some_label                      ; 32-bit encoding for the relative branch displacement    
```

Inline hooking has one inherent limitation that cannot easily be avoided.  It is possible that a jump target exists within the second through fifth bytes of the original function.  Relocating the instructions at the first five bytes of the original function has the unfortunate effect of breaking the jump target, which could lead to a program crash.  Short of scanning every instruction in the program, there is no way to detect this scenario, and the only way to avoid it is to use a different hooking technique altogether.  The risk is small but nevertheless worth acknowledging.  The example code sequence below illustrates the problem.

```asm
original_function:
    mov al, 100                         ; 2 bytes, this instruction will be moved
loop:
    cmp al, 0                           ; 2 bytes, this instruction will be moved
    je end                              ; size does not matter, this instruction will be moved

    ...                                 ; other code in the loop

    dec al
    jmp loop                            ; jump target is within the range of moved instructions

end:
    ret
```

The first three instructions in this function would be moved into a trampoline if this function were hooked.  The `jmp loop` instruction targets the second of these instructions.  Once the first five bytes of this function are overwritten, the target of the `jmp loop` instruction is no longer the intended instruction.  We could scan the rest of this function for references to the first few instructions and patch those references to refer to the trampoline instead, but this does not sufficiently solve the problem for two reasons.  First, there could be additional references from outside the function.  Second, the new displacement value could be too wide for the instruction's encoding, and since there is no guaranteed slack space available we cannot use the jump assist technique here.


## Hookshot Implementation{#implementation}

This section describes the contents and organization of Hookshot's source code, with specific focus on how the principles and techniques developed [previously](#design) are implemented.  Accordingly, this section assumes the reader is familiar with the previous section.  First is a tour of Hookshot's Visual Studio solution file and all of its Visual C++ projects, including a brief description of the code modules (source and header file combinations).  Second is a sketch of the control flow that implements Hookshot's two primary goals of library injection and function call hooking.


### Source Code Organization{#implementation-sln}

This subsection details the specific contents of each Visual C++ project that is contained within the top-level Hookshot solution file.  It is organized by project.


#### HookshotBin{#implementation-sln-bin}

HookshotBin consists of a single source file, `Inject.asm`, which implements the entire contents of the injection payload code and data.  It is divided into two segments, which have non-standard names so that HookshotExe can easily identify the injection code and data sections separately and without confusing them for any code and data that the compiler or linker might insert.  The output from building this project is a binary file that is embedded into HookshotExe as a raw binary resource of type `RCDATA`.  Linker output is a specially-formatted DLL: the linker command-line option `/FILEALIGN:1` is used so that all of the relative virtual addresses included in the PE headers are equal to byte offsets within the file itself.  This makes Hookshot's job easier when it comes time to locate code and data within the file.

Using an assembly file with a metadata section is intended to make the injection payload easier to maintain and edit in the future.  An alternative appraoch would have been to assemble the code and include the resulting binary in C++ source as a byte array, but modifying the code is cumbersome when the payload is in this form.  HookshotExe loads values from the data section of HookshotBin to tell it how the code and data sections are organized, and these values are updated automatically by the assembler in response to any possible changes to the injection payload implementation.

Assembly header files are used to ensure that C++ code and assembly code agree on segment names and data type layouts.  They are also used to enable the assembly code to be written once and be valid for both 32-bit and 64-bit mode.  For example, `Registers.inc` defines register names using `s` as a prefix, and these names expand to the widest possible register given the execution mode, in the same vein as using the `size_t` data type in C++.


#### HookshotDll{#implementation-sln-dll}

HookshotDll compiles to a dynamic library.  Most commonly it is injected by HookshotExe into a process, but it also supports direct linking and loading by applications that only need Hookshot for its ability to hook function calls.  HookshotDll knows how it was loaded into the application and offers slightly different behavior depending whether it was injected or loaded normally as a library.  In the former case, HookshotDll loads hook modules and injected DLLs.  In the latter case, it provides the module that loaded it access to its API.

The remainder of this subsection is a module-by-module summary of the code that comprises HookshotDll.  Modules are presented in increasing alphabetical order.


##### Configuration

Configuration is a general-purpose subsystem for parsing files formatted in standard INI form: name-value pairs optionally namespaced into named sections.  This code can easily be reused outside of Hookshot, as all of the logic for determining what values are allowed to be in a configuration file, and their types, is implemented by subclasses.


##### DependencyProtect

DependencyProtect introduces a layer of abstraction into all of the calls Hookshot itself makes to certain external dependencies.  Currently only the Windows API is abstracted this way.  Instead of invoking the API function directly, most invocations go through this class to invoke using a pointer that can be updated.

The purpose of DependencyProtect is to ensure that developers using Hookshot cannot inadvertantly hook one of its dependencies and interfere with Hookshot's ability to implement its feature set.  A side effect is that Hookshot bypasses any hooks developers attempt to set on the API functions that Hookshot itself uses to implement function call hooks.


##### Globals

Globals provides static storage for miscellaneous global variables that do not fit anywhere else.  These values are initialized automatically when the present running form of Hookshot is loaded into memory and include the instance handle for the running form of Hookshot, process ID, process pseudo-handle, and so on.  This module is compiled into both HookshotDll and HookshotExe.


##### HookStore

HookStore is the top-level data structure that stores and identifies all function call hooks.  Internally the data structure is statically allocated, and all instances just provide an interface to it.  This interface is actually the implementation of the external-facing Hookshot API interface, `IHookshot`.


##### HookshotConfigReader

HookshotConfigReader applies Hookshot-specific customizations to the Configuration module.  It defines the supported layout of a Hookshot configuration file.


##### InjectLanding

InjectLanding implements the control transfer landing point within HookshotDll.  This module contains three functions:
- `InjectLanding`, which is the main entry point for HookshotDll after it accepts control from HookshotExe.  This function invokes the other two functions in this module, cleans up, and executes the final `ret` instruction to transfer control back to the target application's entry point.  Only a prototype of this function exists in C++ because it is written in assembly.  The address of this function is returned to the injection payload code after the HookshotDll initialization function completes.
- `InjectLandingCleanup`, which frees the buffers that HookshotExe allocated for the injection payload.
- `InjectLandingLoadHookModules`, which loads all hook modules and injected DLLs specified in the Hookshot configuration file.


##### InternalHook

InternalHook is an interface, similar to the external StaticHook interface, used within Hookshot to set hooks for Hookshot's own use.  Internal hooks are completely transparent to the developers of hook modules.

Currently, Hookshot uses InternalHook to hook the `CreateProcessA` and `CreateProcessW` Windows API functions.  This is how HookshotDll causes itself to be injected into the child processes spawned by a target application.


##### LibraryInterface

LibraryInterface implements the interfaces between HookshotDll and the outside world.  It provides functions for initializing HookshotDll, loading hook modules, loading injected DLLs, and obtaining `IHookshot` interface pointers, among other things.


##### Message

Message is a general-purpose subsystem for outputting messages.  The functions offered by this module are invoked throughout both HookshotDll and HookshotExe.  It defines the concept of a "message severity" and, based on the severity of a particular message, determines if the message should be output.  Message also examines the state of the process and uses that information to determine the most appropriate form of message output.  It supports three output modes: debugger string, log file, and graphical message box.  This module is compiled into both HookshotDll and HookshotExe.


##### RemoteProcessInjector

Refer to the description of this module in HookshotExe.  It is compiled into both forms of Hookshot and is a close counterpart to the ProcessInjector module exclusive to HookshotExe.


##### Strings

Strings holds and makes available a variety of useful string constants.  Some are compile-time constants, whereas others are dynamically generated at runtime.  The types of strings of interest are the filenames of configuration files, other possible forms of Hookshot, the function name of a hook module's entry point, and so on.  This module is compiled into both HookshotDll and HookshotExe.


##### TemporaryBuffer

TemporaryBuffer offers a general-purpose for allocating, deallocating, and managing large temporary buffers that can be used for any purpose.  Its intention is to replace the typical pattern of stack-allocating large arrays with a much smaller stack-allocated object that is associated with a statically-allocated buffer.

Internally, TemporaryBuffer allocates a large buffer as a global variable, so this is contained inside a data region within the Hookshot module in memory.  The total size and number of such buffers is configurable, and it is up to the user of the module to determine the lifetime of an individual temporary buffer.  If an attempt is made to allocate a temporary buffer and all of the statically-allocated buffer space is already consumed, TemporaryBuffer heap-allocates the buffer.  On destruction, whatever buffer space is linked to a particular TemporaryBuffer instance is automatically deallocated.


##### Trampoline

Trampoline encapsulates all of the logic that implements both of the trampolines associated with a single function call hook.  Its only data element is a 64-byte buffer split into the 16-byte "hook" and 48-byte "original" regions.  The two methods `SetHookFunction` and `SetOriginalFunction` encapsulate all of the logic required to fill, respectively, the "hook" and "original" parts of the trampoline.  In the latter case, this involves copying instructions from the original function and patching any position-dependent instructions as required.


##### TrampolineStore

TrampolineStore manages a block of memory into which Trampoline objects are placed.  Because each Trampoline object contains executable code, TrampolineStore ensures that these blocks are allocated with the correct virtual memory permissions specified.


##### X86Instruction

X86Instruction encapsulates all of the data and logic required to hold, decode, re-encode, and understand the semantics of a single binary instruction.  Its class methods offer the ability to fill memory buffers with `nop` instructions and write `jmp` instructions.

Instances of X86Instruction are created in the `SetOriginalFunction` method of Trampoline, each used to represent one of the instructions loaded from the original function address. X86Instruction wraps [Intel's XED library](https://intelxed.github.io) with a convenient API for querying if an instruction is position-dependent and, if so, modifying its displacement value.


#### HookshotExe{#implementation-sln-exe}

HookshotExe compiles to an executable whose only job is to inject HookshotDll into a process. It supports two modes of execution:
- **Execution with a command-line specified.**  In this mode, HookshotExe creates a new process using the given command-line, injects HookshotDll into the new process, and then exits.  This is the method documented for end users that bootstraps the process of running a target application with Hookshot.
- **Execution with a shared memory handle specified.**  This mode of execution arises when another running form of Hookshot has created a process and needs HookshotExe to inject it.  The command-line argument to HookshotExe specifies the handle value for a shared memory region that contains an instance of `SInjectRequest` (declared in `RemoteProcessInjector.h`), which itself contains all of the data needed to complete the request and return the result.  HookshotExe performs the requested injection, writes results to the shared memory region, and then exits.  This method is used when:
   - HookshotExe creates a process to inject, but there is an architecture mismatch between HookshotExe and the new process (i.e. 32-bit HookshotExe and 64-bit new process or vice versa).  In this situation, HookshotExe spawns an instance of the other architecture version of HookshotExe and uses this method to request that the injection be completed.
   - HookshotDll intercepts an attempt by the target application to spawn a child process.  In this case, HookshotDll creates the process and then uses this method to request that HookshotExe inject it.

The remainder of this subsection is a module-by-module summary of the code that comprises HookshotExe.  Modules are presented in increasing alphabetical order.


##### CodeInjector

CodeInjector's primary responsibility is to copy the injection payload to the address space of the target process.  It leans heavily on the `InjectInfo` class (defined in the Inject module) to obtain information on the injection payload.  Part of copying the injection payload involves filling in an instance of `SInjectData`.  This structured type defines the layout of the data portion of the injection payload.  Some members are filled in by CodeInjector before the target process is allowed to run the injection payload, and others are filled in by the injection payload code to indicate the results of various attempted operations.

Once the payload is copied, CodeInjector allows the target process to run.  It uses the macros defined in the Inject module to synchronize in lock-step with the execution of the injection payload in the target process.  One of the fields of `SInjectData` is a synchronization flag, which is used to implement a simple inter-process spinlock-based synchronization barrier.  Once the injection payload code reaches its final synchronization barrier, CodeInjector suspends the process and retireves the result codes written by the injection payload code to certain fields in `SInjectData`.  At this point its job is completed.


##### Inject

Inject is the C++ interface to the injection payload that is embedded into HookshotExe in the form of HookshotBin.  It provides all of the functionality needed to load, parse, and obtain information about the injection payload, exporting said information to other modules like CodeInjector.  Inject also defines the macros that CodeInjector uses to synchronize in lock-step with the execution of the injection payload code.


##### Globals

Refer to the description of this module in HookshotDll.


##### Message

Refer to the description of this module in HookshotDll.


##### ProcessInjector

ProcessInjector kicks off the process of injecting a process with HookshotDll.  It allocates the code and data regions for the injection payload in the address space of the target process (leaning on `InjectInfo` to know the correct size) and then relies on CodeInjector to do the rest.

Arguably the most important function in ProcessInjector is `InjectProcess` which, as its name suggests, is the main entry point for injecting a process.  `InjectProcess` is an internal function declared and defined in `ProcessInjector.cpp`.  It is invoked with a parameter that identifies the process to inject.  External entry points to ProcessInjector support HookshotExe's two modes of execution: one accepts a command-line and then calls `CreateProcess`, and the other accepts an `SInjectRequest` instance to identify an already-created process.


##### RemoteProcessInjector

RemoteProcessInjector is a small counterpart to ProcessInjector.  Its only purpose is to spawn an instance of HookshotExe, providing it with an instance of `SInjectRequest` via a shared memory region.

This logic is separate from ProcessInjector because both HookshotDll and HookshotExe have the ability to make such a request.  Including ProcessInjector in HookshotDll would also necessitate including CodeInjector and Inject, neither of which are necessary.


##### Strings

Refer to the description of this module in HookshotDll.


##### TemporaryBuffer

Refer to the description of this module in HookshotDll.


#### HookshotTest{#implementation-sln-test}

HookshotTest contains all of the unit tests that exercise Hookshot's ability to hook functions.  It is built as an executable that links against HookshotDll and simply runs through all of the tests that are valid given the current system's execution mode and CPU features.  Its three primary modules are `HookSetFail`, `HookSetSuccess`, and `Custom`, and these work as follows.
- `HookSetFail` captures a common test pattern that exercises a situation in which Hookshot is expected to fail to set a hook.  The sequences of instructions with which Hookshot is presented are designed such that it should detect its inability to set the hook.
- `HookSetSuccess` captures a common test pattern that exercises a situation in which Hookshot is expected to set a hook successfully.  This is where Hookshot is presented with a wide range of instruction patterns, all of which it is expected to be able to handle.
- `Custom` are special one-off tests with their own implementations.  Each is described in the associated source file, `Custom.cpp`.


### Key Operations{#implementation-ops}

This subsection provides an overview of the control flow between the various modules in HookshotExe and HookshotDll, illustrating how they come together in a cohesive implementation that achieves Hookshot's two key goals of library injection and function call hooking.  It is organized by key goal.


#### Library Injection{#implementation-ops-inject}

For the purpose of this description, we assume HookshotExe is executed in the first of its two supported modes: it is given the command line of a target application and tasked with both spawning a new process and injecting it with HookshotDll.

Library injection proceeds as follows.

1. **HookshotExe entry point in `ExeMain.cpp`**
    - Parses command-line parameters and passes them to the ProcessInjector module.

1.  **`ProcessInjector::CreateInjectedProcess`**
    - Uses the Windows API function `CreateProcess` to spawn the new process in suspended state.
    - Makes a record of whether it was invoked and asked to create a suspended process or a running process.
    - Once the process is successfully spawned, it is passed to another method for injection.

1.  **`ProcessInjector::InjectProcess`**
    - Verifies that the end user has authorized Hookshot to act on the target process.
    - If the target process has an architecture mismatch, uses RemoteProcessInjector to do the injection.  We assume this is not the case for the purposes of this explanation.
    - Allocates space in the target process' address space to hold the injection payload, determines the target process' entry point address, and leaves the rest to CodeInjector.

1. **`CodeInjector::SetAndRun`**
    - Performs some checks, and then invokes some other methods in the same class.

1. **`CodeInjector::Set`**
    - Copies over the injection payload, both code and data.  Missing information is not yet available because it comes from system data structures in the target process' address space, which are not filled in until the target process is allowed to run.
    - Initializes most fields in an instance of `SInjectData` and copies that instance to the data section of the injection payload.

1. **`CodeInjector::Run`**
    - Allows the target process to run until it hits the injection payload code.  At this point the system has filled in data structures for the target process.
    - Fills in the remaining areas of the data section, such as the addresses within the target process' address space of some Windows API functions that the injection payload needs to execute.  This information is not available before this point because it depends on the loader having run and the system having filled in data structures.
    - Allows the target to run, while synchronizing in lock-step with its execution of the injection payload code.

1. **Injection payload in HookshotBin's `Inject.asm`**
    - Fixes up the stack so that, among other things, the return address is the target process' entry point.
    - Invokes `LoadLibrary` (address is specified in the data section) to load HookshotDll.
    - Invokes `GetProcAddress` (address is specified in the data section) to find the location of the `HookshotInjectInitialize` function.
    - Invokes `HookshotInjectInitialize` and saves the return value, which is the address of the `InjectLanding` function within HookshotDll.
    - Writes various result codes to the data section.
    - Hits a synchronization barrier it cannot pass.

1. **`CodeInjector::Run`**
    - Suspends the target process.
    - Writes the correct flag value to its data section so it can pass the synchronization barrier next time it is allowed to run.
    - Reads the result values written by the injection payload code and returns appropriate result codes.

1. **`CodeInjector::SetAndRun`**
    - Returns whatever code it receives from the `Run` method.

1. **`ProcessInjector::InjectProcess`**
    - Returns whatever code it receives from CodeInjector.

1. **`ProcessInjector::CreateInjectedProcess`**
    - If the return code does not indicate success, the target process is terminated.
    - Otherwise, if invoked and asked to create a running (i.e. not suspended) process, the target process is resumed.

At this point the job of HookshotExe is completed: HookshotDll is loaded and running in the target process.  To implement automatic injection of target processes, HookshotDll internally hooks the `CreateProcessA` and `CreateProcessW` Windows API functions, and the hook functions HookshotDll provides uses RemoteProcessInjector.  These hooks are implemented transparently to developers using the Hookshot API, meaning that the Hookshot API can still be used to create hooks for these functions.


#### Function Call Hooking{#implementation-ops-hook}

Creation of a function call hook proceeds as follows.

1. **`HookStore::CreateHook`**
    - Performs some argument checks.  Examples:
        - Neither of `originalFunc` nor `hookFunc` are null.
        - Addresses `originalFunc` and `hookFunc` are separated by a safe distance.
        - Address `originalFunc` does not fall within the HookshotDll module itself.  Hooking Hookshot is not permitted.
    - Locates the TrampolineStore object that corresponds to the original function.
        - Behavior differs based on whether we are in 32-bit or 64-bit mode, since in the latter case the Trampoline object needs to be close to the original function.
        - See code for more details on the algorithm for finding the right TrampolineStore object.
    - Requests a new Trampoline object be allocated in the TrampolineStore and, if that is not possible because it is full, attempts to create a new TrampolineStore object to hold more Trampoline objects.
    - Invokes some Trampoline methods to fill both the "hook" and "original" regions of the newly-allocated Trampoline object.
    - Invokes `UpdateProtectedDependencyAddress` (declared in `DependencyProtect.h`).
    - Invokes `HookStore::RedirectExecution`.

1. **`Trampoline::SetHookFunction`**
    - Fills the contents of the Trampoline object's "hook" region with an appropriate `jmp` instruction targetting the hook function.

1. **`Trampoline::SetOriginalFunction`**
    - Fills the contents of the Trampoline object's "original" function by copying, and possibly patching, instructions from the original function address.
    - First, reads instruction into an ordered container of X86Instruction objects, stopping after at least 5 bytes have been read.
    - Second, checks for position-dependent instructions and patches as appropriate.  After each instruction is checked and potentially patched, it is written into the trampoline.  Jump assists are added as needed.
    - Third, appends a `jmp` instruction, if needed, to the rest of the original function (i.e. the part that was not copied to the trampoline).  This step is skipped if the trampoline holds the entire original function.

1. **`UpdateProtectedDependencyAddress`**
    - If the original function address matches one of the protected dependency addresses, the pointer is updated to point to the "original" region of the Trampoline, thus bypassing the hook.

1. **`HookStore::RedirectExecution`**
    - Overwrites the first 5 bytes of the original function address with a `jmp` instruction targetting the "hook" region of the Trampoline.

At this point the function call hook has been set successfully.
