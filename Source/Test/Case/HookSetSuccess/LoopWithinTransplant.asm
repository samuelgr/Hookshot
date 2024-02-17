;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2024
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Small loop that fits within the instruction transplant window, so no adjustment to the loop
; instruction's displacement is needed. Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                LoopWithinTransplant_Original
$begin:
    inc edx
    loop $begin
    mov sax, sdx
    ret
END_HOOKSHOT_TEST_FUNCTION                  LoopWithinTransplant_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                LoopWithinTransplant_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  LoopWithinTransplant_Hook


_TEXT                                       ENDS


END
