;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for injecting DLLs and hooking function calls.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019-2020
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE TestDefinitions.inc


; Tests the most basic and simpliest kind of functions Hookshot can encounter.
; Both original and hook functions load constants into the return value register and immediately return.


_TEXT                                       SEGMENT


BEGIN_HOOKSHOT_TEST_FUNCTION                BasicFunction_Original
	mov sax, kOriginalFunctionResult
	ret
END_HOOKSHOT_TEST_FUNCTION                  BasicFunction_Original


BEGIN_HOOKSHOT_TEST_FUNCTION                BasicFunction_Hook
	mov sax, kHookFunctionResult
	ret
END_HOOKSHOT_TEST_FUNCTION                  BasicFunction_Hook


_TEXT                                       ENDS


END
