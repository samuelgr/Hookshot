;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2024
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot with a non-terminal jump instruction whose displacement prevents it from being
; transplanted. Implemented using a conditional jump with maximum possible rel32 displacement.
; Only valid in 64-bit mode beacuse rel32 can specify any address in the entire address space in
; 32-bit mode.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpForwardTooFar_Test
    ; A jmp instruction with absolute value displacement (instead of a label) cannot be assembled.
    ; Instead, it is presented as a byte sequence.
    BYTE 0fh, 84h, 0ffh, 0ffh, 0ffh, 7fh
    ret
END_HOOKSHOT_TEST_FUNCTION                  JumpForwardTooFar_Test


_TEXT                                       ENDS


END
