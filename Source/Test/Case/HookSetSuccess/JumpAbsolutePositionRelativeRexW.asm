;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2023
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; This test is only valid in 64-bit mode; as written, it will crash in 32-bit mode. It exercises
; RIP-relative addressing for loading an absolute jump target from memory and adds a REX.W to
; the jump instruction, which the encoder might remove. If the encoder removes the prefix,
; without further compensation on the part of Hookshot the transplanted jump displacement will be
; wrong (off by one byte). Should the transplanted displacement be off, the jump target will also
; be off (in this case filled with 1s), likely leading to a program crash. Hook function does
; nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpAbsolutePositionRelativeRexW_Original
IFDEF HOOKSHOT64
    rexw jmp SIZE_T PTR [$jumpTargetAbsolute]
ENDIF
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
END_HOOKSHOT_TEST_FUNCTION                  JumpAbsolutePositionRelativeRexW_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                JumpAbsolutePositionRelativeRexW_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  JumpAbsolutePositionRelativeRexW_Hook


_TEXT                                       ENDS


END
