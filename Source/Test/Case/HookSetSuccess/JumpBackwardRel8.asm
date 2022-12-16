;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2022
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Exercises Hookshot's instruction transplant ability with instructions that include backward rel8 branch displacements that target instructions outside the area being transplanted.
; This test requires that Hookshot correctly increase the width of, and modify, the branch displacement of a jump instruction.
; If it fails to increase the width of the instruction, setting the hook will fail.
; If it gets the new displacement incorrect, the function's return value will also be incorrect.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


    REPEAT 25
        inc eax
    ENDM

    ; If Hookshot does not get the rewritten rel8 branch displacement perfect, the IP will land in one of the two surrounding banks of inc instructions.
    ; Because sax is loaded with the expected return value before the jump, any inc instruction will cause an incorrect return value.
$return:
    ret

    REPEAT 25
        inc eax
    ENDM

    jmp $return


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpBackwardRel8_Original
    mov sax, scx
    nop
    jmp short $return
END_HOOKSHOT_TEST_FUNCTION                  JumpBackwardRel8_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpBackwardRel8_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  JumpBackwardRel8_Hook


_TEXT                                       ENDS


END
