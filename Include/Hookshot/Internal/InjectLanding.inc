;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InjectLanding.inc
;   Landing code assembly declarations.
;   Receives control from injection code, cleans up, and runs the program.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_INJECTLANDING_INC
__HOOKSHOT_INJECTLANDING_INC EQU 1


; --------- FUNCTIONS ---------------------------------------------------------
; See "InjectLanding.h" for documentation.

IFDEF HOOKSHOT64
EXTRN InjectLandingCleanup:PROC
EXTRN InjectLandingLoadHookModules:PROC
ELSE
InjectLanding TEXTEQU <_InjectLanding@0>
EXTRN _InjectLandingCleanup@4:PROC
InjectLandingCleanup TEXTEQU <_InjectLandingCleanup@4>
EXTRN _InjectLandingLoadHookModules@4:PROC
InjectLandingLoadHookModules TEXTEQU <_InjectLandingLoadHookModules@4>
ENDIF


ENDIF ; __HOOKSHOT_INJECTLANDING_INC
