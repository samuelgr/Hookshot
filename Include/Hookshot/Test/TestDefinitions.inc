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


; To create a Hookshot test function used with the Hookshot testing framework, begin it with BEGIN_HOOKSHOT_TEST_FUNCTION and end it with END_HOOKSHOT_TEST_FUNCTION.
; For example:
;     BEGIN_HOOKSHOT_TEST_FUNCTION  MyHookshotTestFunction
;         [code goes here]
;     END_HOOKSHOT_TEST_FUNCTION    MyHookshotTestFunction
; Test patterns as defined in C++ (refer to TestPattern.h) might require that these test functions comply with certain naming conventions.
; Expected return values for original and hook functions are supplied and must be synchronized with the equivalent C++ definitions.
; At the start of a test function created this way, register scx = 0, and register sdx = 1.  This is intended to provide a consistent initial condition without requiring any assembly code.


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

; Function declaration helpers for 32-bit mode
BEGIN_HOOKSHOT_TEST_FUNCTION MACRO name
	@CatStr(<@>, name, <@8>) PROC PUBLIC
ENDM
END_HOOKSHOT_TEST_FUNCTION MACRO name
	@CatStr(<@>, name, <@8>) ENDP
ENDM
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

; Function declaration helpers for 64-bit mode
BEGIN_HOOKSHOT_TEST_FUNCTION MACRO name
	name PROC PUBLIC
ENDM
END_HOOKSHOT_TEST_FUNCTION MACRO name
	name ENDP
ENDM
ENDIF


; Return value expected from the original version of a test function.
kOriginalFunctionResult EQU 1111111

; Return value expected from the hook version of a test function.
kHookFunctionResult EQU 2222222


ENDIF ; __HOOKSHOT_TEST_DEFINITIONS_INC
