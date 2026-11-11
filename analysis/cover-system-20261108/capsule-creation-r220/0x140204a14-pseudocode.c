__int64 __fastcall sub_203A14(
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
        _QWORD *a11,
        __int64 _RCX,
        __int64 a13)
{
  __int64 result; // rax
  __int64 v23; // rdx
  __int64 v32; // rdx
  __int64 v33; // rdi
  __int64 v34; // rsi
  __int64 v35; // r8
  int v36; // eax
  __int64 i; // r9
  __int64 v38; // rbx
  __int64 v39; // rdx
  __int64 v40; // rdi
  __int64 v41; // rsi
  __int64 v42; // rdi
  __int64 v46; // rsi
  __int64 v51; // rdx
  __int64 v52; // rdi
  __int64 v53; // rsi
  __int64 v110; // rdi
  __int64 v111; // rsi
  __int64 v112; // rdi
  _QWORD *v113; // r8
  int v114; // r9d
  int v115; // r15d
  __int64 v116; // r13
  int v136; // edi
  int v137; // r8d
  int v138; // r9d
  __int64 v139; // rax
  __int64 v140; // rdi
  __int64 v141; // rsi
  __int64 v143; // rdx
  __int64 v144; // rdi
  __int64 v145; // rsi
  __int64 v151; // r10
  __int64 *v154; // rdx
  unsigned __int64 v155; // rcx
  __int64 v187; // rsi
  __int64 v231; // rcx
  __int64 v232; // r12
  _QWORD *v233; // rax
  _QWORD *v234; // r15
  _QWORD *j; // rdx
  __int64 v236; // [rsp+0h] [rbp-288h]
  __int64 v237; // [rsp+0h] [rbp-288h]
  __int64 v238; // [rsp+8h] [rbp-280h]
  __int64 v239; // [rsp+8h] [rbp-280h]
  __int64 v240; // [rsp+10h] [rbp-278h]
  int v241; // [rsp+18h] [rbp-270h]
  __int64 v242; // [rsp+40h] [rbp-248h] BYREF
  __int64 v243; // [rsp+48h] [rbp-240h] BYREF
  __int64 v244; // [rsp+50h] [rbp-238h] BYREF
  __int64 v250; // [rsp+A0h] [rbp-1E8h]
  __int64 v251; // [rsp+A8h] [rbp-1E0h]
  __m256 v252; // [rsp+B0h] [rbp-1D8h] BYREF
  __m256 v253; // [rsp+D0h] [rbp-1B8h]
  __m256 v254; // [rsp+F0h] [rbp-198h] BYREF
  __int128 v256; // [rsp+130h] [rbp-158h] BYREF
  _QWORD v257[8]; // [rsp+140h] [rbp-148h] BYREF
  char v260; // [rsp+260h] [rbp-28h] BYREF
  _UNKNOWN *retaddr; // [rsp+288h] [rbp+0h] BYREF

  result = (__int64)&retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
    vmovaps xmmword ptr [rax-58h], xmm8
    vmovaps xmmword ptr [rax-68h], xmm9
    vmovaps xmmword ptr [rax-78h], xmm10
    vmovaps xmmword ptr [rax-88h], xmm11
    vmovaps xmmword ptr [rax-98h], xmm12
    vmovaps xmmword ptr [rax-0A8h], xmm13
    vmovaps xmmword ptr [rax-0B8h], xmm14
    vmovaps xmmword ptr [rax-0C8h], xmm15
  }
  v23 = *(_QWORD *)(_RCX + 1008);
  if ( *(_DWORD *)(v23 + 828) != 0 && a13 == *(_QWORD *)(_RCX + 436) )
  {
    __asm
    {
      vmovss  xmm0, dword ptr [rcx+198h]
      vmulss  xmm2, xmm0, dword ptr [rdx+1E8h]
      vmulss  xmm0, xmm2, dword ptr [rdx+1F4h]
      vmulss  xmm3, xmm0, dword ptr [rcx+1A4h]
      vmovss  dword ptr [rsp+288h+var_240], xmm3
      vmulss  xmm1, xmm2, dword ptr [rdx+1F0h]
      vmulss  xmm15, xmm1, dword ptr [rcx+1A0h]
      vmulss  xmm0, xmm2, dword ptr [rdx+1ECh]
      vmulss  xmm14, xmm0, dword ptr [rcx+19Ch]
      vmovss  dword ptr [rsp+288h+var_238], xmm14
      vmovss  dword ptr [rsp+288h+var_238+4], xmm15
      vmovss  [rsp+288h+var_230], xmm3
    }
    result = sub_2CF600(_RCX, a10, v23, &v244);
    if ( (_DWORD)result != 0 )
    {
      *(_DWORD *)(v33 + 1460) = 0;
      v36 = *(_DWORD *)(v33 + 1268);
      if ( v36 > 0 )
      {
        v35 = 0;
        for ( i = (unsigned int)v36; i != 0; --i )
        {
          if ( (*(_BYTE *)(*(_QWORD *)(*(_QWORD *)(v33 + 1260) + v35) + 128LL) & 1) != 0 )
            ++*(_DWORD *)(v33 + 1460);
          v35 += 8;
        }
      }
      v38 = *(_QWORD *)(v32 + 536);
      v244 = v38;
      (*(void (__fastcall **)(__int64, __int64, _QWORD *, __int64, __int64))(*(_QWORD *)v33 + 792LL))(
        v33,
        v34,
        v257,
        v33,
        v35);
      sub_F10F4(v40, v41, v39, v257);
      _RDX = *(_QWORD *)(v42 + 1008);
      __asm
      {
        vmovups ymm0, ymmword ptr [rdx+0A0h]
        vmovups [rsp+288h+var_1D8], ymm0
        vmovups ymm1, ymmword ptr [rdx+0C0h]
        vmovups [rsp+288h+var_1B8], ymm1
      }
      __asm { vzeroupper }
      sub_F10F4(v42, v46, _RDX, &v252);
      __asm
      {
        vmovups xmm0, [rsp+288h+var_148]
        vmovups xmm1, [rsp+288h+var_138]
        vmovups xmm2, [rsp+288h+var_128]
        vmovups xmm3, [rsp+288h+var_118]
      }
      *(double *)&_XMM0 = sub_136A60(
                            v52,
                            v53,
                            v51,
                            0,
                            *(double *)&_XMM0,
                            *(double *)&_XMM1,
                            *(double *)&_XMM2,
                            *(double *)&_XMM3);
      __asm
      {
        vmovups xmm10, xmm3
        vmovups xmm12, xmm2
        vmovups xmm11, xmm1
        vmovups xmm13, xmm0
        vmovups xmm8, xmmword ptr [rsp+288h+var_1D8]
        vshufps xmm4, xmm8, xmm8, 0FFh
        vmulps  xmm6, xmm4, xmm3
        vshufps xmm5, xmm8, xmm8, 55h ; 'U'
        vmulps  xmm4, xmm5, xmm1
        vaddps  xmm7, xmm6, xmm4
        vshufps xmm5, xmm8, xmm8, 0AAh
        vmulps  xmm6, xmm5, xmm2
        vshufps xmm4, xmm8, xmm8, 0
        vmulps  xmm5, xmm4, xmm0
        vaddps  xmm4, xmm6, xmm5
        vaddps  xmm9, xmm7, xmm4
        vmovups xmm6, xmmword ptr [rsp+288h+var_1D8+10h]
        vshufps xmm0, xmm6, xmm6, 0FFh
        vmulps  xmm3, xmm0, xmm3
        vshufps xmm1, xmm6, xmm6, 55h ; 'U'
        vmulps  xmm2, xmm1, xmm11
        vaddps  xmm5, xmm3, xmm2
        vshufps xmm0, xmm6, xmm6, 0AAh
        vmulps  xmm4, xmm0, xmm12
        vshufps xmm1, xmm6, xmm6, 0
        vmulps  xmm2, xmm1, xmm13
        vaddps  xmm0, xmm4, xmm2
        vaddps  xmm8, xmm5, xmm0
        vmovups xmm6, xmmword ptr [rsp+288h+var_1B8]
        vshufps xmm0, xmm6, xmm6, 0FFh
        vmulps  xmm3, xmm0, xmm10
        vshufps xmm1, xmm6, xmm6, 55h ; 'U'
        vmulps  xmm2, xmm1, xmm11
        vaddps  xmm5, xmm3, xmm2
        vshufps xmm0, xmm6, xmm6, 0AAh
        vmulps  xmm4, xmm0, xmm12
        vshufps xmm1, xmm6, xmm6, 0
        vmulps  xmm2, xmm1, xmm13
        vaddps  xmm0, xmm4, xmm2
        vaddps  xmm7, xmm5, xmm0
        vmovups xmm6, xmmword ptr [rsp+288h+var_1B8+10h]
        vshufps xmm0, xmm6, xmm6, 0FFh
        vmulps  xmm3, xmm0, xmm10
        vshufps xmm1, xmm6, xmm6, 55h ; 'U'
        vmulps  xmm2, xmm1, xmm11
        vaddps  xmm5, xmm3, xmm2
        vshufps xmm0, xmm6, xmm6, 0AAh
        vmulps  xmm4, xmm0, xmm12
        vshufps xmm1, xmm6, xmm6, 0
        vmulps  xmm2, xmm1, xmm13
        vaddps  xmm0, xmm4, xmm2
        vaddps  xmm3, xmm5, xmm0
        vmovups [rsp+288h+var_228], xmm9
        vmovups [rsp+288h+var_218], xmm8
        vmovups [rsp+288h+var_208], xmm7
        vmovups [rsp+288h+var_1F8], xmm3
      }
      result = sub_4ACFA8(v110, v111, v257, &v254, &v252);
      v115 = 0;
      LODWORD(v242) = 0;
      if ( *(int *)(v112 + 1268) > 0 )
      {
        v116 = 0;
        __asm
        {
          vmovss  xmm12, dword ptr [rsp+288h+var_208+8]
          vmovss  xmm13, dword ptr [rsp+288h+var_208+4]
          vmovups ymm0, [rsp+288h+var_178]
          vmovups [rsp+288h+var_E8], ymm0
          vmovups ymm0, [rsp+288h+var_198]
          vmovups [rsp+288h+var_108], ymm0
          vmovss  xmm11, dword ptr cs:17060C4h
          vmovss  xmm10, dword ptr [rsp+288h+var_240]
        }
        do
        {
          __asm { vzeroupper }
          sub_F8D80(
            v112,
            *(_QWORD *)(*(_QWORD *)(v112 + 1260) + v116),
            *(_QWORD *)(*(_QWORD *)(*(_QWORD *)(v112 + 1260) + v116) + 144LL),
            v38,
            (_DWORD)v113,
            v114,
            v236,
            v238);
          __asm
          {
            vmovss  xmm0, dword ptr [rsi+0A0h]
            vaddss  xmm4, xmm0, dword ptr [rdx+rax+24h]
            vmovss  xmm1, dword ptr [rsi+9Ch]
            vaddss  xmm3, xmm1, dword ptr [rdx+rax+20h]
            vmovss  xmm0, dword ptr [rsi+98h]
            vaddss  xmm2, xmm0, dword ptr [rdx+rax+1Ch]
            vmulss  xmm2, xmm2, xmm14
            vmovss  dword ptr [rsi+0DCh], xmm2
            vmulss  xmm5, xmm15, xmm3
            vmovss  dword ptr [rsi+0E0h], xmm5
            vmulss  xmm0, xmm10, xmm4
            vaddss  xmm1, xmm0, dword ptr [rsi+0A8h]
            vmovss  dword ptr [rsi+0E4h], xmm1
            vmovups ymm0, [rsp+288h+var_108]
            vmovups [rsp+288h+var_198], ymm0
            vmovups ymm0, [rsp+288h+var_E8]
            vmovups [rsp+288h+var_178], ymm0
            vmovss  dword ptr [rsp+288h+var_178+10h], xmm2
            vmovss  dword ptr [rsp+288h+var_178+14h], xmm5
            vmovss  dword ptr [rsp+288h+var_178+18h], xmm1
          }
          __asm { vzeroupper }
          v139 = sub_1BF3C8(v136, _RSI, 8, 248, v137, v138, v237, v239, v240);
          _RBX = v139;
          v243 = v139;
          if ( v139 != 0 )
          {
            *(_DWORD *)(v139 + 8) = 4;
            *(_QWORD *)v139 = 20175496;
            *(_QWORD *)(v139 + 136) = 0;
            *(_DWORD *)(v139 + 144) = 0;
            *(_QWORD *)(v139 + 152) = 20175608;
            *(_DWORD *)(v139 + 160) = 1065353216;
            *(_DWORD *)(v139 + 164) = 1017370378;
            *(_DWORD *)(v139 + 168) = 0x40000000;
            *(_DWORD *)(v139 + 172) = 1008981770;
            *(_DWORD *)(v139 + 176) = 1232348160;
            *(_QWORD *)(v139 + 184) = 20175608;
            *(_DWORD *)(v139 + 192) = 1065353216;
            *(_DWORD *)(v139 + 196) = 1017370378;
            *(_DWORD *)(v139 + 200) = 0x40000000;
            *(_DWORD *)(v139 + 204) = 1008981770;
            *(_DWORD *)(v139 + 208) = 1232348160;
            *(_QWORD *)(v139 + 12) = 1065353216;
            *(_QWORD *)(v139 + 20) = 0;
            *(_QWORD *)(v139 + 28) = 1065353216;
            *(_QWORD *)(v139 + 36) = 0;
            *(_QWORD *)(v139 + 44) = 1065353216;
            *(_QWORD *)(v139 + 52) = 0;
            *(_QWORD *)(v139 + 60) = 8;
            *(_QWORD *)(v139 + 72) = 0;
            *(_DWORD *)(v139 + 88) = -1082130432;
            *(_DWORD *)(v139 + 80) = 1065353216;
            *(_DWORD *)(v139 + 84) = -1082130432;
            *(_QWORD *)(v139 + 96) = 0;
            *(_QWORD *)(v139 + 104) = 0;
            *(_QWORD *)(v139 + 112) = 0;
            *(_QWORD *)(v139 + 120) = 0;
            *(_DWORD *)(v139 + 128) = 1065353216;
            *(_DWORD *)(v139 + 132) = 1065353216;
            *(_QWORD *)(v139 + 216) = 1065353216;
            *(_QWORD *)(v139 + 224) = 0;
            *(_DWORD *)(v139 + 232) = 0;
            *(_QWORD *)(v139 + 240) = 0;
          }
          else
          {
            _RBX = 0;
          }
          sub_F1610(v140, v141, &v254, &v252);
          __asm
          {
            vmovups ymm0, [rsp+288h+var_1D8]
            vmovups ymmword ptr [rbx+0Ch], ymm0
          }
          *(float *)(_RBX + 44) = v253.m256_f32[0];
          __asm
          {
            vmovss  xmm0, dword ptr [rsp+288h+var_1B8+4]
            vmovss  dword ptr [rbx+30h], xmm0
            vmovss  xmm1, dword ptr [rsp+288h+var_1B8+8]
            vmovss  dword ptr [rbx+34h], xmm1
            vmovss  xmm0, dword ptr [rsp+288h+var_1B8+0Ch]
            vmovss  dword ptr [rbx+38h], xmm0
            vmulss  xmm2, xmm11, dword ptr [rsi+0A8h]
            vmovss  dword ptr [rbx+84h], xmm2
          }
          *(_DWORD *)(_RBX + 220) = (*(_DWORD *)(v144 + 1300) & 2 | 0x10u) >> 1;
          v151 = *(_QWORD *)(v144 + 1008);
          LODWORD(v243) = *(_DWORD *)(v151 + 348);
          if ( (*(_BYTE *)(v145 + 128) & 2) != 0 )
          {
            __asm { vzeroupper }
            sub_3AF258(v144, v145, 4, &v243, 1);
          }
          LOBYTE(v143) = *(_BYTE *)(v151 + 340);
          __asm { vzeroupper }
          result = sub_10A78C(v144, v145, v143, &v256, &v243);
          __asm
          {
            vmovups xmm0, [rsp+288h+var_158]
            vmovdqu xmmword ptr [rbx+70h], xmm0
          }
          *(_QWORD *)(_RBX + 96) = _RSI;
          v154 = (__int64 *)a11[14];
          v155 = a11[15];
          if ( v155 <= (unsigned __int64)v154 )
          {
            v113 = (_QWORD *)a11[13];
            result = 2 * (unsigned int)(v154 - v113) + 2;
            if ( v113 != nullptr )
              v231 = (__int64)(v155 - (_QWORD)v113) >> 3;
            else
              LODWORD(v231) = 0;
            if ( (unsigned int)v231 < (unsigned int)result )
            {
              v232 = (unsigned int)result;
              v233 = (_QWORD *)MEMORY[0x131BE18](v112, _RSI, v154, v232 * 8);
              v234 = v233;
              v113 = (_QWORD *)a11[14];
              for ( j = (_QWORD *)a11[13]; j != v113; ++j )
                *v233++ = *j;
              if ( a11[13] != 0 )
                MEMORY[0x131BE10]();
              a11[15] = &v234[v232];
              result = (unsigned int)((__int64)(a11[14] - a11[13]) >> 3);
              v154 = &v234[result];
              a11[14] = v154;
              a11[13] = v234;
              v115 = v242;
            }
          }
          *v154 = _RBX;
          a11[14] += 8LL;
          if ( *(_QWORD *)((char *)_RSI + 324) == 0 )
          {
            result = (*(__int64 (__fastcall **)(__int64, _QWORD *, __int64 *, _QWORD *))(*_RSI + 560LL))(
                       v112,
                       _RSI,
                       v154,
                       _RSI);
            if ( (_DWORD)result != 0 )
            {
              v250 = 0;
              result = sub_20569C(
                         v112,
                         (_DWORD)_RSI,
                         v112,
                         *(_QWORD *)((char *)_RSI + 316),
                         0,
                         v114,
                         v236,
                         v238,
                         v240,
                         v241,
                         0,
                         0);
              *(_QWORD *)((char *)_RSI + 324) = result;
              if ( result != 0 )
              {
                *(_QWORD *)(result + 520) = _RSI[22];
                __asm
                {
                  vmovss  xmm7, dword ptr [rsi+0DCh]
                  vmovss  xmm6, dword ptr [rsi+0E0h]
                  vmovss  xmm8, dword ptr [rsi+0E4h]
                  vmulss  xmm1, xmm6, dword ptr [rsp+288h+var_218]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228]
                  vaddss  xmm3, xmm1, xmm0
                  vmulss  xmm1, xmm8, dword ptr [rsp+288h+var_208]
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8]
                  vaddss  xmm9, xmm3, xmm2
                  vmulss  xmm3, xmm6, dword ptr [rsp+288h+var_218+4]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228+4]
                  vaddss  xmm4, xmm3, xmm0
                  vmulss  xmm1, xmm13, xmm8
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8+4]
                  vaddss  xmm5, xmm4, xmm2
                  vmulss  xmm3, xmm6, dword ptr [rsp+288h+var_218+8]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228+8]
                  vaddss  xmm4, xmm3, xmm0
                  vmulss  xmm1, xmm12, xmm8
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8+8]
                  vaddss  xmm0, xmm4, xmm2
                  vsubss  xmm3, xmm0, dword ptr [rsi+0A4h]
                }
                _RCX = *(_QWORD *)((char *)_RSI + 324);
                __asm
                {
                  vunpcklps xmm0, xmm9, xmm5
                  vmovsd  qword ptr [rcx+1D0h], xmm0
                  vmovss  dword ptr [rcx+1D8h], xmm3
                }
                _RAX = *(_QWORD *)((char *)_RSI + 324);
                __asm
                {
                  vmovss  xmm0, dword ptr [rax+1D0h]
                  vdivss  xmm1, xmm0, xmm14
                  vmovss  dword ptr [rax+1D0h], xmm1
                  vmovss  xmm0, dword ptr [rax+1D4h]
                  vdivss  xmm1, xmm0, xmm15
                  vmovss  dword ptr [rax+1D4h], xmm1
                  vmovss  xmm0, dword ptr [rax+1D8h]
                  vdivss  xmm1, xmm0, xmm10
                  vmovss  dword ptr [rax+1D8h], xmm1
                }
                v242 = *(_QWORD *)((char *)_RSI + 324);
                result = sub_C9360(v112, (_DWORD)_RSI, (unsigned int)&v242, (int)v112 + 96, (_DWORD)v113, v114, v236);
              }
            }
          }
          if ( *(_QWORD *)((char *)_RSI + 348) == 0 )
          {
            result = (*(__int64 (__fastcall **)(__int64, _QWORD *, __int64 *, _QWORD *))(*_RSI + 568LL))(
                       v112,
                       _RSI,
                       v154,
                       _RSI);
            if ( (_DWORD)result != 0 )
            {
              v251 = 0;
              result = sub_20569C(v112, v187, v112, *(_QWORD *)(v187 + 340), 0, v114, v236, v238, v240, v241, 0, 0);
              *(_QWORD *)(_RSI + 348) = result;
              if ( result != 0 )
              {
                *(_QWORD *)(result + 520) = *(_QWORD *)(_RSI + 184);
                __asm
                {
                  vmovss  xmm7, dword ptr [rsi+0DCh]
                  vmovss  xmm6, dword ptr [rsi+0E0h]
                  vmovss  xmm8, dword ptr [rsi+0E4h]
                  vmulss  xmm1, xmm6, dword ptr [rsp+288h+var_218]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228]
                  vaddss  xmm3, xmm1, xmm0
                  vmulss  xmm1, xmm8, dword ptr [rsp+288h+var_208]
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8]
                  vaddss  xmm9, xmm3, xmm2
                  vmulss  xmm3, xmm6, dword ptr [rsp+288h+var_218+4]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228+4]
                  vaddss  xmm4, xmm3, xmm0
                  vmulss  xmm1, xmm13, xmm8
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8+4]
                  vaddss  xmm5, xmm4, xmm2
                  vmulss  xmm3, xmm6, dword ptr [rsp+288h+var_218+8]
                  vmulss  xmm0, xmm7, dword ptr [rsp+288h+var_228+8]
                  vaddss  xmm4, xmm3, xmm0
                  vmulss  xmm1, xmm12, xmm8
                  vaddss  xmm2, xmm1, dword ptr [rsp+288h+var_1F8+8]
                  vaddss  xmm0, xmm4, xmm2
                  vsubss  xmm3, xmm0, dword ptr [rsi+0A4h]
                }
                _RCX = *(_QWORD *)(_RSI + 348);
                __asm
                {
                  vunpcklps xmm0, xmm9, xmm5
                  vmovsd  qword ptr [rcx+1D0h], xmm0
                  vmovss  dword ptr [rcx+1D8h], xmm3
                }
                _RAX = *(_QWORD *)(_RSI + 348);
                __asm
                {
                  vmovss  xmm0, dword ptr [rax+1D0h]
                  vdivss  xmm1, xmm0, xmm14
                  vmovss  dword ptr [rax+1D0h], xmm1
                  vmovss  xmm0, dword ptr [rax+1D4h]
                  vdivss  xmm1, xmm0, xmm15
                  vmovss  dword ptr [rax+1D4h], xmm1
                  vmovss  xmm0, dword ptr [rax+1D8h]
                  vdivss  xmm1, xmm0, xmm10
                  vmovss  dword ptr [rax+1D8h], xmm1
                }
                v242 = *(_QWORD *)(_RSI + 348);
                result = sub_C9360(v112, _RSI, (unsigned int)&v242, (int)v112 + 96, (_DWORD)v113, v114, v236);
              }
            }
          }
          LODWORD(v242) = ++v115;
          v116 += 8;
          LODWORD(v38) = v244;
        }
        while ( v115 < *(_DWORD *)(v112 + 1268) );
      }
    }
  }
  _R11 = &v260;
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
    vmovaps xmm14, xmmword ptr [r11-90h]
    vmovaps xmm15, xmmword ptr [r11-0A0h]
  }
  return result;
}
