;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for hooking API calls in spawned processes.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019
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
ELSE
EXTRN _InjectLandingCleanup@4:PROC
InjectLanding TEXTEQU <_InjectLanding@0>
InjectLandingCleanup TEXTEQU <_InjectLandingCleanup@4>
ENDIF


ENDIF ; __HOOKSHOT_INJECTLANDING_INC
