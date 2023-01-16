;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Loop instruction whose target is not within the transplant window.
; Because loop instructions use rel8, a jump assist is required.
; This test requires that the jump assist be calculated with the correct displacement. Otherwise the return value will be incorrect.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                LoopJumpAssist_Original
    mov sax, scx
    loop $return
    
    REPEAT 50
        inc eax
    ENDM
    
    ; If Hookshot does not get the jump assist branch displacement perfect, the IP will land in one of the two surrounding banks of inc instructions.
    ; Because sax is loaded with the expected return value before the jump, any inc instruction will cause an incorrect return value.
$return:
    ret

    REPEAT 50
        inc eax
    ENDM

    jmp $return
END_HOOKSHOT_TEST_FUNCTION                  LoopJumpAssist_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                LoopJumpAssist_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  LoopJumpAssist_Hook


_TEXT                                       ENDS


END
