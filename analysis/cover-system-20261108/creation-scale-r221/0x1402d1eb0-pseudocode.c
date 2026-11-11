_UNKNOWN **__fastcall sub_2D0EB0(__int64 a1, _DWORD a2, unsigned int a3, __int64 a4)
{
  _UNKNOWN **result; // rax
  __int64 v6; // rsi
  int v7; // ecx
  int v8; // edi
  int v9; // esi
  int v10; // r8d
  int v11; // r9d
  __int64 v12; // rax
  __int64 v13; // rdi
  __int64 v14; // rsi
  double v15; // xmm0_8
  double v16; // xmm1_8
  double v17; // xmm2_8
  double v18; // xmm3_8
  double v19; // xmm4_8
  double v20; // xmm5_8
  __m128 v21; // xmm6
  __m128 v22; // xmm7
  __int64 v23; // [rsp+0h] [rbp-98h]
  __int64 v24; // [rsp+8h] [rbp-90h]
  __int64 v25; // [rsp+10h] [rbp-88h]
  int v26; // [rsp+18h] [rbp-80h]
  _BYTE v27[16]; // [rsp+40h] [rbp-58h] BYREF
  _BYTE v28[64]; // [rsp+50h] [rbp-48h] BYREF

  result = (_UNKNOWN **)MEMORY[0x1C1F890];
  if ( *(_QWORD *)(MEMORY[0x1C1F890] + 468LL) != 0 && *(_QWORD *)(a4 + 380) == 0 )
  {
    result = (_UNKNOWN **)(*(__int64 (__fastcall **)(__int64, _QWORD))(*(_QWORD *)a4 + 752LL))(a1, a3);
    if ( result != nullptr )
    {
      sub_F1170(result, v6, v28, a4, v27);
      v12 = sub_2D0F74(v8, v9, 0, v7, v10, v11);
      *(_QWORD *)(a4 + 380) = v12;
      return sub_2C2EFC(
               v15,
               v16,
               v17,
               v18,
               v19,
               v20,
               v21,
               v22,
               v13,
               v14,
               v13,
               v12,
               (__int64)v28,
               (int)v27,
               v23,
               v24,
               v25,
               v26,
               v14,
               a4,
               *(_QWORD *)(MEMORY[0x1C1F890] + 468LL));
    }
  }
  return result;
}
