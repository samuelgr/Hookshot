;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hookshot
;   General-purpose library for hooking API calls in spawned processes.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Authored by Samuel Grossman
; Copyright (c) 2019
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Inject.asm
;   Implementation of all code that gets injected into another process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

INCLUDE Inject.inc
INCLUDE Preamble.inc
INCLUDE Registers.inc
INCLUDE Strings.inc


kStrInjectCodeSectionName                   SEGMENT READ
; --------- TRAMPOLINE --------------------------------------------------------

; Injected to the entry point of a process.
; Initial value is to be overwritten with the actual target address within the code region.
; Address of the initial value is injectTrampolineAddressMarker - sizeof(size_t) in C/C++.

injectTrampolineStart:

    push sax
    mov sax, initialvalue

injectTrampolineAddressMarker:

    call sax

injectTrampolineEnd:


; --------- MAIN CODE ---------------------------------------------------------

; Injected into the code region of a process.
; Reserved bytes at the beginning are to be overwritten with the address of the data region.

injectCodeStart:

    DSZ 0

injectCodeBegin:
    
    ; Save all other general-purpose registers.
    ; Register sax was already saved by the trampoline.
    push sbx
    push scx
    push sdx
    push ssi
    push sdi
    push sbp

    ; Fix up the stack.
    ; Switch the order of the return address and the old value of sax.
    ; Reset the return address to point to the program's original entry point.
    mov sax, SIZE_T PTR [ssp+(7*SIZEOF(SIZE_T))]
    mov scx, SIZE_T PTR [ssp+(6*SIZEOF(SIZE_T))]
    mov SIZE_T PTR [ssp+(6*SIZEOF(SIZE_T))], sax
    sub scx, (injectTrampolineEnd-injectTrampolineStart)
    mov SIZE_T PTR [ssp+(7*SIZEOF(SIZE_T))], scx
    
    ; Get the address of the data region.
    call $next
  $next:
    pop sbp
    mov sbp, SIZE_T PTR [sbp-($next-injectCodeStart)]

    ; Initialize, then synchronize with the injecting process.
    injectSyncInit ssi, sdi
    injectSync ssi, sdi, sbp

    ; Injecting process is filling in required structure values.
    ; Wait for it to finish.
    injectSync ssi, sdi, sbp
    
    ;;;;;;;;;;
    ; TODO
    ;;;;;;;;;;

    ; All injection operations completed successfully.
    ; Perform one final synchronization with the injecting process.
    injectSync ssi, sdi, sbp
    
    ; Restore all general-purpose registers and return to the program's entry point.
    pop sbp
    pop sdi
    pop ssi
    pop sdx
    pop scx
    pop sbx
    pop sax
    ret

injectCodeEnd:
kStrInjectCodeSectionName                   ENDS


kStrInjectMetaSectionName                   SEGMENT READ
    SInjectMeta <kInjectionMetaMagicValue, 0, x1, x2, x3, x4, x5, x6>
kStrInjectMetaSectionName                   ENDS


x1 EQU (injectTrampolineStart-injectTrampolineStart)
x2 EQU (injectTrampolineAddressMarker-injectTrampolineStart)
x3 EQU (injectTrampolineEnd-injectTrampolineStart)
x4 EQU (injectCodeStart-injectTrampolineStart)
x5 EQU (injectCodeBegin-injectTrampolineStart)
x6 EQU (injectCodeEnd-injectTrampolineStart)


END
