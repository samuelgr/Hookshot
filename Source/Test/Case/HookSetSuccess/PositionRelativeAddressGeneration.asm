;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2024
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; When run in 64-bit mode, this test exercises RIP-relative addressing for performing an address
; generation operation. Shifts left the expected return value by 1 position (special short encoding
; of the shl instruction), then shifts it right again by an amount loaded from memory. However,
; before loading from memory, compute the effective address using the lea instruction. If all goes
; well and Hookshot correctly transplants the memory reference, the amount loaded from memory equals
; the number of positions originally shifted left. Hook function does nothing special for this test.


_TEXT                                       SEGMENT


    REPEAT 25
        BYTE 8
    ENDM

$shiftRightAmount:
    BYTE 1

    REPEAT 35
        BYTE 6
    ENDM


BEGIN_HOOKSHOT_TEST_FUNCTION                PositionRelativeAddressGeneration_Original
    shl ecx, 1
    lea sax, BYTE PTR [$shiftRightAmount]
    mov dl, BYTE PTR [sax]
    xchg ecx, edx
    shr edx, cl
    mov eax, edx
    ret
END_HOOKSHOT_TEST_FUNCTION                  PositionRelativeAddressGeneration_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                PositionRelativeAddressGeneration_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  PositionRelativeAddressGeneration_Hook


_TEXT                                       ENDS


END
