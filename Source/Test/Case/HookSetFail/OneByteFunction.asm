;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2025
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot with a single-byte function. Because this function consists of only a single
; byte, it is too short for Hookshot to hook successfully. Some 0 bytes need to be added
; afterwards to avoid Hookshot finding any valid padding.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                OneByteFunction_Test
    ret
    
    REPEAT 8
        BYTE 0
    ENDM
END_HOOKSHOT_TEST_FUNCTION                  OneByteFunction_Test


_TEXT                                       ENDS


END
