;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2022
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Exercises Hookshot's instruction transplant ability with instructions that include backward rel32 branch displacements that target instructions well outside the area being transplanted.
; This test requires that Hookshot correctly modify the branch displacement of a jump instruction. If it gets the new displacement incorrect, the function's return value will also be incorrect.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


    REPEAT 350
        inc eax
    ENDM

    ; If Hookshot does not get the rewritten rel32 branch displacement perfect, the IP will land in one of the two surrounding banks of inc instructions.
    ; Because sax is loaded with the expected return value before the jump, any inc instruction will cause an incorrect return value.
$return:
    ret

    REPEAT 350
        inc eax
    ENDM

    jmp $return


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpBackwardRel32_Original
    mov sax, scx
    jmp near ptr $return
END_HOOKSHOT_TEST_FUNCTION                  JumpBackwardRel32_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpBackwardRel32_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  JumpBackwardRel32_Hook


_TEXT                                       ENDS


END
