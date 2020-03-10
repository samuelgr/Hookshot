;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot with a single-byte function.
; Because this function consists of only a single byte, it is too short for Hookshot to hook successfully.
; A few extra bytes of padding are added afterwards just as a safety measure in case Hookshot decodes too far.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                OneByteFunction_Test
    ret
END_HOOKSHOT_TEST_FUNCTION                  OneByteFunction_Test

    ud2
    ud2
    ud2
    ud2
    ud2


_TEXT                                       ENDS


END
