; Exact 129CC0 six-argument forwarding; every caller retains protected lifetime.
OPTION CASEMAP:NONE
EXTERN GearsAtomOverwriteProtected:PROC
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

PUBLIC GearsAtomOverwriteHook
GearsAtomOverwriteHook PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0e0h]
    .allocstack 0e0h
    lea rbp, [rsp+0e0h]
    .setframe rbp, 0e0h
    .endprolog
    CONTROL_SAVE rsp, 040h
    mov rax, [rbp+030h] ; RSP0+28h: original fifth bits
    mov [rsp+030h], rax
    mov rax, [rbp+038h] ; RSP0+30h: original sixth bits
    mov [rsp+038h], rax
    lea rcx, [rsp+040h]
    mov rdx, [rbp+008h]
    lea r8, [rbp+008h]
    lea r9, [rsp+030h]
    call GearsAtomOverwriteProtected
    CONTROL_LOAD rsp, 040h
    lea rsp, [rbp]
    pop rbp
    ret
GearsAtomOverwriteHook ENDP
PUBLIC GearsAtomRawInvoke6
GearsAtomRawInvoke6 PROC FRAME
    push rbp
    .pushreg rbp
    push rbx
    .pushreg rbx
    lea rsp, [rsp-048h]
    .allocstack 048h
    lea rbp, [rsp+040h]
    .setframe rbp, 040h
    .endprolog
    mov rbx, rcx
    mov [rsp+030h], rdx
    mov [rsp+020h], r8
    mov [rsp+028h], r9
    CONTROL_LOAD rbx, 0
    call QWORD PTR [rsp+030h]
    CONTROL_SAVE rbx, 0
    lea rsp, [rbp+008h]
    pop rbx
    pop rbp
    ret
GearsAtomRawInvoke6 ENDP
END
