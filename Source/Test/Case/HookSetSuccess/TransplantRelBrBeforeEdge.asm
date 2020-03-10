;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Edge case for transplanting relative branch displacements.
; The original function contains a jump with a displacement exactly one less than the minimum displacement that would require adjustment.
; This is achieved by making the jump instruction the second-last instruction transplanted and setting its displacement to zero, so it points to the instruction right at the end of the transplant window.
; If Hookshot does not realize this and assumes the displacement is large enough as to need adjustment, the program will behave incorrectly.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                TransplantRelBrBeforeEdge_Original
    xor eax, eax
    je short $target
$target:
    nop
    mov sax, scx
    ret
END_HOOKSHOT_TEST_FUNCTION                  TransplantRelBrBeforeEdge_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                TransplantRelBrBeforeEdge_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  TransplantRelBrBeforeEdge_Hook


_TEXT                                       ENDS


END
