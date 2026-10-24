; Synthetic test support only. No game code, process access, or hook installation.
OPTION CASEMAP:NONE

EXTERN GearsCoverTestBefore:PROC
EXTERN GearsCoverTestAfter:PROC
EXTERN GearsCoverTestInOriginal:PROC
EXTERN gCoverTestInput:BYTE
EXTERN gCoverTestBefore:BYTE
EXTERN gCoverTestTarget:BYTE
EXTERN gCoverTestReturned:BYTE
EXTERN gCoverTestOutput:BYTE
EXTERN gCoverTestHook:QWORD
EXTERN gCoverTestDriverRsp:QWORD
EXTERN gCoverTestRequestedFlags:QWORD
EXTERN gCoverTestReturnFlags:QWORD
EXTERN gCoverTestReturnRax:QWORD
EXTERN gCoverTestReturnXmm0:BYTE
EXTERN gCoverTestGotRax:QWORD
EXTERN gCoverTestGotXmm0:BYTE
EXTERN gCoverTestGotFlags:QWORD
EXTERN gCoverTestOriginalCalls:DWORD
EXTERN gCoverTestOriginalKind:DWORD
EXTERN gCoverTestShadow:QWORD

.code

; Snapshot layout is asserted in the C++ test. MOVs leave incoming flags alone.
SNAPSHOT MACRO Destination:REQ, EntryDelta:REQ
    mov QWORD PTR [Destination + 000h], rax
    mov QWORD PTR [Destination + 008h], rcx
    mov QWORD PTR [Destination + 010h], rdx
    mov QWORD PTR [Destination + 018h], r8
    mov QWORD PTR [Destination + 020h], r9
    mov QWORD PTR [Destination + 028h], r10
    mov QWORD PTR [Destination + 030h], r11
    lea rax, [rsp + EntryDelta]
    mov QWORD PTR [Destination + 040h], rax
    pushfq
    pop QWORD PTR [Destination + 038h]
    mov rax, [rsp + EntryDelta + 028h]
    mov QWORD PTR [Destination + 048h], rax
    mov rax, [rsp + EntryDelta + 030h]
    mov QWORD PTR [Destination + 050h], rax
    mov rax, [rsp + EntryDelta]
    mov QWORD PTR [Destination + 058h], rax
    movdqu XMMWORD PTR [Destination + 060h], xmm0
    movdqu XMMWORD PTR [Destination + 070h], xmm1
    movdqu XMMWORD PTR [Destination + 080h], xmm2
    movdqu XMMWORD PTR [Destination + 090h], xmm3
    movdqu XMMWORD PTR [Destination + 0a0h], xmm4
    movdqu XMMWORD PTR [Destination + 0b0h], xmm5
ENDM

TEST_BEFORE MACRO Name:REQ, Kind:REQ
    PUBLIC Name
Name PROC FRAME
    ; LEA preserves the input flags for the snapshot below. This callback has
    ; unwind metadata too, so the exception test must traverse both frames.
    push rbp
    .pushreg rbp
    lea rsp, [rsp - 030h]
    .allocstack 030h
    lea rbp, [rsp + 030h]
    .setframe rbp, 030h
    .endprolog
    SNAPSHOT gCoverTestBefore, 038h
    mov ecx, Kind
    call GearsCoverTestBefore
    ; Deliberately destroy every volatile register and arithmetic flags.
    pcmpeqd xmm0, xmm0
    pxor xmm1, xmm1
    pcmpeqd xmm2, xmm2
    pxor xmm3, xmm3
    pcmpeqd xmm4, xmm4
    pxor xmm5, xmm5
    mov rcx, 1111111111111111h
    mov rdx, 2222222222222222h
    mov r8,  3333333333333333h
    mov r9,  4444444444444444h
    mov r10, 5555555555555555h
    mov r11, 6666666666666666h
    xor eax, eax
    lea rsp, [rbp]
    pop rbp
    ret
Name ENDP
ENDM

TEST_BEFORE GearsCoverBlendBefore, 0
TEST_BEFORE GearsCoverChildBefore, 1

PUBLIC GearsCoverTestAfterCallback
GearsCoverTestAfterCallback PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp - 030h]
    .allocstack 030h
    lea rbp, [rsp + 030h]
    .setframe rbp, 030h
    .endprolog
    call GearsCoverTestAfter
    ; Deliberately destroy the original's saved outputs after normal post.
    pcmpeqd xmm0, xmm0
    pxor xmm1, xmm1
    pcmpeqd xmm2, xmm2
    pxor xmm3, xmm3
    pcmpeqd xmm4, xmm4
    pxor xmm5, xmm5
    mov rcx, 1111111111111111h
    mov rdx, 2222222222222222h
    mov r8,  3333333333333333h
    mov r9,  4444444444444444h
    mov r10, 5555555555555555h
    mov r11, 6666666666666666h
    xor eax, eax
    lea rsp, [rbp]
    pop rbp
    ret
