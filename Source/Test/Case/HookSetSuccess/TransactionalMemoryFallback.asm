;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2021
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests Hookshot's ability to transplant transactional memory instructions.
; In 32-bit mode, the xbegin instruction requires a jump assist because it is encoded using rel16.
; If Hookshot does not implement jump assist correctly, the fallback path will not be hit, and the return value will be incorrect.
; Hook function does nothing special for this test.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                TransactionalMemoryFallback_Original
    xbegin $fallback
    xabort 0
    xor eax, eax
    ret

$fallback:
    mov sax, scx
    ret
END_HOOKSHOT_TEST_FUNCTION                  TransactionalMemoryFallback_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                TransactionalMemoryFallback_Hook
    mov sax, scx
    shl sax, 1
    ret
END_HOOKSHOT_TEST_FUNCTION                  TransactionalMemoryFallback_Hook


_TEXT                                       ENDS


END
