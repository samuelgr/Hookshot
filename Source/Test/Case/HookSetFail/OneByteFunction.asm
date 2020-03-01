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


OneByteFunction_Test                        PROC HOOKSHOT_TEST_HELPER_FUNCTION
	ret
OneByteFunction_Test                        ENDP

    nop
    nop
    nop
    nop
    nop


_TEXT                                       ENDS


END
