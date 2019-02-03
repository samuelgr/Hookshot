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


; --------- TYPE DEFINITIONS --------------------------------------------------

; Defines the structure of the data exchanged between the injecting and injected processes.
; Corresponds to the identically-named structured type in "Inject.h" and must be updated when any changes are made there.
; See "Inject.h" for documentation.
SInjectData STRUCT
    sync SIZE_T ?
    ALIGN 128
SInjectData ENDS


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


ENDIF ; __HOOKSHOT_INJECT_INC
