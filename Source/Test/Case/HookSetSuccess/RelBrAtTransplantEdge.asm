;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2022
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Edge case for transplanting relative branch displacements.
; The original function contains a jump with a displacement exactly equal to the minimum displacement that would require adjustment.
; This is achieved by making the jump instruction the last instruction transplanted and setting its displacement to zero, so it points to the instruction immediately after the transplant window.
; If Hookshot does not realize this and assumes the displacement is small enough as not to need adjustment, the program will behave incorrectly.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                RelBrAtTransplantEdge_Original
    xor eax, eax
    je near ptr $target
$target:
    mov sax, scx
    ret
END_HOOKSHOT_TEST_FUNCTION                  RelBrAtTransplantEdge_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                RelBrAtTransplantEdge_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  RelBrAtTransplantEdge_Hook


_TEXT                                       ENDS


END
