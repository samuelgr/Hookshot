;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; When run in 64-bit mode, this test exercises RIP-relative addressing for performing a data operation.
; Shifts left the expected return value by 1 position (special short encoding of the shl instruction), then shifts it right again by an amount loaded from memory.
; If all goes well and Hookshot correctly transplants the memory reference, the amount loaded from memory equals the number of positions originally shifted left.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


    REPEAT 25
        BYTE 8
    ENDM

$shiftRightAmount:
    BYTE 1

    REPEAT 35
        BYTE 6
    ENDM


BEGIN_HOOKSHOT_TEST_FUNCTION                PositionRelativeLoad_Original
    shl ecx, 1
    mov dl, BYTE PTR [$shiftRightAmount]
    xchg ecx, edx
    shr edx, cl
    mov eax, edx
    ret
END_HOOKSHOT_TEST_FUNCTION                  PositionRelativeLoad_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                PositionRelativeLoad_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  PositionRelativeLoad_Hook


_TEXT                                       ENDS


END
