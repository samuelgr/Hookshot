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


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                OneByteFunction_Test
    ret
END_HOOKSHOT_TEST_FUNCTION                  OneByteFunction_Test


_TEXT                                       ENDS


END
