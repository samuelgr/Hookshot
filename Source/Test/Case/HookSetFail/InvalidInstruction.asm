;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2025
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot with a function containing an invalid x86 instruction.
; Expected to fail due to inability to decode the invalid instruction.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                InvalidInstruction_Test
    REPEAT 31
        BYTE 0fh
    ENDM

    ret
END_HOOKSHOT_TEST_FUNCTION                  InvalidInstruction_Test


_TEXT                                       ENDS


END
