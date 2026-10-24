; Windows x64 wrappers for exactly the two certified register-only cover ABIs:
; Blend(self RCX, float XMM1, float XMM2), Child(self RCX, int EDX, float XMM2).
; They do NOT forward arbitrary stack arguments, home contents, the caller's
; original RSP, or its return address. Original return GPRs/XMM0..5/flags survive
; our post callback. Compiler-generated __try/__finally in InvokeProtected owns
; normal/exceptional cleanup; these assembly frames use standard unwind codes.
OPTION CASEMAP:NONE

EXTERN GearsCoverInvokeProtected:PROC

.code

; CallPacket: 00..37 RAX/RCX/RDX/R8/R9/R10/R11, 38 flags, 40..9f XMM0..5.
; Neither macro changes arithmetic flags. RBX/RSP addressing never aliases the
; scratch PUSHFQ/POP below the frame or the original/callback shadow space.
SAVE_PACKET MACRO Base:REQ, Delta:REQ
    mov [Base + Delta + 000h], rax
    mov [Base + Delta + 008h], rcx
    mov [Base + Delta + 010h], rdx
    mov [Base + Delta + 018h], r8
    mov [Base + Delta + 020h], r9
    mov [Base + Delta + 028h], r10
    mov [Base + Delta + 030h], r11
    pushfq
    pop rax
    mov [Base + Delta + 038h], rax
    movdqu [Base + Delta + 040h], xmm0
    movdqu [Base + Delta + 050h], xmm1
    movdqu [Base + Delta + 060h], xmm2
    movdqu [Base + Delta + 070h], xmm3
    movdqu [Base + Delta + 080h], xmm4
    movdqu [Base + Delta + 090h], xmm5
ENDM

LOAD_PACKET MACRO Base:REQ, Delta:REQ
    movdqu xmm0, [Base + Delta + 040h]
    movdqu xmm1, [Base + Delta + 050h]
    movdqu xmm2, [Base + Delta + 060h]
    movdqu xmm3, [Base + Delta + 070h]
    movdqu xmm4, [Base + Delta + 080h]
    movdqu xmm5, [Base + Delta + 090h]
    push QWORD PTR [Base + Delta + 038h]
    popfq
    mov rax, [Base + Delta + 000h]
    mov rcx, [Base + Delta + 008h]
    mov rdx, [Base + Delta + 010h]
    mov r8,  [Base + Delta + 018h]
    mov r9,  [Base + Delta + 020h]
    mov r10, [Base + Delta + 028h]
    mov r11, [Base + Delta + 030h]
ENDM

COVER_THUNK MACRO HookName:REQ, Kind:REQ
    PUBLIC HookName
HookName PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp - 0c0h]
    .allocstack 0c0h
    lea rbp, [rsp + 0c0h]
    .setframe rbp, 0c0h
    .endprolog

    ; 20 bytes of callback shadow, followed by the A0-byte POD packet.
    SAVE_PACKET rsp, 020h
    lea rcx, [rsp + 020h]
    mov edx, Kind
    call GearsCoverInvokeProtected
    LOAD_PACKET rsp, 020h
    lea rsp, [rbp]
    pop rbp
    ret
HookName ENDP
ENDM

COVER_THUNK GearsCoverBlendTickHook, 0
COVER_THUNK GearsCoverChildPropagateHook, 1

; Invoke once from within the compiler-owned protected region. RBX holds the
; packet across the original and is restored by this routine's unwind metadata.
; A fresh 32-byte shadow area belongs to this one original call. Saving outputs
; is the first operation after CALL, before any C++ code or flag-changing test.
PUBLIC GearsCoverRawInvoke
GearsCoverRawInvoke PROC FRAME
    push rbp
    .pushreg rbp
    push rbx
    .pushreg rbx
    lea rsp, [rsp - 038h]
    .allocstack 038h
    lea rbp, [rsp + 030h]
    .setframe rbp, 030h
    .endprolog
    mov rbx, rcx
    mov [rsp + 020h], rdx
    LOAD_PACKET rbx, 0
    call QWORD PTR [rsp + 020h]
    SAVE_PACKET rbx, 0
    lea rsp, [rbp + 008h]
    pop rbx
    pop rbp
    ret
GearsCoverRawInvoke ENDP

END
