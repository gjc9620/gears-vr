__int64 __fastcall sub_3B50C0(
        __int64 a1,
        __int64 a2,
        __int64 a3,
        __int64 a4,
        __int64 a5,
        __int64 a6,
        __int64 a7,
        unsigned __int8 a8,
        char a9,
        int a10)
{
  unsigned __int8 *v10; // rax
  __int64 v12; // rcx
  __int64 v14; // r9
  unsigned __int8 *v15; // rax
  __int64 v16; // rcx
  __int64 v17; // rdi
  __int64 v18; // rsi
  unsigned __int8 *v19; // rax
  __int64 v20; // rcx
  __int64 v21; // rdi
  __int64 v22; // rsi
  __int64 v23; // rdi
  __int64 v24; // rsi
  double v25; // xmm0_8
  double v26; // xmm1_8
  double v27; // xmm2_8
  double v28; // xmm3_8
  double v29; // xmm4_8
  double v30; // xmm5_8
  __m128 v31; // xmm6
  __m128 v32; // xmm7
  _BYTE *v33; // rax
  __int64 result; // rax
  __int64 v38; // rcx
  __int64 v39; // [rsp+0h] [rbp-58h]
  __int64 v40; // [rsp+8h] [rbp-50h]
  __int64 v41; // [rsp+10h] [rbp-48h]
  __int64 v42; // [rsp+18h] [rbp-40h]
  _BYTE v43[40]; // [rsp+30h] [rbp-28h] BYREF

  v10 = *(unsigned __int8 **)(a3 + 40);
  a10 = 0;
  v12 = *(_QWORD *)(a3 + 32);
  v14 = *v10;
  *(_QWORD *)(a3 + 40) = v10 + 1;
  (*(void (__fastcall **)(__int64, __int64, __int64, __int64, int *))(8 * v14 + 0x1B85280))(a5, a4, a3, v12, &a10);
  MEMORY[0x1B9BC18] &= ~2u;
  v15 = *(unsigned __int8 **)(a3 + 40);
  a9 = 0;
  v16 = *v15;
  *(_QWORD *)(a3 + 40) = v15 + 1;
  (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD, char *))(8 * v16 + 0x1B85280))(
    v17,
    v18,
    a3,
    *(_QWORD *)(a3 + 32),
    &a9);
  MEMORY[0x1B9BC18] &= ~2u;
  v19 = *(unsigned __int8 **)(a3 + 40);
  a8 = 0;
  v20 = *v19;
  *(_QWORD *)(a3 + 40) = v19 + 1;
  (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD, unsigned __int8 *))(8 * v20 + 0x1B85280))(
    v21,
    v22,
    a3,
    *(_QWORD *)(a3 + 32),
    &a8);
  v33 = (_BYTE *)(*(_QWORD *)(a3 + 40) + 1LL);
  *(_QWORD *)(a3 + 40) = v33;
  if ( *v33 == 65 )
  {
    v38 = *(_QWORD *)(a3 + 32);
    *(_QWORD *)(a3 + 40) = v33 + 1;
    MEMORY[0x1B85488](v23, v24, a3, v38, 0);
  }
  _RAX = sub_3B51B0(
           v23,
           v24,
           (__int64)v43,
           v24,
           a10,
           a9,
           v25,
           v26,
           v27,
           v28,
           v29,
           v30,
           v31,
           v32,
           v39,
           v40,
           v41,
           v42,
           a8);
  __asm
  {
    vmovsd  xmm0, qword ptr [rax]
    vmovsd  qword ptr [rdi], xmm0
  }
  result = *(unsigned int *)(_RAX + 8);
  *(_DWORD *)(_RDI + 8) = result;
  return result;
}
