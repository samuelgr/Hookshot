;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TestDefinitions.inc
;   Definitions to assist with the creation of test cases.
;   Helps to ensure 32/64-bit portability.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_TEST_DEFINITIONS_INC
__HOOKSHOT_TEST_DEFINITIONS_INC EQU 1


IFDEF HOOKSHOT32
; 32-bit preamble
; Required to generate valid code for modern processors
.686

; 32-bit data and pointer size
SIZE_T TEXTEQU <DWORD>
DSZ TEXTEQU <DD>

; 32-bit full-size register names
; Includes only those registers available in both 32-bit and 64-bit mode
sax TEXTEQU <eax>
sbx TEXTEQU <ebx>
scx TEXTEQU <ecx>
sdx TEXTEQU <edx>
ssi TEXTEQU <esi>
sdi TEXTEQU <edi>
sbp TEXTEQU <ebp>
ssp TEXTEQU <esp>

; Function declaration helper for 32-bit mode
HOOKSHOT_TEST_HELPER_FUNCTION TEXTEQU <C PUBLIC>
ENDIF



IFDEF HOOKSHOT64
; 64-bit data and pointer size
SIZE_T TEXTEQU <QWORD>
DSZ TEXTEQU <DQ>

; 64-bit full-size register names
; Includes only those registers available in both 32-bit and 64-bit mode
sax TEXTEQU <rax>
sbx TEXTEQU <rbx>
scx TEXTEQU <rcx>
sdx TEXTEQU <rdx>
ssi TEXTEQU <rsi>
sdi TEXTEQU <rdi>
sbp TEXTEQU <rbp>
ssp TEXTEQU <rsp>

; Function declaration helper for 32-bit mode
HOOKSHOT_TEST_HELPER_FUNCTION TEXTEQU <PUBLIC>
ENDIF


; Return value expected from the original version of a test function.
kOriginalFunctionResult EQU 1111111

; Return value expected from the hook version of a test function.
kHookFunctionResult EQU 2222222


ENDIF ; __HOOKSHOT_TEST_DEFINITIONS_INC
