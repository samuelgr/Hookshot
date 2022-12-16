;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2022
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InjectLanding.asm
;   Partial landing code implementation.
;   Receives control from injection code, cleans up, and runs the program.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE Functions.inc
INCLUDE InjectLanding.inc
INCLUDE Preamble.inc
INCLUDE Registers.inc


_TEXT                                       SEGMENT


; --------- FUNCTIONS ---------------------------------------------------------
; See "InjectLanding.h" for documentation.

InjectLanding                               PROC PUBLIC
    ; Starting state:
    ;  - data region pointer is in sbp
    ;  - all general-purpose registers have been pushed
    ;  - a stack alignment fix operation has been performed
    ;  - shadow space has been allocated onto the stack to allow API functions to be called
    
    ; Call the injection setup function to load all hook modules.
    mov scx, sbp
    call InjectLandingLoadHookModules

    ; Call the injection cleanup function to free all allocated memory for injection purposes.
    mov scx, sbp
    call InjectLandingCleanup

    ; Clean up the stack after API calls.
    stackStdCallShadowPop
    
    ; Undo the stack alignment fixing operation.
    stackAlignPop

    ; Restore all general-purpose registers and return to the program's entry point.
    pop sbp
    pop sdi
    pop ssi
    pop sdx
    pop scx
    pop sbx
    pop sax
    ret
InjectLanding                               ENDP


_TEXT                                       ENDS


END
