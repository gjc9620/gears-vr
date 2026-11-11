__int64 __fastcall sub_DE7D0(
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a9,
        __int64 a10,
        __int64 a11,
        __int64 a12)
{
  __int64 v20; // rdx
  __int64 v21; // rdi
  __int64 v22; // rsi
  __int64 v23; // rdx
  __int64 v24; // rdi
  __int64 v25; // rsi
  int v26; // ecx
  __int64 v27; // rsi
  int v28; // ebp
  __int64 v29; // rax
  BOOL v30; // r14d
  __int64 v31; // rax
  __int64 (__fastcall *v32)(int, int, int, int, int, int, __int64); // rdx
  __int64 (__fastcall *v33)(); // r8
  __int64 result; // rax
  __int64 v44; // rax
  __int64 v45; // rsi
  __int64 v46; // rdi
  __int64 v47; // rdi
  __int64 v48; // rsi
  __int64 v49; // rdi
  __int64 v50; // rsi
  __int64 v51; // rdi
  __int64 v52; // rsi
  __int64 v53; // rdi
  __int64 v54; // rsi
  __int64 v55; // rsi
  __int64 v56; // rdi
  char v57; // cf
  char v58; // zf
  __int64 v59; // rdi
  __int64 v60; // rsi
  __int64 v85; // rdi
  __int64 v86; // rsi
  __int64 v87; // rdx
  __int64 v88; // rdx
  __int64 v89; // rdi
  __int64 v90; // rsi
  __int64 v91; // rdx
  __int64 v92; // rdi
  __int64 v93; // rsi
  _DWORD v98[4]; // [rsp+20h] [rbp-148h] BYREF
  _DWORD v99[3]; // [rsp+30h] [rbp-138h] BYREF
  _BYTE v100[48]; // [rsp+60h] [rbp-108h] BYREF
  _BYTE v101[192]; // [rsp+90h] [rbp-D8h] BYREF
  char v102; // [rsp+150h] [rbp-18h] BYREF
  _UNKNOWN *retaddr; // [rsp+168h] [rbp+0h] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-28h], xmm6
    vmovaps xmmword ptr [rax-38h], xmm7
    vmovaps xmmword ptr [rax-48h], xmm8
    vmovaps xmmword ptr [rax-58h], xmm9
    vmovaps xmmword ptr [rax-68h], xmm10
    vmovaps xmmword ptr [rax-78h], xmm11
    vmovaps xmmword ptr [rax-88h], xmm12
    vmovaps xmmword ptr [rax-98h], xmm13
  }
  (*(void (**)(void))(*(_QWORD *)a12 + 888LL))();
  (*(void (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)a12 + 704LL))(v21, v22, v20, a12);
  v25 = (*(__int64 (**)(void))(**(_QWORD **)(a12 + 112) + 320LL))();
  if ( (*(_DWORD *)(a12 + 336) & 0x8000) != 0 )
  {
    v44 = *(_QWORD *)(a12 + 120);
    if ( (v44 == 0 || ((unsigned int)&loc_200000 & *(_DWORD *)(v44 + 176)) != 0) && v25 != 0 )
    {
      (*(void (__fastcall **)(__int64, __int64, __int64))(**(_QWORD **)(v25 + 376) + 24LL))(v24, v25, a12);
      (*(void (__fastcall **)(__int64, __int64, __int64))(**(_QWORD **)(v45 + 376) + 16LL))(v46, v45, a12);
    }
  }
  v26 = *(_DWORD *)(a12 + 324);
  v27 = 0;
  v28 = 1;
  v30 = false;
  if ( (v26 & 8) == 0 )
  {
    v29 = *(_QWORD *)(a12 + 120);
    if ( v29 == 0 || (*(_BYTE *)(v29 + 180) & 1) == 0 )
      v30 = true;
  }
  if ( (v26 & 4) != 0
    || (v31 = *(_QWORD *)(a12 + 120)) != 0 && (*(_BYTE *)(v31 + 172) & 2) != 0 && (*(_DWORD *)(a12 + 336) & 0x100) == 0 )
  {
    v28 = 0;
  }
  if ( MEMORY[0x1DA4DE0] > *(_DWORD *)(*(_QWORD *)__readgsqword(0x58u) + 20LL) )
  {
    sub_53E120(v24, 0, v23, 31084000);
    if ( MEMORY[0x1DA4DE0] == -1 )
    {
      sub_2B4368(v24, v27, v87, 31082480);
      MEMORY[0x1DA47F0] = 21874976;
      sub_53DDC0(v89, v90, v88, sub_895E40);
      sub_53E0B4(v92, v93, v91, 31084000);
    }
  }
  if ( *(unsigned __int8 *)(a12 + 318) <= MEMORY[0x1DA4A70] )
  {
    if ( MEMORY[0x1B97B50] != 0 )
    {
      if ( v28 == 0 )
        goto LABEL_15;
      goto LABEL_14;
    }
    if ( v30 )
LABEL_14:
      (*(void (__fastcall **)(__int64, __int64, __int64))(**(_QWORD **)(a12 + 112) + 16LL))(v24, v27, a12);
  }
