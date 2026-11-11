__int64 __fastcall sub_3A4D0C(__int64 a1, __int64 a2, __int64 a3, __int64 a4)
{
  __int64 result; // rax
  __int64 v12; // rdi
  __int64 v13; // rsi
  __int64 v14; // r9
  __int64 v15; // rcx
  _BYTE v16[48]; // [rsp+20h] [rbp-78h] BYREF
  __m256 v17; // [rsp+50h] [rbp-48h] BYREF
  _UNKNOWN *retaddr; // [rsp+98h] [rbp+0h] BYREF

  _R11 = &retaddr;
  result = *(_QWORD *)(a4 + 436);
  if ( result != 0 && (*(_BYTE *)(result + 128) & 1) != 0 )
  {
    __asm
    {
      vmovaps ymm0, ymmword ptr cs:18FF360h
      vmovss  xmm1, dword ptr [rax+0D4h]
      vmovups [rsp+98h+var_48], ymm0
      vmovdqu xmm0, xmmword ptr cs:17061F0h
      vmovups [rsp+98h+var_28], xmm0
      vmovss  xmm0, dword ptr [rax+0D0h]
      vmovss  dword ptr [r11-18h], xmm0
      vmovss  xmm0, dword ptr [rax+0D8h]
      vmovss  dword ptr [r11-14h], xmm1
      vmovss  xmm1, dword ptr cs:17061B8h
      vmovss  dword ptr [r11-10h], xmm0
      vmovss  dword ptr [r11-0Ch], xmm1
    }
    __asm { vzeroupper }
    result = sub_F1610(a1, a2, &v17, v16);
    v15 = *(_QWORD *)(v14 + 1128);
    if ( v15 != 0 )
      return (*(__int64 (__fastcall **)(__int64, __int64, _BYTE *))(*(_QWORD *)v15 + 104LL))(v12, v13, v16);
  }
  return result;
}
