;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for hooking API calls in spawned processes.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019
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