LABEL_15:
  v32 = *(__int64 (__fastcall **)(int, int, int, int, int, int, __int64))(*(_QWORD *)a12 + 880LL);
  if ( v32 == sub_F1C64 )
  {
    v33 = *(__int64 (__fastcall **)())(*(_QWORD *)a12 + 896LL);
    if ( v33 == sub_C9A34 )
    {
      result = *(_QWORD *)(a12 + 380);
      if ( result != 0 )
        v27 = *(_QWORD *)(result + 136);
    }
    else
    {
      result = ((__int64 (__fastcall *)(__int64, __int64, __int64, __int64))v33)(v24, v27, v27, a12);
      v27 = result;
    }
    if ( v27 != 0 )
    {
      result = (*(__int64 (__fastcall **)(__int64, __int64, _QWORD, __int64))(*(_QWORD *)v27 + 232LL))(
                 v24,
                 v27,
                 *(_QWORD *)(*(_QWORD *)v27 + 232LL),
                 v27);
      if ( (_BYTE)result != 0 )
      {
        result = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v48 + 728LL))(
                   v47,
                   v48,
                   128,
                   v48);
        if ( (_BYTE)result != 0 )
        {
          result = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v50 + 728LL))(
                     v49,
                     v50,
                     126,
                     v50);
          if ( (_BYTE)result == 0 )
          {
            sub_F1170(v51, v52, v101, a12, v98);
            sub_F1610(v53, v54, v101, v99);
            result = (*(__int64 (__fastcall **)(__int64, __int64, _BYTE *, __int64))(*(_QWORD *)v55 + 72LL))(
                       v56,
                       v55,
                       v100,
                       v55);
            __asm
            {
              vmovss  xmm0, [rsp+168h+var_148]
              vmovdqu xmm1, xmmword ptr cs:1706090h
              vandps  xmm0, xmm0, xmm1
              vmovss  xmm13, dword ptr cs:17060ACh
              vcomiss xmm0, xmm13
            }
            if ( v57 )
            {
              __asm
              {
                vmovss  xmm0, [rsp+168h+var_144]
                vandps  xmm0, xmm0, xmm1
                vcomiss xmm0, xmm13
              }
              __asm
              {
                vmovss  xmm0, [rsp+168h+var_140]
                vandps  xmm0, xmm0, xmm1
                vcomiss xmm0, xmm13
              }
            }
            else
            {
              __asm
              {
                vmovss  xmm11, [rsp+168h+var_134]
                vmulss  xmm0, xmm11, [rsp+168h+var_124]
                vmulss  xmm1, xmm0, [rsp+168h+var_120]
                vmovss  xmm10, [rsp+168h+var_138]
                vmulss  xmm0, xmm10, [rsp+168h+var_128]
                vmulss  xmm0, xmm0, [rsp+168h+var_118]
                vaddss  xmm2, xmm1, xmm0
                vmovss  xmm4, [rsp+168h+var_130]
                vmulss  xmm0, xmm4, [rsp+168h+var_12C]
                vmulss  xmm0, xmm0, [rsp+168h+var_11C]
                vaddss  xmm3, xmm2, xmm0
                vmulss  xmm1, xmm4, [rsp+168h+var_128]
                vmulss  xmm2, xmm1, [rsp+168h+var_120]
                vsubss  xmm4, xmm3, xmm2
                vmulss  xmm0, xmm11, [rsp+168h+var_12C]
                vmulss  xmm1, xmm0, [rsp+168h+var_118]
                vsubss  xmm3, xmm4, xmm1
                vmulss  xmm2, xmm10, [rsp+168h+var_124]
                vmulss  xmm0, xmm2, [rsp+168h+var_11C]
                vsubss  xmm1, xmm3, xmm0
                vcomiss xmm1, xmm13
              }
              if ( !(v57 | v58) )
              {
                result = sub_F1278(v59, v60, v100, v99);
                if ( (_DWORD)result == 0 )
                  result = (*(__int64 (__fastcall **)(__int64, __int64, _DWORD *, __int64))(*(_QWORD *)v86 + 104LL))(
                             v85,
                             v86,
                             v99,
                             v86);
              }
            }
          }
        }
      }
    }
  }
  else
  {
    result = ((__int64 (__fastcall *)(__int64, __int64, __int64 (__fastcall *)(int, int, int, int, int, int, __int64), __int64))v32)(
               v24,
               v27,
               v32,
               a12);
  }
  _R11 = &v102;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
    vmovaps xmm11, xmmword ptr [r11-60h]
    vmovaps xmm12, xmmword ptr [r11-70h]
    vmovaps xmm13, xmmword ptr [r11-80h]
  }
  return result;
}
