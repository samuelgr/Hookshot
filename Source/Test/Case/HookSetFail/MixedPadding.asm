;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2021
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot with a single-byte function.
; Because this function consists of only a single byte, it is too short for Hookshot to hook successfully.
; Some potential padding instructions of different types are appended, making it not obvious that these are padding bytes.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                MixedPadding_Test
    ret
    
    REPEAT 8
        nop
        int 3
    ENDM
END_HOOKSHOT_TEST_FUNCTION                  MixedPadding_Test


_TEXT                                       ENDS


END
