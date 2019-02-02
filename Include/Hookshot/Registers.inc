;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for hooking API calls in spawned processes.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Registers.inc
;   Size-dependent name mappings for registers used throughtout Hookshot.
;   Helps with 32/64-bit portability of the actual assembly code.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_REGISTERS_INC
__HOOKSHOT_REGISTERS_INC EQU 1


; --------- REGISTER NAME MAPPINGS --------------------------------------------

; The mappings that follow create register names with 's' as a prefix.
; Each mapping expands to the corresponding register at the largest-available size.
; Size depends on whether assembling for a 32-bit or a 64-bit target binary.
; Only registers supported in both modes are included.

IFDEF HOOKSHOT32
; 32-bit mappings
sax TEXTEQU <eax>
sbx TEXTEQU <ebx>
scx TEXTEQU <ecx>
sdx TEXTEQU <edx>
ssi TEXTEQU <esi>
sdi TEXTEQU <edi>
sbp TEXTEQU <ebp>
ssp TEXTEQU <esp>
ENDIF

IFDEF HOOKSHOT64
; 64-bit mappings
sax TEXTEQU <rax>
sbx TEXTEQU <rbx>
scx TEXTEQU <rcx>
sdx TEXTEQU <rdx>
ssi TEXTEQU <rsi>
sdi TEXTEQU <rdi>
sbp TEXTEQU <rbp>
ssp TEXTEQU <rsp>
ENDIF


ENDIF ; __HOOKSHOT_REGISTERS_INC
