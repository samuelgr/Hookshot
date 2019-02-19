;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for hooking API calls in spawned processes.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Inject.inc
;   Assembly interface to injection data structures defined in C/C++.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_INJECT_INC
__HOOKSHOT_INJECT_INC EQU 1


INCLUDE Preamble.inc


; --------- CONSTANTS ---------------------------------------------------------

kInjectionMetaMagicValue EQU 51525354h


; --------- TYPE DEFINITIONS --------------------------------------------------

; Defines the structure of the data exchanged between the injecting and injected processes.
; Corresponds to the identically-named structured type in "Inject.h" and must be updated when any changes are made there.
; See "Inject.h" for documentation.
SInjectData STRUCT
    sync SIZE_T ?
    ALIGN 128
    
    injectionResultCodeSuccess DWORD ?
    injectionResultCodeLoadLibraryFailed DWORD ?
    injectionResultCodeGetProcAddressFailed DWORD ?
    injectionResultCodeInitializationFailed DWORD ?
    ALIGN 128

    injectionResult DWORD ?
    extendedInjectionResult DWORD ?
    ALIGN 128

    funcGetLastError SIZE_T ?
    funcGetProcAddress SIZE_T ?
    funcLoadLibraryA SIZE_T ?
    strLibraryName SIZE_T ?
    strProcName SIZE_T ?
    ALIGN 128
SInjectData ENDS

; Corresponds to the identically-named structured type in "Inject.cpp" and must be updated when any changes are made there.
; See "Inject.cpp" for documentation.
SInjectMeta STRUCT
    magic DWORD ?
    version DWORD ?

    offsetInjectTrampolineStart DWORD ?
    offsetInjectTrampolineAddressMarker DWORD ?
    offsetInjectTrampolineEnd DWORD ?
    offsetInjectCodeStart DWORD ?
    offsetInjectCodeBegin DWORD ?
    offsetInjectCodeEnd DWORD ?
SInjectMeta ENDS


; --------- MACROS ------------------------------------------------------------

; Sets initial state for synchronization between the injecting and injected processes.
; Registers passed in as parameters must be reserved for synchronization purposes only.
injectSyncInit MACRO ssv1, ssv2
    mov ssv1, 1
    mov ssv2, 2
ENDM

; Performs a synchronization operation between the injecting and injected processes.
; Parameters ssv1 and ssv2 must be the same as before.
; Parameter ssid is a register holding the base address of the SInjectData structure.
injectSync MACRO ssv1, ssv2, ssid
    LOCAL $syncwait

    ; Write to the sync flag the value that the injecting process expects to read out of it.
    mov (SInjectData PTR [ssid]).sync, ssv1

    ; Wait for the injecting process to respond by writing the value that the injected process is expecting.
  $syncwait:
    cmp (SInjectData PTR [ssid]).sync, ssv2
    jne $syncwait
    
    ; Increment sync values for the next sync operation.
    add ssv1, 2
    add ssv2, 2
ENDM


IFDEF HOOKSHOT32
; Invokes an __stdcall function that takes no parameters.
; Function address must be placed in sfunc.
; Return value, if any, is loaded into sax.
call0ParamStdCall MACRO sfunc
    call sfunc
ENDM

; Invokes an __stdcall function that takes one integer or pointer parameter.
; Per the __stdcall convention, the argument must be placed onto the stack.
; Parameter must be placed in scx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call1ParamStdCall MACRO sfunc
    push scx
    call sfunc
ENDM

; Invokes an __stdcall function that takes two integer or pointer parameters.
; Per the __stdcall convention, the arguments must be placed onto the stack.
; Parameters must be placed in scx and sdx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call2ParamStdCall MACRO sfunc
    push sdx
    push scx
    call sfunc
ENDM
ENDIF

IFDEF HOOKSHOT64
; Invokes an __stdcall function that takes no parameters.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Function address must be placed in sfunc.
; Return value, if any, is loaded into sax.
call0ParamStdCall MACRO sfunc
    sub ssp, (4*SIZEOF(SIZE_T))
    call sfunc
    add ssp, (4*SIZEOF(SIZE_T))
ENDM

; Invokes an __stdcall function that takes one integer or pointer parameter.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Parameter must be placed in scx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call1ParamStdCall MACRO sfunc
    sub ssp, (4*SIZEOF(SIZE_T))
    call sfunc
    add ssp, (4*SIZEOF(SIZE_T))
ENDM

; Invokes an __stdcall function that takes two integer or pointer parameters.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Parameters must be placed in scx and sdx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call2ParamStdCall MACRO sfunc
    sub ssp, (4*SIZEOF(SIZE_T))
    call sfunc
    add ssp, (4*SIZEOF(SIZE_T))
ENDM
ENDIF


ENDIF ; __HOOKSHOT_INJECT_INC
