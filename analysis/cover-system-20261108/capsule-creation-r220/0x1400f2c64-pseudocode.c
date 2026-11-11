__int64 __fastcall sub_F1C64(
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        __m128 _XMM6,
        __int64 a8,
        __int64 a9,
        __int64 a10,
        __int64 a11)
{
  __int64 result; // rax
  __int64 v13; // rdi
  __int64 v14; // rsi
  __int64 v15; // rbx
  __int64 v18; // rdi
  __int64 v19; // rsi
  __int64 v20; // rdi
  __int64 v21; // rsi
  __int64 v22; // rdi
  __int64 v23; // rsi
  __int64 v24; // rdi
  __int64 v25; // rsi
  __int64 v26; // rdi
  __int64 v27; // rsi
  char v28; // cf
  char v29; // zf
  __int64 v30; // rdi
  __int64 v31; // rsi
  __int64 v55; // rdi
  __int64 v56; // rsi
  _DWORD v61[3]; // [rsp+20h] [rbp-69h] BYREF
  _DWORD v62[4]; // [rsp+50h] [rbp-39h] BYREF
  _BYTE v63[48]; // [rsp+60h] [rbp-29h] BYREF
  _BYTE v64[80]; // [rsp+90h] [rbp+7h] BYREF
  char vars0; // [rsp+E0h] [rbp+57h] BYREF
  _UNKNOWN *retaddr; // [rsp+E8h] [rbp+5Fh] BYREF

  _RAX = &retaddr;
  __asm { vmovaps xmmword ptr [rax-18h], xmm6 }
  result = (*(__int64 (__fastcall **)(__int64, __int64, _QWORD))(*(_QWORD *)a11 + 896LL))(a11, a9, 0);
  v15 = result;
  if ( result != 0 )
  {
    result = (*(__int64 (__fastcall **)(__int64, __int64, _QWORD, __int64))(*(_QWORD *)result + 232LL))(
               v13,
               v14,
               *(_QWORD *)(*(_QWORD *)result + 232LL),
               result);
    if ( (_BYTE)result != 0 )
    {
      result = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v15 + 728LL))(
                 v18,
                 v19,
                 128,
                 v15);
      if ( (_BYTE)result != 0 )
      {
        result = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v15 + 728LL))(
                   v20,
                   v21,
                   126,
                   v15);
        if ( (_BYTE)result == 0 )
        {
          sub_F1170(v22, v23, v64, v22, v62);
          sub_F1610(v24, v25, v64, v61);
          result = (*(__int64 (__fastcall **)(__int64, __int64, _BYTE *, __int64))(*(_QWORD *)v15 + 72LL))(
                     v26,
                     v27,
                     v63,
                     v15);
          __asm
          {
            vmovss  xmm0, [rbp+57h+var_90]
            vmovdqu xmm1, xmmword ptr cs:1706090h
            vandps  xmm0, xmm0, xmm1
            vcomiss xmm0, dword ptr cs:17060ACh
          }
          if ( v28 )
          {
            __asm
            {
              vmovss  xmm0, [rbp+57h+var_8C]
              vandps  xmm0, xmm0, xmm1
              vcomiss xmm0, dword ptr cs:17060ACh
            }
            __asm
            {
              vmovss  xmm0, [rbp+57h+var_88]
              vandps  xmm0, xmm0, xmm1
              vcomiss xmm0, dword ptr cs:17060ACh
            }
          }
          else
          {
            __asm
            {
              vmovss  xmm4, [rbp+57h+var_B8]
              vmovss  xmm6, [rbp+57h+var_BC]
              vmulss  xmm0, xmm6, [rbp+57h+var_AC]
              vmulss  xmm1, xmm0, [rbp+57h+var_A8]
              vmovss  xmm5, [rbp+57h+var_C0]
              vmulss  xmm0, xmm5, [rbp+57h+var_B0]
              vmulss  xmm0, xmm0, [rbp+57h+var_A0]
              vaddss  xmm2, xmm1, xmm0
              vmulss  xmm0, xmm4, [rbp+57h+var_B4]
              vmulss  xmm0, xmm0, [rbp+57h+var_A4]
              vmulss  xmm1, xmm4, [rbp+57h+var_B0]
              vaddss  xmm3, xmm2, xmm0
              vmulss  xmm2, xmm1, [rbp+57h+var_A8]
              vmulss  xmm0, xmm6, [rbp+57h+var_B4]
              vmulss  xmm1, xmm0, [rbp+57h+var_A0]
              vsubss  xmm4, xmm3, xmm2
              vmulss  xmm2, xmm5, [rbp+57h+var_AC]
              vmulss  xmm0, xmm2, [rbp+57h+var_A4]
              vsubss  xmm3, xmm4, xmm1
              vsubss  xmm1, xmm3, xmm0
              vcomiss xmm1, dword ptr cs:17060ACh
            }
            if ( !(v28 | v29) )
            {
              result = sub_F1278(v30, v31, v63, v61);
              if ( (_DWORD)result == 0 )
                result = (*(__int64 (__fastcall **)(__int64, __int64, _DWORD *, __int64))(*(_QWORD *)v15 + 104LL))(
                           v55,
                           v56,
                           v61,
                           v15);
            }
          }
        }
      }
    }
  }
  _R11 = &vars0;
  __asm { vmovaps xmm6, xmmword ptr [r11-10h] }
  return result;
}
