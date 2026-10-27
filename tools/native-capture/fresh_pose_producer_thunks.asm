; Exact F7940 five-argument ABI; Optional is three arguments and NEVER reads a fifth caller slot.
; Full original GPR/XMM0..5/flags packet and OS unwind metadata preserved.
OPTION CASEMAP:NONE
EXTERN GearsFreshInvokeProtected:PROC
.code

CONTROL_SAVE MACRO Base:REQ, Delta:REQ
    mov [Base+Delta+000h], rax
    mov [Base+Delta+008h], rcx
    mov [Base+Delta+010h], rdx
    mov [Base+Delta+018h], r8
    mov [Base+Delta+020h], r9
    mov [Base+Delta+028h], r10
    mov [Base+Delta+030h], r11
    pushfq
    pop rax
    mov [Base+Delta+038h], rax
    movdqu [Base+Delta+040h], xmm0
    movdqu [Base+Delta+050h], xmm1
    movdqu [Base+Delta+060h], xmm2
    movdqu [Base+Delta+070h], xmm3
    movdqu [Base+Delta+080h], xmm4
    movdqu [Base+Delta+090h], xmm5
ENDM
CONTROL_LOAD MACRO Base:REQ, Delta:REQ
    movdqu xmm0, [Base+Delta+040h]
    movdqu xmm1, [Base+Delta+050h]
    movdqu xmm2, [Base+Delta+060h]
    movdqu xmm3, [Base+Delta+070h]
    movdqu xmm4, [Base+Delta+080h]
    movdqu xmm5, [Base+Delta+090h]
    push QWORD PTR [Base+Delta+038h]
    popfq
    mov rax, [Base+Delta+000h]
    mov rcx, [Base+Delta+008h]
    mov rdx, [Base+Delta+010h]
    mov r8, [Base+Delta+018h]
    mov r9, [Base+Delta+020h]
    mov r10, [Base+Delta+028h]
    mov r11, [Base+Delta+030h]
ENDM
CONTROL_THUNK MACRO Name:REQ, Kind:REQ
    PUBLIC Name
Name PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0d0h]
    .allocstack 0d0h
    lea rbp, [rsp+0d0h]
    .setframe rbp, 0d0h
    .endprolog
    CONTROL_SAVE rsp, 030h
    IF Kind EQ 0
        mov rax, [rbp+030h] ; ROOT ONLY: original fifth at RSP0+28h
        mov [rsp+020h], rax
    ELSE
        mov QWORD PTR [rsp+020h], 0 ; Optional does not read caller fifth
    ENDIF
    mov r8, [rbp+008h]
    lea r9, [rbp+008h] ; true original hook-entry RSP, not this bridge frame
    lea rcx, [rsp+030h]
    mov edx, Kind
    call GearsFreshInvokeProtected
    CONTROL_LOAD rsp, 030h
    lea rsp, [rbp]
    pop rbp
    ret
Name ENDP
ENDM
CONTROL_THUNK GearsFreshRootHook, 0
CONTROL_THUNK GearsFreshOptionalHook, 1
PUBLIC GearsFreshRawInvoke5
GearsFreshRawInvoke5 PROC FRAME
    push rbp
    .pushreg rbp
    push rbx
    .pushreg rbx
    lea rsp, [rsp-038h]
    .allocstack 038h
    lea rbp, [rsp+030h]
    .setframe rbp, 030h
    .endprolog
    mov rbx, rcx
    mov [rsp+028h], rdx
    mov [rsp+020h], r8 ; exact original fifth, fresh own shadow space
    CONTROL_LOAD rbx, 0
    call QWORD PTR [rsp+028h]
    CONTROL_SAVE rbx, 0
    lea rsp, [rbp+008h]
    pop rbx
    pop rbp
    ret
GearsFreshRawInvoke5 ENDP
PUBLIC GearsFreshRawInvoke4
GearsFreshRawInvoke4 PROC FRAME
    push rbp
    .pushreg rbp
    push rbx
    .pushreg rbx
    lea rsp, [rsp-038h]
    .allocstack 038h
    lea rbp, [rsp+030h]
    .setframe rbp, 030h
    .endprolog
    mov rbx, rcx
    mov [rsp+020h], rdx ; register-only original target; no fifth argument
    CONTROL_LOAD rbx, 0
    call QWORD PTR [rsp+020h]
    CONTROL_SAVE rbx, 0
    lea rsp, [rbp+008h]
    pop rbx
    pop rbp
    ret
GearsFreshRawInvoke4 ENDP
END
