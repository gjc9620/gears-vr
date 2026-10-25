; Fixed register-only control ABIs; four input registers preserved.
; Reports original RA and true hook-entry RSP to the protected bridge.
; RawInvoke owns fresh shadow space, so original RSP/RA are not substituted.
OPTION CASEMAP:NONE
EXTERN GearsCoverControlInvokeProtected:PROC
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
    lea rsp, [rsp-0c0h]
    .allocstack 0c0h
    lea rbp, [rsp+0c0h]
    .setframe rbp, 0c0h
    .endprolog
    CONTROL_SAVE rsp, 020h
    mov r8, [rbp+008h]
    lea r9, [rbp+008h] ; true original hook-entry RSP, not this bridge frame
    lea rcx, [rsp+020h]
    mov edx, Kind
    call GearsCoverControlInvokeProtected
    CONTROL_LOAD rsp, 020h
    lea rsp, [rbp]
    pop rbp
    ret
Name ENDP
ENDM
CONTROL_THUNK GearsCoverControlLimbHook, 0
CONTROL_THUNK GearsCoverControlSingleHook, 1
CONTROL_THUNK GearsCoverControlWeightHook, 2
END
