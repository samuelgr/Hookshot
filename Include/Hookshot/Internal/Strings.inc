;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2021
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Strings.inc
;   Declaration of common constant string literals.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_STRINGS_INC
__HOOKSHOT_STRINGS_INC EQU 1


; --------- CONSTANTS ---------------------------------------------------------
; See "Strings.h" for documentation.
; Must be updated to correspond to "Strings.cpp" definitions.

IFDEF HOOKSHOT32
kStrInjectCodeSectionName TEXTEQU <_CODE32>
kStrInjectMetaSectionName TEXTEQU <_META32>
ENDIF

IFDEF HOOKSHOT64
kStrInjectCodeSectionName TEXTEQU <_CODE64>
kStrInjectMetaSectionName TEXTEQU <_META64>
ENDIF


ENDIF ; __HOOKSHOT_STRINGS_INC
