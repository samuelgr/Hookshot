;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Inject.asm
;   Implementation of all code that gets injected into another process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE Functions.inc
INCLUDE Inject.inc
INCLUDE Preamble.inc
INCLUDE Registers.inc
INCLUDE Strings.inc


kStrInjectCodeSectionName                   SEGMENT READ


; --------- TRAMPOLINE --------------------------------------------------------

; Injected to the entry point of a process.
; Initial value is to be overwritten with the actual target address within the code region.
; Address of the initial value is injectTrampolineAddressMarker - sizeof(size_t) in C/C++.

injectTrampolineStart:

    push sax
    mov sax, initialvalue

injectTrampolineAddressMarker:

    call sax

injectTrampolineEnd:


; --------- MAIN CODE ---------------------------------------------------------

; Injected into the code region of a process.
; Reserved bytes at the beginning are to be overwritten with the address of the data region.

injectCodeStart:

    DSZ 0

injectCodeBegin:
    
    ; Save all other general-purpose registers.
    ; Register sax was already saved by the trampoline.
    push sbx
    push scx
    push sdx
    push ssi
    push sdi
    push sbp

    ; Fix up the stack.
    ; Switch the order of the return address and the old value of sax.
    ; Reset the return address to point to the program's original entry point.
    mov sax, SIZE_T PTR [ssp+(7*SIZEOF(SIZE_T))]
    mov scx, SIZE_T PTR [ssp+(6*SIZEOF(SIZE_T))]
    mov SIZE_T PTR [ssp+(6*SIZEOF(SIZE_T))], sax
    sub scx, (injectTrampolineEnd-injectTrampolineStart)
    mov SIZE_T PTR [ssp+(7*SIZEOF(SIZE_T))], scx

    ; Fix up the stack.
    ; Ensure it is aligned on a 16-byte boundary.
    stackAlignPush
    
    ; Get the address of the data region.
    call $next
  $next:
    pop sbp
    mov sbp, SIZE_T PTR [sbp-($next-injectCodeStart)]

    ; Initialize, then synchronize with the injecting process.
    injectSyncInit ssi, sdi
    injectSync ssi, sdi, sbp

    ; Injecting process is filling in required structure values.
    ; Wait for it to finish.
    injectSync ssi, sdi, sbp
    
    ; Set up the stack for API calls.
    stackStdCallShadowPush
    
    ; Load the library specified by the injecting process.
    mov scx, (SInjectData PTR [sbp]).strLibraryName
    mov sax, (SInjectData PTR [sbp]).funcLoadLibraryA
    call1ParamStdCall sax
    cmp sax, 0
    je $errorLoadLibrary

    ; Locate the initialization procedure.
    mov scx, sax
    mov sdx, (SInjectData PTR [sbp]).strProcName
    mov sax, (SInjectData PTR [sbp]).funcGetProcAddress
    call2ParamStdCall sax
    cmp sax, 0
    je $errorGetProcAddress

    ; Invoke the initialization procedure.
    call sax
    cmp sax, 0
    je $errorInitialization
    
    ; Indicate success.
    mov ecx, (SInjectData PTR [sbp]).injectionResultCodeSuccess
    mov (SInjectData PTR [sbp]).injectionResult, ecx

  $done:
    ; All injection operations are done.
    ; Perform one final synchronization with the injecting process.
    ; If the injection process failed for any reason, then the injecting process will terminate this process before it passes this barrier.
    injectSync ssi, sdi, sbp
    
    ; Transfer control to the injection library.
    ; Control only gets to this point on successful injection.
    jmp sax
  
  $errorLoadLibrary:
    ; Store the correct error codes and end the operation.
    mov eax, (SInjectData PTR [sbp]).injectionResultCodeLoadLibraryFailed
    mov (SInjectData PTR [sbp]).injectionResult, eax
    mov sax, (SInjectData PTR [sbp]).funcGetLastError
    call0ParamStdCall sax
    mov (SInjectData PTR [sbp]).extendedInjectionResult, eax
    jmp $done
  
  $errorGetProcAddress:
    ; Store the correct error codes and end the operation.
    mov eax, (SInjectData PTR [sbp]).injectionResultCodeGetProcAddressFailed
    mov (SInjectData PTR [sbp]).injectionResult, eax
    mov sax, (SInjectData PTR [sbp]).funcGetLastError
    call0ParamStdCall sax
    mov (SInjectData PTR [sbp]).extendedInjectionResult, eax
    jmp $done
  
  $errorInitialization:
    ; Store the correct error codes and end the operation.
    mov eax, (SInjectData PTR [sbp]).injectionResultCodeInitializationFailed
    mov (SInjectData PTR [sbp]).injectionResult, eax
    mov sax, (SInjectData PTR [sbp]).funcGetLastError
    call0ParamStdCall sax
    mov (SInjectData PTR [sbp]).extendedInjectionResult, eax
    jmp $done

injectCodeEnd:


kStrInjectCodeSectionName                   ENDS


kStrInjectMetaSectionName                   SEGMENT READ


    SInjectMeta <kInjectionMetaMagicValue, 0, x1, x2, x3, x4, x5, x6>


kStrInjectMetaSectionName                   ENDS


x1 EQU (injectTrampolineStart-injectTrampolineStart)
x2 EQU (injectTrampolineAddressMarker-injectTrampolineStart)
x3 EQU (injectTrampolineEnd-injectTrampolineStart)
x4 EQU (injectCodeStart-injectTrampolineStart)
x5 EQU (injectCodeBegin-injectTrampolineStart)
x6 EQU (injectCodeEnd-injectTrampolineStart)


END
