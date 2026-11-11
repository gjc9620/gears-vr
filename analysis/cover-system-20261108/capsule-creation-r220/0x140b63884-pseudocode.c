_BYTE *__fastcall sub_B62884(__int64 a1, __int64 a2, __int64 a3, __int64 a4)
{
  unsigned __int8 *v4; // rax
  __int64 v6; // rcx
  __int64 v8; // r8
  __int64 v9; // rdx
  __int64 v10; // rdi
  __int64 v11; // rsi
  _BYTE *v12; // rdi
  _BYTE *result; // rax
  __int64 v14; // rcx
  __int64 v15; // rdi
  __int64 v16; // rsi
  __int64 v17; // rax
  __int64 v18; // rdi
  __int64 v19; // rsi
  __int64 (__fastcall *v20)(__int64, __int64, __int64, __int64); // r9
  __int64 v21; // r10
  _BYTE v22[48]; // [rsp+20h] [rbp-78h] BYREF
  _BYTE v23[64]; // [rsp+50h] [rbp-48h] BYREF

  MEMORY[0x1B9BC28] = 0;
  v4 = *(unsigned __int8 **)(a3 + 40);
  v6 = *(_QWORD *)(a3 + 32);
  v8 = *v4;
  *(_QWORD *)(a3 + 40) = v4 + 1;
  (*(void (__fastcall **)(__int64, __int64, __int64, __int64, _BYTE *))(8 * v8 + 0x1B85280))(a1, a4, a3, v6, v23);
  if ( MEMORY[0x1B9BC20] != 0 )
    (*(void (__fastcall **)(__int64, __int64, _QWORD))(*MEMORY[0x1B9BC20] + 368LL))(v10, v11, MEMORY[0x1B9BC30]);
  v12 = v23;
  if ( MEMORY[0x1B9BC28] != 0 )
    v12 = (_BYTE *)MEMORY[0x1B9BC28];
  result = (_BYTE *)(*(_QWORD *)(a3 + 40) + 1LL);
  *(_QWORD *)(a3 + 40) = result;
  if ( *result == 65 )
  {
    v14 = *(_QWORD *)(a3 + 32);
    *(_QWORD *)(a3 + 40) = result + 1;
    result = (_BYTE *)MEMORY[0x1B85488](v12, v11, a3, v14, 0);
  }
  if ( *(_QWORD *)(v11 + 208) != 0 )
  {
    sub_F10F4(v12, v11, v9, v12);
    v17 = sub_F1610(v15, v16, v15, v22);
    return (_BYTE *)v20(v18, v19, v17, v21);
  }
  return result;
}