GearsCoverTestAfterCallback ENDP

TEST_TARGET MACRO Name:REQ, Kind:REQ
    PUBLIC Name
Name PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp - 030h]
    .allocstack 030h
    lea rbp, [rsp + 030h]
    .setframe rbp, 030h
    .endprolog
    SNAPSHOT gCoverTestTarget, 038h
    inc DWORD PTR [gCoverTestOriginalCalls]
    mov DWORD PTR [gCoverTestOriginalKind], Kind
    mov ecx, Kind
    call GearsCoverTestInOriginal
    mov rax, [gCoverTestReturnRax]
    mov rcx, QWORD PTR [gCoverTestOutput + 008h]
    mov rdx, QWORD PTR [gCoverTestOutput + 010h]
    mov r8,  QWORD PTR [gCoverTestOutput + 018h]
    mov r9,  QWORD PTR [gCoverTestOutput + 020h]
    mov r10, QWORD PTR [gCoverTestOutput + 028h]
    mov r11, QWORD PTR [gCoverTestOutput + 030h]
    movdqu xmm0, XMMWORD PTR [gCoverTestOutput + 060h]
    movdqu xmm1, XMMWORD PTR [gCoverTestOutput + 070h]
    movdqu xmm2, XMMWORD PTR [gCoverTestOutput + 080h]
    movdqu xmm3, XMMWORD PTR [gCoverTestOutput + 090h]
    movdqu xmm4, XMMWORD PTR [gCoverTestOutput + 0a0h]
    movdqu xmm5, XMMWORD PTR [gCoverTestOutput + 0b0h]
    push QWORD PTR [gCoverTestReturnFlags]
    popfq
    lea rsp, [rbp]
    pop rbp
    ret
Name ENDP
ENDM

TEST_TARGET GearsCoverTestBlendTarget, 0
TEST_TARGET GearsCoverTestChildTarget, 1

PUBLIC GearsCoverTestInvoke
PUBLIC GearsCoverTestAfterCall
GearsCoverTestInvoke PROC FRAME
    push rbp
    .pushreg rbp
    sub rsp, 030h
    .allocstack 030h
    lea rbp, [rsp + 030h]
    .setframe rbp, 030h
    .endprolog
    mov [gCoverTestDriverRsp], rsp
    mov rax, 0a0a0a0a0a0a0a0a0h
    mov [rsp + 000h], rax
    mov rax, 0b1b1b1b1b1b1b1b1h
    mov [rsp + 008h], rax
    mov rax, 0c2c2c2c2c2c2c2c2h
    mov [rsp + 010h], rax
    mov rax, 0d3d3d3d3d3d3d3d3h
    mov [rsp + 018h], rax
    mov rax, 0123456789abcdefh
    mov [rsp + 020h], rax
    mov rax, 0fedcba9876543210h
    mov [rsp + 028h], rax
    mov rax, QWORD PTR [gCoverTestInput + 000h]
    mov rcx, QWORD PTR [gCoverTestInput + 008h]
    mov rdx, QWORD PTR [gCoverTestInput + 010h]
    mov r8,  QWORD PTR [gCoverTestInput + 018h]
    mov r9,  QWORD PTR [gCoverTestInput + 020h]
    mov r10, QWORD PTR [gCoverTestInput + 028h]
    mov r11, QWORD PTR [gCoverTestInput + 030h]
    movdqu xmm0, XMMWORD PTR [gCoverTestInput + 060h]
    movdqu xmm1, XMMWORD PTR [gCoverTestInput + 070h]
    movdqu xmm2, XMMWORD PTR [gCoverTestInput + 080h]
    movdqu xmm3, XMMWORD PTR [gCoverTestInput + 090h]
    movdqu xmm4, XMMWORD PTR [gCoverTestInput + 0a0h]
    movdqu xmm5, XMMWORD PTR [gCoverTestInput + 0b0h]
    push QWORD PTR [gCoverTestRequestedFlags]
    popfq
    call QWORD PTR [gCoverTestHook]
GearsCoverTestAfterCall LABEL BYTE
    mov [gCoverTestGotRax], rax
    movdqu XMMWORD PTR [gCoverTestGotXmm0], xmm0
    pushfq
    pop QWORD PTR [gCoverTestGotFlags]
    SNAPSHOT gCoverTestReturned, 0
    mov rax, [rsp + 000h]
    mov [gCoverTestShadow + 000h], rax
    mov rax, [rsp + 008h]
    mov [gCoverTestShadow + 008h], rax
    mov rax, [rsp + 010h]
    mov [gCoverTestShadow + 010h], rax
    mov rax, [rsp + 018h]
    mov [gCoverTestShadow + 018h], rax
    lea rsp, [rbp]
    pop rbp
    ret
GearsCoverTestInvoke ENDP

END
