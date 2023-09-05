;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InjectLanding.inc
;   Landing code assembly declarations. Receives control from injection code, cleans up, and runs
;   the program.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF __HOOKSHOT_INJECTLANDING_INC
__HOOKSHOT_INJECTLANDING_INC EQU 1


IFDEF HOOKSHOT64
EXTRN InjectLandingCleanup:PROC
EXTRN InjectLandingLoadHookModules:PROC
ELSE
InjectLanding TEXTEQU <@InjectLanding@0>
EXTRN @InjectLandingCleanup@4:PROC
InjectLandingCleanup TEXTEQU <@InjectLandingCleanup@4>
EXTRN @InjectLandingLoadHookModules@4:PROC
InjectLandingLoadHookModules TEXTEQU <@InjectLandingLoadHookModules@4>
ENDIF


ENDIF ; __HOOKSHOT_INJECTLANDING_INC
