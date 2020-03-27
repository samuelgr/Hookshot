;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Preamble.inc
;   Definitions that should appear at the start of a portable assembly file.
;   Helps with 32/64-bit portability of the actual assembly code.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_PREAMBLE_INC
__HOOKSHOT_PREAMBLE_INC EQU 1


; --------- DIRECTIVES --------------------------------------------------------

IFDEF HOOKSHOT32
; 32-bit preamble
; Required to generate valid code for modern processors
.686

; 32-bit data and pointer size
SIZE_T TEXTEQU <DWORD>
DSZ TEXTEQU <DD>

; 32-bit unsigned constant
; Used as a placeholder for an immediate 32-bit value that is to be replaced before code execution
initialvalue EQU 01234567h
ENDIF



IFDEF HOOKSHOT64
; 64-bit data and pointer size
SIZE_T TEXTEQU <QWORD>
DSZ TEXTEQU <DQ>

; 64-bit unsigned constant
; Used as a placeholder for an immediate 64-bit value that is to be replaced before code execution
initialvalue EQU 0123456789abcdefh
ENDIF


ENDIF ; __HOOKSHOT_PREAMBLE_INC
