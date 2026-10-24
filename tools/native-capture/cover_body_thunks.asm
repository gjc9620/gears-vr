; Windows x64 pre-only observers. No return type or stack argument is decoded.
; The callbacks must obey the Windows x64 ABI (including nonvolatile registers).
; No install, target discovery, patching, or exception handler is provided here.
OPTION CASEMAP:NONE

EXTERN GearsCoverBlendBefore:PROC
EXTERN GearsCoverChildBefore:PROC
EXTERN GearsCoverBlendOriginal:QWORD
EXTERN GearsCoverChildOriginal:QWORD

.code

; RBP permits a normal Windows epilog without changing restored flags. It also
; gives the unwinder a stable frame during the balanced PUSHFQ/POP pairs in the
; body. LEA in the prolog is deliberate: SUB would destroy the incoming flags.
; The allocation is described by UWOP_ALLOC_LARGE; it does not need SUB to run.
; Layout: 00..1f callback shadow, 20..57 GPRs, 58 flags, 60..bf XMM0..5.
COVER_PRE_THUNK MACRO HookName:REQ, BeforeName:REQ, OriginalName:REQ
    PUBLIC HookName
HookName PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp - 0c0h]
    .allocstack 0c0h
    lea rbp, [rsp + 0c0h]
    .setframe rbp, 0c0h
    .endprolog

    mov [rsp + 020h], rax
    mov [rsp + 028h], rcx
    mov [rsp + 030h], rdx
    mov [rsp + 038h], r8
    mov [rsp + 040h], r9
    mov [rsp + 048h], r10
    mov [rsp + 050h], r11
    pushfq
    pop rax
    mov [rsp + 058h], rax
    movdqu [rsp + 060h], xmm0
    movdqu [rsp + 070h], xmm1
    movdqu [rsp + 080h], xmm2
    movdqu [rsp + 090h], xmm3
    movdqu [rsp + 0a0h], xmm4
    movdqu [rsp + 0b0h], xmm5

    ; Original RCX/RDX/R8/R9 and positional XMM arguments remain in place.
    ; RSP is 16-byte aligned, with 32 bytes of fresh callback shadow space.
    call BeforeName

    movdqu xmm0, [rsp + 060h]
    movdqu xmm1, [rsp + 070h]
    movdqu xmm2, [rsp + 080h]
    movdqu xmm3, [rsp + 090h]
    movdqu xmm4, [rsp + 0a0h]
    movdqu xmm5, [rsp + 0b0h]
    push QWORD PTR [rsp + 058h]
    popfq
    mov rax, [rsp + 020h]
    mov rcx, [rsp + 028h]
    mov rdx, [rsp + 030h]
    mov r8, [rsp + 038h]
    mov r9, [rsp + 040h]
    mov r10, [rsp + 048h]
    mov r11, [rsp + 050h]

    ; This is a recognized frame-register epilog. No instruction below changes
    ; flags. The original sees the original stack, return address, and arguments;
    ; its RAX/XMM0 return (of any type) returns directly to the original caller.
    lea rsp, [rbp]
    pop rbp
    jmp QWORD PTR [OriginalName]
HookName ENDP
ENDM

COVER_PRE_THUNK GearsCoverBlendTickHook, GearsCoverBlendBefore, GearsCoverBlendOriginal
COVER_PRE_THUNK GearsCoverChildPropagateHook, GearsCoverChildBefore, GearsCoverChildOriginal

END
