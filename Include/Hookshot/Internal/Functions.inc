;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Functions.inc
;   Macros to assist with API function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_FUNCTIONS_INC
__HOOKSHOT_FUNCTIONS_INC EQU 1



INCLUDE Preamble.inc
INCLUDE Registers.inc


; --------- MACROS ------------------------------------------------------------

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

; Sets up shadow stack space for use with calling __stdcall functions.
; Not required in 32-bit mode.
stackStdCallShadowPush MACRO
ENDM

; Cleans up shadow stack space after calling __stdcall functions.
; Not required in 32-bit mode.
stackStdCallShadowPop MACRO
ENDM
ENDIF

IFDEF HOOKSHOT64
; Invokes an __stdcall function that takes no parameters.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Function address must be placed in sfunc.
; Return value, if any, is loaded into sax.
call0ParamStdCall MACRO sfunc
    call sfunc
ENDM

; Invokes an __stdcall function that takes one integer or pointer parameter.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Parameter must be placed in scx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call1ParamStdCall MACRO sfunc
    call sfunc
ENDM

; Invokes an __stdcall function that takes two integer or pointer parameters.
; Per the x64 convention, a shadow space must be allocated in the stack for all four integer parameters.
; Parameters must be placed in scx and sdx, and the function address in sfunc.
; Return value, if any, is loaded into sax.
call2ParamStdCall MACRO sfunc
    call sfunc
ENDM

; Sets up shadow stack space for use with calling __stdcall functions.
; In 64-bit mode arguments are passed in registers but still need to be allocated on the stack.
stackStdCallShadowPush MACRO
    sub ssp, (4*SIZEOF(SIZE_T))
ENDM

; Cleans up shadow stack space after calling __stdcall functions.
; In 64-bit mode arguments are passed in registers but still need to be allocated on the stack.
stackStdCallShadowPop MACRO
    add ssp, (4*SIZEOF(SIZE_T))
ENDM
ENDIF

; Ensures the stack is aligned on a 16-byte boundary.
; Causes things to be pushed onto the stack to achieve this alignment.
; Uses sax as scratch.
stackAlignPush MACRO
    mov sax, ssp
    and sax, 15
    add sax, (16-SIZEOF(SIZE_T))
    sub ssp, sax
    push sax
ENDM

; Undoes the operations that previously aligned the stack on a 16-byte boundary.
; Causes things to be popped off the stack.
; Uses sax as scratch.
stackAlignPop MACRO
    pop sax
    add ssp, sax
ENDM


ENDIF ; __HOOKSHOT_FUNCTIONS_INC
