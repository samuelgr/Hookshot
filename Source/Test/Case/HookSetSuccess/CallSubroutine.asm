;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests calling subroutines from within the original function. Intended to give Hookshot more
; variety in the position-relative operations it is expected to transplant successfully. Hook
; function does nothing special for this test.


_TEXT                                       SEGMENT


$func1:
    mov sax, scx
    shl sax, 1
    ret

$func2:
    shr sax, 1
    ret


BEGIN_HOOKSHOT_TEST_FUNCTION                CallSubroutine_Original
    call $func1
    call $func2
    ret
END_HOOKSHOT_TEST_FUNCTION                  CallSubroutine_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                CallSubroutine_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  CallSubroutine_Hook


_TEXT                                       ENDS


END
