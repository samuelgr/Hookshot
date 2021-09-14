;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2021
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests the most basic and simpliest kind of functions Hookshot can encounter.
; Both original and hook functions simply load the expected values into the return value register and immediately return.
; Extra nop instructions are added to ensure the original function is long enough to hook.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                BasicFunction_Original
    mov sax, scx
    nop
    nop
    nop
    ret
END_HOOKSHOT_TEST_FUNCTION                  BasicFunction_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                BasicFunction_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  BasicFunction_Hook


_TEXT                                       ENDS


END
