;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2025
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; When run in 64-bit mode, this test exercises RIP-relative addressing for loading an absolute
; jump target from memory. Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpAbsolutePositionRelative_Original
    jmp SIZE_T PTR [$jumpTargetAbsolute]
    ud2
    ud2

$return:
    mov sax, scx
    ret
    
    REPEAT 25
        BYTE 255
    ENDM

$jumpTargetAbsolute:
    SIZE_T $return

    REPEAT 35
        BYTE 255
    ENDM
END_HOOKSHOT_TEST_FUNCTION                  JumpAbsolutePositionRelative_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpAbsolutePositionRelative_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  JumpAbsolutePositionRelative_Hook


_TEXT                                       ENDS


END
