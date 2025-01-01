;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2025
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests a very basic function but with padding instructions after the original function. Both
; original and hook functions load the expected values into the return value register and
; immediately return. Original function is too small to hook unless Hookshot properly consumes the
; padding instructions located after it.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                ShortFunctionWithPadding_Original
    mov sax, scx
    ret

    REPEAT 8
        int 3
    ENDM
END_HOOKSHOT_TEST_FUNCTION                  ShortFunctionWithPadding_Original



BEGIN_HOOKSHOT_TEST_FUNCTION                ShortFunctionWithPadding_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  ShortFunctionWithPadding_Hook


_TEXT                                       ENDS


END
