__int64 __fastcall sub_2D220C(
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
        __int64 a12,
        __int64 a13,
        __int64 a14,
        __int64 a15,
        char a16,
        __int64 a17)
{
  __int64 v17; // rbx
  __int64 result; // rax
  __int64 v23; // r8
  __int64 *v24; // rdi
  __int64 v37; // rdi
  __int64 v38; // rsi
  int v49; // edi
  int v50; // esi
  int v51; // r8d
  int v52; // r9d
  __int64 v61; // rdi
  __int64 v62; // rdi
  __int64 v63; // rsi
  __int64 v64; // r8
  __int64 v65; // rdi
  __int64 v66; // rsi
  __int64 v67; // r8
  __int64 v68; // rdx
  __int64 v69; // rdi
  __int64 v70; // rsi
  __int64 **v71; // rdx
  __int64 v72; // rdi
  __int64 v73; // rsi
  __int64 **v77; // r8
  unsigned int v78; // eax
  __int64 v79; // rcx
  __int64 v80; // r14
  __int64 **v81; // rax
  __int64 **v82; // rbx
  __int64 **v83; // r8
  __int64 v86; // rax
  __int64 v87; // rdx
  _QWORD *v88; // rdi
  __int64 v89; // rsi
  _QWORD *v90; // rbx
  __int64 v91; // rdi
  __int64 v92; // rsi
  __int64 v93; // rcx
  __int64 v94; // rdi
  __int64 v95; // rsi
  int v96; // r8d
  int v97; // r9d
  __int64 v98; // r10
  __int64 v99; // r11
  __int128 v100; // [rsp+20h] [rbp-E0h] BYREF
  __m256 v101; // [rsp+30h] [rbp-D0h] BYREF
  int v102; // [rsp+50h] [rbp-B0h]
  _QWORD *v106; // [rsp+60h] [rbp-A0h]
  __int64 v108; // [rsp+6Ch] [rbp-94h]
  __int64 v109; // [rsp+78h] [rbp-88h]
  __int64 v110; // [rsp+80h] [rbp-80h]
  __int64 v111; // [rsp+88h] [rbp-78h]
  int v112; // [rsp+90h] [rbp-70h]
  __int64 **v113; // [rsp+98h] [rbp-68h]
  __int64 **v114; // [rsp+A0h] [rbp-60h]
  __int64 **v115; // [rsp+A8h] [rbp-58h]
  _QWORD v116[4]; // [rsp+C0h] [rbp-40h] BYREF
  int v136; // [rsp+12Ch] [rbp+2Ch]
  int v139; // [rsp+138h] [rbp+38h]
  __int64 v142; // [rsp+150h] [rbp+50h] BYREF
  int v143; // [rsp+158h] [rbp+58h]
  __int64 v149; // [rsp+18Ch] [rbp+8Ch]
  __int64 v150; // [rsp+198h] [rbp+98h]
  __m256 v159; // [rsp+1E0h] [rbp+E0h] BYREF
  int v160; // [rsp+200h] [rbp+100h]
  __int128 v164; // [rsp+210h] [rbp+110h] BYREF
  char v170; // [rsp+290h] [rbp+190h] BYREF
  _UNKNOWN *retaddr; // [rsp+2B8h] [rbp+1B8h] BYREF

  _RAX = &retaddr;
  a17 = v17;
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
    vmovaps xmmword ptr [rax-58h], xmm8
    vmovaps xmmword ptr [rax-68h], xmm9
  }
  result = *(unsigned int *)(a12 + 596);
  v23 = (unsigned int)result ^ ((unsigned int)result ^ ((_DWORD)a11 << 25)) & 0x2000000;
  *(_DWORD *)(a12 + 596) = v23;
  v24 = (__int64 *)(a12 + 1128);
  if ( (a11 & 1) == 0 || *v24 != 0 )
  {
    if ( (v23 & 0x2000000) == 0 && *v24 != 0 )
    {
      v93 = *(_QWORD *)(MEMORY[0x1C1F890] + 468LL);
      if ( v93 != 0 )
      {
        a15 = *v24;
        result = sub_2AB6C8(v24, a12, a11, v93, v23);
        if ( result != 0 )
        {
          if ( *(_DWORD *)(v98 + 416) != 0 && *(_DWORD *)(v98 + 424) == 1 )
            result = sub_154150(v94, v95, (unsigned int)&a15, 30269248, v96, v97);
          else
            result = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)result + 64LL))(
                       v94,
                       v95,
                       v99,
                       result);
        }
      }
    }
  }
  else
  {
    _RCX = *(_QWORD *)(a12 + 436);
    if ( _RCX != 0 && (*(_BYTE *)(_RCX + 128) & 1) != 0 )
    {
      result = MEMORY[0x1C1F890];
      if ( *(_QWORD *)(MEMORY[0x1C1F890] + 468LL) != 0 )
      {
        __asm
        {
          vmovdqu xmm8, xmmword ptr cs:1706210h
          vmovups [rbp+1B0h+var_A0], xmm8
          vmovaps ymm0, ymmword ptr cs:18FF3E0h
          vmovups [rbp+1B0h+var_90], ymm0
          vmovss  xmm0, dword ptr [rcx+0D0h]
          vmovss  [rbp+1B0h+var_70], xmm0
          vmovss  xmm1, dword ptr [rcx+0D4h]
          vmovss  [rbp+1B0h+var_6C], xmm1
          vmovss  xmm0, dword ptr [rcx+0D8h]
          vmovss  [rbp+1B0h+var_68], xmm0
          vmovss  xmm9, dword ptr cs:17061B8h
          vmovss  [rbp+1B0h+var_64], xmm9
        }
        __asm { vzeroupper }
        sub_F1610(v24, a12, &v164, &v159);
        sub_576E65(v37, v38, 0, &v101, 136);
        v106 = nullptr;
        v109 = 0;
        v110 = 0;
        v108 = 2;
        v111 = 0;
        v114 = v113;
        v112 = 1;
        __asm
        {
          vmovups ymm0, [rbp+1B0h+var_D0]
          vmovups [rsp+2B0h+var_280], ymm0
        }
        v102 = v160;
        __asm
        {
          vmovss  xmm0, [rbp+1B0h+var_AC]
          vmovss  [rsp+2B0h+var_25C], xmm0
          vmovss  xmm1, [rbp+1B0h+var_A8]
          vmovss  [rsp+2B0h+var_258], xmm1
          vmovss  xmm0, [rbp+1B0h+var_A4]
          vmovss  [rsp+2B0h+var_254], xmm0
          vmovss  [rsp+2B0h+var_248], xmm9
        }
        v143 = 2;
        v142 = 20215536;
        __asm
        {
          vmovss  [rbp+1B0h+var_154], xmm9
          vmovaps ymm0, ymmword ptr cs:1706420h
          vmovups [rbp+1B0h+var_150], ymm0
          vxorps  xmm6, xmm6, xmm6
          vmovss  [rbp+1B0h+var_128], xmm6
          vmovss  [rbp+1B0h+var_12C], xmm6
          vmovss  [rbp+1B0h+var_130], xmm6
        }
        v149 = 8;
        v150 = 0;
        __asm
        {
          vmovss  xmm7, dword ptr cs:1706190h
          vmovss  [rbp+1B0h+var_108], xmm7
          vmovss  [rbp+1B0h+var_110], xmm9
          vmovss  [rbp+1B0h+var_10C], xmm7
          vpxor   xmm0, xmm0, xmm0
          vmovdqu [rbp+1B0h+var_100], xmm0
          vpxor   xmm1, xmm1, xmm1
          vmovups [rbp+1B0h+var_F0], xmm1
          vmovss  [rbp+1B0h+var_D8], xmm6
          vmovss  [rbp+1B0h+var_DC], xmm6
          vmovss  [rbp+1B0h+var_E0], xmm6
        }
        __asm { vzeroupper }
        sub_18AF94(v49, v50, (unsigned int)&v100, v50, v51, v52);
        __asm
        {
          vmovss  xmm1, dword ptr [rsi+400h]
          vaddss  xmm0, xmm1, dword ptr [rsp+2B0h+var_290]
          vmovss  xmm3, dword ptr cs:17060C4h
          vmulss  xmm0, xmm0, xmm3
          vmovss  [rbp+1B0h+var_E0], xmm0
          vaddss  xmm1, xmm1, dword ptr [rsp+2B0h+var_290+4]
          vmulss  xmm2, xmm1, xmm3
          vmovss  [rbp+1B0h+var_DC], xmm2
          vmulss  xmm1, xmm3, dword ptr [rsp+2B0h+var_290+8]
          vmovss  [rbp+1B0h+var_D8], xmm1
        }
        sub_3AF258(v61, _RSI, 0, &a16, 1);
        sub_3AF258(v62, v63, 6, &a16, v64);
        sub_3AF258(v65, v66, 5, &a16, v67);
        LOBYTE(v68) = 2;
        sub_10A78C(v69, v70, v68, &v100, &a16);
        __asm
        {
          vmovups xmm0, [rsp+2B0h+var_290]
          vmovdqu [rbp+1B0h+var_F0], xmm0
        }
        v77 = v114;
        if ( v115 <= v114 )
        {
          v71 = v113;
          v78 = 2 * (v114 - v113) + 2;
          if ( v113 != nullptr )
            v79 = v115 - v113;
          else
            LODWORD(v79) = 0;
          if ( (unsigned int)v79 < v78 )
          {
            v80 = v78;
            v81 = (__int64 **)MEMORY[0x131BE18](v72, v73, v113, v80 * 8);
            v82 = v81;
            v83 = v114;
            v71 = v113;
            if ( v113 != v114 )
            {
              do
                *v81++ = *v71++;
              while ( v71 != v83 );
              v71 = v113;
            }
            if ( v71 != nullptr )
            {
              MEMORY[0x131BE10](v72, v73, v71, v71, v83);
              v71 = v113;
            }
            v115 = &v82[v80];
            v77 = &v82[(unsigned int)(v114 - v71)];
            v114 = v77;
            v113 = v82;
            __asm { vmovdqu xmm8, xmmword ptr cs:1706210h }
          }
        }
        *v77 = &v142;
        ++v114;
        __asm
        {
          vmovups [rbp+1B0h+var_1F0], xmm8
          vmovups [rbp+1B0h+var_1E0], xmm8
          vmovss  [rbp+1B0h+var_1D0], xmm9
          vmovss  [rbp+1B0h+var_1C4], xmm6
          vmovss  [rbp+1B0h+var_1C8], xmm6
          vmovss  [rbp+1B0h+var_1CC], xmm6
          vmovss  [rbp+1B0h+var_1B8], xmm6
          vmovss  [rbp+1B0h+var_1BC], xmm6
          vmovss  [rbp+1B0h+var_1C0], xmm6
          vmovss  [rbp+1B0h+var_1A8], xmm6
          vmovss  [rbp+1B0h+var_1AC], xmm6
          vmovss  [rbp+1B0h+var_1B0], xmm6
          vmovss  [rbp+1B0h+var_19C], xmm6
          vmovss  [rbp+1B0h+var_1A0], xmm6
          vmovss  [rbp+1B0h+var_1A4], xmm6
          vmovss  xmm0, dword ptr cs:17064ECh
          vmovss  [rbp+1B0h+var_198], xmm0
          vmovss  [rbp+1B0h+var_1B4], xmm6
          vmovss  [rbp+1B0h+var_194], xmm6
          vmovss  xmm0, dword ptr cs:17061C4h
          vmovss  [rbp+1B0h+var_190], xmm0
          vmovss  [rbp+1B0h+var_18C], xmm7
          vmovss  [rbp+1B0h+var_180], xmm7
          vmovss  [rbp+1B0h+var_17C], xmm7
          vmovss  [rbp+1B0h+var_188], xmm6
        }
        v139 = 4;
        __asm
        {
          vmovss  [rbp+1B0h+var_174], xmm7
          vmovss  [rbp+1B0h+var_170], xmm6
        }
        v136 = 2432;
        v106 = v116;
        v86 = sub_2AB6C8(v72, v73, v71, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL), v77);
        if ( v86 != 0 )
        {
          v90 = (_QWORD *)(*(__int64 (__fastcall **)(_QWORD *, __int64, __m256 *, __int64))(*(_QWORD *)v86 + 56LL))(
                            v88,
                            v89,
                            &v101,
                            v86);
          (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD *))(*v90 + 168LL))(v91, v92, 2, v90);
          v90[1] = v89;
          *v88 = v90;
        }
        v142 = 20215384;
        result = sub_2CFE88(v88, v89, v87, &v101);
      }
    }
  }
  _R11 = &v170;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
  }
  return result;
}
