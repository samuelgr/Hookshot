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


BasicFunction_Original                      PROC HOOKSHOT_TEST_HELPER_FUNCTION
	mov sax, kOriginalFunctionResult
	ret
BasicFunction_Original                      ENDP


BasicFunction_Hook                          PROC HOOKSHOT_TEST_HELPER_FUNCTION
	mov sax, kHookFunctionResult
	ret
BasicFunction_Hook                          ENDP


_TEXT                                       ENDS


END
